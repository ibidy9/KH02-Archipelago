#include "../native/registration_policy.hpp"
#include "../native/registration_catalog.hpp"
#include <cwchar>
#include <cassert>
#include <iostream>
#include <stdexcept>
static uint32_t permit(void*){return 1;}
static Kh02RegistrationRequest request(){Kh02RegistrationRequest p{};p.size=sizeof(p);p.version=1;p.pawn=0x10000;p.context=0x20000;p.code=0x600;p.show=1;p.permit=permit;static int owner;p.owner_context=&owner;return p;}
struct Backend {
 bool ready=true,prepared=true,fail=false,revoke=false,throwing=false,mutate_prepare=false,mutate_validate=false;
 unsigned calls{},validations{};
 bool prepare(Kh02RegistrationRequest& p){assert(p.busy==1);if(mutate_prepare)p.code=0xc00;return prepared;}
 bool validate(Kh02RegistrationRequest& p){++validations;if(mutate_validate)p.pawn=0x30000;return ready;}
 bool grant(Kh02RegistrationRequest& p){assert(p.entered==1);++calls;
  assert(!kh02_registration::invoke(&p,*this));
  if(throwing)throw std::runtime_error("after entry");if(revoke)ready=false;return !fail;}
};
int main(){
 unsigned allowed=0;
 for(unsigned c=0;c<0x1000;++c){assert(kh02_registration::allowed(c)==(kh02_registration::key_for_code(c)!=nullptr));if(kh02_registration::allowed(c))++allowed;}
 assert(allowed==55);assert(std::wcscmp(kh02_registration::key_for_code(0x600),L"DEC_ITEM_01")==0);assert(std::wcscmp(kh02_registration::key_for_code(0xc03),L"NAVI_MAP_DW04")==0);assert(!kh02_registration::allowed(0x62d));
 for(unsigned code:{0x600u,0x633u,0xc00u,0xc03u}){
  auto p=request();p.code=code;Backend b;assert(kh02_registration::invoke(&p,b)==1 && p.entered && !p.fault && !p.busy && b.calls==1);
  assert(!kh02_registration::invoke(&p,b) && b.calls==1);
 }
 for(unsigned mode=0;mode<12;++mode){auto p=request();Backend b;
  switch(mode){case 0:p.size=0;break;case 1:p.version=2;break;case 2:p.pawn=0;break;
   case 3:p.context=0;break;case 4:p.code=0x62d;break;case 5:p.show=0;break;
   case 6:p.permit=nullptr;break;case 7:p.owner_context=nullptr;break;case 8:p.entered=1;break;
   case 9:p.fault=1;break;case 10:p.busy=1;break;case 11:p.reserved=1;break;}
  assert(!kh02_registration::invoke(&p,b) && !b.calls);
 }
 {auto p=request();Backend b;b.ready=false;assert(!kh02_registration::invoke(&p,b) && !p.entered && !b.calls);}
 {auto p=request();Backend b;b.prepared=false;assert(!kh02_registration::invoke(&p,b) && !p.entered && !b.calls);}
 for(unsigned mode=0;mode<2;++mode){auto p=request();Backend b;if(mode)b.revoke=true;else b.fail=true;
  assert(!kh02_registration::invoke(&p,b) && p.entered && p.fault && !p.busy && b.calls==1);
  assert(!kh02_registration::invoke(&p,b) && b.calls==1);}
 {auto p=request();Backend b;b.throwing=true;try{kh02_registration::invoke(&p,b);assert(false);}catch(const std::runtime_error&){}
  assert(p.entered && p.fault && !p.busy && b.calls==1);}
 for(unsigned mode=0;mode<2;++mode){auto p=request();Backend b;if(mode)b.mutate_prepare=true;else b.mutate_validate=true;assert(!kh02_registration::invoke(&p,b) && !p.entered && !b.calls);}
 std::cout<<"PASS registration closed catalog, ABI, reentry, pre-entry refusal, consumed failure, exception and no duplicate\n";
}
