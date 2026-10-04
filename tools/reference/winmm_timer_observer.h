/* Minimal 32-bit Win32 declarations for the reference-only timer observer. */
#define API __attribute__((stdcall))
#define IMPORT __declspec(dllimport)
typedef unsigned U32;
typedef unsigned long long U64;
typedef void* HANDLE;
typedef void(API *Callback)(U32,U32,U32,U32,U32);
IMPORT void* API GetProcAddress(HANDLE,const char*);
IMPORT HANDLE API LoadLibraryA(const char*);
IMPORT HANDLE API GetModuleHandleA(const char*);
IMPORT U32 API GetEnvironmentVariableA(const char*,char*,U32);
IMPORT HANDLE API CreateFileA(const char*,U32,U32,void*,U32,U32,HANDLE);
IMPORT int API WriteFile(HANDLE,const void*,U32,U32*,void*);
IMPORT HANDLE API GetProcessHeap(void);
IMPORT void* API HeapAlloc(HANDLE,U32,U32);
IMPORT U32 API GetTickCount(void);
IMPORT U32 API GetCurrentThreadId(void);
IMPORT HANDLE API CreateMutexA(void*,int,const char*);
IMPORT U32 API WaitForSingleObject(HANDLE,U32);
IMPORT int API ReleaseMutex(HANDLE);
IMPORT int API QueryPerformanceCounter(U64*);
IMPORT int API QueryPerformanceFrequency(U64*);
IMPORT void API ExitProcess(U32);
IMPORT int API DisableThreadLibraryCalls(HANDLE);
IMPORT HANDLE API CreateEventA(void*,int,int,const char*);
IMPORT int API SetEvent(HANDLE);
IMPORT int API InterlockedIncrement(volatile int*);
IMPORT HANDLE API GetStdHandle(int);

IMPORT int API InterlockedCompareExchange(volatile int*,int,int);
IMPORT int API InterlockedExchange(volatile int*,int);
IMPORT void API Sleep(U32);
IMPORT void API OutputDebugStringA(const char*);
