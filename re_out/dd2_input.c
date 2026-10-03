/* dd2_input.c — hand-written platform/runtime input shim (Stage 3: playability).
 *
 * The engine's live-input model (verified from the binary):
 *   - The Win32 message loop calls Translate_Keypress(vkey, lparam) on WM_KEYDOWN/WM_KEYUP,
 *     where (lparam & 0x80000000) != 0 means KEY-UP. It sets the _pad_* boolean globals by
 *     matching vkey against the configurable keymap that Setup_Pad() loads.
 *   - Per frame, Read pads those _pad_* booleans into the control word at _DAT_0071c048.
 * Translate platform events to Windows virtual-key codes, maintain pressed
 * states and enter the real window procedure (which also cancels movies).
 * SDL and browser adapters both use this shared bridge; engine code is untouched.
 *
 * Diagnostic Setup_Pad(1) uses the "joystick"/keyboard-2 map @0x46757a:
 *   ENTER=fire/accept, ESC=back, arrows=steer/accel/brake, F1/F2, SPACE, W/S/A/Z.
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern void Translate_Keypress(unsigned int vkey, unsigned int lparam_flags);
extern int FUN_004132f0(void*,unsigned,unsigned,unsigned);

/* Windows virtual-key codes the default keymaps use (see the two 14-byte maps at 0x467568/0x46757a). */
enum {
    VK_RETURN=0x0d, VK_ESCAPE=0x1b, VK_SPACE=0x20,
    VK_LEFT=0x25, VK_UP=0x26, VK_RIGHT=0x27, VK_DOWN=0x28,
    VK_F1=0x70, VK_F2=0x71,
    VK_A=0x41, VK_S=0x53, VK_W=0x57, VK_Z=0x5a
};

/* win32 GetKeyState backing store (dd2_stubs.c reads it): 1 = key currently down, indexed by VK.
 * The original polls GetKeyState() in the keyboard-rebind screen (FUN_0044fe64) to detect which
 * key the user pressed to bind; our GetKeyState was stubbed to 0 so rebinding never advanced. */
unsigned char dd2_keystate[256];

/* The window receives generic modifier VKs, whereas GetKeyState exposes
 * both physical sides. Keep those independent of the event direction: a
 * left release remains KEYUP when the right side is still held.
 * Message classes also follow the real Wine10 USER32 reference, including
 * Alt combinations and F10. Only bit31 of lparam is consumed by DD2; this
 * bridge does not claim scan-code/layout or character-message fidelity. */
static int alt_pressed;
void dd2_key_event(unsigned int vk, int down)
{
    unsigned int message=down ? 0x100u : 0x101u, generic=vk;
    int control=dd2_keystate[0x11], alt=dd2_keystate[0x12];
    if (vk==0xa4 || vk==0xa5 || vk==0x12) {
        if (down && !control) { message=0x104u; alt_pressed=1; }
        else if (!down && alt && alt_pressed) { message=0x105u; alt_pressed=0; }
    } else if (vk==0xa2 || vk==0xa3 || vk==0x11) {
        if (!down && alt) { message=0x105u; alt_pressed=0; }
    } else if (vk==0x79 || (!control && alt)) {
        message=down ? 0x104u : 0x105u; alt_pressed=0;
    }
    if (vk < 256) dd2_keystate[vk]=down ? 1 : 0;
    if (vk>=0xa0 && vk<=0xa5) {
        generic=0x10u+(vk-0xa0u)/2;
        dd2_keystate[generic]=dd2_keystate[vk&~1u] || dd2_keystate[vk|1u];
    }
    FUN_004132f0((void*)(uintptr_t)*(uint32_t*)(uintptr_t)0x46047c,
                message,generic,down ? 0u : 0x80000000u);
}

/* Map a browser KeyboardEvent.code string to a Windows VK code (0 = unmapped/ignore).
 * The JS side passes event.code (layout-independent physical key names). */
unsigned int dd2_browser_key_to_vk(const char* code)
{
    if (!code) return 0;
    if (!strcmp(code,"ArrowLeft"))  return VK_LEFT;
    if (!strcmp(code,"ArrowUp"))    return VK_UP;
    if (!strcmp(code,"ArrowRight")) return VK_RIGHT;
    if (!strcmp(code,"ArrowDown"))  return VK_DOWN;
    if (!strcmp(code,"Enter"))      return VK_RETURN;
    if (!strcmp(code,"Escape"))     return VK_ESCAPE;
    if (!strcmp(code,"Space"))      return VK_SPACE;
    if (!strcmp(code,"F1"))         return VK_F1;
    if (!strcmp(code,"F2"))         return VK_F2;
    if (!strcmp(code,"F10"))        return 0x79;
    if (!strcmp(code,"ShiftLeft"))  return 0xa0;
    if (!strcmp(code,"ShiftRight")) return 0xa1;
    if (!strcmp(code,"ControlLeft")) return 0xa2;
    if (!strcmp(code,"ControlRight")) return 0xa3;
    if (!strcmp(code,"AltLeft"))    return 0xa4;
    if (!strcmp(code,"AltRight"))   return 0xa5;
    /* The original rebind scan list includes all letter/digit physical keys
     * (VK_A..VK_Z = 0x41.., VK_0..VK_9 = 0x30..); unbound VKs are ignored
     * by Translate_Keypress; only keymap-matched keys set _pad_* bits. Covers KeyA/S/W/Z too. */
    if (!strncmp(code,"Key",3)   && code[3]>='A' && code[3]<='Z' && code[4]==0) return 0x41u + (unsigned)(code[3]-'A');
    if (!strncmp(code,"Digit",5) && code[5]>='0' && code[5]<='9' && code[6]==0) return 0x30u + (unsigned)(code[5]-'0');
    return 0;
}

/* Convenience for the JS/native shim: translate + dispatch in one call. Returns the VK used (0=ignored). */
unsigned int dd2_browser_key_event(const char* code, int down)
{
    unsigned int vk = dd2_browser_key_to_vk(code);
    if (vk) dd2_key_event(vk, down);
    return vk;
}

/* Self-test: prove the input path flips the engine's pad-state globals. Requires Setup_Pad() to have
 * run first (the active keymap must be loaded). Returns 0 on success, nonzero on the first failure.
 * _pad_lup @0x463043, _pad_ldown @0x463044, _pad_lleft @0x463045, _pad_start @0x463040. */
int dd2_input_selftest(void)
{
    volatile unsigned char* pad_lup   = (unsigned char*)(uintptr_t)0x463043;
    volatile unsigned char* pad_ldown = (unsigned char*)(uintptr_t)0x463044;
    volatile unsigned char* pad_lleft = (unsigned char*)(uintptr_t)0x463045;
    int fails = 0;
    unsigned i;
    struct { const char* code; volatile unsigned char* flag; const char* name; } cases[3];
    cases[0].code="ArrowUp";   cases[0].flag=pad_lup;   cases[0].name="_pad_lup";
    cases[1].code="ArrowDown"; cases[1].flag=pad_ldown; cases[1].name="_pad_ldown";
    cases[2].code="ArrowLeft"; cases[2].flag=pad_lleft; cases[2].name="_pad_lleft";
    for (i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        int after_down, after_up, ok;
        *cases[i].flag = 0;
        dd2_browser_key_event(cases[i].code, 1);   /* press */
        after_down = *cases[i].flag;
        dd2_browser_key_event(cases[i].code, 0);   /* release */
        after_up = *cases[i].flag;
        ok = (after_down != 0) && (after_up == 0);
        fprintf(stderr, "[input-selftest] %-9s -> %-10s press=%d release=%d %s\n",
                cases[i].code, cases[i].name, after_down, after_up, ok ? "OK" : "FAIL");
        if (!ok) fails++;
    }
    return fails;
}
