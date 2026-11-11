-- One-event context inspection only: returned facts are not delivery authority.
local map_identity=require("map_identity")
local M = {}
local function present(value) return value ~= nil and value:IsValid() end
local function address(value) return string.format("0x%X", value:GetAddress()) end
function M.inspect(find, reads, controller, announced_pawn)
    if not present(controller) then return nil, "no_controller" end
    if not present(announced_pawn) then return nil, "no_announced_pawn" end
    local class = announced_pawn:GetClass()
    if not present(class) then return nil, "no_pawn_class" end
    if class:GetFName():ToString() ~= "p_ex002_Pawn_C" then return nil, "not_aqua" end
    -- Record paths first: useful diagnostics survive a later failed read.
    local facts = {controller=address(controller),pawn=address(announced_pawn),
        controller_name=controller:GetFullName(),pawn_name=announced_pawn:GetFullName(),
        authorizes_delivery=false}
    local function read(operation,receiver,...)
        local ok,value,reason=pcall(reads.call,find,operation,receiver,...)
        if not ok then return nil,"read_error:" .. operation .. ":" .. type(value) .. ":" .. tostring(value):sub(1,240) end
        return value,reason
    end
    local gameplay = find("/Script/Engine.Default__GameplayStatics")
    local local_controller, why = read("local_controller",gameplay,controller,0)
    if not present(local_controller) then return nil, why or "no_local_controller", facts end
    if local_controller:GetAddress() ~= controller:GetAddress() then
        return nil,"not_local_controller",facts
    end
    local current, reason = read("current_pawn",controller)
    if not present(current) then return nil,reason or "no_current_pawn",facts end
    if current:GetAddress() ~= announced_pawn:GetAddress() then return nil,"pawn_changed",facts end
    facts.context_matches=true
    -- Copy and compare the owning paths; do not receive a temporary FString.
    local map,reason=map_identity.from_names(facts.controller_name,facts.pawn_name)
    facts.level=map and map.level or ""
    facts.map_package=map and map.package or ""
    facts.level_error=reason
    return facts
end
return M
