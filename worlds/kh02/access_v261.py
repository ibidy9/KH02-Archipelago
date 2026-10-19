"""Evaluate the same declarative rules exported into the tracker."""
import json
from importlib.resources import files
RULES=json.loads(files(__package__).joinpath('data/access-rules-v261.json').read_text())

def evaluate(world,state,rule):
    if isinstance(rule,bool):return rule
    op,*args=rule
    if op=='all':return all(evaluate(world,state,x) for x in args)
    if op=='any':return any(evaluate(world,state,x) for x in args)
    if op=='item':return world.owns(state,args[0])
    if op=='world':return world.world_open(state,args[0])
    if op=='selected_requirement':return world.selected_requirement(state)
    if op=='rush_unlock':return world.rush_requirement(state)
    if op=='mickey_all_worlds':return world.options.mickey.current_key=='randomized_all_worlds'
    if op=='mickey':return world.options.mickey.current_key=='vanilla' or world.owns(state,'Mickey')
    if op=='wardrobe_any':return world.wardrobe.any_piece(lambda name:world.owns(state,name))
    if op=='wardrobe_categories':return world.wardrobe.all_categories(lambda name:world.owns(state,name))
    raise ValueError('Unknown access rule: '+str(op))

def rule_for(world,row):
    record=RULES['locations'][str(row['id'])]
    if record['name']!=row['name']:raise ValueError('Rule/catalog identity mismatch')
    return lambda state:evaluate(world,state,record['rule'])
