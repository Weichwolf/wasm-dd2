/* indirect-call dispatch: VA->C-function map + startup relocation of fn-pointers in the image */
#include "ghidra_compat.h"
extern unsigned char* g_image;
extern int AI_Com_Server();
extern int Add_Computer_Info();
extern int Allocate_Font_Buffers();
extern int Allocate_Sound_Effect();
extern int Amplitude();
extern int ApplyMatrixLV();
extern int ApplyWheelOverlay();
extern int Barrier_Collision();
extern int Barrier_Corner_Collision();
extern int Bonnet_Smoke();
extern int Boot_Lost_Geometry();
extern int Button_Pressed();
extern int CD_Check();
extern int CD_Close();
extern int CD_Pause();
extern int CD_Restart();
extern int CLUT_Animation();
extern int Calc_Car_Angles_Square();
extern int Calc_Car_Tilt();
extern int Calc_Head_On_Clsn_Dynamics();
extern int Calc_Null_Suspension();
extern int Calc_Object_Angles();
extern int Calc_Object_MatrixYZX();
extern int Calc_Suspension_Right_Wheels();
extern int Calc_Track_Positions();
extern int Calculate_Finish();
extern int Calculate_Results();
extern int Call_Loaded_Game();
extern int Camera_Pad_Control();
extern int Car_1pt_Motion_3D();
extern int Car_2pt_Motion_3D();
extern int Car_Camera();
extern int Car_Drive_2pt_Motion();
extern int Car_Drive_Motion();
extern int Car_Fly_Motion();
extern int Car_Fly_Motion_3D();
extern int Car_Grounded_Motion_3D();
extern int Car_Landed();
extern int Car_Landed_On_Corner();
extern int Car_Movement();
extern int Championship();
extern int Change_Bonnet_Clut();
extern int Change_Boot_Clut();
extern int CheckPointScoring();
extern int Check_2D_Car_Collision();
extern int Check_Bonnet_Removal();
extern int Check_Boot_Removal();
extern int Check_For_CD_Loop();
extern int Check_Ground_Car_Collision();
extern int Check_League_Standing();
extern int Check_Space_Car_Collision();
extern int ClearOTagR();
extern int CloseHandle();
extern int Close_Application();
extern int Control_Car_Replay();
extern int CreateEventA();
extern int CreateFileA();
extern int CreateMutexA();
extern int CreateThread();
extern int Create_Object();
extern int DDRelease();
extern int DSGetWaveResource();
extern int DSLoadSoundBuffer();
extern int Debug_Stub();
extern int Decompress();
extern int Decrunch_Object_Block();
extern int DeleteFileA();
extern int DeleteFileMC();
extern int DemoMode();
extern int Determine_AI();
extern int DirectDrawCreate();
extern int DirectSoundCreate();
extern int Display_Position_Pointers();
extern int Display_Season_Status();
extern int Do_Car_Collisions();
extern int Do_End_Of_Season_Stuff();
extern int Do_The_Floaty_Camera_Thing();
extern int DopplerFrequency();
extern int DrawFlagObject();
extern int DrawLensFlare();
extern int DrawOTag();
extern int DrawParticles();
extern int DrawPrim();
extern int Draw_All();
extern int Draw_Car();
extern int Draw_Dynamic_Objects();
extern int Draw_Font_Poly();
extern int Draw_Other_Objects();
extern int Draw_Overlays();
extern int Draw_Scene_Object();
extern int Draw_Screen_Lines();
extern int Draw_Screen_Polys();
extern int Draw_Semi_Trans_Poly();
extern int Draw_Sky();
extern int Draw_Slab();
extern int Draw_Subdiv_Object();
extern int Draw_Tile();
extern int DupFileCheck();
extern int Duplicate_Font();
extern int End_Of_Season();
extern int Enter_Driver_Names();
extern int ExitProcess();
extern int ExitThread();
extern int FUN_0041033a();
extern int FUN_0041080d();
extern int FUN_00410f74();
extern int FUN_004110a4();
extern int FUN_004111e8();
extern int FUN_0041132c();
extern int FUN_00411a04();
extern int FUN_00411b34();
extern int FUN_00411c78();
extern int FUN_00411ebc();
extern int FUN_0041243c();
extern int FUN_00412694();
extern int FUN_004128a6();
extern int FUN_00412e8c();
extern int FUN_00412e9c();
extern int FUN_00413014();
extern int FUN_00413070();
extern int FUN_004130b0();
extern int FUN_004132b0();
extern int FUN_00413448();
extern int FUN_00413b4e();
extern int FUN_00413dc8();
extern int FUN_00413f45();
extern int FUN_00413fd2();
extern int FUN_00414055();
extern int FUN_00414360();
extern int FUN_004148d0();
extern int FUN_00415160();
extern int FUN_004153d4();
extern int FUN_00415404();
extern int FUN_00415448();
extern int FUN_004154b8();
extern int FUN_0041574c();
extern int FUN_004157e8();
extern int FUN_00415894();
extern int FUN_004159a8();
extern int FUN_00415a94();
extern int FUN_00415b50();
extern int FUN_00415f64();
extern int FUN_00416044();
extern int FUN_0041611c();
extern int FUN_0041612c();
extern int FUN_00416264();
extern int FUN_004163b4();
extern int FUN_0041643c();
extern int FUN_00416494();
extern int FUN_004164d4();
extern int FUN_004165a4();
extern int FUN_004166c4();
extern int FUN_00416714();
extern int FUN_00416a10();
extern int FUN_0041a2f4();
extern int FUN_0041f6a0();
extern int FUN_0041fb7c();
extern int FUN_0041fe68();
extern int FUN_0041ff50();
extern int FUN_0042003c();
extern int FUN_00420060();
extern int FUN_004202ac();
extern int FUN_004203a0();
extern int FUN_004205d8();
extern int FUN_00420b1c();
extern int FUN_00420cf0();
extern int FUN_00420e6c();
extern int FUN_00420ee8();
extern int FUN_00422064();
extern int FUN_004220cc();
extern int FUN_00422128();
extern int FUN_00422184();
extern int FUN_004221ec();
extern int FUN_00422358();
extern int FUN_00422548();
extern int FUN_0042293c();
extern int FUN_00422c74();
extern int FUN_00423210();
extern int FUN_0042353c();
extern int FUN_004237c0();
extern int FUN_00423804();
extern int FUN_00424930();
extern int FUN_00425314();
extern int FUN_00425a98();
extern int FUN_00425b88();
extern int FUN_00425e74();
extern int FUN_00426ab4();
extern int FUN_00428548();
extern int FUN_004287c0();
extern int FUN_004288d0();
extern int FUN_0042895c();
extern int FUN_004295e4();
extern int FUN_00429994();
extern int FUN_0042a4d8();
extern int FUN_0042b5f0();
extern int FUN_0042c1a4();
extern int FUN_0042fa4c();
extern int FUN_0042fd58();
extern int FUN_00430698();
extern int FUN_004308b8();
extern int FUN_00430a30();
extern int FUN_00430bde();
extern int FUN_00430efc();
extern int FUN_00432f98();
extern int FUN_00433a70();
extern int FUN_004351d0();
extern int FUN_00435254();
extern int FUN_00436c04();
extern int FUN_00436cfc();
extern int FUN_00436dd0();
extern int FUN_0043709c();
extern int FUN_004371fc();
extern int FUN_0043b26c();
extern int FUN_0043b7bc();
extern int FUN_0043c4e8();
extern int FUN_0043c55c();
extern int FUN_0043c5d0();
extern int FUN_0043c738();
extern int FUN_0043dbd8();
extern int FUN_0043dd00();
extern int FUN_00440980();
extern int FUN_00440ac4();
extern int FUN_00440f60();
extern int FUN_00441394();
extern int FUN_0044282c();
extern int FUN_00442b08();
extern int FUN_004430b8();
extern int FUN_00443b10();
extern int FUN_00443f90();
extern int FUN_00444048();
extern int FUN_004448e0();
extern int FUN_00444a60();
extern int FUN_00444e3c();
extern int FUN_00444fcc();
extern int FUN_00445030();
extern int FUN_004450dc();
extern int FUN_00445b40();
extern int FUN_00445b78();
extern int FUN_00446c10();
extern int FUN_004471c0();
extern int FUN_004477a0();
extern int FUN_00447960();
extern int FUN_00447a9c();
extern int FUN_004480bc();
extern int FUN_00448228();
extern int FUN_004496d8();
extern int FUN_00449728();
extern int FUN_00449928();
extern int FUN_00449958();
extern int FUN_00449c54();
extern int FUN_00449dd8();
extern int FUN_00449ee0();
extern int FUN_0044a1a4();
extern int FUN_0044a2b0();
extern int FUN_0044a32c();
extern int FUN_0044a3a0();
extern int FUN_0044a4e4();
extern int FUN_0044a904();
extern int FUN_0044aa28();
extern int FUN_0044aaf0();
extern int FUN_0044ac9c();
extern int FUN_0044aec8();
extern int FUN_0044b5c0();
extern int FUN_0044b684();
extern int FUN_0044b970();
extern int FUN_0044b9c0();
extern int FUN_0044bb88();
extern int FUN_0044c418();
extern int FUN_0044c538();
extern int FUN_0044c800();
extern int FUN_0044c8e8();
extern int FUN_0044cb48();
extern int FUN_0044ccb8();
extern int FUN_0044ced4();
extern int FUN_0044cef0();
extern int FUN_0044d0e4();
extern int FUN_0044d3d4();
extern int FUN_0044d538();
extern int FUN_0044d630();
extern int FUN_0044d650();
extern int FUN_0044d974();
extern int FUN_0044db70();
extern int FUN_0044e148();
extern int FUN_0044e170();
extern int FUN_0044e440();
extern int FUN_0044e618();
extern int FUN_0044e6a0();
extern int FUN_0044eb1c();
extern int FUN_0044ecac();
extern int FUN_0044f5c8();
extern int FUN_0044f65c();
extern int FUN_0044f6b0();
extern int FUN_0044f814();
extern int FUN_0044fbc0();
extern int FUN_0044fca4();
extern int FUN_0044fce8();
extern int FUN_0044fdb4();
extern int FUN_0044ff48();
extern int FUN_00450640();
extern int FUN_0045099c();
extern int FUN_00450c7c();
extern int FUN_00450e20();
extern int FUN_004518ac();
extern int FUN_00451a80();
extern int FUN_00451ac0();
extern int FUN_00451ae0();
extern int FUN_00451d4c();
extern int FUN_00451d6c();
extern int FUN_00451fdc();
extern int FUN_00451ff0();
extern int FUN_0045202c();
extern int FUN_00452090();
extern int FUN_004525a0();
extern int FUN_00452790();
extern int FUN_004527f0();
extern int FUN_004529d4();
extern int FUN_00452a34();
extern int FUN_00452d30();
extern int FUN_00452da0();
extern int FUN_00452dc0();
extern int FUN_00452fa4();
extern int FUN_00452fc4();
extern int FUN_00453580();
extern int FUN_004535c0();
extern int FUN_004539d0();
extern int FUN_004539f0();
extern int FUN_00453a18();
extern int FUN_00453a20();
extern int FUN_00453b98();
extern int FUN_00454158();
extern int FUN_004541e8();
extern int FUN_00454278();
extern int FUN_00454308();
extern int FUN_00454398();
extern int FUN_00454550();
extern int FUN_00454804();
extern int FUN_00454a70();
extern int FUN_00454e9c();
extern int FUN_00454f0c();
extern int FUN_004551b0();
extern int FUN_00455260();
extern int FUN_0045572c();
extern int FUN_004559d4();
extern int FUN_00455de4();
extern int FUN_00455edb();
extern int FUN_00456034();
extern int FUN_0045607b();
extern int FUN_0045644e();
extern int FUN_0045645e();
extern int FUN_00456717();
extern int FUN_0045672e();
extern int FUN_00456af2();
extern int FUN_00456b67();
extern int FUN_00456bf3();
extern int FUN_00456cae();
extern int FUN_00456cdf();
extern int FUN_0045703b();
extern int FUN_00457040();
extern int FUN_00457041();
extern int FUN_0045704f();
extern int FUN_004571e4();
extern int FUN_004572f8();
extern int FUN_00457408();
extern int FUN_00457571();
extern int FUN_00457631();
extern int FUN_004576c5();
extern int FUN_00457d39();
extern int FUN_00457e84();
extern int FUN_00457ee9();
extern int FUN_00457f0f();
extern int FUN_00457f40();
extern int FUN_00457f9f();
extern int FUN_0045809c();
extern int FUN_004580b7();
extern int FUN_004585e8();
extern int FUN_00458a31();
extern int FUN_00458b45();
extern int FUN_00458bc7();
extern int FUN_00458cc5();
extern int FUN_00458e2f();
extern int FUN_00459968();
extern int FUN_00459a88();
extern int FUN_00459b19();
extern int FUN_00459b51();
extern int FUN_00459bc5();
extern int FUN_00459c68();
extern int FUN_00459e4a();
extern int FUN_00459e6e();
extern int FUN_00459ea8();
extern int FUN_00459f57();
extern int FUN_00459fc0();
extern int FUN_00459ff5();
extern int FUN_0045a10b();
extern int FUN_0045a3b0();
extern int FUN_0045a453();
extern int FUN_0045a4c6();
extern int FUN_0045a9f1();
extern int FUN_0045aa4a();
extern int FUN_0045aa9f();
extern int FUN_0045aaac();
extern int FUN_0045acb6();
extern int FUN_0045ad67();
extern int FUN_0045ad94();
extern int FUN_0045ae0c();
extern int FUN_0045aeaa();
extern int FUN_0045b3d9();
extern int FUN_0045b5ae();
extern int FUN_0045b5d4();
extern int FUN_0045b688();
extern int FUN_0045b6f3();
extern int FUN_0045b812();
extern int FUN_0045b814();
extern int FUN_0045bdda();
extern int FUN_0045be98();
extern int FUN_0045c2ab();
extern int File_Load();
extern int Find_Lowest_Corner();
extern int Fire();
extern int FirstSavedGame();
extern int Flying_Objects_Pos_Ang();
extern int FreeParticle();
extern int Front_End();
extern int GTERPS();
extern int GTERPT();
extern int GTERT();
extern int Generate_Surface_Normals();
extern int Generate_Transparency_Tables();
extern int GetCommandLineA();
extern int GetConsoleMode();
extern int GetCurrentProcessId();
extern int GetCurrentThread();
extern int GetCurrentThreadId();
extern int GetEnvironmentStrings();
extern int GetFileType();
extern int GetLastError();
extern int GetModuleFileNameA();
extern int GetModuleHandleA();
extern int GetStdHandle();
extern int GetVersion();
extern int Get_Car_Angle();
extern int Get_Corner_Positions();
extern int Get_Current_Recording_Season();
extern int Get_Direction_Cosines();
extern int Get_Race_Positions();
extern int Ground_Collision();
extern int Highlight_Area();
extern int InitCardBlocks();
extern int InitCardSystem();
extern int Init_Application();
extern int Init_Car_Cluts();
extern int Init_Car_Doors();
extern int Init_Car_Graphics();
extern int Init_Damage_Indicator();
extern int Init_End_Race();
extern int Init_Flag();
extern int Init_Front_End();
extern int Init_Game();
extern int Init_Graphics();
extern int Init_League_Info();
extern int Init_LensFlare();
extern int Init_Main();
extern int Init_MultiLeague_Info();
extern int Init_Overlays();
extern int Init_Primitive_Buffer();
extern int Init_Rollercoaster();
extern int Init_Scene();
extern int Init_Scene_Objects();
extern int Init_Sky();
extern int Init_StockCar_Championship();
extern int Init_StockCar_MultiChamp();
extern int Init_Texture_Animation();
extern int Init_The_Floaty_Camera();
extern int Init_Track_Strip_Numbers();
extern int Init_Wild_Bill();
extern int Init_Wrecking_Championship();
extern int InitialiseAI();
extern int InitialiseDenting();
extern int InitialiseParticleSystem();
extern int Initialise_Pause_Mode();
extern int Interpolate_Direction_Vectors_Left();
extern int Kill_Sound();
extern int LoadCardFile();
extern int LoadCardFiles();
extern int LoadImage();
extern int LoadSave();
extern int Load_Card_File();
extern int Load_Cluts();
extern int Load_Completion_Status();
extern int Load_First_Config();
extern int Load_Game_Vags();
extern int Load_Sprite_Info();
extern int Load_Textures();
extern int Loading_Screen_From_Slab();
extern int LocalAlloc();
extern int LocalFree();
extern int MPE_InitHeap();
extern int MPE_free();
extern int MPE_malloc();
extern int Make_Car_Fly();
extern int Map_Height();
extern int Mask_Point_In_Quad();
extern int Modify_Sound();
extern int Modify_Sprite();
extern int Modify_TDF();
extern int MoveImageClut();
extern int Move_Forward_Strip();
extern int MulMatrix2();
extern int MultiChamp();
extern int Obstacle_Ahead();
extern int Order_Cars();
extern int OuterProduct12();
extern int PC_Write_File();
extern int Pause_Mode();
extern int Pit_Camera_Control();
extern int Play_Click_FX();
extern int Play_Game();
extern int Play_Intro();
extern int Play_Movie();
extern int Play_Sound();
extern int Play_Xtro();
extern int Point_Camera();
extern int PopMatrix();
extern int Practice_Over();
extern int Pre_Rotate();
extern int Print();
extern int Print_Draw();
extern int Print_Font();
extern int Print_Ink();
extern int Print_InkRGB();
extern int Print_Locate();
extern int Profile_Init();
extern int Promote_And_Relegate();
extern int PushMatrix();
extern int PutDispEnv();
extern int PutDrawEnv();
extern int Race_Over();
extern int ReadConsoleInputA();
extern int ReadFile();
extern int Read_Directory();
extern int Recommended_Acceleration();
extern int Record_Event();
extern int ReleaseMutex();
extern int Remove_Object();
extern int Remove_Scene_Objects();
extern int Request_Flying_Object();
extern int Reset_League_Info();
extern int Reset_Primitive_Buffer();
extern int RotMatrixX();
extern int RotMatrixY();
extern int RotMatrixYXZ();
extern int RotMatrixZ();
extern int RotTrans();
extern int RotTransPers();
extern int Rotate_Slab_On();
extern int SaveCardFile();
extern int Save_Game();
extern int Search_For_Sprite();
extern int Search_For_Strip();
extern int Secret();
extern int Select_Champ();
extern int Select_ChampQS();
extern int Select_DDPract();
extern int Select_Pract();
extern int Select_TimeT();
extern int Select_Total();
extern int SetConsoleMode();
extern int SetEvent();
extern int SetFilePointer();
extern int SetFogNearFar();
extern int SetHighLight();
extern int SetPalette();
extern int SetStdHandle();
extern int SetVideoMode();
extern int Set_Ambient_Light();
extern int Set_Depth_Cue();
extern int Set_Draw_Mode();
extern int Set_Load_Textures();
extern int Set_Object();
extern int Set_World_Matrix();
extern int Set_World_Position();
extern int Set_World_View();
extern int Set_Zclip();
extern int Setup_Controller();
extern int Setup_Debris();
extern int Setup_Driver_Names();
extern int Setup_Flying_Objects();
extern int Setup_Font();
extern int Setup_Joystick();
extern int Setup_Object_Block();
extern int Setup_Pad();
extern int Setup_Screen_Lines();
extern int Setup_Screen_Text();
extern int Setup_Sprite();
extern int Smoke();
extern int Sort_Leagues();
extern int Sort_MultiLeague();
extern int Sort_RacePos();
extern int Sound_Init();
extern int Sound_Remove();
extern int Sound_Restart();
extern int Sound_Stop();
extern int Sparking();
extern int Sparks();
extern int Start_CD_Audio();
extern int Start_New_Season_Stats();
extern int Start_Roll();
extern int Steam();
extern int Strip_Distance();
extern int Strip_Trigger_Handler();
extern int Swap_Buffers();
extern int System_Error();
extern int Terminate_Replay();
extern int Terminate_Replay_Bodge();
extern int TextureDentHiCar();
extern int TextureDentMidCar();
extern int Texture_Animation();
extern int TlsAlloc();
extern int TlsFree();
extern int TlsGetValue();
extern int TlsSetValue();
extern int Toggle_Car();
extern int Toggle_Track();
extern int Track_Follow();
extern int TransformEnemyWheels();
extern int Translate_Keypress();
extern int UndentCar();
extern int Unlock_Channel();
extern int UpdateFlag();
extern int Update_Championship_Stats();
extern int Update_Debris();
extern int Update_Flying_Objects();
extern int Update_Jimmy_Spunk_Times();
extern int Update_League_Info();
extern int Update_Object();
extern int Update_Other_Objects();
extern int Update_Race_CountDown();
extern int Update_Scene_Objects();
extern int Update_Track_Stats();
extern int VSync();
extern int VSyncCallback();
extern int VVDraw_Object();
extern int VectorNormalS();
extern int VectorNormalSS();
extern int View_BestLaps();
extern int View_Frontend_Replay();
extern int WaitForSingleObject();
extern int WriteConsoleA();
extern int WriteFile();
extern int Zero_Flying_Object();
extern int _FtoS();
extern int _Scale();
extern int _Scale10V();
extern int __AccessSemaphore();
extern int __CHP();
extern int __CloseSemaphore();
extern int __CommonInit();
extern int __DoneExceptionHandler();
extern int __EnterWVIDEO();
extern int __ExpandDGROUP();
extern int __FiniRtns();
extern int __GetNTAccessAttr();
extern int __GetNTShareAttr();
extern int __HasLeadingZero();
extern int __HeapManager_expand();
extern int __IOMode();
extern int __InitMultipleThread();
extern int __InitRtns();
extern int __InitThreadData();
extern int __MemAllocator();
extern int __MemFree();
extern int __NTAddThread();
extern int __NTAtMaxFiles();
extern int __NTConsoleInput();
extern int __NTGetFakeHandle();
extern int __NTInit();
extern int __NTMainInit();
extern int __NTRemoveFileHandle();
extern int __NTRemoveThread();
extern int __NTThreadInit();
extern int __Nan_Inf();
extern int __NewExceptionHandler();
extern int __ReleaseSemaphore();
extern int __RemoveThreadData();
extern int __SigFini();
extern int __WinMain();
extern int __allocfp();
extern int __chktty();
extern int __cnvs2d();
extern int __exit();
extern int __fatal_runtime_error();
extern int __fdiv_m32();
extern int __fdiv_m64();
extern int __fdivp_sti_st();
extern int __flushall();
extern int __freefp();
extern int __full_io_exit();
extern int __get_std_stream();
extern int __init_80x87();
extern int __initthread();
extern int __ioalloc();
extern int __math1err();
extern int __matherr();
extern int __nmemneed();
extern int __null_int23_exit();
extern int __open_flags();
extern int __prtf();
extern int __purgefp();
extern int __qwrite();
extern int __rterrmsg();
extern int __set_EDOM();
extern int __set_ERANGE();
extern int __set_doserrno();
extern int __set_errno_nt();
extern int __shutdown_stream();
extern int __sigfpe_handler();
extern int __threadid();
extern int _beginthread();
extern int _control87();
extern int _endthread();
extern int _fpreset();
extern int _itoa();
extern int _lseek();
extern int _ltoa();
extern int _matherr();
extern int _nheapshrink();
extern int _stricmp();
extern int _tolower();
extern int _toupper();
extern int dosretax();
extern int draw_face_3pt_flat_dpq();
extern int draw_face_3pt_gour_dpq();
extern int draw_face_3pt_pict_dpq();
extern int draw_face_3pt_pict_dpq_lit();
extern int draw_face_3pt_text_dpq();
extern int draw_face_3pt_text_dpq_squash();
extern int draw_face_4pt_pict_dpq();
extern int draw_face_4pt_pict_dpq_lit();
extern int draw_face_sprite();
extern int draw_face_sprite_dpq();
extern int draw_half();
extern int draw_text_half_trans();
extern int flushall();
extern int getch();
extern int getche();
extern int getpid();
extern int gte_MulMatrix0();
extern int gte_SetRotMatrix();
extern int gte_dpcs();
extern int gte_ncds();
extern int isatty();
extern int itoa();
extern int joyGetDevCapsA();
extern int joyGetPos();
extern int ltoa();
extern int mciSendCommandA();
extern int nexpand();
extern int nfree();
extern int nmalloc();
extern int nrealloc();
extern int open();
extern int putch();
extern int rcos();
extern int rsin();
extern int sopen();
extern int tell();
extern int timeBeginPeriod();
extern int timeEndPeriod();
extern int timeKillEvent();
extern int timeSetEvent();
extern int ultoa();
extern int unlink();
extern int utoa();
extern int wstart2_();
extern int Load_Null();
extern int FUN_00414fb0();
extern int Load_Texture2();
typedef struct{unsigned va;void*fn;}dd2_fnent;
dd2_fnent dd2_fnmap[]={
{0x00414f30,(void*)&Load_Null},
{0x00414fb0,(void*)&FUN_00414fb0},
{0x00414ff4,(void*)&Load_Texture2},
{0x431374,(void*)&AI_Com_Server},
{0x44c48c,(void*)&Add_Computer_Info},
{0x420fc0,(void*)&Allocate_Font_Buffers},
{0x44801c,(void*)&Allocate_Sound_Effect},
{0x447dfc,(void*)&Amplitude},
{0x413d0b,(void*)&ApplyMatrixLV},
{0x435860,(void*)&ApplyWheelOverlay},
{0x433e40,(void*)&Barrier_Collision},
{0x434b4c,(void*)&Barrier_Corner_Collision},
{0x42a9b0,(void*)&Bonnet_Smoke},
{0x444e9c,(void*)&Boot_Lost_Geometry},
{0x4516ac,(void*)&Button_Pressed},
{0x416620,(void*)&CD_Check},
{0x4160ec,(void*)&CD_Close},
{0x416294,(void*)&CD_Pause},
{0x4162c4,(void*)&CD_Restart},
{0x42ca3c,(void*)&CLUT_Animation},
{0x441270,(void*)&Calc_Car_Angles_Square},
{0x4410f4,(void*)&Calc_Car_Tilt},
{0x43840c,(void*)&Calc_Head_On_Clsn_Dynamics},
{0x444ddc,(void*)&Calc_Null_Suspension},
{0x420488,(void*)&Calc_Object_Angles},
{0x420414,(void*)&Calc_Object_MatrixYZX},
{0x444c1c,(void*)&Calc_Suspension_Right_Wheels},
{0x443860,(void*)&Calc_Track_Positions},
{0x44be58,(void*)&Calculate_Finish},
{0x44c090,(void*)&Calculate_Results},
{0x44b938,(void*)&Call_Loaded_Game},
{0x429738,(void*)&Camera_Pad_Control},
{0x43fb24,(void*)&Car_1pt_Motion_3D},
{0x43e504,(void*)&Car_2pt_Motion_3D},
{0x42968c,(void*)&Car_Camera},
{0x44216c,(void*)&Car_Drive_2pt_Motion},
{0x4413ac,(void*)&Car_Drive_Motion},
{0x4427cc,(void*)&Car_Fly_Motion},
{0x43d68c,(void*)&Car_Fly_Motion_3D},
{0x43d8e4,(void*)&Car_Grounded_Motion_3D},
{0x43d418,(void*)&Car_Landed},
{0x43d5d4,(void*)&Car_Landed_On_Corner},
{0x442cdc,(void*)&Car_Movement},
{0x44bc08,(void*)&Championship},
{0x43b380,(void*)&Change_Bonnet_Clut},
{0x43b4b8,(void*)&Change_Boot_Clut},
{0x433c04,(void*)&CheckPointScoring},
{0x43978c,(void*)&Check_2D_Car_Collision},
{0x445554,(void*)&Check_Bonnet_Removal},
{0x4455e0,(void*)&Check_Boot_Removal},
{0x4161d8,(void*)&Check_For_CD_Loop},
{0x439a04,(void*)&Check_Ground_Car_Collision},
{0x44c76c,(void*)&Check_League_Standing},
{0x43a100,(void*)&Check_Space_Car_Collision},
{0x41281c,(void*)&ClearOTagR},
{0x45c3dc,(void*)&CloseHandle},
{0x412a94,(void*)&Close_Application},
{0x447360,(void*)&Control_Car_Replay},
{0x45c33a,(void*)&CreateEventA},
{0x45c418,(void*)&CreateFileA},
{0x45c39a,(void*)&CreateMutexA},
{0x45c334,(void*)&CreateThread},
{0x41fc00,(void*)&Create_Object},
{0x413208,(void*)&DDRelease},
{0x4157b0,(void*)&DSGetWaveResource},
{0x415658,(void*)&DSLoadSoundBuffer},
{0x4236c0,(void*)&Debug_Stub},
{0x415550,(void*)&Decompress},
{0x430908,(void*)&Decrunch_Object_Block},
{0x45c370,(void*)&DeleteFileA},
{0x4232f8,(void*)&DeleteFileMC},
{0x44b4e0,(void*)&DemoMode},
{0x432c4c,(void*)&Determine_AI},
{0x45c436,(void*)&DirectDrawCreate},
{0x45c42a,(void*)&DirectSoundCreate},
{0x42ff7c,(void*)&Display_Position_Pointers},
{0x455358,(void*)&Display_Season_Status},
{0x43a400,(void*)&Do_Car_Collisions},
{0x44c1d4,(void*)&Do_End_Of_Season_Stuff},
{0x429f10,(void*)&Do_The_Floaty_Camera_Thing},
{0x447ea8,(void*)&DopplerFrequency},
{0x42d628,(void*)&DrawFlagObject},
{0x42dca8,(void*)&DrawLensFlare},
{0x412843,(void*)&DrawOTag},
{0x436ca4,(void*)&DrawParticles},
{0x412885,(void*)&DrawPrim},
{0x420b6c,(void*)&Draw_All},
{0x42c02c,(void*)&Draw_Car},
{0x42d468,(void*)&Draw_Dynamic_Objects},
{0x420d44,(void*)&Draw_Font_Poly},
{0x42ced8,(void*)&Draw_Other_Objects},
{0x42e784,(void*)&Draw_Overlays},
{0x430464,(void*)&Draw_Scene_Object},
{0x451308,(void*)&Draw_Screen_Lines},
{0x450ed4,(void*)&Draw_Screen_Polys},
{0x4518f8,(void*)&Draw_Semi_Trans_Poly},
{0x431200,(void*)&Draw_Sky},
{0x4517b8,(void*)&Draw_Slab},
{0x41fdbc,(void*)&Draw_Subdiv_Object},
{0x420bb8,(void*)&Draw_Tile},
{0x4234f0,(void*)&DupFileCheck},
{0x4210c4,(void*)&Duplicate_Font},
{0x453d48,(void*)&End_Of_Season},
{0x4520f0,(void*)&Enter_Driver_Names},
{0x45c412,(void*)&ExitProcess},
{0x45c32e,(void*)&ExitThread},
{0x41033a,(void*)&FUN_0041033a},
{0x41080d,(void*)&FUN_0041080d},
{0x410f74,(void*)&FUN_00410f74},
{0x4110a4,(void*)&FUN_004110a4},
{0x4111e8,(void*)&FUN_004111e8},
{0x41132c,(void*)&FUN_0041132c},
{0x411a04,(void*)&FUN_00411a04},
{0x411b34,(void*)&FUN_00411b34},
{0x411c78,(void*)&FUN_00411c78},
{0x411ebc,(void*)&FUN_00411ebc},
{0x41243c,(void*)&FUN_0041243c},
{0x412694,(void*)&FUN_00412694},
{0x4128a6,(void*)&FUN_004128a6},
{0x412e8c,(void*)&FUN_00412e8c},
{0x412e9c,(void*)&FUN_00412e9c},
{0x413014,(void*)&FUN_00413014},
{0x413070,(void*)&FUN_00413070},
{0x4130b0,(void*)&FUN_004130b0},
{0x4132b0,(void*)&FUN_004132b0},
{0x413448,(void*)&FUN_00413448},
{0x413b4e,(void*)&FUN_00413b4e},
{0x413dc8,(void*)&FUN_00413dc8},
{0x413f45,(void*)&FUN_00413f45},
{0x413fd2,(void*)&FUN_00413fd2},
{0x414055,(void*)&FUN_00414055},
{0x414360,(void*)&FUN_00414360},
{0x4148d0,(void*)&FUN_004148d0},
{0x415160,(void*)&FUN_00415160},
{0x4153d4,(void*)&FUN_004153d4},
{0x415404,(void*)&FUN_00415404},
{0x415448,(void*)&FUN_00415448},
{0x4154b8,(void*)&FUN_004154b8},
{0x41574c,(void*)&FUN_0041574c},
{0x4157e8,(void*)&FUN_004157e8},
{0x415894,(void*)&FUN_00415894},
{0x4159a8,(void*)&FUN_004159a8},
{0x415a94,(void*)&FUN_00415a94},
{0x415b50,(void*)&FUN_00415b50},
{0x415f64,(void*)&FUN_00415f64},
{0x416044,(void*)&FUN_00416044},
{0x41611c,(void*)&FUN_0041611c},
{0x41612c,(void*)&FUN_0041612c},
{0x416264,(void*)&FUN_00416264},
{0x4163b4,(void*)&FUN_004163b4},
{0x41643c,(void*)&FUN_0041643c},
{0x416494,(void*)&FUN_00416494},
{0x4164d4,(void*)&FUN_004164d4},
{0x4165a4,(void*)&FUN_004165a4},
{0x4166c4,(void*)&FUN_004166c4},
{0x416714,(void*)&FUN_00416714},
{0x416a10,(void*)&FUN_00416a10},
{0x41a2f4,(void*)&FUN_0041a2f4},
{0x41f6a0,(void*)&FUN_0041f6a0},
{0x41fb7c,(void*)&FUN_0041fb7c},
{0x41fe68,(void*)&FUN_0041fe68},
{0x41ff50,(void*)&FUN_0041ff50},
{0x42003c,(void*)&FUN_0042003c},
{0x420060,(void*)&FUN_00420060},
{0x4202ac,(void*)&FUN_004202ac},
{0x4203a0,(void*)&FUN_004203a0},
{0x4205d8,(void*)&FUN_004205d8},
{0x420b1c,(void*)&FUN_00420b1c},
{0x420cf0,(void*)&FUN_00420cf0},
{0x420e6c,(void*)&FUN_00420e6c},
{0x420ee8,(void*)&FUN_00420ee8},
{0x422064,(void*)&FUN_00422064},
{0x4220cc,(void*)&FUN_004220cc},
{0x422128,(void*)&FUN_00422128},
{0x422184,(void*)&FUN_00422184},
{0x4221ec,(void*)&FUN_004221ec},
{0x422358,(void*)&FUN_00422358},
{0x422548,(void*)&FUN_00422548},
{0x42293c,(void*)&FUN_0042293c},
{0x422c74,(void*)&FUN_00422c74},
{0x423210,(void*)&FUN_00423210},
{0x42353c,(void*)&FUN_0042353c},
{0x4237c0,(void*)&FUN_004237c0},
{0x423804,(void*)&FUN_00423804},
{0x424930,(void*)&FUN_00424930},
{0x425314,(void*)&FUN_00425314},
{0x425a98,(void*)&FUN_00425a98},
{0x425b88,(void*)&FUN_00425b88},
{0x425e74,(void*)&FUN_00425e74},
{0x426ab4,(void*)&FUN_00426ab4},
{0x428548,(void*)&FUN_00428548},
{0x4287c0,(void*)&FUN_004287c0},
{0x4288d0,(void*)&FUN_004288d0},
{0x42895c,(void*)&FUN_0042895c},
{0x4295e4,(void*)&FUN_004295e4},
{0x429994,(void*)&FUN_00429994},
{0x42a4d8,(void*)&FUN_0042a4d8},
{0x42b5f0,(void*)&FUN_0042b5f0},
{0x42c1a4,(void*)&FUN_0042c1a4},
{0x42fa4c,(void*)&FUN_0042fa4c},
{0x42fd58,(void*)&FUN_0042fd58},
{0x430698,(void*)&FUN_00430698},
{0x4308b8,(void*)&FUN_004308b8},
{0x430a30,(void*)&FUN_00430a30},
{0x430bde,(void*)&FUN_00430bde},
{0x430efc,(void*)&FUN_00430efc},
{0x432f98,(void*)&FUN_00432f98},
{0x433a70,(void*)&FUN_00433a70},
{0x4351d0,(void*)&FUN_004351d0},
{0x435254,(void*)&FUN_00435254},
{0x436c04,(void*)&FUN_00436c04},
{0x436cfc,(void*)&FUN_00436cfc},
{0x436dd0,(void*)&FUN_00436dd0},
{0x43709c,(void*)&FUN_0043709c},
{0x4371fc,(void*)&FUN_004371fc},
{0x43b26c,(void*)&FUN_0043b26c},
{0x43b7bc,(void*)&FUN_0043b7bc},
{0x43c4e8,(void*)&FUN_0043c4e8},
{0x43c55c,(void*)&FUN_0043c55c},
{0x43c5d0,(void*)&FUN_0043c5d0},
{0x43c738,(void*)&FUN_0043c738},
{0x43dbd8,(void*)&FUN_0043dbd8},
{0x43dd00,(void*)&FUN_0043dd00},
{0x440980,(void*)&FUN_00440980},
{0x440ac4,(void*)&FUN_00440ac4},
{0x440f60,(void*)&FUN_00440f60},
{0x441394,(void*)&FUN_00441394},
{0x44282c,(void*)&FUN_0044282c},
{0x442b08,(void*)&FUN_00442b08},
{0x4430b8,(void*)&FUN_004430b8},
{0x443b10,(void*)&FUN_00443b10},
{0x443f90,(void*)&FUN_00443f90},
{0x444048,(void*)&FUN_00444048},
{0x4448e0,(void*)&FUN_004448e0},
{0x444a60,(void*)&FUN_00444a60},
{0x444e3c,(void*)&FUN_00444e3c},
{0x444fcc,(void*)&FUN_00444fcc},
{0x445030,(void*)&FUN_00445030},
{0x4450dc,(void*)&FUN_004450dc},
{0x445b40,(void*)&FUN_00445b40},
{0x445b78,(void*)&FUN_00445b78},
{0x446c10,(void*)&FUN_00446c10},
{0x4471c0,(void*)&FUN_004471c0},
{0x4477a0,(void*)&FUN_004477a0},
{0x447960,(void*)&FUN_00447960},
{0x447a9c,(void*)&FUN_00447a9c},
{0x4480bc,(void*)&FUN_004480bc},
{0x448228,(void*)&FUN_00448228},
{0x4496d8,(void*)&FUN_004496d8},
{0x449728,(void*)&FUN_00449728},
{0x449928,(void*)&FUN_00449928},
{0x449958,(void*)&FUN_00449958},
{0x449c54,(void*)&FUN_00449c54},
{0x449dd8,(void*)&FUN_00449dd8},
{0x449ee0,(void*)&FUN_00449ee0},
{0x44a1a4,(void*)&FUN_0044a1a4},
{0x44a2b0,(void*)&FUN_0044a2b0},
{0x44a32c,(void*)&FUN_0044a32c},
{0x44a3a0,(void*)&FUN_0044a3a0},
{0x44a4e4,(void*)&FUN_0044a4e4},
{0x44a904,(void*)&FUN_0044a904},
{0x44aa28,(void*)&FUN_0044aa28},
{0x44aaf0,(void*)&FUN_0044aaf0},
{0x44ac9c,(void*)&FUN_0044ac9c},
{0x44aec8,(void*)&FUN_0044aec8},
{0x44b5c0,(void*)&FUN_0044b5c0},
{0x44b684,(void*)&FUN_0044b684},
{0x44b970,(void*)&FUN_0044b970},
{0x44b9c0,(void*)&FUN_0044b9c0},
{0x44bb88,(void*)&FUN_0044bb88},
{0x44c418,(void*)&FUN_0044c418},
{0x44c538,(void*)&FUN_0044c538},
{0x44c800,(void*)&FUN_0044c800},
{0x44c8e8,(void*)&FUN_0044c8e8},
{0x44cb48,(void*)&FUN_0044cb48},
{0x44ccb8,(void*)&FUN_0044ccb8},
{0x44ced4,(void*)&FUN_0044ced4},
{0x44cef0,(void*)&FUN_0044cef0},
{0x44d0e4,(void*)&FUN_0044d0e4},
{0x44d3d4,(void*)&FUN_0044d3d4},
{0x44d538,(void*)&FUN_0044d538},
{0x44d630,(void*)&FUN_0044d630},
{0x44d650,(void*)&FUN_0044d650},
{0x44d974,(void*)&FUN_0044d974},
{0x44db70,(void*)&FUN_0044db70},
{0x44e148,(void*)&FUN_0044e148},
{0x44e170,(void*)&FUN_0044e170},
{0x44e440,(void*)&FUN_0044e440},
{0x44e618,(void*)&FUN_0044e618},
{0x44e6a0,(void*)&FUN_0044e6a0},
{0x44eb1c,(void*)&FUN_0044eb1c},
{0x44ecac,(void*)&FUN_0044ecac},
{0x44f5c8,(void*)&FUN_0044f5c8},
{0x44f65c,(void*)&FUN_0044f65c},
{0x44f6b0,(void*)&FUN_0044f6b0},
{0x44f814,(void*)&FUN_0044f814},
{0x44fbc0,(void*)&FUN_0044fbc0},
{0x44fca4,(void*)&FUN_0044fca4},
{0x44fce8,(void*)&FUN_0044fce8},
{0x44fdb4,(void*)&FUN_0044fdb4},
{0x44ff48,(void*)&FUN_0044ff48},
{0x450640,(void*)&FUN_00450640},
{0x45099c,(void*)&FUN_0045099c},
{0x450c7c,(void*)&FUN_00450c7c},
{0x450e20,(void*)&FUN_00450e20},
{0x4518ac,(void*)&FUN_004518ac},
{0x451a80,(void*)&FUN_00451a80},
{0x451ac0,(void*)&FUN_00451ac0},
{0x451ae0,(void*)&FUN_00451ae0},
{0x451d4c,(void*)&FUN_00451d4c},
{0x451d6c,(void*)&FUN_00451d6c},
{0x451fdc,(void*)&FUN_00451fdc},
{0x451ff0,(void*)&FUN_00451ff0},
{0x45202c,(void*)&FUN_0045202c},
{0x452090,(void*)&FUN_00452090},
{0x4525a0,(void*)&FUN_004525a0},
{0x452790,(void*)&FUN_00452790},
{0x4527f0,(void*)&FUN_004527f0},
{0x4529d4,(void*)&FUN_004529d4},
{0x452a34,(void*)&FUN_00452a34},
{0x452d30,(void*)&FUN_00452d30},
{0x452da0,(void*)&FUN_00452da0},
{0x452dc0,(void*)&FUN_00452dc0},
{0x452fa4,(void*)&FUN_00452fa4},
{0x452fc4,(void*)&FUN_00452fc4},
{0x453580,(void*)&FUN_00453580},
{0x4535c0,(void*)&FUN_004535c0},
{0x4539d0,(void*)&FUN_004539d0},
{0x4539f0,(void*)&FUN_004539f0},
{0x453a18,(void*)&FUN_00453a18},
{0x453a20,(void*)&FUN_00453a20},
{0x453b98,(void*)&FUN_00453b98},
{0x454158,(void*)&FUN_00454158},
{0x4541e8,(void*)&FUN_004541e8},
{0x454278,(void*)&FUN_00454278},
{0x454308,(void*)&FUN_00454308},
{0x454398,(void*)&FUN_00454398},
{0x454550,(void*)&FUN_00454550},
{0x454804,(void*)&FUN_00454804},
{0x454a70,(void*)&FUN_00454a70},
{0x454e9c,(void*)&FUN_00454e9c},
{0x454f0c,(void*)&FUN_00454f0c},
{0x4551b0,(void*)&FUN_004551b0},
{0x455260,(void*)&FUN_00455260},
{0x45572c,(void*)&FUN_0045572c},
{0x4559d4,(void*)&FUN_004559d4},
{0x455de4,(void*)&FUN_00455de4},
{0x455edb,(void*)&FUN_00455edb},
{0x456034,(void*)&FUN_00456034},
{0x45607b,(void*)&FUN_0045607b},
{0x45644e,(void*)&FUN_0045644e},
{0x45645e,(void*)&FUN_0045645e},
{0x456717,(void*)&FUN_00456717},
{0x45672e,(void*)&FUN_0045672e},
{0x456af2,(void*)&FUN_00456af2},
{0x456b67,(void*)&FUN_00456b67},
{0x456bf3,(void*)&FUN_00456bf3},
{0x456cae,(void*)&FUN_00456cae},
{0x456cdf,(void*)&FUN_00456cdf},
{0x45703b,(void*)&FUN_0045703b},
{0x457040,(void*)&FUN_00457040},
{0x457041,(void*)&FUN_00457041},
{0x45704f,(void*)&FUN_0045704f},
{0x4571e4,(void*)&FUN_004571e4},
{0x4572f8,(void*)&FUN_004572f8},
{0x457408,(void*)&FUN_00457408},
{0x457571,(void*)&FUN_00457571},
{0x457631,(void*)&FUN_00457631},
{0x4576c5,(void*)&FUN_004576c5},
{0x457d39,(void*)&FUN_00457d39},
{0x457e84,(void*)&FUN_00457e84},
{0x457ee9,(void*)&FUN_00457ee9},
{0x457f0f,(void*)&FUN_00457f0f},
{0x457f40,(void*)&FUN_00457f40},
{0x457f9f,(void*)&FUN_00457f9f},
{0x45809c,(void*)&FUN_0045809c},
{0x4580b7,(void*)&FUN_004580b7},
{0x4585e8,(void*)&FUN_004585e8},
{0x458a31,(void*)&FUN_00458a31},
{0x458b45,(void*)&FUN_00458b45},
{0x458bc7,(void*)&FUN_00458bc7},
{0x458cc5,(void*)&FUN_00458cc5},
{0x458e2f,(void*)&FUN_00458e2f},
{0x459968,(void*)&FUN_00459968},
{0x459a88,(void*)&FUN_00459a88},
{0x459b19,(void*)&FUN_00459b19},
{0x459b51,(void*)&FUN_00459b51},
{0x459bc5,(void*)&FUN_00459bc5},
{0x459c68,(void*)&FUN_00459c68},
{0x459e4a,(void*)&FUN_00459e4a},
{0x459e6e,(void*)&FUN_00459e6e},
{0x459ea8,(void*)&FUN_00459ea8},
{0x459f57,(void*)&FUN_00459f57},
{0x459fc0,(void*)&FUN_00459fc0},
{0x459ff5,(void*)&FUN_00459ff5},
{0x45a10b,(void*)&FUN_0045a10b},
{0x45a3b0,(void*)&FUN_0045a3b0},
{0x45a453,(void*)&FUN_0045a453},
{0x45a4c6,(void*)&FUN_0045a4c6},
{0x45a9f1,(void*)&FUN_0045a9f1},
{0x45aa4a,(void*)&FUN_0045aa4a},
{0x45aa9f,(void*)&FUN_0045aa9f},
{0x45aaac,(void*)&FUN_0045aaac},
{0x45acb6,(void*)&FUN_0045acb6},
{0x45ad67,(void*)&FUN_0045ad67},
{0x45ad94,(void*)&FUN_0045ad94},
{0x45ae0c,(void*)&FUN_0045ae0c},
{0x45aeaa,(void*)&FUN_0045aeaa},
{0x45b3d9,(void*)&FUN_0045b3d9},
{0x45b5ae,(void*)&FUN_0045b5ae},
{0x45b5d4,(void*)&FUN_0045b5d4},
{0x45b688,(void*)&FUN_0045b688},
{0x45b6f3,(void*)&FUN_0045b6f3},
{0x45b812,(void*)&FUN_0045b812},
{0x45b814,(void*)&FUN_0045b814},
{0x45bdda,(void*)&FUN_0045bdda},
{0x45be98,(void*)&FUN_0045be98},
{0x45c2ab,(void*)&FUN_0045c2ab},
{0x415354,(void*)&File_Load},
{0x43d194,(void*)&Find_Lowest_Corner},
{0x4365cc,(void*)&Fire},
{0x4234c0,(void*)&FirstSavedGame},
{0x4259e8,(void*)&Flying_Objects_Pos_Ang},
{0x436c34,(void*)&FreeParticle},
{0x4500e8,(void*)&Front_End},
{0x4139a7,(void*)&GTERPS},
{0x4139e9,(void*)&GTERPT},
{0x41397e,(void*)&GTERT},
{0x426788,(void*)&Generate_Surface_Normals},
{0x413544,(void*)&Generate_Transparency_Tables},
{0x45c400,(void*)&GetCommandLineA},
{0x45c35e,(void*)&GetConsoleMode},
{0x45c3e2,(void*)&GetCurrentProcessId},
{0x45c340,(void*)&GetCurrentThread},
{0x45c3a0,(void*)&GetCurrentThreadId},
{0x45c40c,(void*)&GetEnvironmentStrings},
{0x45c3be,(void*)&GetFileType},
{0x45c3c4,(void*)&GetLastError},
{0x45c406,(void*)&GetModuleFileNameA},
{0x45c3f4,(void*)&GetModuleHandleA},
{0x45c3ca,(void*)&GetStdHandle},
{0x45c3fa,(void*)&GetVersion},
{0x432fd8,(void*)&Get_Car_Angle},
{0x43c910,(void*)&Get_Corner_Positions},
{0x44d648,(void*)&Get_Current_Recording_Season},
{0x433004,(void*)&Get_Direction_Cosines},
{0x443b8c,(void*)&Get_Race_Positions},
{0x43cf10,(void*)&Ground_Collision},
{0x43b5c0,(void*)&Highlight_Area},
{0x4234dc,(void*)&InitCardBlocks},
{0x4230f0,(void*)&InitCardSystem},
{0x412910,(void*)&Init_Application},
{0x43a684,(void*)&Init_Car_Cluts},
{0x43af34,(void*)&Init_Car_Doors},
{0x42c2d8,(void*)&Init_Car_Graphics},
{0x42a398,(void*)&Init_Damage_Indicator},
{0x443840,(void*)&Init_End_Race},
{0x42d4d0,(void*)&Init_Flag},
{0x44b148,(void*)&Init_Front_End},
{0x445c74,(void*)&Init_Game},
{0x445a70,(void*)&Init_Graphics},
{0x44c270,(void*)&Init_League_Info},
{0x42db08,(void*)&Init_LensFlare},
{0x4456e4,(void*)&Init_Main},
{0x44c344,(void*)&Init_MultiLeague_Info},
{0x42e024,(void*)&Init_Overlays},
{0x420d70,(void*)&Init_Primitive_Buffer},
{0x42c540,(void*)&Init_Rollercoaster},
{0x43023c,(void*)&Init_Scene},
{0x430cac,(void*)&Init_Scene_Objects},
{0x430e08,(void*)&Init_Sky},
{0x44bb44,(void*)&Init_StockCar_Championship},
{0x44bbc8,(void*)&Init_StockCar_MultiChamp},
{0x42c774,(void*)&Init_Texture_Animation},
{0x429ec0,(void*)&Init_The_Floaty_Camera},
{0x443da4,(void*)&Init_Track_Strip_Numbers},
{0x42c510,(void*)&Init_Wild_Bill},
{0x44bb00,(void*)&Init_Wrecking_Championship},
{0x433188,(void*)&InitialiseAI},
{0x425d70,(void*)&InitialiseDenting},
{0x43604c,(void*)&InitialiseParticleSystem},
{0x445e40,(void*)&Initialise_Pause_Mode},
{0x433074,(void*)&Interpolate_Direction_Vectors_Left},
{0x415c0c,(void*)&Kill_Sound},
{0x423464,(void*)&LoadCardFile},
{0x423390,(void*)&LoadCardFiles},
{0x412f3c,(void*)&LoadImage},
{0x4493a0,(void*)&LoadSave},
{0x44ac68,(void*)&Load_Card_File},
{0x41501c,(void*)&Load_Cluts},
{0x44b6b4,(void*)&Load_Completion_Status},
{0x44ac28,(void*)&Load_First_Config},
{0x4481d0,(void*)&Load_Game_Vags},
{0x416670,(void*)&Load_Sprite_Info},
{0x414f38,(void*)&Load_Textures},
{0x44b5d4,(void*)&Loading_Screen_From_Slab},
{0x45c36a,(void*)&LocalAlloc},
{0x45c34c,(void*)&LocalFree},
{0x4235c0,(void*)&MPE_InitHeap},
{0x42364c,(void*)&MPE_free},
{0x4235e4,(void*)&MPE_malloc},
{0x43d1f0,(void*)&Make_Car_Fly},
{0x427e40,(void*)&Map_Height},
{0x426964,(void*)&Mask_Point_In_Quad},
{0x415ec0,(void*)&Modify_Sound},
{0x416934,(void*)&Modify_Sprite},
{0x4457e8,(void*)&Modify_TDF},
{0x412fa8,(void*)&MoveImageClut},
{0x42872c,(void*)&Move_Forward_Strip},
{0x4137de,(void*)&MulMatrix2},
{0x44bd08,(void*)&MultiChamp},
{0x4334ec,(void*)&Obstacle_Ahead},
{0x44c7b0,(void*)&Order_Cars},
{0x414d68,(void*)&OuterProduct12},
{0x423744,(void*)&PC_Write_File},
{0x446260,(void*)&Pause_Mode},
{0x429a18,(void*)&Pit_Camera_Control},
{0x451aa0,(void*)&Play_Click_FX},
{0x423a20,(void*)&Play_Game},
{0x445dc0,(void*)&Play_Intro},
{0x414e30,(void*)&Play_Movie},
{0x415cd8,(void*)&Play_Sound},
{0x445dfc,(void*)&Play_Xtro},
{0x4207ac,(void*)&Point_Camera},
{0x414304,(void*)&PopMatrix},
{0x452a80,(void*)&Practice_Over},
{0x41fcfc,(void*)&Pre_Rotate},
{0x4212d4,(void*)&Print},
{0x421fb0,(void*)&Print_Draw},
{0x421204,(void*)&Print_Font},
{0x42125c,(void*)&Print_Ink},
{0x42129c,(void*)&Print_InkRGB},
{0x4211b8,(void*)&Print_Locate},
{0x423990,(void*)&Profile_Init},
{0x44c6c4,(void*)&Promote_And_Relegate},
{0x4142a8,(void*)&PushMatrix},
{0x412c60,(void*)&PutDispEnv},
{0x412bb4,(void*)&PutDrawEnv},
{0x454b68,(void*)&Race_Over},
{0x45c364,(void*)&ReadConsoleInputA},
{0x45c3e8,(void*)&ReadFile},
{0x4152f4,(void*)&Read_Directory},
{0x432f58,(void*)&Recommended_Acceleration},
{0x447800,(void*)&Record_Event},
{0x45c38e,(void*)&ReleaseMutex},
{0x41fcac,(void*)&Remove_Object},
{0x430da8,(void*)&Remove_Scene_Objects},
{0x4252d0,(void*)&Request_Flying_Object},
{0x44c2d8,(void*)&Reset_League_Info},
{0x420de4,(void*)&Reset_Primitive_Buffer},
{0x414540,(void*)&RotMatrixX},
{0x414670,(void*)&RotMatrixY},
{0x4144a0,(void*)&RotMatrixYXZ},
{0x4147a0,(void*)&RotMatrixZ},
{0x4149bc,(void*)&RotTrans},
{0x414a40,(void*)&RotTransPers},
{0x450d0c,(void*)&Rotate_Slab_On},
{0x42322c,(void*)&SaveCardFile},
{0x4536f0,(void*)&Save_Game},
{0x416688,(void*)&Search_For_Sprite},
{0x428850,(void*)&Search_For_Strip},
{0x44dcfc,(void*)&Secret},
{0x453044,(void*)&Select_Champ},
{0x4530bc,(void*)&Select_ChampQS},
{0x4531b0,(void*)&Select_DDPract},
{0x453100,(void*)&Select_Pract},
{0x45313c,(void*)&Select_TimeT},
{0x453170,(void*)&Select_Total},
{0x45c358,(void*)&SetConsoleMode},
{0x45c346,(void*)&SetEvent},
{0x45c3ee,(void*)&SetFilePointer},
{0x41424c,(void*)&SetFogNearFar},
{0x446fe0,(void*)&SetHighLight},
{0x412cb0,(void*)&SetPalette},
{0x45c3d0,(void*)&SetStdHandle},
{0x412b2c,(void*)&SetVideoMode},
{0x420268,(void*)&Set_Ambient_Light},
{0x42027c,(void*)&Set_Depth_Cue},
{0x420aa0,(void*)&Set_Draw_Mode},
{0x445764,(void*)&Set_Load_Textures},
{0x41fc5c,(void*)&Set_Object},
{0x4200dc,(void*)&Set_World_Matrix},
{0x4200bc,(void*)&Set_World_Position},
{0x42011c,(void*)&Set_World_View},
{0x420074,(void*)&Set_Zclip},
{0x422ba8,(void*)&Setup_Controller},
{0x42440c,(void*)&Setup_Debris},
{0x44c390,(void*)&Setup_Driver_Names},
{0x424a58,(void*)&Setup_Flying_Objects},
{0x421078,(void*)&Setup_Font},
{0x422bc0,(void*)&Setup_Joystick},
{0x430778,(void*)&Setup_Object_Block},
{0x44ba20,(void*)&Setup_Pad},
{0x4512f8,(void*)&Setup_Screen_Lines},
{0x451288,(void*)&Setup_Screen_Text},
{0x4166ec,(void*)&Setup_Sprite},
{0x4361e8,(void*)&Smoke},
{0x44c2f0,(void*)&Sort_Leagues},
{0x44c5a8,(void*)&Sort_MultiLeague},
{0x44c5e0,(void*)&Sort_RacePos},
{0x415a88,(void*)&Sound_Init},
{0x415b08,(void*)&Sound_Remove},
{0x415bc8,(void*)&Sound_Restart},
{0x415b70,(void*)&Sound_Stop},
{0x448eb0,(void*)&Sparking},
{0x4366bc,(void*)&Sparks},
{0x41619c,(void*)&Start_CD_Audio},
{0x44d390,(void*)&Start_New_Season_Stats},
{0x436f60,(void*)&Start_Roll},
{0x436980,(void*)&Steam},
{0x433a50,(void*)&Strip_Distance},
{0x448e4c,(void*)&Strip_Trigger_Handler},
{0x420cc4,(void*)&Swap_Buffers},
{0x42379c,(void*)&System_Error},
{0x4478ec,(void*)&Terminate_Replay},
{0x447920,(void*)&Terminate_Replay_Bodge},
{0x43b940,(void*)&TextureDentHiCar},
{0x43bf2c,(void*)&TextureDentMidCar},
{0x42c934,(void*)&Texture_Animation},
{0x45c382,(void*)&TlsAlloc},
{0x45c376,(void*)&TlsFree},
{0x45c388,(void*)&TlsGetValue},
{0x45c37c,(void*)&TlsSetValue},
{0x450944,(void*)&Toggle_Car},
{0x4508bc,(void*)&Toggle_Track},
{0x426ec4,(void*)&Track_Follow},
{0x435740,(void*)&TransformEnemyWheels},
{0x422f20,(void*)&Translate_Keypress},
{0x446c5c,(void*)&UndentCar},
{0x415fc4,(void*)&Unlock_Channel},
{0x42d998,(void*)&UpdateFlag},
{0x44d5d8,(void*)&Update_Championship_Stats},
{0x424510,(void*)&Update_Debris},
{0x425150,(void*)&Update_Flying_Objects},
{0x44d69c,(void*)&Update_Jimmy_Spunk_Times},
{0x44c508,(void*)&Update_League_Info},
{0x41ff24,(void*)&Update_Object},
{0x42cac8,(void*)&Update_Other_Objects},
{0x42fc70,(void*)&Update_Race_CountDown},
{0x430a90,(void*)&Update_Scene_Objects},
{0x44d478,(void*)&Update_Track_Stats},
{0x412bac,(void*)&VSync},
{0x412e7c,(void*)&VSyncCallback},
{0x43038c,(void*)&VVDraw_Object},
{0x4143c4,(void*)&VectorNormalS},
{0x41442c,(void*)&VectorNormalSS},
{0x44fe00,(void*)&View_BestLaps},
{0x44b4bc,(void*)&View_Frontend_Replay},
{0x45c394,(void*)&WaitForSingleObject},
{0x45c352,(void*)&WriteConsoleA},
{0x45c3d6,(void*)&WriteFile},
{0x42528c,(void*)&Zero_Flying_Object},
{0x45a53c,(void*)&_FtoS},
{0x45af7a,(void*)&_Scale},
{0x45afd2,(void*)&_Scale10V},
{0x4592b1,(void*)&__AccessSemaphore},
{0x45665c,(void*)&__CHP},
{0x459287,(void*)&__CloseSemaphore},
{0x459e9d,(void*)&__CommonInit},
{0x459268,(void*)&__DoneExceptionHandler},
{0x45ad3c,(void*)&__EnterWVIDEO},
{0x459c55,(void*)&__ExpandDGROUP},
{0x4597a1,(void*)&__FiniRtns},
{0x458abe,(void*)&__GetNTAccessAttr},
{0x458af9,(void*)&__GetNTShareAttr},
{0x45a368,(void*)&__HasLeadingZero},
{0x45aac1,(void*)&__HeapManager_expand},
{0x458c6f,(void*)&__IOMode},
{0x459561,(void*)&__InitMultipleThread},
{0x459756,(void*)&__InitRtns},
{0x459410,(void*)&__InitThreadData},
{0x45787e,(void*)&__MemAllocator},
{0x457926,(void*)&__MemFree},
{0x459496,(void*)&__NTAddThread},
{0x45889e,(void*)&__NTAtMaxFiles},
{0x45ae01,(void*)&__NTConsoleInput},
{0x458a70,(void*)&__NTGetFakeHandle},
{0x45705d,(void*)&__NTInit},
{0x457180,(void*)&__NTMainInit},
{0x458a0b,(void*)&__NTRemoveFileHandle},
{0x4594ed,(void*)&__NTRemoveThread},
{0x45944e,(void*)&__NTThreadInit},
{0x45ae17,(void*)&__Nan_Inf},
{0x459228,(void*)&__NewExceptionHandler},
{0x459314,(void*)&__ReleaseSemaphore},
{0x45a061,(void*)&__RemoveThreadData},
{0x458e5c,(void*)&__SigFini},
{0x4587c0,(void*)&__WinMain},
{0x456ef2,(void*)&__allocfp},
{0x456ffc,(void*)&__chktty},
{0x459aed,(void*)&__cnvs2d},
{0x4571c3,(void*)&__exit},
{0x45a146,(void*)&__fatal_runtime_error},
{0x45be00,(void*)&__fdiv_m32},
{0x45be4c,(void*)&__fdiv_m64},
{0x45bdc7,(void*)&__fdivp_sti_st},
{0x4598ea,(void*)&__flushall},
{0x456fa7,(void*)&__freefp},
{0x459865,(void*)&__full_io_exit},
{0x45c1e1,(void*)&__get_std_stream},
{0x459b2a,(void*)&__init_80x87},
{0x45ace2,(void*)&__initthread},
{0x457388,(void*)&__ioalloc},
{0x45bfab,(void*)&__math1err},
{0x45c1dc,(void*)&__matherr},
{0x459caf,(void*)&__nmemneed},
{0x456a32,(void*)&__null_int23_exit},
{0x455d15,(void*)&__open_flags},
{0x457a31,(void*)&__prtf},
{0x456fde,(void*)&__purgefp},
{0x458608,(void*)&__qwrite},
{0x45c13b,(void*)&__rterrmsg},
{0x456cc0,(void*)&__set_EDOM},
{0x456ccb,(void*)&__set_ERANGE},
{0x456ced,(void*)&__set_doserrno},
{0x458c19,(void*)&__set_errno_nt},
{0x456428,(void*)&__shutdown_stream},
{0x458ce6,(void*)&__sigfpe_handler},
{0x45702f,(void*)&__threadid},
{0x45acb9,(void*)&_beginthread},
{0x459f6a,(void*)&_control87},
{0x45acdb,(void*)&_endthread},
{0x45771e,(void*)&_fpreset},
{0x459d29,(void*)&_itoa},
{0x4572de,(void*)&_lseek},
{0x459e22,(void*)&_ltoa},
{0x45c188,(void*)&_matherr},
{0x45a9e5,(void*)&_nheapshrink},
{0x458b30,(void*)&_stricmp},
{0x456ca0,(void*)&_tolower},
{0x459e3c,(void*)&_toupper},
{0x458baa,(void*)&dosretax},
{0x41828c,(void*)&draw_face_3pt_flat_dpq},
{0x41c054,(void*)&draw_face_3pt_gour_dpq},
{0x41d360,(void*)&draw_face_3pt_pict_dpq},
{0x41d5e8,(void*)&draw_face_3pt_pict_dpq_lit},
{0x4199b0,(void*)&draw_face_3pt_text_dpq},
{0x419b94,(void*)&draw_face_3pt_text_dpq_squash},
{0x41df54,(void*)&draw_face_4pt_pict_dpq},
{0x41e184,(void*)&draw_face_4pt_pict_dpq_lit},
{0x41e4f4,(void*)&draw_face_sprite},
{0x41e948,(void*)&draw_face_sprite_dpq},
{0x41066a,(void*)&draw_half},
{0x4109e8,(void*)&draw_text_half_trans},
{0x4598df,(void*)&flushall},
{0x45a281,(void*)&getch},
{0x45992d,(void*)&getche},
{0x45762c,(void*)&getpid},
{0x4137af,(void*)&gte_MulMatrix0},
{0x414950,(void*)&gte_SetRotMatrix},
{0x4140c4,(void*)&gte_dpcs},
{0x414128,(void*)&gte_ncds},
{0x458c28,(void*)&isatty},
{0x459d43,(void*)&itoa},
{0x45c41e,(void*)&joyGetDevCapsA},
{0x45c424,(void*)&joyGetPos},
{0x459ded,(void*)&ltoa},
{0x45c430,(void*)&mciSendCommandA},
{0x45ac6c,(void*)&nexpand},
{0x456688,(void*)&nfree},
{0x45777d,(void*)&nmalloc},
{0x459ebd,(void*)&nrealloc},
{0x456cfb,(void*)&open},
{0x45a304,(void*)&putch},
{0x4138f9,(void*)&rcos},
{0x4138e0,(void*)&rsin},
{0x456d1d,(void*)&sopen},
{0x457346,(void*)&tell},
{0x45c44e,(void*)&timeBeginPeriod},
{0x45c442,(void*)&timeEndPeriod},
{0x45c43c,(void*)&timeKillEvent},
{0x45c448,(void*)&timeSetEvent},
{0x459d9d,(void*)&ultoa},
{0x459953,(void*)&unlink},
{0x459cd7,(void*)&utoa},
{0x456a74,(void*)&wstart2_},
};
int dd2_fnmap_n=sizeof(dd2_fnmap)/sizeof(dd2_fnmap[0]);
static void* g_lut[0x50000];
void dd2_relocate(void){
  int i; unsigned off;
  for(i=0;i<dd2_fnmap_n;i++){unsigned v=dd2_fnmap[i].va; if(v>=0x410000&&v<0x460000) g_lut[v-0x410000]=dd2_fnmap[i].fn;}
  for(off=0x60000; off+4<=0x540000; off+=4){
    unsigned w=*(unsigned*)(g_image+off);
    if(w>=0x410000&&w<0x460000){void*fn=g_lut[w-0x410000]; if(fn)*(void**)(g_image+off)=fn;}
  }
}
