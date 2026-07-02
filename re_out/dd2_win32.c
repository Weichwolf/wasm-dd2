/* Win32 USER32/GDI shim stubs with correct arg counts (WASM enforces call signatures).
   All args i32 in wasm32; handle-returners give non-null, GetMessage=0 ends the loop. */
void* CreateWindowExA(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j,int k,int l){ return (void*)1; }
void* CreateSolidBrush(int c){ return (void*)1; }
void* LoadCursorA(int a,int b){ return (void*)1; }
void* LoadIconA(int a,int b){ return (void*)1; }
int RegisterClassA(int a){ return 1; }
int DefWindowProcA(int a,int b,int c,int d){ return 0; }
int DeleteObject(int a){ return 1; }
int DestroyWindow(int a){ return 1; }
int DispatchMessageA(int a){ return 0; }
int GetMessageA(int a,int b,int c,int d){ return 0; }
int MessageBoxA(int a,const char* text,const char* caption,int d){ (void)a;(void)d; { extern int fprintf(); extern void* stderr; fprintf(stderr,"[MessageBox] %s: %s\n",caption?caption:"",text?text:""); } return 1; }
int PeekMessageA(int a,int b,int c,int d,int e){ return 0; }
void PostQuitMessage(int a){ }
int ShowCursor(int a){ return 0; }
int ShowWindow(int a,int b){ return 1; }
int TranslateMessage(int a){ return 0; }
int UpdateWindow(int a){ return 1; }
int WaitMessage(void){ return 1; }
