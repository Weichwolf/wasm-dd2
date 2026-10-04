"""Real virtual keys used by navigation and the original keyboard-bind menu."""
NAVIGATION_KEYS={'Left':0x25,'Right':0x27,'Up':0x26,'Down':0x28,
                 'Return':0x0d,'Escape':0x1b,'F1':0x70,'F2':0x71}
KEYS={**NAVIGATION_KEYS,'Space':0x20,
      **{chr(k):k for k in range(ord('A'),ord('Z')+1)},
      **{str(k):ord(str(k)) for k in range(10)}}
