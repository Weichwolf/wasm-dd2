import gdb,json,time
from pathlib import Path

def run(output):
    output=Path(output); inferior=gdb.selected_inferior()
    journal=(output/'events.jsonl').open('x'); pending=[None]; waiting=[False]
    def read(address,n):return inferior.read_memory(address,n).tobytes()
    def word(address):return int.from_bytes(read(address,4),'little')
    def emit(kind,**extra):
        row=dict(kind=kind,host_ns=time.monotonic_ns(),pc=int(gdb.parse_and_eval('$pc')),
                 active=word(0x46042c),timer=word(0x460474),timer_fires=word(0x460484),
                 phase=word(0x4699cc),cf=word(0x462ff0),level=word(0x936ff4),
                 mapping=read(0x46302c,14).hex(),input_flags=read(0x46303f,17).hex(),**extra)
        journal.write(json.dumps(row)+'\n');journal.flush()
    class Point(gdb.Breakpoint):
        def __init__(self,address,kind):
            super().__init__(f'*0x{address:x}',type=gdb.BP_HARDWARE_BREAKPOINT);self.silent=True;self.kind=kind
        def stop(self):
            if (output/'stop').exists():return True
            if self.kind=='message':
                esp=int(gdb.parse_and_eval('$esp'));message=word(esp+8)
                if message in (0x1c,6,7,8,0x100,0x101,0x104,0x105):
                    emit('message',message=message,wparam=word(esp+12),lparam=word(esp+16))
                if message==0x1c:pending[0]=word(esp+12)
            elif self.kind=='activation' and pending[0] is not None:
                emit('activation-return',requested_active=pending[0]);pending[0]=None
            elif self.kind=='wait':
                emit('wait-enter');waiting[0]=True
            elif self.kind=='return' and waiting[0]:
                emit('wait-return');waiting[0]=False
            return False
    points=[Point(0x4132f0,'message'),Point(0x41344d,'activation'),
            Point(0x4130a3,'wait'),Point(0x4130aa,'return')]
    emit('attached');(output/'ready').write_text(str(inferior.pid))
    try:
        gdb.execute('continue')
        if not (output/'stop').exists():raise RuntimeError('Unexpected original observer stop')
        emit('observer-stop')
    finally:
        for point in points:point.delete()
        journal.close()
