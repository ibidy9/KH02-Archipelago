#include "registration_policy.hpp"
#include "registration_catalog.hpp"

namespace {
struct RegistrationBackend {
    PlainHint captured{};
    ue::UObject *pawn{};
    ue::UObject *library{};
    ue::UObject *gameplay{};
    ue::UObject *game_instance{};
    ue::UFunction *award{};
    ue::UFunction *getter{};
    ue::FName item_name{};
    uint32_t code{};
    uint64_t context{}, pawn_id{};
    std::wstring key;
    // Diagnostic only: identifies the last boundary reached, never authorizes it.
    const wchar_t *stage = L"request-policy";

    bool validate(Kh02RegistrationRequest &request) {
        stage = L"owner-permit-and-request";
        if (!bridge_thread() || !request.permit || request.permit(request.owner_context) != 1 ||
            request.code != code || request.context != context || request.pawn != pawn_id ||
            request.show != 1 || request.revision != captured.revision)
            return false;
        PlainHint current;
        stage = L"weak-context-and-reflected-objects";
        if (!hint_copy(current) || current.revision != captured.revision ||
            current.pawn != pawn_id || current.controller != captured.controller ||
            hinted_instance(current, false) != pawn || !live(library) || !live(gameplay) ||
            !live(award) || !live(getter))
            return false;

        struct Params {
            ue::UObject *world;
            ue::UObject *result;
        };

        stage = L"game-instance-identity";
        Params params{pawn, nullptr};
        gameplay->ProcessEvent(getter, &params);
        // Compare the SDK-returned identity to the independently guarded owner before
        // reading it; never cast a supplied integer into a UObject.
        if (!params.result || reinterpret_cast<uintptr_t>(params.result) != context ||
            !live(params.result))
            return false;
        if (game_instance && params.result != game_instance)
            return false;
        game_instance = params.result;
        PlainHint after;
        stage = L"post-getter-context";
        return request.permit(request.owner_context) == 1 && hint_copy(after) &&
               after.revision == captured.revision && after.pawn == pawn_id &&
               hinted_instance(after, false) == pawn;
    }

    bool prepare(Kh02RegistrationRequest &request) {
        stage = L"initial-owner-context";
        if (!bridge_thread() || request.permit(request.owner_context) != 1 ||
            !hint_copy(captured) || !captured.weak_ready || captured.pawn != request.pawn)
            return false;
        pawn_id = request.pawn;
        context = request.context;
        code = request.code;
        stage = L"requested-revision";
        if (request.revision && request.revision != captured.revision)
            return false;
        request.revision = captured.revision;
        pawn = hinted_instance(captured, false);
        stage = L"independent-player-observation";
        if (!pawn || !native_observe(captured.controller, captured.pawn, captured.controller_name,
                                     captured.pawn_name, false, &captured))
            return false;
        stage = L"reflection-contract";
        gameplay = exact(L"/Script/Engine.Default__GameplayStatics");
        library = exact(L"/Script/TresGame.Default__TresGameBlueprintLibrary");
        getter = function(L"/Script/Engine.GameplayStatics:GetGameInstance", 0x14022401, 2, 16, 8,
                          {{L"WorldContextObject", 0, L"ObjectProperty"},
                           {L"ReturnValue", 8, L"ObjectProperty"}});
        award = grant_function();
        if (!gameplay || !library || !getter || !award || !validate(request))
            return false;
        stage = L"catalog-name";
        const auto *exact_key = kh02_registration::key_for_code(code);
        if (!exact_key)
            return false;
        key = exact_key;
        item_name = ue::FName(key.c_str(), ue::FNAME_Find);
        // Finding names only: never add names or use a borrowed SDK string buffer.
        if (item_name.ToString() != key)
            return false;
        stage = L"award-library-refresh";
        library = exact(L"/Script/TresGame.Default__TresGameBlueprintLibrary");
        return library && validate(request);
    }

    bool grant(Kh02RegistrationRequest &request) {
        stage = L"award-entry";

        struct Params {
            ue::UObject *world;
            ue::FName name;
            uint8_t show, reserved[7];
        };

        static_assert(sizeof(Params) == 24 && offsetof(Params, show) == 16);
        // GameplayStatics getter must be invoked on its own CDO, not the item CDO.
        if (!pawn || !library || !award || request.code != code)
            return false;
        Params params{pawn, item_name, 1, {}};
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02Registration] ENTER code=0x{:X} key={} receipt_ack=false\n"), code, key);
        library->ProcessEvent(award, &params);
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02Registration] RETURN code=0x{:X} receipt_ack=false\n"), code);
        return true;
    }
};

uint32_t registration_invoke(Kh02RegistrationRequest *request) {
    try {
        RegistrationBackend backend;
        const auto started = GetTickCount64();
        const auto result = kh02_registration::invoke(request, backend);
        RC::Output::send<RC::LogLevel::Default>(
            STR("[KH02Registration] RESULT result={} code=0x{:X} entered={} fault={} stage={} elapsed_ms={} receipt_ack=false\n"),
            result, request ? request->code : 0, request ? request->entered : 0,
            request ? request->fault : 0, backend.stage, GetTickCount64() - started);
        return result;
    } catch (const std::exception &error) {
        if (request) {
            request->fault = 1;
            request->busy = 0;
        }
        bridge_exception(L"registration", 3, error.what());
        return 0;
    } catch (...) {
        if (request) {
            request->fault = 1;
            request->busy = 0;
        }
        bridge_exception(L"registration", 3, "non-standard C++ exception");
        return 0;
    }
}
} // namespace

extern "C" __declspec(dllexport) uint32_t Kh02UE4SSGetRegistrationApiV1(uint32_t version,
                                                                        uint32_t size,
                                                                        Kh02RegistrationApi *out) {
    if (!out || version != 1 || size != sizeof(*out))
        return 0;
    *out = {sizeof(*out), 1, sizeof(Kh02RegistrationRequest), 0, registration_invoke};
    return 1;
}
