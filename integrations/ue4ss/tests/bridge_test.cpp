#include "../native/dispatch_log.hpp"
#include "../native/context_lifetime.hpp"
#include "../native/weak_identity.hpp"
#include "../native/bridge_dispatch.hpp"
#ifdef NDEBUG
#error Dispatcher test assertions must be enabled
#endif
#include <cassert>
#include <stdexcept>
#include <iostream>
using namespace kh02_bridge;
static uint32_t permit(void*){return 1;}
static Kh02UE4SSRequest request(){Kh02UE4SSRequest p{};p.size=sizeof(p);p.version=1;p.pawn=0x10000;p.permit=permit;static int owner;p.owner_context=&owner;return p;}
struct Backend {
 bool ready=true,fail_convert=false,fail_grant=false,revoke=false,throw_grant=false;
 unsigned resolves{},conversions{},grants{};
 bool resolve(Kh02UE4SSRequest& p,uint64_t& world){++resolves;world=ready?p.pawn:0;return ready;}
 bool convert(Kh02UE4SSRequest& p,Kh02UE4SSConvert& f){
  ++conversions;uint64_t w=0;assert(!check_request(&p,&w,*this)); // Reentry while busy refuses.
  f.result={100u+f.battle,0};if(revoke)ready=false;return !fail_convert;
 }
 bool grant(Kh02UE4SSRequest& p,const Kh02UE4SSGrant&){
  ++grants;assert(p.entered==1);if(throw_grant)throw std::runtime_error("after entry");
  if(revoke)ready=false;return !fail_grant;
 }
};
static void converted(Kh02UE4SSRequest& p,Backend& b,unsigned battle=0){
 Kh02UE4SSConvert f{};f.world=p.pawn;f.battle=static_cast<uint8_t>(battle);
 assert(invoke(&p,1,&f,sizeof(f),b)==1 && p.converted && f.result.index==100+battle);
}
static Kh02UE4SSGrant grant_frame(const Kh02UE4SSRequest& p){Kh02UE4SSGrant f{};f.world=p.pawn;f.name=p.name;f.show=1;return f;}
void weak_identity_tests(){
 struct Ref {int* object;int captured,current;bool valid;int* Get()const{return valid && captured>0 && captured==current?object:nullptr;}};
 int a=1,b=2,reads=0;auto validate=[&](int* p){++reads;return p==&a;};
 const auto id=reinterpret_cast<std::uintptr_t>(&a);
 assert(kh02_host::checked_weak_identity(Ref{&a,7,7,true},id,validate)==&a && reads==1);
 reads=0;
 assert(!kh02_host::checked_weak_identity(Ref{&a,7,8,true},id,validate) && !reads); // Same address, new serial.
 assert(!kh02_host::checked_weak_identity(Ref{&a,0,0,true},id,validate) && !reads);
 assert(!kh02_host::checked_weak_identity(Ref{&a,7,7,false},id,validate) && !reads);
 assert(!kh02_host::checked_weak_identity(Ref{&b,7,7,true},id,validate) && !reads);
 assert(!kh02_host::checked_weak_identity(Ref{&a,7,7,true},0,validate) && !reads);
 auto reject=[&](int*){++reads;return false;};
 assert(!kh02_host::checked_weak_identity(Ref{&a,7,7,true},id,reject) && reads==1);
 std::cout<<"PASS weak identity: slot reuse/serial, zero serial, dead object, wrong address, zero expected, validation refusal\n";
}
void context_lifetime_tests(){
 std::uint64_t revision=0;
 for(unsigned i=0;i<10000;++i){assert(kh02_host::advance_context(revision));assert(revision==i+1);}
 revision=std::numeric_limits<std::uint64_t>::max()-1;
 assert(kh02_host::advance_context(revision));assert(!kh02_host::advance_context(revision));
 assert(revision==std::numeric_limits<std::uint64_t>::max());
 std::atomic_flag flag=ATOMIC_FLAG_INIT;
 {kh02_host::ObservationLease first(flag);assert(first);
  {kh02_host::ObservationLease nested(flag);assert(!nested);}
  kh02_host::ObservationLease still_nested(flag);assert(!still_nested);}
 {kh02_host::ObservationLease next(flag);assert(next);}
 try{kh02_host::ObservationLease fail(flag);assert(fail);throw std::runtime_error("unwind");}catch(const std::runtime_error&){}
 {kh02_host::ObservationLease after(flag);assert(after);}
 std::cout<<"PASS context lifetime: 10000 revisions, overflow refusal, reentry exclusion and exception release\n";
}
void dispatch_log_tests(){
 kh02_sdk::DispatchLog log;Kh02SdkPumpStatus status{};status.core_status=1;status.window_commands=63;status.sdk_commands=1;
 assert(log.observe(status,0));
 for(unsigned i=1;i<5000;++i){status.sdk_commands=i+1;assert(!log.observe(status,i));}
 ++status.sdk_commands;assert(log.observe(status,5000));
 assert(!log.observe(status,10000)); // no new work, no periodic spam
 status.core_status=2;assert(log.observe(status,10001));assert(!log.observe(status,10002));
 status.window_commands=64;assert(log.observe(status,10003));
 status.sdk_commands=1;assert(log.observe(status,10004));
 ++status.sdk_commands;assert(!log.observe(status,9999)); // clock regression cannot flood
 std::cout<<"PASS dispatch log gate: first/status/bootstrap/counter anomaly and periodic summaries; unchanged pumping\n";
}
int main(){
 dispatch_log_tests();
 context_lifetime_tests();
 weak_identity_tests();
 static_assert(sizeof(Kh02UE4SSRequest)==80 && sizeof(Kh02UE4SSApi)==32);
 for(unsigned i=0;i<7;++i){auto p=request();Backend b;converted(p,b,i);auto f=grant_frame(p);assert(invoke(&p,2,&f,24,b)==1 && b.grants==1);assert(!invoke(&p,2,&f,24,b) && b.grants==1);uint64_t w=0;assert(check_request(&p,&w,b)==1);}
 for(unsigned i=0;i<7;++i){auto p=request();Backend b;switch(i){case 0:p.version=2;break;case 1:p.size=0;break;case 2:p.pawn=0;break;case 3:p.permit=nullptr;break;case 4:p.owner_context=nullptr;break;case 5:p.reserved=1;break;case 6:p.fault=1;break;}uint64_t w=99;assert(!check_request(&p,&w,b) && w==0 && b.resolves==0);}
 {auto p=request();Backend b;b.ready=false;uint64_t w=0;assert(!check_request(&p,&w,b));assert(!p.entered && !b.grants);}
 {auto p=request();Backend b;auto f=grant_frame(p);assert(!invoke(&p,2,&f,24,b) && !b.grants);}
 for(unsigned i=0;i<5;++i){auto p=request();Backend b;converted(p,b);auto f=grant_frame(p);switch(i){case 0:f.world++;break;case 1:f.name.index++;break;case 2:f.name.number++;break;case 3:f.show=2;break;case 4:f.reserved[3]=1;break;}assert(!invoke(&p,2,&f,24,b) && !b.grants && !p.entered && !p.busy);}
 {auto p=request();Backend b;Kh02UE4SSConvert f{};f.world=p.pawn;f.battle=7;assert(!invoke(&p,1,&f,24,b));f.battle=0;f.reserved[0]=1;assert(!invoke(&p,1,&f,24,b));f.reserved[0]=0;f.result.index=1;assert(!invoke(&p,1,&f,24,b));assert(!b.conversions);}
 {auto p=request();Backend b;Kh02UE4SSConvert f{};f.world=p.pawn;assert(!invoke(&p,0,&f,24,b));assert(!invoke(&p,1,&f,23,b));assert(!invoke(&p,1,nullptr,24,b));assert(!b.resolves);}
 {auto p=request();Backend b;b.fail_convert=true;Kh02UE4SSConvert f{};f.world=p.pawn;assert(!invoke(&p,1,&f,24,b));assert(!p.converted && !p.entered && !p.busy && !f.result.index);}
 {auto p=request();Backend b;b.revoke=true;Kh02UE4SSConvert f{};f.world=p.pawn;assert(!invoke(&p,1,&f,24,b));assert(!p.converted && !p.entered && !b.grants);}
 {auto p=request();Backend b;converted(p,b);b.fail_grant=true;auto f=grant_frame(p);assert(!invoke(&p,2,&f,24,b));assert(p.fault && p.entered && !p.busy);assert(!invoke(&p,2,&f,24,b) && b.grants==1);}
 {auto p=request();Backend b;converted(p,b);b.revoke=true;auto f=grant_frame(p);assert(!invoke(&p,2,&f,24,b) && p.fault && p.entered);}
 {auto p=request();Backend b;converted(p,b);b.throw_grant=true;auto f=grant_frame(p);bool threw=false;try{invoke(&p,2,&f,24,b);}catch(const std::runtime_error&){threw=true;}assert(threw && p.fault && p.entered && !p.busy);}
 std::cout<<"PASS receipt host dispatch: seven items; invalid ABI/owner/context/frames; reentry; converter failure/revocation; single entry; uncertain failure/exception after entry\n";
}
