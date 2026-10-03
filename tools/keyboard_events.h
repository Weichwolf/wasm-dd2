/* Shared physical transitions, not expected answers. USER32 supplies the
 * oracle; ports consume the same side-specific VK or browser code. */
static const struct {unsigned key;int down;const char* code;} events[]={
    {0xa0,1,"ShiftLeft"},{0xa1,1,"ShiftRight"},{0xa0,0,"ShiftLeft"},{0xa1,0,"ShiftRight"},
    {0xa2,1,"ControlLeft"},{0xa3,1,"ControlRight"},{0xa2,0,"ControlLeft"},{0xa3,0,"ControlRight"},
    {0xa4,1,"AltLeft"},{0xa5,1,"AltRight"},{0xa4,0,"AltLeft"},{0xa5,0,"AltRight"},
    {0xa4,1,"AltLeft"},{0x41,1,"KeyA"},{0x41,0,"KeyA"},{0xa4,0,"AltLeft"},
    {0x79,1,"F10"},{0x79,0,"F10"},{0x41,1,"KeyA"},{0x41,1,"KeyA"},{0x41,0,"KeyA"},
    {0xa4,1,"AltLeft"},{0xa4,0,"AltLeft"},
    {0xa2,1,"ControlLeft"},{0xa4,1,"AltLeft"},{0x41,1,"KeyA"},{0x41,0,"KeyA"},{0xa2,0,"ControlLeft"},{0xa4,0,"AltLeft"},
    {0xa4,1,"AltLeft"},{0xa3,1,"ControlRight"},{0xa3,0,"ControlRight"},{0xa4,0,"AltLeft"},
    {0xa2,1,"ControlLeft"},{0x79,1,"F10"},{0x79,0,"F10"},{0xa2,0,"ControlLeft"},
    {0xa1,1,"ShiftRight"},{0xa1,1,"ShiftRight"},{0xa1,0,"ShiftRight"}};
