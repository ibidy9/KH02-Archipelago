-- Rule definitions are exported unchanged from the APWorld's JSON catalog.
local rules=require('scripts.shared-access-rules')
local data=require('scripts.access-data')
local M={}
local function has(name)
    local id=data.items[name]
    return id~=nil and Tracker:ProviderCountForCode('i'..id)>0
end
local function every(list,fn)
    for _,v in ipairs(list) do if not fn(v) then return false end end
    return true
end
local function some(list,fn)
    for _,v in ipairs(list) do if fn(v) then return true end end
    return false
end
local categories={'head','arms','back','pattern'}
local function category(c,complete)
    return some(data.wardrobe[c],function(group)
        return complete and every(group,has) or not complete and some(group,has)
    end)
end
local function zodiacs()
    for _,id in pairs(data.items) do
        if id>=0x020223 and id<=0x02022e and Tracker:ProviderCountForCode('i'..id)==0 then return false end
    end
    return true
end
local function requirement(options)
    if options.requirement=='wayfinders' then return every({"Terra's Wayfinder","Aqua's Wayfinder","Ventus's Wayfinder"},has) end
    if options.requirement=='zodiacs' then return zodiacs() end
    if options.requirement=='wardrobe_sets' then return every(categories,function(c) return category(c,true) end) end
    return false
end
local function evaluate(rule,options)
    if type(rule)=='boolean' then return rule end
    local op=rule[1]
    if op=='all' then
        for i=2,#rule do if not evaluate(rule[i],options) then return false end end
        return true
    end
    if op=='any' then
        for i=2,#rule do if evaluate(rule[i],options) then return true end end
        return false
    end
    if op=='item' then return has(rule[2]) end
    if op=='world' then return options.start==rule[2] or has(rule[2]..' Access') end
    if op=='selected_requirement' then return requirement(options) end
    if op=='rush_unlock' then return options.goal=='boss_rush' and requirement(options) or options.goal~='boss_rush' and zodiacs() end
    if op=='mickey_all_worlds' then return options.mickey=='randomized_all_worlds' end
    if op=='mickey' then return options.mickey=='vanilla' or has('Mickey') end
    if op=='wardrobe_any' then return some(categories,function(c) return category(c,false) end) end
    if op=='wardrobe_categories' then return every(categories,function(c) return category(c,false) end) end
    error('Unknown access rule '..tostring(op))
end
function M.check(id,options)
    local row=rules.locations[tostring(id)]
    if not row then return AccessibilityLevel.None end
    if evaluate(row.green or row.rule,options) then return AccessibilityLevel.Normal end
    if row.yellow and evaluate(row.yellow,options) then return AccessibilityLevel.SequenceBreak end
    return AccessibilityLevel.None
end
M.evaluate=evaluate
return M
