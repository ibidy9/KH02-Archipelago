-- Read-only adapter under development. Not loaded by the released APWorld.
-- Resolve by exact UFunction path instead of dynamic object.Member lookup.
local M = {}
local specs = {
    local_controller={path="/Script/Engine.GameplayStatics:GetPlayerController",arity=2,flags=0x14022401,
        parameters={{"WorldContextObject",0,"ObjectProperty"},{"PlayerIndex",8,"IntProperty"},{"ReturnValue",16,"ObjectProperty"}}},
    current_pawn={path="/Script/Engine.Controller:K2_GetPawn",arity=0,flags=0x54020401,
        parameters={{"ReturnValue",0,"ObjectProperty"}}},
}
local function present(value)
    return value ~= nil and value:IsValid()
end
function M.call(find, operation, receiver, ...)
    local spec = specs[operation]
    if not spec then return nil, "unsupported_read" end
    local path=spec.path
    if select("#", ...) ~= spec.arity then return nil, "wrong_argument_count" end
    if not present(receiver) then return nil, "missing_receiver" end
    local fn = find(path)
    if not present(fn) then return nil, "missing_function:" .. path end
    if fn:type() ~= "UFunction" then return nil, "wrong_wrapper:" .. path end
    if fn:GetFullName() ~= "Function " .. path then return nil, "wrong_function:" .. path end
    -- File-only constructors supplied these exact parameter contracts. Refuse
    -- a mismatch before entering ProcessEvent; this is not a layout guess.
    local flags=fn:GetFunctionFlags()
    if flags~=spec.flags then return nil,"flags_changed:"..path..":observed="..tostring(flags) end
    local reflection=fn:Reflection()
    for _,parameter in ipairs(spec.parameters) do
        local name,offset,kind=table.unpack(parameter)
        local property=reflection:GetProperty(name)
        if not present(property) then return nil,"missing_parameter:"..name end
        local observed_offset,observed_name=property:GetOffset_Internal(),property:GetFullName()
        if observed_offset~=offset or observed_name~=kind.." "..path.."."..name then
            return nil,"parameter_changed:"..name..":offset="..tostring(observed_offset)..":name="..tostring(observed_name)
        end
    end
    -- Direct UFunction __call handles an explicit receiver when StaticFindObject
    -- supplied no bound base. Avoid UObject.CallFunction: pinned source checks
    -- derives_from_function(), while UFunction overrides derives_from_ufunction().
    -- This alternative still needs live validation; no gameplay writer exists.
    return fn(receiver, ...)
end
return M
