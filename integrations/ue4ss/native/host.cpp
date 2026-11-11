// UE4SS engine adapter for the shared receipt owner and scoped gameplay helpers.
// Shared-owner/receipt authorization remains in the C transport and adapter.
#define NOMINMAX
#include <Windows.h>
#include <Mod/CppUserModBase.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include "mines_notice_policy.hpp"
#include <Unreal/UObjectArray.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/FWeakObjectPtr.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "thread_policy.hpp"
#include "object_path.hpp"
#include "object_identity.hpp"
#include "bridge_dispatch.hpp"
#include "weak_identity.hpp"
#include "context_lifetime.hpp"
#include <mutex>
// Diagnostic-only expressions; all validation remains outside this macro.
#define KH02_TRACE(...) ((void)0)
namespace ue = RC::Unreal;

namespace {
std::atomic<bool> active{false}, engine_ready{false};
std::atomic_flag observing = ATOMIC_FLAG_INIT;

struct Windows {
    DWORD pid{}, count{}, thread{}, window_pid{};
};

BOOL CALLBACK own_window(HWND window, LPARAM opaque) {
    auto &result = *reinterpret_cast<Windows *>(opaque);
    DWORD pid = 0;
    const DWORD thread = GetWindowThreadProcessId(window, &pid);
    if (pid == result.pid && IsWindowVisible(window) && GetWindow(window, GW_OWNER) == nullptr) {
        ++result.count;
        result.thread = thread;
        result.window_pid = pid;
    }
    return TRUE;
}

bool live(ue::UObject *object) {
    if (!object)
        return false;
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] CHECK begin object=0x{:X}\n"),
        reinterpret_cast<std::uintptr_t>(object)));
    const auto index = object->GetInternalIndex();
    KH02_TRACE(
        RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptHost] CHECK index={}\n"), index));
    const auto count = ue::UObjectArray::GetNumElements();
    KH02_TRACE(
        RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptHost] CHECK count={}\n"), count));
    if (index < 0 || index >= count)
        return false;
    const auto *item = object->GetObjectItem();
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptHost] CHECK slot=0x{:X}\n"),
                                                       reinterpret_cast<std::uintptr_t>(item)));
    const bool valid = item && item->GetUObject() == object &&
                       ue::UObjectArray::IsValid(const_cast<ue::FUObjectItem *>(item), false);
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] CHECK end valid={}\n"), valid));
    return valid;
}

ue::UObject *exact(const std::wstring &path) {
    if (!path.starts_with(L"/Script/"))
        return nullptr;
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] LOOKUP begin path={}\n"), path));
    auto *object = ue::UObjectGlobals::StaticFindObject(nullptr, nullptr, path.c_str());
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] LOOKUP returned object=0x{:X}\n"),
        reinterpret_cast<std::uintptr_t>(object)));
    return live(object) ? object : nullptr;
}

ue::UObject *fresh_instance(const std::wstring &path, std::uint64_t expected, const wchar_t *role) {
    // Object is mandatory SDK initialization state. No optional Controller/Pawn
    // class-name lookup, no untyped name cache and no actor snapshot pool.
    // The original exact concrete classes and independent getters are checked
    // after this lookup; the accepted player/pawn contract is unchanged.
    auto *base = ue::UObject::StaticClass();
    if (!live(base) || base->GetClassPrivate() != ue::UClass::StaticClass() ||
        base->GetNamePrivate().ToString() != L"Object")
        return nullptr;
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] FRESH begin path={} expected=0x{:X} role={} filter=SDK.Object\n"),
        path, expected, role));
    auto *found = ue::UObjectGlobals::StaticFindObject(base, nullptr, path.c_str());
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] FRESH returned object=0x{:X} matches={}\n"),
        reinterpret_cast<std::uintptr_t>(found),
        found && reinterpret_cast<std::uintptr_t>(found) == expected));
    return kh02_host::validate_identity(found, expected, live);
}

std::wstring path_from_full(const std::string &name) {
    const auto path = kh02_host::copied_object_path(name);
    return path.empty() ? std::wstring{} : RC::to_wstring(path);
}

struct Field {
    const wchar_t *name;
    int offset;
    const wchar_t *kind;
};

ue::UFunction *function(const wchar_t *path, std::uint32_t flags, unsigned count, unsigned size,
                        unsigned result, std::initializer_list<Field> fields) {
    auto *object = exact(path);
    if (!object || object->GetClassPrivate() != ue::UFunction::StaticClass()) {
        RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptHost] FUNCTION_REFUSED path={}\n"),
                                                path);
        return nullptr;
    }
    auto *fn = static_cast<ue::UFunction *>(object);
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] CONTRACT path={} flags=0x{:X} count={} size={} return={}\n"), path,
        static_cast<unsigned>(fn->GetFunctionFlags()), static_cast<unsigned>(fn->GetNumParms()),
        fn->GetParmsSize(), fn->GetReturnValueOffset()));
    if (fn->GetFunctionFlags() != flags || fn->GetNumParms() != count ||
        fn->GetParmsSize() != size || fn->GetReturnValueOffset() != result) {
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02ReceiptHost] CONTRACT_REFUSED path={} flags=0x{:X} count={} size={} return={}\n"),
            path, static_cast<unsigned>(fn->GetFunctionFlags()),
            static_cast<unsigned>(fn->GetNumParms()), fn->GetParmsSize(),
            fn->GetReturnValueOffset());
        return nullptr;
    }
    for (const auto &field : fields) {
        auto *property = fn->FindProperty(ue::FName(field.name));
        if (!property || property->GetOffset_Internal() != field.offset ||
            property->GetClass().GetFName().ToString() != field.kind) {
            RC::Output::send<RC::LogLevel::Default>(
                STR("[KH02ReceiptHost] FIELD_REFUSED path={} field={}\n"), path, field.name);
            return nullptr;
        }
    }
    return fn;
}

struct ControllerParams {
    ue::UObject *world{};
    std::int32_t index{};
    std::uint32_t pad{};
    ue::UObject *result{};
};

struct PawnParams {
    ue::UObject *result{};
};

struct Name {
    std::uint32_t index{}, number{};
};

struct ConvertParams {
    ue::UObject *world{};
    std::uint8_t battle{}, reserved[7]{};
    Name result{};
};

static_assert(sizeof(ControllerParams) == 24 && offsetof(ControllerParams, result) == 16);
static_assert(sizeof(PawnParams) == 8 && sizeof(ConvertParams) == 24 &&
              offsetof(ConvertParams, result) == 16);
static_assert(sizeof(RC::CppUserModBase) == 192, "Pinned SDK /MD release layout");
static_assert(sizeof(ue::FName) == 8, "Supported non-case-preserving runtime only");
ue::UFunction *grant_function();
bool bridge_thread();
void sdk_owner_post(ue::UObject *, ue::UFunction *, void *);

struct PlainHint {
    std::uint64_t controller{}, pawn{}, revision{};
    std::string controller_name, pawn_name;
    ue::FWeakObjectPtr controller_ref{}, pawn_ref{};
    bool weak_ready{};
};

static_assert(sizeof(ue::FWeakObjectPtr) == 8, "Pinned SDK weak-reference layout");

ue::UObject *hinted_instance(const PlainHint &hint, bool controller) {
    const auto id = controller ? hint.controller : hint.pawn;
    if (hint.weak_ready) {
        // SDK checks slot serial and validity before returning a pointer. Compare
        // the returned identity BEFORE any object reads. A stale ready reference
        // fails closed; it never falls back to an address/name search.
        const auto &ref = controller ? hint.controller_ref : hint.pawn_ref;
        return kh02_host::checked_weak_identity(ref, id, live);
    }
    // A zero/unavailable engine serial at initial capture is not invented or
    // allocated by this mod. Preserve the passed exact-name path in that case.
    return fresh_instance(path_from_full(controller ? hint.controller_name : hint.pawn_name), id,
                          controller ? L"controller" : L"pawn");
}

std::mutex hint_mutex;
PlainHint latest_hint;

bool store_hint(ue::UObject *controller, ue::UObject *pawn, const std::string &cn,
                const std::string &pn) {
    // Called only after the original fresh lookup, type and independent getter
    // checks on this event's verified objects. Retain weak handles, not UObjects.
    PlainHint next{};
    next.controller = reinterpret_cast<std::uintptr_t>(controller);
    next.pawn = reinterpret_cast<std::uintptr_t>(pawn);
    next.controller_name = cn;
    next.pawn_name = pn;
    next.controller_ref = controller;
    next.pawn_ref = pawn;
    const bool controller_ready = next.controller_ref.Get() == controller,
               pawn_ready = next.pawn_ref.Get() == pawn;
    next.weak_ready = controller_ready && pawn_ready;
    RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptBridge] WEAK_CONTEXT ready={} controller_ready={} pawn_ready={} fallback={}\n"),
        next.weak_ready, controller_ready, pawn_ready, !next.weak_ready);
    std::lock_guard lock(hint_mutex);
    next.revision = latest_hint.revision;
    if (!kh02_host::advance_context(next.revision)) {
        latest_hint = PlainHint{};
        latest_hint.revision = next.revision;
        return false;
    }
    latest_hint = next;
    RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptBridge] CONTEXT_ACCEPT revision={}\n"),
                                            next.revision);
    return true;
}

bool hint_copy(PlainHint &out) {
    std::unique_lock lock(hint_mutex, std::try_to_lock);
    if (!lock.owns_lock() || !latest_hint.revision || !latest_hint.controller || !latest_hint.pawn)
        return false;
    out = latest_hint;
    return true;
}

bool invalidate_hint() {
    if (!bridge_thread())
        return false;
    std::lock_guard lock(hint_mutex);
    auto revision = latest_hint.revision;
    const bool advanced = kh02_host::advance_context(revision);
    latest_hint = PlainHint{};
    latest_hint.revision = revision;
    RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptBridge] CONTEXT_INVALIDATE revision={} advanced={}\n"), revision,
        advanced);
    return advanced;
}

bool native_observe(std::uint64_t controller_id, std::uint64_t pawn_id,
                    const std::string &controller_name, const std::string &pawn_name,
                    bool publish_hint = true, const PlainHint *captured = nullptr) {
    auto refuse = [](const wchar_t *reason) {
        RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptHost] REFUSE reason={}\n"),
                                                reason);
        return false;
    };
    Windows windows{};
    windows.pid = GetCurrentProcessId();
    EnumWindows(own_window, reinterpret_cast<LPARAM>(&windows));
    const DWORD thread = GetCurrentThreadId();
    const kh02_host::ThreadEvidence evidence{windows.pid, thread, windows.window_pid,
                                             windows.thread, windows.count};
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] THREAD pid={} callback={} window={} count={} same={}\n"),
        windows.pid, thread, windows.thread, windows.count,
        kh02_host::same_owner_thread(evidence)));
    if (!active || !engine_ready || !kh02_host::same_owner_thread(evidence))
        return refuse(L"host_or_thread_not_ready");
    KH02_TRACE(
        RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptHost] PHASE copy_paths_begin\n")));
    const auto controller_path = path_from_full(controller_name),
               pawn_path = path_from_full(pawn_name);
    KH02_TRACE(
        RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptHost] PHASE copy_paths_end\n")));
    if (controller_path.empty() || pawn_path.empty())
        return refuse(L"invalid_object_path");
    // Integers supplied by Lua are compared, never cast/dereferenced as UObjects.
    auto *controller = captured ? hinted_instance(*captured, true)
                                : fresh_instance(controller_path, controller_id, L"controller");
    auto *pawn =
        captured ? hinted_instance(*captured, false) : fresh_instance(pawn_path, pawn_id, L"pawn");
    if (!controller || !pawn || reinterpret_cast<std::uintptr_t>(controller) != controller_id ||
        reinterpret_cast<std::uintptr_t>(pawn) != pawn_id)
        return refuse(L"fresh_object_identity_mismatch");
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] PHASE controller_class_begin\n")));
    auto *controller_class = controller->GetClassPrivate();
    if (!live(controller_class) ||
        controller_class->GetNamePrivate().ToString() != L"TresPlayerControllerSora")
        return refuse(L"controller_class");
    KH02_TRACE(
        RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptHost] PHASE pawn_class_begin\n")));
    auto *pawn_class = pawn->GetClassPrivate();
    if (!live(pawn_class) || pawn_class->GetNamePrivate().ToString() != L"p_ex002_Pawn_C")
        return refuse(L"pawn_class");
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] PHASE getter_contract_begin\n")));
    auto *gameplay = exact(L"/Script/Engine.Default__GameplayStatics");
    auto *get_controller =
        function(L"/Script/Engine.GameplayStatics:GetPlayerController", 0x14022401, 3, 24, 16,
                 {{L"WorldContextObject", 0, L"ObjectProperty"},
                  {L"PlayerIndex", 8, L"IntProperty"},
                  {L"ReturnValue", 16, L"ObjectProperty"}});
    auto *get_pawn = function(L"/Script/Engine.Controller:K2_GetPawn", 0x54020401, 1, 8, 0,
                              {{L"ReturnValue", 0, L"ObjectProperty"}});
    if (!gameplay || !get_controller || !get_pawn)
        return refuse(L"getter_contract");
    ControllerParams pc{};
    pc.world = controller;
    gameplay->ProcessEvent(get_controller, &pc);
    if (pc.result != controller || !live(controller) || !live(pawn))
        return refuse(L"native_controller_mismatch");
    PawnParams possessed{};
    controller->ProcessEvent(get_pawn, &possessed);
    if (possessed.result != pawn || !live(controller) || !live(pawn))
        return refuse(L"native_pawn_mismatch");
    KH02_TRACE(RC::Output::send<RC::LogLevel::Default>(
        STR("[KH02ReceiptHost] NATIVE_CONTEXT controller=0x{:X} pawn=0x{:X} matches=true authority=false\n"),
        controller_id, pawn_id));
    if (publish_hint) {
        // Optional read-only conversion: a missing early catalog is reported, not
        // treated as authority or permission to call the item grant.
        auto *library = exact(L"/Script/TresGame.Default__TresGameBlueprintLibrary");
        auto *converter = function(
            L"/Script/TresGame.TresGameBlueprintLibrary:BP_TresConvertBattleItemIDtoKeyName",
            0x14022401, 3, 24, 16,
            {{L"WorldContextObject", 0, L"ObjectProperty"},
             {L"InBattleItemID", 8, L"ByteProperty"},
             {L"ReturnValue", 16, L"NameProperty"}});
        if (library && converter)
            for (unsigned battle = 0; battle < 2; ++battle) {
                ConvertParams params{};
                params.world = controller;
                params.battle = static_cast<std::uint8_t>(battle);
                library->ProcessEvent(converter, &params);
                RC::Output::send<RC::LogLevel::Default>(
                    STR("[KH02ReceiptHost] CONVERT battle={} index={} number={} inventory_write=false\n"),
                    battle, params.result.index, params.result.number);
            }
        else
            RC::Output::send<RC::LogLevel::Default>(
                STR("[KH02ReceiptHost] CONVERT unavailable; no grant attempted\n"));
        const bool grant_ready = grant_function() != nullptr;
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02ReceiptBridge] GRANT_CONTRACT ready={} receipt_ack=false\n"), grant_ready);
        // Context facts do not authorize an award. Keep a current lookup hint even
        // if an item function is not yet available; every receipt call rechecks it.
        if (!store_hint(controller, pawn, controller_name, pawn_name))
            return refuse(L"context_revision_exhausted");
    }
    return true;
}
} // namespace

class Kh02ReceiptHost final : public RC::CppUserModBase {
  public:
    Kh02ReceiptHost() {
        ModName = STR("KH02ReceiptHost");
        ModVersion = STR("0.8-registration");
        ModDescription = STR("Receipt-backed native host experiment");
        ModAuthors = STR("KH02 AP");
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02ReceiptHost] CONSTRUCTED MSVC={} CRT=MD observation_writes=false receipt_bridge=true\n"),
            _MSC_VER);
    }

    void on_unreal_init() override {
        engine_ready = true;
        RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptHost] UNREAL_READY\n"));
    }

    void on_lua_start(RC::StringViewType name, RC::LuaMadeSimple::Lua &lua,
                      RC::LuaMadeSimple::Lua &, RC::LuaMadeSimple::Lua &,
                      std::vector<RC::LuaMadeSimple::Lua *> &) override {
        if (name != STR("KH02ReceiptHost"))
            return;
        // LuaMadeSimple retains std::function manager code until process shutdown.
        // Keep this code mapped through that lifetime, even after mod destruction.
        // The OS releases the mapping on normal game exit; no cross-process pin.
        HMODULE lifetime_module{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                    GET_MODULE_HANDLE_EX_FLAG_PIN,
                                reinterpret_cast<LPCWSTR>(&native_observe), &lifetime_module)) {
            RC::Output::send<RC::LogLevel::Default>(
                STR("[KH02ReceiptHost] LIFETIME_REFUSED error={}\n"), GetLastError());
            return;
        }
        active = true;
        ue::Hook::RegisterProcessEventPostCallback(&sdk_owner_post);
        lua.register_function("KH02ReceiptObserve", [](const RC::LuaMadeSimple::Lua &call) -> int {
            kh02_host::ObservationLease lease(observing);
            if (!active || !lease || call.get_stack_size() != 4) {
                call.set_bool(false);
                return 1;
            }
            try {
                if (!call.is_integer()) {
                    call.set_bool(false);
                    return 1;
                }
                const auto controller = call.get_integer();
                if (!call.is_integer()) {
                    call.set_bool(false);
                    return 1;
                }
                const auto pawn = call.get_integer();
                if (!call.is_string()) {
                    call.set_bool(false);
                    return 1;
                }
                const auto cn = call.get_string();
                if (cn.size() > kh02_host::max_context_name) {
                    call.set_bool(false);
                    return 1;
                }
                const std::string controller_name(cn);
                if (!call.is_string()) {
                    call.set_bool(false);
                    return 1;
                }
                const auto pn = call.get_string();
                if (pn.size() > kh02_host::max_context_name) {
                    call.set_bool(false);
                    return 1;
                }
                const std::string pawn_name(pn);
                call.set_bool(controller > 0 && pawn > 0 &&
                              native_observe(controller, pawn, controller_name, pawn_name));
            } catch (const std::exception &error) {
                wchar_t message[256]{};
                const int bytes = static_cast<int>(strnlen_s(error.what(), 512));
                if (bytes > 0)
                    MultiByteToWideChar(CP_UTF8, 0, error.what(), bytes, message, 255);
                RC::Output::send<RC::LogLevel::Default>(
                    STR("[KH02ReceiptHost] NATIVE_EXCEPTION detail={} writes=false\n"), message);
                call.set_bool(false);
            }
            return 1;
        });
        lua.register_function(
            "KH02ReceiptInvalidate", [](const RC::LuaMadeSimple::Lua &call) -> int {
                try {
                    call.set_bool(call.get_stack_size() == 0 && invalidate_hint());
                } catch (const std::exception &) {
                    call.set_bool(false);
                }
                return 1;
            });
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02ReceiptHost] LUA_BOUND lifetime=bounded_context\n"));
        RC::Output::send<RC::LogLevel::Default>(STR("[KH02ReceiptBridge] READY pid={} abi=1\n"),
                                                GetCurrentProcessId());
    }

    void on_lua_stop(RC::StringViewType name, RC::LuaMadeSimple::Lua &, RC::LuaMadeSimple::Lua &,
                     RC::LuaMadeSimple::Lua &, std::vector<RC::LuaMadeSimple::Lua *> &) override {
        if (name == STR("KH02ReceiptHost"))
            active = false;
    }

    ~Kh02ReceiptHost() override {
        active = false;
    }
};

extern "C" __declspec(dllexport) RC::CppUserModBase *start_mod() {
    return new Kh02ReceiptHost();
}

extern "C" __declspec(dllexport) void uninstall_mod(RC::CppUserModBase *mod) {
    delete mod;
}

namespace {
bool bridge_thread() {
    Windows windows{};
    windows.pid = GetCurrentProcessId();
    EnumWindows(own_window, reinterpret_cast<LPARAM>(&windows));
    return active && engine_ready &&
           kh02_host::same_owner_thread({windows.pid, GetCurrentThreadId(), windows.window_pid,
                                         windows.thread, windows.count});
}

ue::UFunction *grant_function() {
    // Native C++ storage is 24 bytes, but UFunction::InitializeDerivedMembers
    // computes ParmsSize from the final bool at 16 + ElementSize 1, without
    // trailing struct alignment. Do not confuse these two sizes.
    return function(L"/Script/TresGame.TresGameBlueprintLibrary:BP_TresGetItem", 0x4022401, 3, 17,
                    0xffff,
                    {{L"WorldContextObject", 0, L"ObjectProperty"},
                     {L"InItemKeyName", 8, L"NameProperty"},
                     {L"bShowInfo", 16, L"BoolProperty"}});
}

ue::UFunction *convert_function() {
    return function(
        L"/Script/TresGame.TresGameBlueprintLibrary:BP_TresConvertBattleItemIDtoKeyName",
        0x14022401, 3, 24, 16,
        {{L"WorldContextObject", 0, L"ObjectProperty"},
         {L"InBattleItemID", 8, L"ByteProperty"},
         {L"ReturnValue", 16, L"NameProperty"}});
}

struct ReceiptBackend {
    bool resolve(Kh02UE4SSRequest &request, uint64_t &world) {
        world = 0;
        // Reject wrong-thread calls before invoking the original owner's thread guard.
        if (!bridge_thread() || request.permit(request.owner_context) != 1)
            return false;
        PlainHint hint;
        if (!hint_copy(hint) || hint.pawn != request.pawn ||
            (request.revision && request.revision != hint.revision))
            return false;
        if (!native_observe(hint.controller, hint.pawn, hint.controller_name, hint.pawn_name, false,
                            &hint))
            return false;
        if (!exact(L"/Script/TresGame.Default__TresGameBlueprintLibrary") || !convert_function() ||
            !grant_function())
            return false;
        PlainHint again;
        if (!bridge_thread() || request.permit(request.owner_context) != 1 || !hint_copy(again) ||
            again.revision != hint.revision || again.pawn != hint.pawn ||
            again.controller != hint.controller)
            return false;
        request.revision = hint.revision;
        world = hint.pawn;
        return true;
    }

    bool receiver(Kh02UE4SSRequest &request, ue::UObject *&pawn, ue::UObject *&library) {
        uint64_t world = 0;
        if (!resolve(request, world))
            return false;
        PlainHint hint;
        if (!hint_copy(hint) || hint.revision != request.revision || hint.pawn != request.pawn)
            return false;
        pawn = hinted_instance(hint, false);
        library = exact(L"/Script/TresGame.Default__TresGameBlueprintLibrary");
        return pawn && library;
    }

    bool convert(Kh02UE4SSRequest &request, Kh02UE4SSConvert &frame) {
        ue::UObject *pawn{}, *library{};
        if (!receiver(request, pawn, library))
            return false;
        auto *fn = convert_function();
        uint64_t world = 0;
        if (!fn || !resolve(request, world) || reinterpret_cast<std::uintptr_t>(pawn) != world)
            return false;
        ConvertParams params{};
        params.world = pawn;
        params.battle = frame.battle;
        library->ProcessEvent(fn, &params);
        frame.result = {params.result.index, params.result.number};
        return frame.result.index != 0;
    }

    bool grant(Kh02UE4SSRequest &request, const Kh02UE4SSGrant &frame) {
        struct Params {
            ue::UObject *world;
            Name name;
            uint8_t show, reserved[7];
        };

        static_assert(sizeof(Params) == 24 && offsetof(Params, show) == 16);
        ue::UObject *pawn{}, *library{};
        if (!receiver(request, pawn, library))
            return false;
        auto *fn = grant_function();
        uint64_t world = 0;
        if (!fn || !resolve(request, world) || reinterpret_cast<std::uintptr_t>(pawn) != world)
            return false;
        Params params{};
        params.world = pawn;
        params.name = {frame.name.index, frame.name.number};
        params.show = frame.show;
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02ReceiptBridge] GRANT_ENTER pawn=0x{:X} name={} receipt_ack=false\n"), world,
            frame.name.index);
        library->ProcessEvent(fn, &params);
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02ReceiptBridge] GRANT_RETURN receipt_ack=false\n"));
        return true;
    }
};

void bridge_exception(const wchar_t *stage, uint32_t operation, const char *what) noexcept {
    try {
        wchar_t message[256]{};
        const int count = static_cast<int>(strnlen_s(what, 512));
        if (count)
            MultiByteToWideChar(CP_UTF8, 0, what, count, message, 255);
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02ReceiptBridge] EXCEPTION stage={} operation={} detail={} receipt_ack=false\n"),
            stage, operation, message);
    } catch (...) {
    }
}

uint32_t receipt_check(void *opaque, uint64_t *world) {
    const auto started = GetTickCount64();
    try {
        ReceiptBackend backend;
        const auto result =
            kh02_bridge::check_request(static_cast<Kh02UE4SSRequest *>(opaque), world, backend);
        try {
            RC::Output::send<RC::LogLevel::Default>(
                STR("[KH02ReceiptBridge] CHECK_RETURN result={} elapsed_ms={} receipt_ack=false\n"),
                result, GetTickCount64() - started);
        } catch (...) {
        }
        return result;
    } catch (const std::exception &error) {
        if (world)
            *world = 0;
        bridge_exception(L"check", 0, error.what());
        return 0;
    } catch (...) {
        if (world)
            *world = 0;
        bridge_exception(L"check", 0, "non-standard C++ exception");
        return 0;
    }
}

uint32_t receipt_invoke(void *opaque, uint32_t operation, void *data, uint32_t size) {
    auto *request = static_cast<Kh02UE4SSRequest *>(opaque);
    const auto started = GetTickCount64();
    try {
        ReceiptBackend backend;
        const auto result = kh02_bridge::invoke(request, operation, data, size, backend);
        try {
            RC::Output::send<RC::LogLevel::Default>(
                STR("[KH02ReceiptBridge] OP_RETURN operation={} result={} elapsed_ms={} receipt_ack=false\n"),
                operation, result, GetTickCount64() - started);
        } catch (...) {
        }
        return result;
    } catch (const std::exception &error) {
        if (request) {
            request->fault = 1;
            request->busy = 0;
        }
        bridge_exception(L"invoke", operation, error.what());
        return 0;
    } catch (...) {
        if (request) {
            request->fault = 1;
            request->busy = 0;
        }
        bridge_exception(L"invoke", operation, "non-standard C++ exception");
        return 0;
    }
}
} // namespace

extern "C" __declspec(dllexport) uint32_t Kh02UE4SSGetApiV1(uint32_t version, uint32_t size,
                                                            Kh02UE4SSApi *out) {
    if (!out || version != KH02_UE4SS_BRIDGE_VERSION || size != sizeof(*out))
        return 0;
    *out = {sizeof(*out),
            KH02_UE4SS_BRIDGE_VERSION,
            sizeof(Kh02UE4SSRequest),
            KH02_UE4SS_BRIDGE_FEATURES,
            receipt_check,
            receipt_invoke};
    return 1;
}

#include "sdk_owner_pump.hpp"

#include "registration_host.inc"
