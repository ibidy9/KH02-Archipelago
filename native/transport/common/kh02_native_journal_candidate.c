#include "kh02_native_journal_candidate.h"
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>
typedef struct sqlite3 Sqlite;
typedef struct sqlite3_stmt Statement;
typedef struct Api {
    HMODULE module;
    const char *(*version)(void);
    int (*open)(const char*,Sqlite**,int,const char*);
    int (*close)(Sqlite*);
    int (*exec)(Sqlite*,const char*,void*,void*,char**);
    int (*prepare)(Sqlite*,const char*,int,Statement**,const char**);
    int (*step)(Statement*);
    int (*finalize)(Statement*);
    int (*blob)(Statement*,int,const void*,int,void(*)(void*));
    int (*text)(Statement*,int,const char*,int,void(*)(void*));
    const void *(*column)(Statement*,int);
    int (*bytes)(Statement*,int);
    int (*autocommit)(Sqlite*);
    int (*timeout)(Sqlite*,int);
} Api;
struct Kh02NativeJournal {SRWLOCK lock;DWORD owner;volatile LONG closed;Api api;Sqlite *db;};
typedef struct Bytes {const unsigned char *p;uint32_t n;} Bytes;
#ifdef KH02_NATIVE_JOURNAL_SELFTEST
static uint32_t fault;
static void (*test_hook)(uint32_t,void*);
static void *test_context;
MEMBER_API void Kh02NativeJournalTestFault(uint32_t value){fault=value;}
MEMBER_API void Kh02NativeJournalTestHook(void(*hook)(uint32_t,void*),void *context){test_hook=hook;test_context=context;}
#endif
static int point(uint32_t value) {
#ifdef KH02_NATIVE_JOURNAL_SELFTEST
    if(test_hook)test_hook(value,test_context);
    return fault!=value;
#else
    (void)value;return 1;
#endif
}
static int digest(Bytes *parts,uint32_t count,int framed,char out[65]) {
    BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_HASH_HANDLE hash=NULL;unsigned char *object=NULL,raw[32];
    DWORD length=0,written=0;uint32_t i;int ok=0;static const char hex[]="0123456789abcdef";
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
    if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,(PUCHAR)&length,sizeof(length),&written,0)<0 || written!=sizeof(length) || !length)goto done;
    object=HeapAlloc(GetProcessHeap(),0,length);if(!object)goto done;
    if(BCryptCreateHash(algorithm,&hash,object,length,NULL,0,0)<0)goto done;
    for(i=0;i<count;i++) {
        uint64_t n=parts[i].n;
        if(framed && BCryptHashData(hash,(PUCHAR)&n,8,0)<0)goto done;
        if(BCryptHashData(hash,(PUCHAR)parts[i].p,parts[i].n,0)<0)goto done;
    }
    if(BCryptFinishHash(hash,raw,32,0)<0)goto done;
    for(i=0;i<32;i++){out[i*2]=hex[raw[i]>>4];out[i*2+1]=hex[raw[i]&15];}out[64]=0;ok=1;
done:
    if(hash)BCryptDestroyHash(hash);if(object)HeapFree(GetProcessHeap(),0,object);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);return ok;
}
static int load(Api *a) {
    a->module=LoadLibraryExW(L"winsqlite3.dll",NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!a->module)return 0;
#define LOAD(field,name) do {FARPROC symbol=GetProcAddress(a->module,name);if(!symbol)return 0;memcpy(&a->field,&symbol,sizeof(symbol));}while(0)
    LOAD(version,"sqlite3_libversion");LOAD(open,"sqlite3_open_v2");LOAD(close,"sqlite3_close");
    LOAD(exec,"sqlite3_exec");LOAD(prepare,"sqlite3_prepare_v2");LOAD(step,"sqlite3_step");LOAD(finalize,"sqlite3_finalize");
    LOAD(blob,"sqlite3_bind_blob");LOAD(text,"sqlite3_bind_text");LOAD(column,"sqlite3_column_blob");
    LOAD(bytes,"sqlite3_column_bytes");LOAD(autocommit,"sqlite3_get_autocommit");LOAD(timeout,"sqlite3_busy_timeout");
#undef LOAD
    return !strcmp(a->version(),"3.43.2");
}
static int execute(Kh02NativeJournal *j,const char *sql){return j->api.exec(j->db,sql,NULL,NULL,NULL)==0;}
static int prepare(Kh02NativeJournal *j,const char *sql,Statement **out){return j->api.prepare(j->db,sql,-1,out,NULL)==0;}
static Bytes column(Kh02NativeJournal *j,Statement *s,int index) {
    Bytes b={(const unsigned char*)j->api.column(s,index),(uint32_t)j->api.bytes(s,index)};return b;
}
static int equal(Kh02NativeJournal *j,Statement *s,int index,Bytes b) {
    Bytes other=column(j,s,index);return other.n==b.n && (!b.n || (other.p && !memcmp(other.p,b.p,b.n)));
}
static int bind(Kh02NativeJournal *j,Statement *s,int index,Bytes b,int text) {
    if(b.n>0x7fffffff)return 0;
    return (text?j->api.text(s,index,(const char*)b.p,(int)b.n,NULL):j->api.blob(s,index,b.p,(int)b.n,NULL))==0;
}
static int single(Kh02NativeJournal *j,const char *sql,const char *value) {
    Statement *s=NULL;int ok=0;Bytes expected={(const unsigned char*)value,(uint32_t)strlen(value)};
    if(prepare(j,sql,&s) && j->api.step(s)==100 && equal(j,s,0,expected) && j->api.step(s)==101)ok=1;
    if(s && j->api.finalize(s)!=0)ok=0;return ok;
}
/* Native open checks physical integrity, stored checksums and record links. The stronger
   canonical/schema audit remains a required offline Python preflight;
   this append backend does not resolve loads or trust arbitrary database rows. */
static int audit(Kh02NativeJournal *j) {
    Statement *s=NULL;int status,ok=0;char record_hash[65],hash[65];Bytes p[3];
    if(!prepare(j,"SELECT scope,record_hash,record,members,digest FROM records",&s))goto done;
    while((status=j->api.step(s))==100) {
        p[0]=column(j,s,0);p[1]=column(j,s,2);p[2]=column(j,s,3);
        if(!p[0].p || !p[0].n || !p[1].p || p[1].n!=KH02_BIND_RECORD || !p[2].p || !p[2].n ||
           !digest(&p[1],1,0,record_hash) || !digest(p,3,1,hash) ||
           !equal(j,s,1,(Bytes){(unsigned char*)record_hash,64}) || !equal(j,s,4,(Bytes){(unsigned char*)hash,64}))goto done;
    }
    if(status!=101)goto done;
    if(j->api.finalize(s)!=0){s=NULL;goto done;}s=NULL;
    if(!prepare(j,"SELECT token,container,association,binding FROM publications",&s))goto done;
    while((status=j->api.step(s))==100) {
        p[0]=column(j,s,0);p[1]=column(j,s,1);p[2]=column(j,s,2);
        if(!p[0].p || !p[0].n || p[0].n>1024 || !p[1].p || p[1].n!=KH02_BIND_SIZE || !p[2].p || !p[2].n ||
           !digest(p,3,1,hash) || !equal(j,s,3,(Bytes){(unsigned char*)hash,64}))goto done;
    }
    ok=status==101;
done:
    if(s && j->api.finalize(s)!=0)ok=0;return ok;
}
static int links(Kh02NativeJournal *j) {
    /* Extract whole JSON arrays, preserving their exact integer/escape text.
       Scalar receipt IDs must never pass through SQLite floating-point values. */
    return single(j,
        "SELECT count(*) FROM publications p,json_each(p.association,'$.bindings') b "
        "LEFT JOIN records r ON r.scope=CAST(json_extract(p.association,'$.scope') AS BLOB) "
        "AND r.record=substr(p.container,177+15168*json_extract(b.value,'$[0]'),15168) "
        "AND r.members=CAST(json_extract(p.association,'$.sets['||json_extract(b.value,'$[1]')||']') AS BLOB) "
        "WHERE r.record_hash IS NULL","0") &&
        single(j,"SELECT count(*) FROM records r WHERE NOT EXISTS ("
        "SELECT 1 FROM publications p,json_each(p.association,'$.bindings') b "
        "WHERE r.scope=CAST(json_extract(p.association,'$.scope') AS BLOB) "
        "AND r.record=substr(p.container,177+15168*json_extract(b.value,'$[0]'),15168) "
        "AND r.members=CAST(json_extract(p.association,'$.sets['||json_extract(b.value,'$[1]')||']') AS BLOB))","0");
}
Kh02NativeJournal *Kh02NativeJournalOpen(const char *path) {
    Kh02NativeJournal *j;
    if(!path || !*path)return NULL;
    j=VirtualAlloc(NULL,sizeof(*j),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!j)return NULL;
    InitializeSRWLock(&j->lock);j->owner=GetCurrentThreadId();
    if(!load(&j->api) || j->api.open(path,&j->db,2|0x10000,NULL)!=0 || j->api.timeout(j->db,0)!=0 ||
       !execute(j,"PRAGMA synchronous=FULL") || !single(j,"PRAGMA synchronous","2") ||
       !single(j,"SELECT version FROM metadata","1") || !single(j,"PRAGMA quick_check","ok") || !audit(j) || !links(j)) {
        if(j->db)j->api.close(j->db);if(j->api.module)FreeLibrary(j->api.module);VirtualFree(j,0,MEM_RELEASE);return NULL;
    }
    return j;
}
static Bytes part(const Kh02CanonicalAssociation *p,Kh02CanonicalPart value) {
    Bytes out={(const unsigned char*)p+value.offset,value.size};return out;
}
static int bounds(const Kh02CanonicalAssociation *p,Kh02CanonicalPart value) {
    return value.offset>=sizeof(*p) && value.offset<=p->size && value.size<=p->size-value.offset;
}
uint32_t Kh02NativeJournalAppend(Kh02NativeJournal *j,const Kh02CanonicalAssociation *p,char *binding) {
    Statement *s=NULL;Bytes publication[3],scope,record,members,parts[3];char hash[65],record_hash[65],record_digest[65];
    uint32_t status=KH02_JOURNAL_FAILED,i;int step,existing;
    if(!binding)return KH02_JOURNAL_REFUSED;binding[0]=0;
    if(!j || !p || p->size<sizeof(*p) || p->size>20*1024*1024 || p->set_count>64 ||
       !bounds(p,p->token) || !bounds(p,p->container) || !bounds(p,p->scope) || !bounds(p,p->association) ||
       !p->token.size || p->token.size>1024 || p->container.size!=KH02_BIND_SIZE || !p->scope.size || !p->association.size)return KH02_JOURNAL_REFUSED;
    for(i=0;i<p->set_count;i++)if(!bounds(p,p->sets[i]) || !p->sets[i].size)return KH02_JOURNAL_REFUSED;
    for(i=0;i<100;i++)if(p->records[i]!=UINT32_MAX && p->records[i]>=p->set_count)return KH02_JOURNAL_REFUSED;
    if(!TryAcquireSRWLockExclusive(&j->lock))return KH02_JOURNAL_BUSY;
    if(j->closed){ReleaseSRWLockExclusive(&j->lock);return KH02_JOURNAL_CLOSED;}
    publication[0]=part(p,p->token);publication[1]=part(p,p->container);publication[2]=part(p,p->association);scope=part(p,p->scope);
    if(!digest(publication,3,1,hash) || !execute(j,"BEGIN IMMEDIATE"))goto done;
    if(!prepare(j,"SELECT container,association,binding FROM publications WHERE token=?",&s) || !bind(j,s,1,publication[0],1))goto done;
    step=j->api.step(s);existing=step==100;
    if(existing && (!equal(j,s,0,publication[1]) || !equal(j,s,1,publication[2]) || !equal(j,s,2,(Bytes){(unsigned char*)hash,64})))goto done;
    if(step!=100 && step!=101)goto done;
    if(j->api.finalize(s)!=0){s=NULL;goto done;}s=NULL;
    for(i=0;i<100;i++)if(p->records[i]!=UINT32_MAX) {
        record=(Bytes){publication[1].p+0xb0+i*KH02_BIND_RECORD,KH02_BIND_RECORD};members=part(p,p->sets[p->records[i]]);
        parts[0]=scope;parts[1]=record;parts[2]=members;
        if(!digest(&record,1,0,record_hash) || !digest(parts,3,1,record_digest) ||
           !prepare(j,"SELECT record,members,digest FROM records WHERE scope=? AND record_hash=?",&s) ||
           !bind(j,s,1,scope,0) || !bind(j,s,2,(Bytes){(unsigned char*)record_hash,64},1))goto done;
        step=j->api.step(s);
        if(step==100 && (!equal(j,s,0,record) || !equal(j,s,1,members) || !equal(j,s,2,(Bytes){(unsigned char*)record_digest,64})))goto done;
        if(step!=100 && step!=101)goto done;
        if(j->api.finalize(s)!=0){s=NULL;goto done;}s=NULL;
        if(step==101) {
            if(!prepare(j,"INSERT INTO records VALUES(?,?,?,?,?)",&s) || !bind(j,s,1,scope,0) ||
               !bind(j,s,2,(Bytes){(unsigned char*)record_hash,64},1) || !bind(j,s,3,record,0) || !bind(j,s,4,members,0) ||
               !bind(j,s,5,(Bytes){(unsigned char*)record_digest,64},1) || j->api.step(s)!=101)goto done;
            if(j->api.finalize(s)!=0){s=NULL;goto done;}s=NULL;
        }
        if(!point(1))goto done;
    }
    if(!existing) {
        if(!prepare(j,"INSERT INTO publications VALUES(?,?,?,?)",&s) || !bind(j,s,1,publication[0],1) ||
           !bind(j,s,2,publication[1],0) || !bind(j,s,3,publication[2],0) || !bind(j,s,4,(Bytes){(unsigned char*)hash,64},1) || j->api.step(s)!=101)goto done;
        if(j->api.finalize(s)!=0){s=NULL;goto done;}s=NULL;
    }
    if(!point(2) || j->closed || !execute(j,"COMMIT") || !j->api.autocommit(j->db) || !point(3) || j->closed)goto done;
    memcpy(binding,hash,65);status=KH02_JOURNAL_DURABLE;
done:
    if(s)j->api.finalize(s);
    if(!j->api.autocommit(j->db)) {
        if(!execute(j,"ROLLBACK") || !j->api.autocommit(j->db))InterlockedExchange(&j->closed,1);
    }
    ReleaseSRWLockExclusive(&j->lock);return status;
}
uint32_t Kh02NativeJournalClose(Kh02NativeJournal **owner) {
    Kh02NativeJournal *j;
    if(!owner)return KH02_JOURNAL_REFUSED;j=*owner;if(!j)return KH02_JOURNAL_DURABLE;
    if(j->owner!=GetCurrentThreadId())return KH02_JOURNAL_REFUSED;
    InterlockedExchange(&j->closed,1);
    if(!TryAcquireSRWLockExclusive(&j->lock))return KH02_JOURNAL_BUSY;
    if(j->db && j->api.close(j->db)!=0){ReleaseSRWLockExclusive(&j->lock);return KH02_JOURNAL_FAILED;}j->db=NULL;
    if(j->api.module){FreeLibrary(j->api.module);j->api.module=NULL;}
    ReleaseSRWLockExclusive(&j->lock);
    if(!VirtualFree(j,0,MEM_RELEASE))return KH02_JOURNAL_FAILED;*owner=NULL;return KH02_JOURNAL_DURABLE;
}
#ifdef KH02_NATIVE_JOURNAL_SELFTEST
MEMBER_API uint32_t Kh02NativeJournalTestPageLimit(Kh02NativeJournal *j,uint32_t pages) {
    char sql[80];int ok;
    if(!j || !pages || !TryAcquireSRWLockExclusive(&j->lock))return 0;
    snprintf(sql,sizeof(sql),"PRAGMA max_page_count=%u",pages);ok=execute(j,sql);
    ReleaseSRWLockExclusive(&j->lock);return ok;
}
#endif
