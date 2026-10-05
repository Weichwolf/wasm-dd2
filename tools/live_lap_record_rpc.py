"""Readonly browser observations -> ordinary keyboard intentions on stdout."""
import base64
import json
import sys
from live_lap_record_driver import ArcRoadKeyboardDriver


def main():
    driver=ArcRoadKeyboardDriver()
    memory={}
    def read(address,size):
        address &= 0xffffffff
        return bytes(memory[(address+i)&0xffffffff] for i in range(size))
    for line in sys.stdin:
        data=json.loads(line)
        for address,encoded in data['segments']:
            raw=base64.b64decode(encoded,validate=True)
            for index,byte in enumerate(raw):memory[(address+index)&0xffffffff]=byte
        if data['operation']=='geometry':
            response=dict(pass_=True,stored_bytes=len(memory))
        else:
            tick=int.from_bytes(read(0x7746c0,4),'little',signed=True)
            response=driver.controls(read,tick)
            response['tick']=tick
        print(json.dumps(response),flush=True)


if __name__=='__main__':main()
