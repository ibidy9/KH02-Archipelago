"""Lua 5.4 script behavior with fake framework callbacks; never loads UE4SS."""

from pathlib import Path
import sys, unittest

R = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(R / "tools/poptracker-deps"))
from lupa.lua54 import LuaRuntime

SCRIPT = Path(__file__).parents[1] / "Scripts/main.lua"


class ScriptTests(unittest.TestCase):
    def start(self):
        self.lua = LuaRuntime(unpack_returned_tuples=True)
        self.lua.execute("""
  logs={};queue={};order={};observed=0;invalidated=0;probes=0
  ready=true;thread_ok=true
  print=function(s) table.insert(logs,s) end
  StaticFindObject=function() error('No engine lookup in fixture') end
  package.preload.context_reader=function() return {inspect=function(_,_,c,p)
   table.insert(order,'inspect')
   if not ready then return nil,'not_aqua' end
   return {controller_name='Controller C',pawn_name='Pawn P',map_package='fixture'}
  end} end
  package.preload.reflected_reads=function() return {} end
  ExecuteInGameThread=function(f) table.insert(queue,f) end
  KH02ReceiptInvalidate=function() invalidated=invalidated+1;table.insert(order,'invalidate');return true end
  KH02ReceiptObserve=function(...) observed=observed+1;table.insert(order,'observe');return true end
  KH02ReceiptScheduleProbe=function(ticket) probes=probes+1;return thread_ok end
  RegisterHook=function(path,a,b)
   assert(path=='/Script/Engine.PlayerController:ClientRestart');pre=a;post=b;return 1,2
  end
  local object={GetAddress=function() return 1 end}
  wrapper={get=function() return object end}
  function event() pre(wrapper,wrapper);post(wrapper,wrapper) end
  function drain() local q=queue;queue={};for _,f in ipairs(q) do f() end end
  """)
        self.lua.execute(SCRIPT.read_text())

    def test_no_32_event_cutoff_without_scheduling_tasks(self):
        self.start()
        self.lua.execute("for i=1,80 do event() end;drain()")
        g = self.lua.globals()
        self.assertEqual(80, g.observed)
        self.assertEqual(80, g.invalidated)
        self.assertEqual(0, g.probes)
        self.assertEqual(0, len(g.queue))
        self.assertEqual(["invalidate", "inspect", "observe"], [g.order[i] for i in range(1, 4)])

    def test_title_refusal_still_invalidates_and_reload_can_observe(self):
        self.start()
        self.lua.execute("drain();ready=false;event();ready=true;event();drain()")
        g = self.lua.globals()
        self.assertEqual(2, g.invalidated)
        self.assertEqual(1, g.observed)
        self.assertTrue(any("not_aqua" in g.logs[i] for i in range(1, len(g.logs) + 1)))

    def test_probe_helper_is_never_called(self):
        self.start()
        self.lua.execute("thread_ok=false;event();drain()")
        g = self.lua.globals()
        self.assertEqual(1, g.observed)
        self.assertTrue(g.probes == 0)

    def test_observation_does_not_depend_on_scheduler_helper(self):
        self.start()
        self.lua.execute("event();drain()")
        self.assertEqual(1, self.lua.globals().observed)
        # Compile a fresh environment with scheduler missing before startup capture.
        self.start()
        self.lua.execute("ExecuteInGameThread=nil")
        self.lua.execute(SCRIPT.read_text())
        self.lua.execute("event()")
        g = self.lua.globals()
        self.assertTrue(g.probes == 0)


if __name__ == "__main__":
    unittest.main()
