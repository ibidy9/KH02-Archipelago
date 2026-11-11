-- Exact ClientRestart observation; no inventory effect, timer or polling.
local reader=require("context_reader")
local reads=require("reflected_reads")
local raw=0
local function emit(s) print("[KH02ReceiptLua] "..s.."\n") end
local function invalidate()
 local ok,result=pcall(KH02ReceiptInvalidate)
 if not ok or result~=true then emit("INVALIDATE_REFUSED call_ok="..tostring(ok)) end
end
local function inspect(Context,NewPawn)
 raw=math.min(raw+1,1000000000)
 local ok,err=pcall(function()
  local controller,pawn=Context:get(),NewPawn:get()
  local facts,reason=reader.inspect(StaticFindObject,reads,controller,pawn)
  if not facts then emit("REFUSE raw="..raw.." reason="..tostring(reason));return end
  local accepted=KH02ReceiptObserve(controller:GetAddress(),pawn:GetAddress(),facts.controller_name,facts.pawn_name)
  emit("OBSERVE raw="..raw.." native_accepted="..tostring(accepted).." map="..facts.map_package.." authority=false")
 end)
 if not ok then emit("ERROR raw="..raw.." value_type="..type(err).." detail="..tostring(err):sub(1,240)) end
end
local ok,pre,post=pcall(RegisterHook,"/Script/Engine.PlayerController:ClientRestart",invalidate,inspect)
if ok then emit("REGISTERED context lifecycle; no fixed observation cutoff")
else emit("REGISTRATION_FAILED "..tostring(pre):sub(1,240)) end
