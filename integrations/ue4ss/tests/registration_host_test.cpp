// Compile the actual new backend against a tiny fake SDK. No UE4SS/core DLL is
// loaded into this executable and no process, save, or game is accessed.
#include <cstdint>
#include <cstddef>
#include <string>
#include <initializer_list>
#include <cassert>
#include <iostream>
#include <stdexcept>
#define STR(x) L##x
namespace RC {
enum class LogLevel {Default};
namespace Output {template<LogLevel,typename... Args> void send(const wchar_t*,Args&&...) {}}
}
namespace ue {
enum EFindName {FNAME_Find};
static bool missing_name=false;
struct FName {
 std::wstring value;
 FName()=default;
 FName(const wchar_t* p,EFindName){if(!missing_name)value=p;}
 std::wstring ToString()const{return value;}
};
struct UFunction {unsigned kind;};
struct UObject {unsigned role;void ProcessEvent(UFunction*,void*);};
}
static bool thread_ready=true,permit_ready=true,weak_ready=true,contract_ready=true,observe_ready=true;
static bool instance_mismatch=false,change_on_grant=false,throw_on_grant=false;
static unsigned grants,getters,exceptions;
static std::wstring last_key;
static ue::UObject controller{1},pawn{2},gameplay{3},library{4},instance{5},foreign{6};
static ue::UFunction getter{1},award{2};
struct PlainHint {
 uint64_t controller{},pawn{},revision{};bool weak_ready=true;
 std::string controller_name,pawn_name;
};
static PlainHint hint;
static bool bridge_thread(){return thread_ready;}
static bool hint_copy(PlainHint& out){out=hint;return true;}
static ue::UObject* hinted_instance(const PlainHint& p,bool){return weak_ready && p.pawn==reinterpret_cast<uintptr_t>(&pawn)?&pawn:nullptr;}
static bool live(void* p){return p!=nullptr;}
static bool native_observe(uint64_t,uint64_t,const std::string&,const std::string&,bool,const PlainHint*){return observe_ready;}
static ue::UObject* exact(const wchar_t* path){
 const std::wstring p(path);
 if(p==L"/Script/Engine.Default__GameplayStatics")return &gameplay;
 if(p==L"/Script/TresGame.Default__TresGameBlueprintLibrary")return &library;
 return nullptr;
}
struct Field {const wchar_t* name;int offset;const wchar_t* kind;};
static ue::UFunction* function(const wchar_t* path,uint32_t flags,unsigned count,unsigned size,unsigned result,std::initializer_list<Field> fields){
 assert(std::wstring(path)==L"/Script/Engine.GameplayStatics:GetGameInstance");
 assert(flags==0x14022401 && count==2 && size==16 && result==8 && fields.size()==2);
 return contract_ready?&getter:nullptr;
}
static ue::UFunction* grant_function(){return contract_ready?&award:nullptr;}
static uint64_t GetTickCount64(){return 1;}
static void bridge_exception(const wchar_t*,uint32_t,const char*){++exceptions;}
#define static_assert(...) // Fake FName owns a string; real host asserts native 8-byte layout separately.
#include "../native/registration_host.inc"
#undef static_assert
void ue::UObject::ProcessEvent(ue::UFunction* fn,void* opaque) {
 if(fn==&getter){
  assert(this==&gameplay); // CDO mismatch here would be an invalid real call.
  struct Params {ue::UObject* world;ue::UObject* result;};
  auto* p=static_cast<Params*>(opaque);assert(p->world==&pawn);++getters;
  p->result=instance_mismatch?&foreign:&instance;return;
 }
 assert(fn==&award && this==&library);
 struct Params {ue::UObject* world;ue::FName name;uint8_t show,reserved[7];};
 auto* p=static_cast<Params*>(opaque);assert(p->world==&pawn && p->show==1);
 for(auto v:p->reserved)assert(!v);
 ++grants;last_key=p->name.ToString();
 if(change_on_grant)++hint.revision;
 if(throw_on_grant)throw std::runtime_error("simulated post-entry exception");
}
static uint32_t permit(void*){return permit_ready;}
static Kh02RegistrationRequest request(uint32_t code=0x600){
 thread_ready=permit_ready=weak_ready=contract_ready=observe_ready=true;
 instance_mismatch=change_on_grant=throw_on_grant=ue::missing_name=false;
 grants=getters=exceptions=0;last_key.clear();
 hint={reinterpret_cast<uintptr_t>(&controller),reinterpret_cast<uintptr_t>(&pawn),1,true,"controller","pawn"};
 Kh02RegistrationRequest p{};p.size=sizeof(p);p.version=1;p.pawn=hint.pawn;
 p.context=reinterpret_cast<uintptr_t>(&instance);p.code=code;p.show=1;
 p.permit=permit;p.owner_context=&instance;return p;
}
int main(){
 {auto p=request();instance_mismatch=true;RegistrationBackend b;
  assert(!b.prepare(p) && !p.entered && !grants);
  assert(std::wstring(b.stage)==L"game-instance-identity");}
 {auto p=request();contract_ready=false;RegistrationBackend b;
  assert(!b.prepare(p) && !p.entered && !grants);
  assert(std::wstring(b.stage)==L"reflection-contract");}
 {Kh02RegistrationApi api{};assert(!Kh02UE4SSGetRegistrationApiV1(2,sizeof(api),&api));
  assert(!Kh02UE4SSGetRegistrationApiV1(1,0,&api));assert(!Kh02UE4SSGetRegistrationApiV1(1,sizeof(api),nullptr));
  assert(Kh02UE4SSGetRegistrationApiV1(1,sizeof(api),&api)==1 && api.version==1 && api.request_size==sizeof(Kh02RegistrationRequest) && api.invoke && !api.reserved);}

 {auto p=request();assert(registration_invoke(&p)==1 && p.entered && !p.fault && grants==1 && getters>=3 && last_key==L"DEC_ITEM_01");
  assert(!registration_invoke(&p) && grants==1);}
 {auto p=request(0xc02);assert(registration_invoke(&p)==1 && last_key==L"NAVI_MAP_DW03" && grants==1);}
 for(unsigned mode=0;mode<9;++mode){auto p=request();
  switch(mode){case 0:thread_ready=false;break;case 1:permit_ready=false;break;
   case 2:weak_ready=false;break;case 3:contract_ready=false;break;case 4:observe_ready=false;break;
   case 5:instance_mismatch=true;break;case 6:ue::missing_name=true;break;
   case 7:p.revision=2;break;case 8:hint.weak_ready=false;break;}
  assert(!registration_invoke(&p) && !p.entered && !grants);
 }
 {auto p=request();change_on_grant=true;assert(!registration_invoke(&p) && p.entered && p.fault && grants==1);}
 {auto p=request();throw_on_grant=true;assert(!registration_invoke(&p) && p.entered && p.fault && !p.busy && grants==1 && exceptions==1);}
 std::cout<<"PASS actual registration backend: CDO/receiver, getter contract, independent context, exact keys, stale/weak/permit refusal, epoch change and exception\n";
}
