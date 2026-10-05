"""Control only the name callback return in the isolated mapped x86 fixture.

This deliberately substitutes the UI result, not its implementation. Original
record comparison/history/field/name stores execute normally. No original
machine-code or engine-data writes are made by this observer. The host fixture
supplies all component inputs and the name buffer before each call. This cannot
accept live name entry, menus, persistence, ordinary races or original A/V.
"""
import json
from pathlib import Path
import gdb


def run(output):
    events=[]
    gdb.execute('starti',to_string=True)
    inferior=gdb.selected_inferior()
    point=gdb.Breakpoint('*0x4522b0',type=gdb.BP_HARDWARE_BREAKPOINT)
    point.silent=True
    try:
        while True:
            gdb.execute('continue',to_string=True)
            if gdb.selected_inferior().pid==0:
                if int(gdb.parse_and_eval('$_exitcode'))!=0:
                    raise RuntimeError('Original component process failed')
                break
            pc=int(gdb.parse_and_eval('$eip'))&0xffffffff
            if pc!=0x4522b0:raise RuntimeError('Unexpected original stop')
            sp=int(gdb.parse_and_eval('$esp'))&0xffffffff
            stack=inferior.read_memory(sp,12).tobytes()
            words=[int.from_bytes(stack[i:i+4],'little') for i in (0,4,8)]
            if words!=[0x44d8dc,0,1]:raise RuntimeError('Unexpected actual original callback caller/arguments')
            response=int(gdb.parse_and_eval('name_response'))
            if response not in (0,1):raise RuntimeError('Name callback response outside contract')
            case=int(gdb.parse_and_eval('case_id'))
            if not 0<=case<3528:raise RuntimeError('Original component case outside bound')
            events.append(dict(case=case,caller=words[0],arguments=words[1:],response=response))
            # This is the same cdecl return the native/WASM fixture supplies.
            # Update only host-fixture bookkeeping and x86 return registers.
            gdb.execute('set variable name_calls = name_calls + 1',to_string=True)
            gdb.execute(f'set $eax = {response}',to_string=True)
            gdb.execute(f'set $esp = {sp+4}',to_string=True)
            gdb.execute(f'set $eip = {words[0]}',to_string=True)
    finally:
        point.delete()
        Path(output).write_text(json.dumps(dict(scope=__doc__.strip(),events=events,
            original_machine_code_writes=False,original_engine_data_writes=False,
            controlled_ui_callback=True),indent=2)+'\n')
