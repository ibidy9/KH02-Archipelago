#ifdef NDEBUG
#error Assertions must remain enabled
#endif
#include "../native/sdk_core_loader.hpp"
#include <cassert>
#include <iostream>
int wmain(int argc,wchar_t** argv){
 assert(argc==4);std::wstring mode=argv[1];std::string hash;for(const wchar_t c:std::wstring(argv[2])){assert(c<128);hash.push_back(static_cast<char>(c));}
 const char* accepted[]={hash.c_str()};auto path=kh02_sdk::runtime_core_path();assert(!path.empty());
 HMODULE module{};
 if(mode==L"decoy")module=LoadLibraryW(argv[3]);
 else if(mode!=L"absent")module=LoadLibraryW(path.c_str());
 if(mode!=L"absent")assert(module);
 auto core=kh02_sdk::loaded_core(path,accepted);
 if(mode==L"absent" || mode==L"decoy"){assert(core.state==kh02_sdk::CoreState::not_loaded && !core.pump);}
 else if(mode==L"wrong"){assert(core.state==kh02_sdk::CoreState::refused && !core.pump);}
 else{assert(mode==L"valid" && core.state==kh02_sdk::CoreState::ready && core.pump);
  Kh02SdkPumpStatus status{};assert(core.pump(1,sizeof(status),&status)==1 && status.sdk_commands==1 && status.pid==GetCurrentProcessId());}
 if(module){using Calls=unsigned(*)();auto calls=reinterpret_cast<Calls>(GetProcAddress(module,"Kh02MockCalls"));assert(calls);assert(calls()==(mode==L"valid"?1u:0u));}
 std::cout<<"PASS core loader: exact already-loaded module, path/hash, no implicit load or decoy calls\n";
}
