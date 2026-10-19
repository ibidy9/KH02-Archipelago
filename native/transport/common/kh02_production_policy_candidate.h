#ifndef KH02_PRODUCTION_POLICY_CANDIDATE_H
#define KH02_PRODUCTION_POLICY_CANDIDATE_H
#include <stdint.h>
#ifdef _WIN32
#define KH02_POLICY_API __declspec(dllexport)
#else
#define KH02_POLICY_API
#endif

enum { KH02_POLICY_OK=1, KH02_POLICY_REFUSED=2, KH02_POLICY_BUSY=3, KH02_POLICY_UNCERTAIN=4 };
enum { KH02_POLICY_MAGIC=0x4b485050, KH02_POLICY_VERSION=1, KH02_ITEM_SCHEMA_VERSION=2 };
enum { KH02_ABILITY_MASK=0x1b, KH02_SPELL_MASK=0x0f, KH02_WORLD_MASK=0x0f };

typedef struct Kh02PolicyWire {
    uint32_t magic,version,size,item_schema_version;
    uint64_t owner_cookie,process_cookie,authority;
    uint32_t revision_low,revision_high;
    unsigned char scope_binding[32];
    uint32_t hp_items,mp_items,ability_mask,spell_mask,world_mask,reserved;
} Kh02PolicyWire;

typedef struct Kh02PolicyState {
    uint64_t owner_cookie,process_cookie,authority,revision,generation;
    unsigned char scope_binding[32];
    uint32_t hp_items,mp_items,ability_mask,spell_mask,world_mask;
    uint32_t installed,busy,faulted,reserved;
} Kh02PolicyState;

typedef struct Kh02PolicyProjection {
    uint64_t revision,generation;
    uint32_t hp_items,mp_items,ability_mask,spell_mask,world_mask,reserved;
} Kh02PolicyProjection;
typedef uint32_t (*Kh02PolicyApply)(void*,const Kh02PolicyProjection*);
typedef uint32_t (*Kh02PolicyCommit)(void*,const unsigned char*,uint32_t);

KH02_POLICY_API uint32_t Kh02PolicyInit(Kh02PolicyState*,uint64_t,uint64_t,const unsigned char*);
KH02_POLICY_API uint32_t Kh02PolicyInstall(Kh02PolicyState*,const Kh02PolicyWire*,uint64_t);
KH02_POLICY_API uint32_t Kh02PolicyProject(Kh02PolicyState*,uint64_t,uint64_t,Kh02PolicyProjection*);
KH02_POLICY_API uint32_t Kh02PolicyFinish(Kh02PolicyState*,uint64_t,uint32_t);
KH02_POLICY_API uint32_t Kh02PolicyRevoke(Kh02PolicyState*,uint64_t);
KH02_POLICY_API uint32_t Kh02PolicyScopeBinding(uint32_t,uint32_t,const unsigned char*,uint32_t,unsigned char*);
KH02_POLICY_API uint32_t Kh02PolicyProcessChallenge(uint64_t*);
KH02_POLICY_API uint32_t Kh02PolicyTransaction(Kh02PolicyState*,const Kh02PolicyWire*,
    uint64_t,uint64_t,const unsigned char*,uint32_t,Kh02PolicyApply,Kh02PolicyCommit,void*);
#endif
