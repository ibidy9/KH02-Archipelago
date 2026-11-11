-- Pure parsing of copied UObject full names; no engine calls or FStrings.
local M={}
local function owner(name)
    if type(name)~="string" or #name>512 or name:find("[\r\n]") then return nil end
    local world=name:match(" (/Game/Levels/[^ :]+):PersistentLevel%.")
    if not world then return nil end
    local package,asset=world:match("^(.+)%.([^./]+)$")
    if not package or package:match("([^/]+)$")~=asset then return nil end
    return {world=world,package=package,level=asset}
end
function M.from_names(controller_name,pawn_name)
    local controller,pawn=owner(controller_name),owner(pawn_name)
    if not controller or not pawn then return nil,"unrecognized_owner_path" end
    if controller.world~=pawn.world then return nil,"owner_map_mismatch" end
    return controller
end
return M
