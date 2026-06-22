/* Win32 USER32/GDI shim STUBS (link surface). Real SDL3/WebGL impls come later for execution.
   Handle-returners give non-null; GetMessage returns 0 (WM_QUIT) so the message loop terminates. */
void* CreateWindowExA(){ return (void*)1; }
void* CreateSolidBrush(){ return (void*)1; }
void* LoadCursorA(){ return (void*)1; }
void* LoadIconA(){ return (void*)1; }
int RegisterClassA(){ return 1; }
int DefWindowProcA(){ return 0; }
int DeleteObject(){ return 1; }
int DestroyWindow(){ return 1; }
int DispatchMessageA(){ return 0; }
int GetMessageA(){ return 0; }   /* 0 => WM_QUIT => message loop exits */
int MessageBoxA(){ return 1; }
int PeekMessageA(){ return 0; }
void PostQuitMessage(){ }
int ShowCursor(){ return 0; }
int ShowWindow(){ return 1; }
int TranslateMessage(){ return 0; }
int UpdateWindow(){ return 1; }
int WaitMessage(){ return 1; }
