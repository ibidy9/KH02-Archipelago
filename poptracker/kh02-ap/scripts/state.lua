-- Fixed-size AP state. No retained packets, timers, or callback registration here.
local M = {}
function M.new(catalog, emit_item, emit_location, emit_options)
    local self = {last_index = -1, counts = {}, checked = {}, options = {known = false}}
    function self:clear(slot)
        self.last_index = -1
        self.counts = {}
        self.checked = {}
        slot = type(slot) == 'table' and slot or {}
        local supported = ((slot.schema_version == 3 or slot.schema_version == 4) and slot.item_schema_version == 5)
            or ((slot.schema_version == 5 or slot.schema_version == 6) and slot.item_schema_version == 6)
        local known = supported
            and (slot.goal == 'demon_tide' or slot.goal == 'boss_rush')
            and (slot.goal_requirement == 'wayfinders' or slot.goal_requirement == 'zodiacs'
                 or slot.goal_requirement == 'wardrobe_sets')
            and (slot.mickey == 'vanilla' or slot.mickey == 'randomized_depths' or slot.mickey == 'randomized_all_worlds')
        self.options = {known = known, goal = known and slot.goal or nil,
            requirement = known and slot.goal_requirement or nil,
            mickey = known and slot.mickey or nil,
            start = known and slot.starting_world or nil}
        for id, _ in pairs(catalog.items) do emit_item(id, 0) end
        for id, _ in pairs(catalog.locations) do emit_location(id, false) end
        emit_options(self.options)
    end
    function self:item(index, id)
        if type(index) ~= 'number' or index % 1 ~= 0 or index < 0 or index <= self.last_index then return end
        -- AP sends its received history in order. Reset belongs to Clear.
        self.last_index = index
        local entry = catalog.items[id]
        if not entry then return end
        local count = math.min((self.counts[id] or 0) + 1, entry.max)
        if count ~= self.counts[id] then
            self.counts[id] = count
            emit_item(id, count)
        end
    end
    function self:location(id)
        if not catalog.locations[id] or self.checked[id] then return end
        self.checked[id] = true
        emit_location(id, true)
    end
    function self:show_wayfinders()
        return not self.options.known or self.options.requirement == 'wayfinders'
    end
    function self:show_mickey()
        return not self.options.known or (self.options.mickey == 'randomized_depths' or self.options.mickey == 'randomized_all_worlds')
    end
    function self:show_location(id)
        local entry = catalog.locations[id]
        return entry ~= nil and (not self.options.known or not entry.tide_only or self.options.goal == 'demon_tide')
    end
    return self
end
return M
