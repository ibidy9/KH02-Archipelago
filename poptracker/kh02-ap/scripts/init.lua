local catalog = require('scripts.catalog')
local State = require('scripts.state')
local Access = require('scripts.access')
Tracker:AddItems('items/items.json')
local selected_layout = nil
local state
local function emit_item(id, count)
    local entry = catalog.items[id]
    local item = Tracker:FindObjectForCode(entry.code)
    if not item then return end
    if entry.count then item.AcquiredCount = count else item.Active = count > 0 end
end
local function emit_location(id, checked)
    local section = Tracker:FindObjectForCode(catalog.locations[id].path)
    if section then section.AvailableChestCount = checked and 0 or section.ChestCount end
end
local function emit_options(options)
    local wayfinders = not options.known or options.requirement == 'wayfinders'
    local mickey = not options.known or (options.mickey == 'randomized_depths' or options.mickey == 'randomized_all_worlds')
    local layout = 'layouts/overview-' .. (wayfinders and '1' or '0') .. (mickey and '1' or '0') .. '.json'
    -- Only replace the named overview on an option change, never per item or frame.
    if layout ~= selected_layout then
        assert(Tracker:AddLayouts(layout), 'Could not load overview')
        selected_layout = layout
    end
    local start = catalog.starts[options.start]
    if start then emit_item(start, 1) end
end
state = State.new(catalog, emit_item, emit_location, emit_options)
function show_check(id) return state:show_location(tonumber(id)) end
function access_check(id) return Access.check(tostring(id), state.options) end
Tracker:AddMaps('maps/maps.json')
Tracker:AddLocations('locations/checks.json')
state:clear(nil)
Tracker:AddLayouts('layouts/base.json')
local function guarded(fn)
    Tracker.BulkUpdate = true
    local ok, error = pcall(fn)
    Tracker.BulkUpdate = false
    if not ok then print('KH02 tracker update failed: ' .. tostring(error)) end
end
if Archipelago then
    Archipelago:AddClearHandler('KH02 clear', function(slot)
        guarded(function() state:clear(slot) end)
    end)
    Archipelago:AddItemHandler('KH02 item', function(index, id)
        guarded(function() state:item(index, id) end)
    end)
    Archipelago:AddLocationHandler('KH02 location', function(id)
        guarded(function() state:location(id) end)
    end)
end
