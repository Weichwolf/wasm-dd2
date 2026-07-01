/* indirect-call dispatch: VA->C-function map + startup relocation of fn-pointers in the image */
#include "ghidra_compat.h"
extern unsigned char* g_image;
#define Parse_RIFF_Wave FUN_004159f8
#define Parse_Sound_Bank FUN_004164e4
#define Sound_Volume_To_dB FUN_00416714
#define Draw_Object_Polys FUN_0041ff98
#define Compute_Bowl_Cell_Normals FUN_00426be4
#define Play_Race_Start_Sounds FUN_00428a8c
#define Draw_Scene_Object_Blocks FUN_004307c8
#define Update_Particles FUN_00436e2c
#define Compute_Car_Screen_Pos FUN_00442c38
#define Update_Engine_Sound FUN_00447bdc
#define Update_Commentary FUN_00448368
extern int draw_text_half();
extern int FUN_0041033a();
extern int draw_half();
extern int FUN_0041080d();
extern int draw_text_half_trans();
extern int FUN_00410d50();
extern int FUN_00410d58();
extern int FUN_00410e44();
extern int FUN_00410f74();
extern int FUN_004110a4();
extern int FUN_004111e8();
extern int FUN_0041132c();
extern int FUN_004114c0();
extern int FUN_00411654();
extern int FUN_004117e8();
extern int FUN_004118d4();
extern int FUN_00411a04();
extern int FUN_00411b34();
extern int FUN_00411c78();
extern int FUN_00411ebc();
extern int FUN_0041243c();
extern int FUN_00412694();
extern int ClearOTagR();
extern int DrawOTag();
extern int DrawPrim();
extern int FUN_004128a6();
extern int Init_Application();
extern int Close_Application();
extern int SetVideoMode();
extern int VSync();
extern int PutDrawEnv();
extern int PutDispEnv();
extern int SetPalette();
extern int VSyncCallback();
extern int FUN_00412e8c();
extern int FUN_00412e9c();
extern int LoadImage();
extern int MoveImageClut();
extern int FUN_00413014();
extern int FUN_00413070();
extern int FUN_004130b0();
extern int DDRelease();
extern int FUN_004132b0();
extern int FUN_00413448();
extern int Generate_Transparency_Tables();
extern int gte_MulMatrix0();
extern int MulMatrix2();
extern int rsin();
extern int rcos();
extern int GTERT();
extern int GTERPS();
extern int GTERPT();
extern int FUN_00413b4e();
extern int ApplyMatrixLV();
extern int FUN_00413dc8();
extern int FUN_00413f45();
extern int FUN_00413fd2();
extern int FUN_00414055();
extern int gte_dpcs();
extern int gte_ncds();
extern int SetFogNearFar();
extern int PushMatrix();
extern int PopMatrix();
extern int FUN_00414360();
extern int VectorNormalS();
extern int VectorNormalSS();
extern int RotMatrixYXZ();
extern int RotMatrixX();
extern int RotMatrixY();
extern int RotMatrixZ();
extern int FUN_004148d0();
extern int gte_SetRotMatrix();
extern int RotTrans();
extern int RotTransPers();
extern int OuterProduct12();
extern int Play_Movie();
extern int Load_Null();
extern int Load_Textures();
extern int FUN_00414fb0();
extern int Load_Texture2();
extern int Load_Cluts();
extern int FUN_00415160();
extern int Read_Directory();
extern int File_Load();
extern int FUN_004153d4();
extern int FUN_00415404();
extern int FUN_00415448();
extern int FUN_004154b8();
extern int Decompress();
extern int DSLoadSoundBuffer();
extern int FUN_0041574c();
extern int DSGetWaveResource();
extern int FUN_004157e8();
extern int FUN_00415894();
extern int FUN_004159a8();
extern int Sound_Init();
extern int FUN_00415a94();
extern int Sound_Remove();
extern int FUN_00415b50();
extern int Sound_Stop();
extern int Sound_Restart();
extern int Kill_Sound();
extern int Play_Sound();
extern int Modify_Sound();
extern int FUN_00415f64();
extern int Unlock_Channel();
extern int FUN_00416044();
extern int CD_Close();
extern int FUN_0041611c();
extern int FUN_0041612c();
extern int Start_CD_Audio();
extern int Check_For_CD_Loop();
extern int FUN_00416264();
extern int CD_Pause();
extern int CD_Restart();
extern int FUN_004163b4();
extern int FUN_0041643c();
extern int FUN_00416494();
extern int FUN_004164d4();
extern int FUN_004165a4();
extern int CD_Check();
extern int Load_Sprite_Info();
extern int Search_For_Sprite();
extern int FUN_004166c4();
extern int Setup_Sprite();
extern int FUN_00416714();
extern int Modify_Sprite();
extern int FUN_00416a10();
extern int FUN_00417ea0();
extern int draw_face_3pt_flat();
extern int draw_face_3pt_flat_lit();
extern int draw_face_3pt_flat_dpq();
extern int draw_face_3pt_flat_dpq_lit();
extern int FUN_0041861c();
extern int draw_face_4pt_flat();
extern int draw_face_4pt_flat_lit();
extern int draw_face_4pt_flat_dpq();
extern int draw_face_4pt_flat_dpq_lit();
extern int FUN_00418ed0();
extern int draw_face_3pt_text();
extern int draw_face_3pt_text_squash();
extern int draw_face_3pt_text_lit();
extern int draw_face_3pt_text_dpq();
extern int draw_face_3pt_text_dpq_squash();
extern int draw_face_3pt_text_dpq_lit();
extern int FUN_0041a2f4();
extern int draw_face_4pt_text();
extern int draw_face_4pt_text_squash();
extern int draw_face_4pt_text_lit();
extern int draw_face_4pt_text_dpq();
extern int draw_face_4pt_text_dpq_squash();
extern int draw_face_4pt_text_dpq_lit();
extern int FUN_0041bc0c();
extern int FUN_0041bc68();
extern int draw_face_3pt_gour();
extern int draw_face_3pt_gour_lit();
extern int draw_face_3pt_gour_dpq();
extern int draw_face_3pt_gour_dpq_lit();
extern int FUN_0041c3e4();
extern int FUN_0041c440();
extern int draw_face_4pt_gour();
extern int draw_face_4pt_gour_lit();
extern int draw_face_4pt_gour_dpq();
extern int draw_face_4pt_gour_dpq_lit();
extern int FUN_0041ccf8();
extern int FUN_0041cdd0();
extern int draw_face_3pt_pict();
extern int draw_face_3pt_pict_lit();
extern int draw_face_3pt_pict_dpq();
extern int draw_face_3pt_pict_dpq_lit();
extern int FUN_0041d834();
extern int FUN_0041d918();
extern int draw_face_4pt_pict();
extern int draw_face_4pt_pict_lit();
extern int draw_face_4pt_pict_dpq();
extern int draw_face_4pt_pict_dpq_lit();
extern int setup_face_sprite();
extern int draw_face_sprite();
extern int draw_face_sprite_dpq();
extern int FUN_0041edbc();
extern int draw_face_tilt_sprite_dpq();
extern int FUN_0041f6a0();
extern int FUN_0041f900();
extern int FUN_0041fb7c();
extern int Create_Object();
extern int Set_Object();
extern int Remove_Object();
extern int Pre_Rotate();
extern int Draw_Subdiv_Object();
extern int FUN_0041fe68();
extern int Update_Object();
extern int FUN_0041ff50();
extern int FUN_0042003c();
extern int FUN_00420060();
extern int Set_Zclip();
extern int Set_World_Position();
extern int Set_World_Matrix();
extern int Set_World_View();
extern int Set_Ambient_Light();
extern int Set_Depth_Cue();
extern int FUN_004202ac();
extern int FUN_004203a0();
extern int Calc_Object_MatrixYZX();
extern int Calc_Object_Angles();
extern int FUN_004205d8();
extern int Point_Camera();
extern int Set_Draw_Mode();
extern int FUN_00420b1c();
extern int Draw_All();
extern int Draw_Tile();
extern int Swap_Buffers();
extern int FUN_00420cf0();
extern int Draw_Font_Poly();
extern int Init_Primitive_Buffer();
extern int Reset_Primitive_Buffer();
extern int FUN_00420e6c();
extern int FUN_00420ee8();
extern int Allocate_Font_Buffers();
extern int Setup_Font();
extern int Duplicate_Font();
extern int Print_Locate();
extern int Print_Font();
extern int Print_Ink();
extern int Print_InkRGB();
extern int Print();
extern int Print_Draw();
extern int FUN_00422064();
extern int FUN_004220cc();
extern int FUN_00422128();
extern int FUN_00422184();
extern int FUN_004221ec();
extern int FUN_00422358();
extern int FUN_00422548();
extern int FUN_0042293c();
extern int Setup_Controller();
extern int Setup_Joystick();
extern int FUN_00422c74();
extern int Translate_Keypress();
extern int FUN_0042304c();
extern int InitCardSystem();
extern int FUN_00423210();
extern int SaveCardFile();
extern int DeleteFileMC();
extern int LoadCardFiles();
extern int LoadCardFile();
extern int FirstSavedGame();
extern int InitCardBlocks();
extern int DupFileCheck();
extern int FUN_0042353c();
extern int MPE_InitHeap();
extern int MPE_malloc();
extern int MPE_free();
extern int Debug_Stub();
extern int PC_Write_File();
extern int System_Error();
extern int FUN_004237c0();
extern int FUN_00423804();
extern int Profile_Init();
extern int Play_Game();
extern int Setup_Debris();
extern int Update_Debris();
extern int FUN_00424930();
extern int Setup_Flying_Objects();
extern int Update_Flying_Objects();
extern int Zero_Flying_Object();
extern int Request_Flying_Object();
extern int FUN_00425314();
extern int Flying_Objects_Pos_Ang();
extern int FUN_00425a98();
extern int FUN_00425b88();
extern int InitialiseDenting();
extern int FUN_00425e74();
extern int Generate_Surface_Normals();
extern int Mask_Point_In_Quad();
extern int FUN_00426ab4();
extern int Track_Follow();
extern int Map_Height();
extern int FUN_00428548();
extern int Move_Forward_Strip();
extern int FUN_004287c0();
extern int Search_For_Strip();
extern int FUN_004288d0();
extern int FUN_0042895c();
extern int FUN_004295e4();
extern int Car_Camera();
extern int Camera_Pad_Control();
extern int FUN_00429994();
extern int Pit_Camera_Control();
extern int Init_The_Floaty_Camera();
extern int Do_The_Floaty_Camera_Thing();
extern int Init_Damage_Indicator();
extern int FUN_0042a4d8();
extern int FUN_0042a6da();
extern int FUN_0042a703();
extern int FUN_0042a72c();
extern int FUN_0042a752();
extern int FUN_0042a778();
extern int FUN_0042a79e();
extern int Bonnet_Smoke();
extern int FUN_0042b5f0();
extern int Draw_Car();
extern int FUN_0042c1a4();
extern int Init_Car_Graphics();
extern int Init_Wild_Bill();
extern int Init_Rollercoaster();
extern int Init_Texture_Animation();
extern int Texture_Animation();
extern int CLUT_Animation();
extern int Update_Other_Objects();
extern int Draw_Other_Objects();
extern int Draw_Dynamic_Objects();
extern int Init_Flag();
extern int DrawFlagObject();
extern int UpdateFlag();
extern int Init_LensFlare();
extern int DrawLensFlare();
extern int Init_Overlays();
extern int Draw_Overlays();
extern int FUN_0042fa4c();
extern int Update_Race_CountDown();
extern int FUN_0042fd58();
extern int Display_Position_Pointers();
extern int Init_Scene();
extern int VVDraw_Object();
extern int Draw_Scene_Object();
extern int FUN_00430698();
extern int Setup_Object_Block();
extern int FUN_004308b8();
extern int Decrunch_Object_Block();
extern int FUN_00430a30();
extern int Update_Scene_Objects();
extern int FUN_00430bde();
extern int Init_Scene_Objects();
extern int Remove_Scene_Objects();
extern int Init_Sky();
extern int FUN_00430efc();
extern int Draw_Sky();
extern int AI_Com_Server();
extern int Determine_AI();
extern int Recommended_Acceleration();
extern int FUN_00432f98();
extern int Get_Car_Angle();
extern int Get_Direction_Cosines();
extern int Interpolate_Direction_Vectors_Left();
extern int InitialiseAI();
extern int Obstacle_Ahead();
extern int Strip_Distance();
extern int FUN_00433a70();
extern int CheckPointScoring();
extern int Barrier_Collision();
extern int Barrier_Corner_Collision();
extern int FUN_004351d0();
extern int FUN_00435254();
extern int TransformEnemyWheels();
extern int ApplyWheelOverlay();
extern int InitialiseParticleSystem();
extern int Smoke();
extern int SmokeCtrl();
extern int FireCtrl();
extern int Fire();
extern int Sparks();
extern int SparksCtrl();
extern int Steam();
extern int SteamCtrl();
extern int FUN_00436c04();
extern int FreeParticle();
extern int DrawParticles();
extern int FUN_00436cfc();
extern int FUN_00436dd0();
extern int Start_Roll();
extern int FUN_0043709c();
extern int FUN_004371fc();
extern int Calc_Head_On_Clsn_Dynamics();
extern int Check_2D_Car_Collision();
extern int Check_Ground_Car_Collision();
extern int Check_Space_Car_Collision();
extern int Do_Car_Collisions();
extern int Init_Car_Cluts();
extern int Init_Car_Doors();
extern int FUN_0043b26c();
extern int Change_Bonnet_Clut();
extern int Change_Boot_Clut();
extern int Highlight_Area();
extern int FUN_0043b7bc();
extern int TextureDentHiCar();
extern int TextureDentMidCar();
extern int FUN_0043c4e8();
extern int FUN_0043c55c();
extern int FUN_0043c5d0();
extern int FUN_0043c738();
extern int Get_Corner_Positions();
extern int Ground_Collision();
extern int Find_Lowest_Corner();
extern int Make_Car_Fly();
extern int Car_Landed();
extern int Car_Landed_On_Corner();
extern int Car_Fly_Motion_3D();
extern int Car_Grounded_Motion_3D();
extern int FUN_0043dbd8();
extern int FUN_0043dd00();
extern int Car_2pt_Motion_3D();
extern int Car_1pt_Motion_3D();
extern int FUN_00440980();
extern int FUN_00440ac4();
extern int FUN_00440f60();
extern int Calc_Car_Tilt();
extern int Calc_Car_Angles_Square();
extern int FUN_00441394();
extern int Car_Drive_Motion();
extern int Car_Drive_2pt_Motion();
extern int Car_Fly_Motion();
extern int FUN_0044282c();
extern int FUN_00442b08();
extern int Car_Movement();
extern int FUN_004430b8();
extern int Init_End_Race();
extern int Calc_Track_Positions();
extern int FUN_00443b10();
extern int Get_Race_Positions();
extern int Init_Track_Strip_Numbers();
extern int FUN_00443f90();
extern int FUN_00444048();
extern int FUN_004448e0();
extern int FUN_00444a60();
extern int Calc_Suspension_Right_Wheels();
extern int Calc_Null_Suspension();
extern int FUN_00444e3c();
extern int Boot_Lost_Geometry();
extern int FUN_00444fcc();
extern int FUN_00445030();
extern int FUN_004450dc();
extern int Check_Bonnet_Removal();
extern int Check_Boot_Removal();
extern int Init_Main();
extern int Set_Load_Textures();
extern int Modify_TDF();
extern int Init_Graphics();
extern int FUN_00445b40();
extern int FUN_00445b78();
extern int Init_Game();
extern int Play_Intro();
extern int Play_Xtro();
extern int Initialise_Pause_Mode();
extern int Pause_Mode();
extern int FUN_00446c10();
extern int UndentCar();
extern int SetHighLight();
extern int FUN_004471c0();
extern int Control_Car_Replay();
extern int FUN_004477a0();
extern int Record_Event();
extern int Terminate_Replay();
extern int Terminate_Replay_Bodge();
extern int FUN_00447960();
extern int FUN_00447a9c();
extern int Amplitude();
extern int DopplerFrequency();
extern int Allocate_Sound_Effect();
extern int FUN_004480bc();
extern int Load_Game_Vags();
extern int FUN_00448228();
extern int FUN_00448d10();
extern int PitOut();
extern int PitIn();
extern int Pit_Stop1();
extern int Pit_Stop2();
extern int Strip_Trigger_Handler();
extern int Sparking();
extern int LoadSave();
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
extern int Load_First_Config();
extern int Load_Card_File();
extern int FUN_0044ac9c();
extern int FUN_0044aec8();
extern int Init_Front_End();
extern int View_Frontend_Replay();
extern int DemoMode();
extern int FUN_0044b5c0();
extern int Loading_Screen_From_Slab();
extern int FUN_0044b684();
extern int Load_Completion_Status();
extern int Call_Loaded_Game();
extern int FUN_0044b970();
extern int FUN_0044b9c0();
extern int Setup_Pad();
extern int Init_Wrecking_Championship();
extern int Init_StockCar_Championship();
extern int FUN_0044bb88();
extern int Init_StockCar_MultiChamp();
extern int Championship();
extern int MultiChamp();
extern int Calculate_Finish();
extern int Calculate_Results();
extern int Do_End_Of_Season_Stuff();
extern int Init_League_Info();
extern int Reset_League_Info();
extern int Sort_Leagues();
extern int Init_MultiLeague_Info();
extern int Setup_Driver_Names();
extern int FUN_0044c418();
extern int Add_Computer_Info();
extern int Update_League_Info();
extern int FUN_0044c538();
extern int Sort_MultiLeague();
extern int Sort_RacePos();
extern int Promote_And_Relegate();
extern int Check_League_Standing();
extern int Order_Cars();
extern int FUN_0044c800();
extern int FUN_0044c8e8();
extern int FUN_0044cb48();
extern int FUN_0044ccb8();
extern int FUN_0044ced4();
extern int FUN_0044cef0();
extern int FUN_0044d0e4();
extern int Start_New_Season_Stats();
extern int FUN_0044d3d4();
extern int Update_Track_Stats();
extern int FUN_0044d538();
extern int Update_Championship_Stats();
extern int FUN_0044d630();
extern int Get_Current_Recording_Season();
extern int FUN_0044d650();
extern int Update_Jimmy_Spunk_Times();
extern int FUN_0044d974();
extern int FUN_0044db70();
extern int Secret();
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
extern int View_BestLaps();
extern int FUN_0044ff48();
extern int Front_End();
extern int FUN_00450640();
extern int Toggle_Track();
extern int Toggle_Car();
extern int FUN_0045099c();
extern int FUN_00450c7c();
extern int Rotate_Slab_On();
extern int FUN_00450e20();
extern int Draw_Screen_Polys();
extern int Setup_Screen_Text();
extern int Setup_Screen_Lines();
extern int Draw_Screen_Lines();
extern int Button_Pressed();
extern int Draw_Slab();
extern int FUN_004518ac();
extern int Draw_Semi_Trans_Poly();
extern int FUN_00451a80();
extern int Play_Click_FX();
extern int FUN_00451ac0();
extern int FUN_00451ae0();
extern int FUN_00451d4c();
extern int FUN_00451d6c();
extern int FUN_00451fdc();
extern int FUN_00451ff0();
extern int FUN_0045202c();
extern int FUN_00452090();
extern int Enter_Driver_Names();
extern int FUN_004525a0();
extern int FUN_00452790();
extern int FUN_004527f0();
extern int FUN_004529d4();
extern int FUN_00452a34();
extern int Practice_Over();
extern int FUN_00452d30();
extern int FUN_00452da0();
extern int FUN_00452dc0();
extern int FUN_00452fa4();
extern int FUN_00452fc4();
extern int Select_Champ();
extern int Select_ChampQS();
extern int Select_Pract();
extern int Select_TimeT();
extern int Select_Total();
extern int Select_DDPract();
extern int FUN_00453580();
extern int FUN_004535c0();
extern int Save_Game();
extern int FUN_004539d0();
extern int FUN_004539f0();
extern int FUN_00453a18();
extern int FUN_00453a20();
extern int FUN_00453b98();
extern int End_Of_Season();
extern int FUN_00454158();
extern int FUN_004541e8();
extern int FUN_00454278();
extern int FUN_00454308();
extern int FUN_00454398();
extern int FUN_00454550();
extern int FUN_00454804();
extern int FUN_00454a70();
extern int Race_Over();
extern int FUN_00454e9c();
extern int FUN_00454f0c();
extern int FUN_004551b0();
extern int FUN_00455260();
extern int Display_Season_Status();
extern int FUN_0045572c();
extern int FUN_004559d4();
extern int __open_flags();
extern int FUN_00455de4();
extern int FUN_00455edb();
extern int FUN_00455f3b();
extern int FUN_00455fb0();
extern int FUN_00456034();
extern int FUN_0045607b();
extern int __shutdown_stream();
extern int FUN_0045644e();
extern int FUN_0045645e();
extern int __doclose();
extern int __CHP();
extern int nfree();
extern int FUN_00456717();
extern int FUN_0045672e();
extern int __null_int23_exit();
extern int _exit();
extern int wstart2_();
extern int FUN_00456af2();
extern int FUN_00456b30();
extern int FUN_00456b67();
extern int FUN_00456bf3();
extern int _tolower();
extern int FUN_00456cae();
extern int __set_EDOM();
extern int __set_ERANGE();
extern int FUN_00456cdf();
extern int __set_doserrno();
extern int open();
extern int sopen();
extern int __allocfp();
extern int __freefp();
extern int __purgefp();
extern int __chktty();
extern int __threadid();
extern int FUN_0045703b();
extern int FUN_00457040();
extern int FUN_00457041();
extern int FUN_0045704f();
extern int __NTInit();
extern int __NTMainInit();
extern int __exit();
extern int FUN_004571e4();
extern int _lseek();
extern int FUN_004572f8();
extern int tell();
extern int __ioalloc();
extern int FUN_00457408();
extern int fgetc();
extern int __filbuf();
extern int FUN_00457571();
extern int getpid();
extern int FUN_00457631();
extern int FUN_004576c5();
extern int _fpreset();
extern int nmalloc();
extern int __MemAllocator();
extern int __MemFree();
extern int __prtf();
extern int FUN_00457d39();
extern int FUN_00457e84();
extern int FUN_00457ee9();
extern int FUN_00457f0f();
extern int FUN_00457f40();
extern int FUN_00457f9f();
extern int FUN_0045809c();
extern int FUN_004580b7();
extern int FUN_004585e8();
extern int __qwrite();
extern int fputc();
extern int __WinMain();
extern int __NTAtMaxFiles();
extern int __NTAddFileHandle();
extern int FUN_00458964();
extern int __NTRemoveFileHandle();
extern int FUN_00458a31();
extern int __NTGetFakeHandle();
extern int __GetNTAccessAttr();
extern int __GetNTShareAttr();
extern int _stricmp();
extern int FUN_00458b45();
extern int dosretax();
extern int FUN_00458bc7();
extern int __set_errno_nt();
extern int isatty();
extern int __IOMode();
extern int FUN_00458cc5();
extern int __sigfpe_handler();
extern int signal();
extern int raise();
extern int FUN_00458e2f();
extern int __SigFini();
extern int __NewExceptionHandler();
extern int __DoneExceptionHandler();
extern int __CloseSemaphore();
extern int __AccessSemaphore();
extern int __ReleaseSemaphore();
extern int __InitThreadData();
extern int __NTThreadInit();
extern int __NTAddThread();
extern int __NTRemoveThread();
extern int __InitMultipleThread();
extern int __InitRtns();
extern int __FiniRtns();
extern int __full_io_exit();
extern int FUN_0045987f();
extern int flushall();
extern int __flushall();
extern int getche();
extern int unlink();
extern int FUN_00459968();
extern int FUN_00459a88();
extern int __cnvs2d();
extern int FUN_00459b19();
extern int __init_80x87();
extern int FUN_00459b51();
extern int FUN_00459bc5();
extern int __ExpandDGROUP();
extern int FUN_00459c68();
extern int __nmemneed();
extern int utoa();
extern int _itoa();
extern int itoa();
extern int ultoa();
extern int ltoa();
extern int _ltoa();
extern int _toupper();
extern int FUN_00459e4a();
extern int FUN_00459e6e();
extern int __CommonInit();
extern int FUN_00459ea8();
extern int nrealloc();
extern int FUN_00459f57();
extern int FUN_00459fc0();
extern int FUN_00459ff5();
typedef struct{unsigned va;void*fn;}dd2_fnent;
dd2_fnent dd2_fnmap[]={
{0x00410010,(void*)&draw_text_half},
{0x0041033a,(void*)&FUN_0041033a},
{0x0041066a,(void*)&draw_half},
{0x0041080d,(void*)&FUN_0041080d},
{0x004109e8,(void*)&draw_text_half_trans},
{0x00410d50,(void*)&FUN_00410d50},
{0x00410d58,(void*)&FUN_00410d58},
{0x00410e44,(void*)&FUN_00410e44},
{0x00410f74,(void*)&FUN_00410f74},
{0x004110a4,(void*)&FUN_004110a4},
{0x004111e8,(void*)&FUN_004111e8},
{0x0041132c,(void*)&FUN_0041132c},
{0x004114c0,(void*)&FUN_004114c0},
{0x00411654,(void*)&FUN_00411654},
{0x004117e8,(void*)&FUN_004117e8},
{0x004118d4,(void*)&FUN_004118d4},
{0x00411a04,(void*)&FUN_00411a04},
{0x00411b34,(void*)&FUN_00411b34},
{0x00411c78,(void*)&FUN_00411c78},
{0x00411ebc,(void*)&FUN_00411ebc},
{0x0041243c,(void*)&FUN_0041243c},
{0x00412694,(void*)&FUN_00412694},
{0x0041281c,(void*)&ClearOTagR},
{0x00412843,(void*)&DrawOTag},
{0x00412885,(void*)&DrawPrim},
{0x004128a6,(void*)&FUN_004128a6},
{0x00412910,(void*)&Init_Application},
{0x00412a94,(void*)&Close_Application},
{0x00412b2c,(void*)&SetVideoMode},
{0x00412bac,(void*)&VSync},
{0x00412bb4,(void*)&PutDrawEnv},
{0x00412c60,(void*)&PutDispEnv},
{0x00412cb0,(void*)&SetPalette},
{0x00412e7c,(void*)&VSyncCallback},
{0x00412e8c,(void*)&FUN_00412e8c},
{0x00412e9c,(void*)&FUN_00412e9c},
{0x00412f3c,(void*)&LoadImage},
{0x00412fa8,(void*)&MoveImageClut},
{0x00413014,(void*)&FUN_00413014},
{0x00413070,(void*)&FUN_00413070},
{0x004130b0,(void*)&FUN_004130b0},
{0x00413208,(void*)&DDRelease},
{0x004132b0,(void*)&FUN_004132b0},
{0x00413448,(void*)&FUN_00413448},
{0x00413544,(void*)&Generate_Transparency_Tables},
{0x004137af,(void*)&gte_MulMatrix0},
{0x004137de,(void*)&MulMatrix2},
{0x004138e0,(void*)&rsin},
{0x004138f9,(void*)&rcos},
{0x0041397e,(void*)&GTERT},
{0x004139a7,(void*)&GTERPS},
{0x004139e9,(void*)&GTERPT},
{0x00413b4e,(void*)&FUN_00413b4e},
{0x00413d0b,(void*)&ApplyMatrixLV},
{0x00413dc8,(void*)&FUN_00413dc8},
{0x00413f45,(void*)&FUN_00413f45},
{0x00413fd2,(void*)&FUN_00413fd2},
{0x00414055,(void*)&FUN_00414055},
{0x004140c4,(void*)&gte_dpcs},
{0x00414128,(void*)&gte_ncds},
{0x0041424c,(void*)&SetFogNearFar},
{0x004142a8,(void*)&PushMatrix},
{0x00414304,(void*)&PopMatrix},
{0x00414360,(void*)&FUN_00414360},
{0x004143c4,(void*)&VectorNormalS},
{0x0041442c,(void*)&VectorNormalSS},
{0x004144a0,(void*)&RotMatrixYXZ},
{0x00414540,(void*)&RotMatrixX},
{0x00414670,(void*)&RotMatrixY},
{0x004147a0,(void*)&RotMatrixZ},
{0x004148d0,(void*)&FUN_004148d0},
{0x00414950,(void*)&gte_SetRotMatrix},
{0x004149bc,(void*)&RotTrans},
{0x00414a40,(void*)&RotTransPers},
{0x00414d68,(void*)&OuterProduct12},
{0x00414e30,(void*)&Play_Movie},
{0x00414f30,(void*)&Load_Null},
{0x00414f38,(void*)&Load_Textures},
{0x00414fb0,(void*)&FUN_00414fb0},
{0x00414ff4,(void*)&Load_Texture2},
{0x0041501c,(void*)&Load_Cluts},
{0x00415160,(void*)&FUN_00415160},
{0x004152f4,(void*)&Read_Directory},
{0x00415354,(void*)&File_Load},
{0x004153d4,(void*)&FUN_004153d4},
{0x00415404,(void*)&FUN_00415404},
{0x00415448,(void*)&FUN_00415448},
{0x004154b8,(void*)&FUN_004154b8},
{0x00415550,(void*)&Decompress},
{0x00415658,(void*)&DSLoadSoundBuffer},
{0x0041574c,(void*)&FUN_0041574c},
{0x004157b0,(void*)&DSGetWaveResource},
{0x004157e8,(void*)&FUN_004157e8},
{0x00415894,(void*)&FUN_00415894},
{0x004159a8,(void*)&FUN_004159a8},
{0x00415a88,(void*)&Sound_Init},
{0x00415a94,(void*)&FUN_00415a94},
{0x00415b08,(void*)&Sound_Remove},
{0x00415b50,(void*)&FUN_00415b50},
{0x00415b70,(void*)&Sound_Stop},
{0x00415bc8,(void*)&Sound_Restart},
{0x00415c0c,(void*)&Kill_Sound},
{0x00415cd8,(void*)&Play_Sound},
{0x00415ec0,(void*)&Modify_Sound},
{0x00415f64,(void*)&FUN_00415f64},
{0x00415fc4,(void*)&Unlock_Channel},
{0x00416044,(void*)&FUN_00416044},
{0x004160ec,(void*)&CD_Close},
{0x0041611c,(void*)&FUN_0041611c},
{0x0041612c,(void*)&FUN_0041612c},
{0x0041619c,(void*)&Start_CD_Audio},
{0x004161d8,(void*)&Check_For_CD_Loop},
{0x00416264,(void*)&FUN_00416264},
{0x00416294,(void*)&CD_Pause},
{0x004162c4,(void*)&CD_Restart},
{0x004163b4,(void*)&FUN_004163b4},
{0x0041643c,(void*)&FUN_0041643c},
{0x00416494,(void*)&FUN_00416494},
{0x004164d4,(void*)&FUN_004164d4},
{0x004165a4,(void*)&FUN_004165a4},
{0x00416620,(void*)&CD_Check},
{0x00416670,(void*)&Load_Sprite_Info},
{0x00416688,(void*)&Search_For_Sprite},
{0x004166c4,(void*)&FUN_004166c4},
{0x004166ec,(void*)&Setup_Sprite},
{0x00416714,(void*)&FUN_00416714},
{0x00416934,(void*)&Modify_Sprite},
{0x00416a10,(void*)&FUN_00416a10},
{0x00417ea0,(void*)&FUN_00417ea0},
{0x00417efc,(void*)&draw_face_3pt_flat},
{0x004180c8,(void*)&draw_face_3pt_flat_lit},
{0x0041828c,(void*)&draw_face_3pt_flat_dpq},
{0x00418458,(void*)&draw_face_3pt_flat_dpq_lit},
{0x0041861c,(void*)&FUN_0041861c},
{0x00418678,(void*)&draw_face_4pt_flat},
{0x0041888c,(void*)&draw_face_4pt_flat_lit},
{0x00418aa4,(void*)&draw_face_4pt_flat_dpq},
{0x00418cb8,(void*)&draw_face_4pt_flat_dpq_lit},
{0x00418ed0,(void*)&FUN_00418ed0},
{0x00418fe0,(void*)&draw_face_3pt_text},
{0x004191ac,(void*)&draw_face_3pt_text_squash},
{0x004196a8,(void*)&draw_face_3pt_text_lit},
{0x004199b0,(void*)&draw_face_3pt_text_dpq},
{0x00419b94,(void*)&draw_face_3pt_text_dpq_squash},
{0x0041a0ac,(void*)&draw_face_3pt_text_dpq_lit},
{0x0041a2f4,(void*)&FUN_0041a2f4},
{0x0041a40c,(void*)&draw_face_4pt_text},
{0x0041a66c,(void*)&draw_face_4pt_text_squash},
{0x0041acf4,(void*)&draw_face_4pt_text_lit},
{0x0041b054,(void*)&draw_face_4pt_text_dpq},
{0x0041b2d8,(void*)&draw_face_4pt_text_dpq_squash},
{0x0041b97c,(void*)&draw_face_4pt_text_dpq_lit},
{0x0041bc0c,(void*)&FUN_0041bc0c},
{0x0041bc68,(void*)&FUN_0041bc68},
{0x0041bcc4,(void*)&draw_face_3pt_gour},
{0x0041be90,(void*)&draw_face_3pt_gour_lit},
{0x0041c054,(void*)&draw_face_3pt_gour_dpq},
{0x0041c220,(void*)&draw_face_3pt_gour_dpq_lit},
{0x0041c3e4,(void*)&FUN_0041c3e4},
{0x0041c440,(void*)&FUN_0041c440},
{0x0041c49c,(void*)&draw_face_4pt_gour},
{0x0041c6b0,(void*)&draw_face_4pt_gour_lit},
{0x0041c8c8,(void*)&draw_face_4pt_gour_dpq},
{0x0041cae0,(void*)&draw_face_4pt_gour_dpq_lit},
{0x0041ccf8,(void*)&FUN_0041ccf8},
{0x0041cdd0,(void*)&FUN_0041cdd0},
{0x0041ce8c,(void*)&draw_face_3pt_pict},
{0x0041d058,(void*)&draw_face_3pt_pict_lit},
{0x0041d360,(void*)&draw_face_3pt_pict_dpq},
{0x0041d5e8,(void*)&draw_face_3pt_pict_dpq_lit},
{0x0041d834,(void*)&FUN_0041d834},
{0x0041d918,(void*)&FUN_0041d918},
{0x0041d9e0,(void*)&draw_face_4pt_pict},
{0x0041dbf4,(void*)&draw_face_4pt_pict_lit},
{0x0041df54,(void*)&draw_face_4pt_pict_dpq},
{0x0041e184,(void*)&draw_face_4pt_pict_dpq_lit},
{0x0041e418,(void*)&setup_face_sprite},
{0x0041e4f4,(void*)&draw_face_sprite},
{0x0041e948,(void*)&draw_face_sprite_dpq},
{0x0041edbc,(void*)&FUN_0041edbc},
{0x0041f220,(void*)&draw_face_tilt_sprite_dpq},
{0x0041f6a0,(void*)&FUN_0041f6a0},
{0x0041f900,(void*)&FUN_0041f900},
{0x0041fb7c,(void*)&FUN_0041fb7c},
{0x0041fc00,(void*)&Create_Object},
{0x0041fc5c,(void*)&Set_Object},
{0x0041fcac,(void*)&Remove_Object},
{0x0041fcfc,(void*)&Pre_Rotate},
{0x0041fdbc,(void*)&Draw_Subdiv_Object},
{0x0041fe68,(void*)&FUN_0041fe68},
{0x0041ff24,(void*)&Update_Object},
{0x0041ff50,(void*)&FUN_0041ff50},
{0x0042003c,(void*)&FUN_0042003c},
{0x00420060,(void*)&FUN_00420060},
{0x00420074,(void*)&Set_Zclip},
{0x004200bc,(void*)&Set_World_Position},
{0x004200dc,(void*)&Set_World_Matrix},
{0x0042011c,(void*)&Set_World_View},
{0x00420268,(void*)&Set_Ambient_Light},
{0x0042027c,(void*)&Set_Depth_Cue},
{0x004202ac,(void*)&FUN_004202ac},
{0x004203a0,(void*)&FUN_004203a0},
{0x00420414,(void*)&Calc_Object_MatrixYZX},
{0x00420488,(void*)&Calc_Object_Angles},
{0x004205d8,(void*)&FUN_004205d8},
{0x004207ac,(void*)&Point_Camera},
{0x00420aa0,(void*)&Set_Draw_Mode},
{0x00420b1c,(void*)&FUN_00420b1c},
{0x00420b6c,(void*)&Draw_All},
{0x00420bb8,(void*)&Draw_Tile},
{0x00420cc4,(void*)&Swap_Buffers},
{0x00420cf0,(void*)&FUN_00420cf0},
{0x00420d44,(void*)&Draw_Font_Poly},
{0x00420d70,(void*)&Init_Primitive_Buffer},
{0x00420de4,(void*)&Reset_Primitive_Buffer},
{0x00420e6c,(void*)&FUN_00420e6c},
{0x00420ee8,(void*)&FUN_00420ee8},
{0x00420fc0,(void*)&Allocate_Font_Buffers},
{0x00421078,(void*)&Setup_Font},
{0x004210c4,(void*)&Duplicate_Font},
{0x004211b8,(void*)&Print_Locate},
{0x00421204,(void*)&Print_Font},
{0x0042125c,(void*)&Print_Ink},
{0x0042129c,(void*)&Print_InkRGB},
{0x004212d4,(void*)&Print},
{0x00421fb0,(void*)&Print_Draw},
{0x00422064,(void*)&FUN_00422064},
{0x004220cc,(void*)&FUN_004220cc},
{0x00422128,(void*)&FUN_00422128},
{0x00422184,(void*)&FUN_00422184},
{0x004221ec,(void*)&FUN_004221ec},
{0x00422358,(void*)&FUN_00422358},
{0x00422548,(void*)&FUN_00422548},
{0x0042293c,(void*)&FUN_0042293c},
{0x00422ba8,(void*)&Setup_Controller},
{0x00422bc0,(void*)&Setup_Joystick},
{0x00422c74,(void*)&FUN_00422c74},
{0x00422f20,(void*)&Translate_Keypress},
{0x0042304c,(void*)&FUN_0042304c},
{0x004230f0,(void*)&InitCardSystem},
{0x00423210,(void*)&FUN_00423210},
{0x0042322c,(void*)&SaveCardFile},
{0x004232f8,(void*)&DeleteFileMC},
{0x00423390,(void*)&LoadCardFiles},
{0x00423464,(void*)&LoadCardFile},
{0x004234c0,(void*)&FirstSavedGame},
{0x004234dc,(void*)&InitCardBlocks},
{0x004234f0,(void*)&DupFileCheck},
{0x0042353c,(void*)&FUN_0042353c},
{0x004235c0,(void*)&MPE_InitHeap},
{0x004235e4,(void*)&MPE_malloc},
{0x0042364c,(void*)&MPE_free},
{0x004236c0,(void*)&Debug_Stub},
{0x00423744,(void*)&PC_Write_File},
{0x0042379c,(void*)&System_Error},
{0x004237c0,(void*)&FUN_004237c0},
{0x00423804,(void*)&FUN_00423804},
{0x00423990,(void*)&Profile_Init},
{0x00423a20,(void*)&Play_Game},
{0x0042440c,(void*)&Setup_Debris},
{0x00424510,(void*)&Update_Debris},
{0x00424930,(void*)&FUN_00424930},
{0x00424a58,(void*)&Setup_Flying_Objects},
{0x00425150,(void*)&Update_Flying_Objects},
{0x0042528c,(void*)&Zero_Flying_Object},
{0x004252d0,(void*)&Request_Flying_Object},
{0x00425314,(void*)&FUN_00425314},
{0x004259e8,(void*)&Flying_Objects_Pos_Ang},
{0x00425a98,(void*)&FUN_00425a98},
{0x00425b88,(void*)&FUN_00425b88},
{0x00425d70,(void*)&InitialiseDenting},
{0x00425e74,(void*)&FUN_00425e74},
{0x00426788,(void*)&Generate_Surface_Normals},
{0x00426964,(void*)&Mask_Point_In_Quad},
{0x00426ab4,(void*)&FUN_00426ab4},
{0x00426ec4,(void*)&Track_Follow},
{0x00427e40,(void*)&Map_Height},
{0x00428548,(void*)&FUN_00428548},
{0x0042872c,(void*)&Move_Forward_Strip},
{0x004287c0,(void*)&FUN_004287c0},
{0x00428850,(void*)&Search_For_Strip},
{0x004288d0,(void*)&FUN_004288d0},
{0x0042895c,(void*)&FUN_0042895c},
{0x004295e4,(void*)&FUN_004295e4},
{0x0042968c,(void*)&Car_Camera},
{0x00429738,(void*)&Camera_Pad_Control},
{0x00429994,(void*)&FUN_00429994},
{0x00429a18,(void*)&Pit_Camera_Control},
{0x00429ec0,(void*)&Init_The_Floaty_Camera},
{0x00429f10,(void*)&Do_The_Floaty_Camera_Thing},
{0x0042a398,(void*)&Init_Damage_Indicator},
{0x0042a4d8,(void*)&FUN_0042a4d8},
{0x0042a6da,(void*)&FUN_0042a6da},
{0x0042a703,(void*)&FUN_0042a703},
{0x0042a72c,(void*)&FUN_0042a72c},
{0x0042a752,(void*)&FUN_0042a752},
{0x0042a778,(void*)&FUN_0042a778},
{0x0042a79e,(void*)&FUN_0042a79e},
{0x0042a9b0,(void*)&Bonnet_Smoke},
{0x0042b5f0,(void*)&FUN_0042b5f0},
{0x0042c02c,(void*)&Draw_Car},
{0x0042c1a4,(void*)&FUN_0042c1a4},
{0x0042c2d8,(void*)&Init_Car_Graphics},
{0x0042c510,(void*)&Init_Wild_Bill},
{0x0042c540,(void*)&Init_Rollercoaster},
{0x0042c774,(void*)&Init_Texture_Animation},
{0x0042c934,(void*)&Texture_Animation},
{0x0042ca3c,(void*)&CLUT_Animation},
{0x0042cac8,(void*)&Update_Other_Objects},
{0x0042ced8,(void*)&Draw_Other_Objects},
{0x0042d468,(void*)&Draw_Dynamic_Objects},
{0x0042d4d0,(void*)&Init_Flag},
{0x0042d628,(void*)&DrawFlagObject},
{0x0042d998,(void*)&UpdateFlag},
{0x0042db08,(void*)&Init_LensFlare},
{0x0042dca8,(void*)&DrawLensFlare},
{0x0042e024,(void*)&Init_Overlays},
{0x0042e784,(void*)&Draw_Overlays},
{0x0042fa4c,(void*)&FUN_0042fa4c},
{0x0042fc70,(void*)&Update_Race_CountDown},
{0x0042fd58,(void*)&FUN_0042fd58},
{0x0042ff7c,(void*)&Display_Position_Pointers},
{0x0043023c,(void*)&Init_Scene},
{0x0043038c,(void*)&VVDraw_Object},
{0x00430464,(void*)&Draw_Scene_Object},
{0x00430698,(void*)&FUN_00430698},
{0x00430778,(void*)&Setup_Object_Block},
{0x004308b8,(void*)&FUN_004308b8},
{0x00430908,(void*)&Decrunch_Object_Block},
{0x00430a30,(void*)&FUN_00430a30},
{0x00430a90,(void*)&Update_Scene_Objects},
{0x00430bde,(void*)&FUN_00430bde},
{0x00430cac,(void*)&Init_Scene_Objects},
{0x00430da8,(void*)&Remove_Scene_Objects},
{0x00430e08,(void*)&Init_Sky},
{0x00430efc,(void*)&FUN_00430efc},
{0x00431200,(void*)&Draw_Sky},
{0x00431374,(void*)&AI_Com_Server},
{0x00432c4c,(void*)&Determine_AI},
{0x00432f58,(void*)&Recommended_Acceleration},
{0x00432f98,(void*)&FUN_00432f98},
{0x00432fd8,(void*)&Get_Car_Angle},
{0x00433004,(void*)&Get_Direction_Cosines},
{0x00433074,(void*)&Interpolate_Direction_Vectors_Left},
{0x00433188,(void*)&InitialiseAI},
{0x004334ec,(void*)&Obstacle_Ahead},
{0x00433a50,(void*)&Strip_Distance},
{0x00433a70,(void*)&FUN_00433a70},
{0x00433c04,(void*)&CheckPointScoring},
{0x00433e40,(void*)&Barrier_Collision},
{0x00434b4c,(void*)&Barrier_Corner_Collision},
{0x004351d0,(void*)&FUN_004351d0},
{0x00435254,(void*)&FUN_00435254},
{0x00435740,(void*)&TransformEnemyWheels},
{0x00435860,(void*)&ApplyWheelOverlay},
{0x0043604c,(void*)&InitialiseParticleSystem},
{0x004361e8,(void*)&Smoke},
{0x004362b0,(void*)&SmokeCtrl},
{0x00436448,(void*)&FireCtrl},
{0x004365cc,(void*)&Fire},
{0x004366bc,(void*)&Sparks},
{0x00436804,(void*)&SparksCtrl},
{0x00436980,(void*)&Steam},
{0x00436a6c,(void*)&SteamCtrl},
{0x00436c04,(void*)&FUN_00436c04},
{0x00436c34,(void*)&FreeParticle},
{0x00436ca4,(void*)&DrawParticles},
{0x00436cfc,(void*)&FUN_00436cfc},
{0x00436dd0,(void*)&FUN_00436dd0},
{0x00436f60,(void*)&Start_Roll},
{0x0043709c,(void*)&FUN_0043709c},
{0x004371fc,(void*)&FUN_004371fc},
{0x0043840c,(void*)&Calc_Head_On_Clsn_Dynamics},
{0x0043978c,(void*)&Check_2D_Car_Collision},
{0x00439a04,(void*)&Check_Ground_Car_Collision},
{0x0043a100,(void*)&Check_Space_Car_Collision},
{0x0043a400,(void*)&Do_Car_Collisions},
{0x0043a684,(void*)&Init_Car_Cluts},
{0x0043af34,(void*)&Init_Car_Doors},
{0x0043b26c,(void*)&FUN_0043b26c},
{0x0043b380,(void*)&Change_Bonnet_Clut},
{0x0043b4b8,(void*)&Change_Boot_Clut},
{0x0043b5c0,(void*)&Highlight_Area},
{0x0043b7bc,(void*)&FUN_0043b7bc},
{0x0043b940,(void*)&TextureDentHiCar},
{0x0043bf2c,(void*)&TextureDentMidCar},
{0x0043c4e8,(void*)&FUN_0043c4e8},
{0x0043c55c,(void*)&FUN_0043c55c},
{0x0043c5d0,(void*)&FUN_0043c5d0},
{0x0043c738,(void*)&FUN_0043c738},
{0x0043c910,(void*)&Get_Corner_Positions},
{0x0043cf10,(void*)&Ground_Collision},
{0x0043d194,(void*)&Find_Lowest_Corner},
{0x0043d1f0,(void*)&Make_Car_Fly},
{0x0043d418,(void*)&Car_Landed},
{0x0043d5d4,(void*)&Car_Landed_On_Corner},
{0x0043d68c,(void*)&Car_Fly_Motion_3D},
{0x0043d8e4,(void*)&Car_Grounded_Motion_3D},
{0x0043dbd8,(void*)&FUN_0043dbd8},
{0x0043dd00,(void*)&FUN_0043dd00},
{0x0043e504,(void*)&Car_2pt_Motion_3D},
{0x0043fb24,(void*)&Car_1pt_Motion_3D},
{0x00440980,(void*)&FUN_00440980},
{0x00440ac4,(void*)&FUN_00440ac4},
{0x00440f60,(void*)&FUN_00440f60},
{0x004410f4,(void*)&Calc_Car_Tilt},
{0x00441270,(void*)&Calc_Car_Angles_Square},
{0x00441394,(void*)&FUN_00441394},
{0x004413ac,(void*)&Car_Drive_Motion},
{0x0044216c,(void*)&Car_Drive_2pt_Motion},
{0x004427cc,(void*)&Car_Fly_Motion},
{0x0044282c,(void*)&FUN_0044282c},
{0x00442b08,(void*)&FUN_00442b08},
{0x00442cdc,(void*)&Car_Movement},
{0x004430b8,(void*)&FUN_004430b8},
{0x00443840,(void*)&Init_End_Race},
{0x00443860,(void*)&Calc_Track_Positions},
{0x00443b10,(void*)&FUN_00443b10},
{0x00443b8c,(void*)&Get_Race_Positions},
{0x00443da4,(void*)&Init_Track_Strip_Numbers},
{0x00443f90,(void*)&FUN_00443f90},
{0x00444048,(void*)&FUN_00444048},
{0x004448e0,(void*)&FUN_004448e0},
{0x00444a60,(void*)&FUN_00444a60},
{0x00444c1c,(void*)&Calc_Suspension_Right_Wheels},
{0x00444ddc,(void*)&Calc_Null_Suspension},
{0x00444e3c,(void*)&FUN_00444e3c},
{0x00444e9c,(void*)&Boot_Lost_Geometry},
{0x00444fcc,(void*)&FUN_00444fcc},
{0x00445030,(void*)&FUN_00445030},
{0x004450dc,(void*)&FUN_004450dc},
{0x00445554,(void*)&Check_Bonnet_Removal},
{0x004455e0,(void*)&Check_Boot_Removal},
{0x004456e4,(void*)&Init_Main},
{0x00445764,(void*)&Set_Load_Textures},
{0x004457e8,(void*)&Modify_TDF},
{0x00445a70,(void*)&Init_Graphics},
{0x00445b40,(void*)&FUN_00445b40},
{0x00445b78,(void*)&FUN_00445b78},
{0x00445c74,(void*)&Init_Game},
{0x00445dc0,(void*)&Play_Intro},
{0x00445dfc,(void*)&Play_Xtro},
{0x00445e40,(void*)&Initialise_Pause_Mode},
{0x00446260,(void*)&Pause_Mode},
{0x00446c10,(void*)&FUN_00446c10},
{0x00446c5c,(void*)&UndentCar},
{0x00446fe0,(void*)&SetHighLight},
{0x004471c0,(void*)&FUN_004471c0},
{0x00447360,(void*)&Control_Car_Replay},
{0x004477a0,(void*)&FUN_004477a0},
{0x00447800,(void*)&Record_Event},
{0x004478ec,(void*)&Terminate_Replay},
{0x00447920,(void*)&Terminate_Replay_Bodge},
{0x00447960,(void*)&FUN_00447960},
{0x00447a9c,(void*)&FUN_00447a9c},
{0x00447dfc,(void*)&Amplitude},
{0x00447ea8,(void*)&DopplerFrequency},
{0x0044801c,(void*)&Allocate_Sound_Effect},
{0x004480bc,(void*)&FUN_004480bc},
{0x004481d0,(void*)&Load_Game_Vags},
{0x00448228,(void*)&FUN_00448228},
{0x00448d10,(void*)&FUN_00448d10},
{0x00448d18,(void*)&PitOut},
{0x00448d48,(void*)&PitIn},
{0x00448e0c,(void*)&Pit_Stop1},
{0x00448e48,(void*)&Pit_Stop2},
{0x00448e4c,(void*)&Strip_Trigger_Handler},
{0x00448eb0,(void*)&Sparking},
{0x004493a0,(void*)&LoadSave},
{0x004496d8,(void*)&FUN_004496d8},
{0x00449728,(void*)&FUN_00449728},
{0x00449928,(void*)&FUN_00449928},
{0x00449958,(void*)&FUN_00449958},
{0x00449c54,(void*)&FUN_00449c54},
{0x00449dd8,(void*)&FUN_00449dd8},
{0x00449ee0,(void*)&FUN_00449ee0},
{0x0044a1a4,(void*)&FUN_0044a1a4},
{0x0044a2b0,(void*)&FUN_0044a2b0},
{0x0044a32c,(void*)&FUN_0044a32c},
{0x0044a3a0,(void*)&FUN_0044a3a0},
{0x0044a4e4,(void*)&FUN_0044a4e4},
{0x0044a904,(void*)&FUN_0044a904},
{0x0044aa28,(void*)&FUN_0044aa28},
{0x0044aaf0,(void*)&FUN_0044aaf0},
{0x0044ac28,(void*)&Load_First_Config},
{0x0044ac68,(void*)&Load_Card_File},
{0x0044ac9c,(void*)&FUN_0044ac9c},
{0x0044aec8,(void*)&FUN_0044aec8},
{0x0044b148,(void*)&Init_Front_End},
{0x0044b4bc,(void*)&View_Frontend_Replay},
{0x0044b4e0,(void*)&DemoMode},
{0x0044b5c0,(void*)&FUN_0044b5c0},
{0x0044b5d4,(void*)&Loading_Screen_From_Slab},
{0x0044b684,(void*)&FUN_0044b684},
{0x0044b6b4,(void*)&Load_Completion_Status},
{0x0044b938,(void*)&Call_Loaded_Game},
{0x0044b970,(void*)&FUN_0044b970},
{0x0044b9c0,(void*)&FUN_0044b9c0},
{0x0044ba20,(void*)&Setup_Pad},
{0x0044bb00,(void*)&Init_Wrecking_Championship},
{0x0044bb44,(void*)&Init_StockCar_Championship},
{0x0044bb88,(void*)&FUN_0044bb88},
{0x0044bbc8,(void*)&Init_StockCar_MultiChamp},
{0x0044bc08,(void*)&Championship},
{0x0044bd08,(void*)&MultiChamp},
{0x0044be58,(void*)&Calculate_Finish},
{0x0044c090,(void*)&Calculate_Results},
{0x0044c1d4,(void*)&Do_End_Of_Season_Stuff},
{0x0044c270,(void*)&Init_League_Info},
{0x0044c2d8,(void*)&Reset_League_Info},
{0x0044c2f0,(void*)&Sort_Leagues},
{0x0044c344,(void*)&Init_MultiLeague_Info},
{0x0044c390,(void*)&Setup_Driver_Names},
{0x0044c418,(void*)&FUN_0044c418},
{0x0044c48c,(void*)&Add_Computer_Info},
{0x0044c508,(void*)&Update_League_Info},
{0x0044c538,(void*)&FUN_0044c538},
{0x0044c5a8,(void*)&Sort_MultiLeague},
{0x0044c5e0,(void*)&Sort_RacePos},
{0x0044c6c4,(void*)&Promote_And_Relegate},
{0x0044c76c,(void*)&Check_League_Standing},
{0x0044c7b0,(void*)&Order_Cars},
{0x0044c800,(void*)&FUN_0044c800},
{0x0044c8e8,(void*)&FUN_0044c8e8},
{0x0044cb48,(void*)&FUN_0044cb48},
{0x0044ccb8,(void*)&FUN_0044ccb8},
{0x0044ced4,(void*)&FUN_0044ced4},
{0x0044cef0,(void*)&FUN_0044cef0},
{0x0044d0e4,(void*)&FUN_0044d0e4},
{0x0044d390,(void*)&Start_New_Season_Stats},
{0x0044d3d4,(void*)&FUN_0044d3d4},
{0x0044d478,(void*)&Update_Track_Stats},
{0x0044d538,(void*)&FUN_0044d538},
{0x0044d5d8,(void*)&Update_Championship_Stats},
{0x0044d630,(void*)&FUN_0044d630},
{0x0044d648,(void*)&Get_Current_Recording_Season},
{0x0044d650,(void*)&FUN_0044d650},
{0x0044d69c,(void*)&Update_Jimmy_Spunk_Times},
{0x0044d974,(void*)&FUN_0044d974},
{0x0044db70,(void*)&FUN_0044db70},
{0x0044dcfc,(void*)&Secret},
{0x0044e148,(void*)&FUN_0044e148},
{0x0044e170,(void*)&FUN_0044e170},
{0x0044e440,(void*)&FUN_0044e440},
{0x0044e618,(void*)&FUN_0044e618},
{0x0044e6a0,(void*)&FUN_0044e6a0},
{0x0044eb1c,(void*)&FUN_0044eb1c},
{0x0044ecac,(void*)&FUN_0044ecac},
{0x0044f5c8,(void*)&FUN_0044f5c8},
{0x0044f65c,(void*)&FUN_0044f65c},
{0x0044f6b0,(void*)&FUN_0044f6b0},
{0x0044f814,(void*)&FUN_0044f814},
{0x0044fbc0,(void*)&FUN_0044fbc0},
{0x0044fca4,(void*)&FUN_0044fca4},
{0x0044fce8,(void*)&FUN_0044fce8},
{0x0044fdb4,(void*)&FUN_0044fdb4},
{0x0044fe00,(void*)&View_BestLaps},
{0x0044ff48,(void*)&FUN_0044ff48},
{0x004500e8,(void*)&Front_End},
{0x00450640,(void*)&FUN_00450640},
{0x004508bc,(void*)&Toggle_Track},
{0x00450944,(void*)&Toggle_Car},
{0x0045099c,(void*)&FUN_0045099c},
{0x00450c7c,(void*)&FUN_00450c7c},
{0x00450d0c,(void*)&Rotate_Slab_On},
{0x00450e20,(void*)&FUN_00450e20},
{0x00450ed4,(void*)&Draw_Screen_Polys},
{0x00451288,(void*)&Setup_Screen_Text},
{0x004512f8,(void*)&Setup_Screen_Lines},
{0x00451308,(void*)&Draw_Screen_Lines},
{0x004516ac,(void*)&Button_Pressed},
{0x004517b8,(void*)&Draw_Slab},
{0x004518ac,(void*)&FUN_004518ac},
{0x004518f8,(void*)&Draw_Semi_Trans_Poly},
{0x00451a80,(void*)&FUN_00451a80},
{0x00451aa0,(void*)&Play_Click_FX},
{0x00451ac0,(void*)&FUN_00451ac0},
{0x00451ae0,(void*)&FUN_00451ae0},
{0x00451d4c,(void*)&FUN_00451d4c},
{0x00451d6c,(void*)&FUN_00451d6c},
{0x00451fdc,(void*)&FUN_00451fdc},
{0x00451ff0,(void*)&FUN_00451ff0},
{0x0045202c,(void*)&FUN_0045202c},
{0x00452090,(void*)&FUN_00452090},
{0x004520f0,(void*)&Enter_Driver_Names},
{0x004525a0,(void*)&FUN_004525a0},
{0x00452790,(void*)&FUN_00452790},
{0x004527f0,(void*)&FUN_004527f0},
{0x004529d4,(void*)&FUN_004529d4},
{0x00452a34,(void*)&FUN_00452a34},
{0x00452a80,(void*)&Practice_Over},
{0x00452d30,(void*)&FUN_00452d30},
{0x00452da0,(void*)&FUN_00452da0},
{0x00452dc0,(void*)&FUN_00452dc0},
{0x00452fa4,(void*)&FUN_00452fa4},
{0x00452fc4,(void*)&FUN_00452fc4},
{0x00453044,(void*)&Select_Champ},
{0x004530bc,(void*)&Select_ChampQS},
{0x00453100,(void*)&Select_Pract},
{0x0045313c,(void*)&Select_TimeT},
{0x00453170,(void*)&Select_Total},
{0x004531b0,(void*)&Select_DDPract},
{0x00453580,(void*)&FUN_00453580},
{0x004535c0,(void*)&FUN_004535c0},
{0x004536f0,(void*)&Save_Game},
{0x004539d0,(void*)&FUN_004539d0},
{0x004539f0,(void*)&FUN_004539f0},
{0x00453a18,(void*)&FUN_00453a18},
{0x00453a20,(void*)&FUN_00453a20},
{0x00453b98,(void*)&FUN_00453b98},
{0x00453d48,(void*)&End_Of_Season},
{0x00454158,(void*)&FUN_00454158},
{0x004541e8,(void*)&FUN_004541e8},
{0x00454278,(void*)&FUN_00454278},
{0x00454308,(void*)&FUN_00454308},
{0x00454398,(void*)&FUN_00454398},
{0x00454550,(void*)&FUN_00454550},
{0x00454804,(void*)&FUN_00454804},
{0x00454a70,(void*)&FUN_00454a70},
{0x00454b68,(void*)&Race_Over},
{0x00454e9c,(void*)&FUN_00454e9c},
{0x00454f0c,(void*)&FUN_00454f0c},
{0x004551b0,(void*)&FUN_004551b0},
{0x00455260,(void*)&FUN_00455260},
{0x00455358,(void*)&Display_Season_Status},
{0x0045572c,(void*)&FUN_0045572c},
{0x004559d4,(void*)&FUN_004559d4},
{0x00455d15,(void*)&__open_flags},
{0x00455de4,(void*)&FUN_00455de4},
{0x00455edb,(void*)&FUN_00455edb},
{0x00455f3b,(void*)&FUN_00455f3b},
{0x00455fb0,(void*)&FUN_00455fb0},
{0x00456034,(void*)&FUN_00456034},
{0x0045607b,(void*)&FUN_0045607b},
{0x00456428,(void*)&__shutdown_stream},
{0x0045644e,(void*)&FUN_0045644e},
{0x0045645e,(void*)&FUN_0045645e},
{0x004564d3,(void*)&__doclose},
{0x0045665c,(void*)&__CHP},
{0x00456688,(void*)&nfree},
{0x00456717,(void*)&FUN_00456717},
{0x0045672e,(void*)&FUN_0045672e},
{0x00456a32,(void*)&__null_int23_exit},
{0x00456a4d,(void*)&_exit},
{0x00456a74,(void*)&wstart2_},
{0x00456af2,(void*)&FUN_00456af2},
{0x00456b30,(void*)&FUN_00456b30},
{0x00456b67,(void*)&FUN_00456b67},
{0x00456bf3,(void*)&FUN_00456bf3},
{0x00456ca0,(void*)&_tolower},
{0x00456cae,(void*)&FUN_00456cae},
{0x00456cc0,(void*)&__set_EDOM},
{0x00456ccb,(void*)&__set_ERANGE},
{0x00456cdf,(void*)&FUN_00456cdf},
{0x00456ced,(void*)&__set_doserrno},
{0x00456cfb,(void*)&open},
{0x00456d1d,(void*)&sopen},
{0x00456ef2,(void*)&__allocfp},
{0x00456fa7,(void*)&__freefp},
{0x00456fde,(void*)&__purgefp},
{0x00456ffc,(void*)&__chktty},
{0x0045702f,(void*)&__threadid},
{0x0045703b,(void*)&FUN_0045703b},
{0x00457040,(void*)&FUN_00457040},
{0x00457041,(void*)&FUN_00457041},
{0x0045704f,(void*)&FUN_0045704f},
{0x0045705d,(void*)&__NTInit},
{0x00457180,(void*)&__NTMainInit},
{0x004571c3,(void*)&__exit},
{0x004571e4,(void*)&FUN_004571e4},
{0x004572de,(void*)&_lseek},
{0x004572f8,(void*)&FUN_004572f8},
{0x00457346,(void*)&tell},
{0x00457388,(void*)&__ioalloc},
{0x00457408,(void*)&FUN_00457408},
{0x00457497,(void*)&fgetc},
{0x00457542,(void*)&__filbuf},
{0x00457571,(void*)&FUN_00457571},
{0x0045762c,(void*)&getpid},
{0x00457631,(void*)&FUN_00457631},
{0x004576c5,(void*)&FUN_004576c5},
{0x0045771e,(void*)&_fpreset},
{0x0045777d,(void*)&nmalloc},
{0x0045787e,(void*)&__MemAllocator},
{0x00457926,(void*)&__MemFree},
{0x00457a31,(void*)&__prtf},
{0x00457d39,(void*)&FUN_00457d39},
{0x00457e84,(void*)&FUN_00457e84},
{0x00457ee9,(void*)&FUN_00457ee9},
{0x00457f0f,(void*)&FUN_00457f0f},
{0x00457f40,(void*)&FUN_00457f40},
{0x00457f9f,(void*)&FUN_00457f9f},
{0x0045809c,(void*)&FUN_0045809c},
{0x004580b7,(void*)&FUN_004580b7},
{0x004585e8,(void*)&FUN_004585e8},
{0x00458608,(void*)&__qwrite},
{0x004586c4,(void*)&fputc},
{0x004587c0,(void*)&__WinMain},
{0x0045889e,(void*)&__NTAtMaxFiles},
{0x004588df,(void*)&__NTAddFileHandle},
{0x00458964,(void*)&FUN_00458964},
{0x00458a0b,(void*)&__NTRemoveFileHandle},
{0x00458a31,(void*)&FUN_00458a31},
{0x00458a70,(void*)&__NTGetFakeHandle},
{0x00458abe,(void*)&__GetNTAccessAttr},
{0x00458af9,(void*)&__GetNTShareAttr},
{0x00458b30,(void*)&_stricmp},
{0x00458b45,(void*)&FUN_00458b45},
{0x00458baa,(void*)&dosretax},
{0x00458bc7,(void*)&FUN_00458bc7},
{0x00458c19,(void*)&__set_errno_nt},
{0x00458c28,(void*)&isatty},
{0x00458c6f,(void*)&__IOMode},
{0x00458cc5,(void*)&FUN_00458cc5},
{0x00458ce6,(void*)&__sigfpe_handler},
{0x00458d31,(void*)&signal},
{0x00458db0,(void*)&raise},
{0x00458e2f,(void*)&FUN_00458e2f},
{0x00458e5c,(void*)&__SigFini},
{0x00459228,(void*)&__NewExceptionHandler},
{0x00459268,(void*)&__DoneExceptionHandler},
{0x00459287,(void*)&__CloseSemaphore},
{0x004592b1,(void*)&__AccessSemaphore},
{0x00459314,(void*)&__ReleaseSemaphore},
{0x00459410,(void*)&__InitThreadData},
{0x0045944e,(void*)&__NTThreadInit},
{0x00459496,(void*)&__NTAddThread},
{0x004594ed,(void*)&__NTRemoveThread},
{0x00459561,(void*)&__InitMultipleThread},
{0x00459756,(void*)&__InitRtns},
{0x004597a1,(void*)&__FiniRtns},
{0x00459865,(void*)&__full_io_exit},
{0x0045987f,(void*)&FUN_0045987f},
{0x004598df,(void*)&flushall},
{0x004598ea,(void*)&__flushall},
{0x0045992d,(void*)&getche},
{0x00459953,(void*)&unlink},
{0x00459968,(void*)&FUN_00459968},
{0x00459a88,(void*)&FUN_00459a88},
{0x00459aed,(void*)&__cnvs2d},
{0x00459b19,(void*)&FUN_00459b19},
{0x00459b2a,(void*)&__init_80x87},
{0x00459b51,(void*)&FUN_00459b51},
{0x00459bc5,(void*)&FUN_00459bc5},
{0x00459c55,(void*)&__ExpandDGROUP},
{0x00459c68,(void*)&FUN_00459c68},
{0x00459caf,(void*)&__nmemneed},
{0x00459cd7,(void*)&utoa},
{0x00459d29,(void*)&_itoa},
{0x00459d43,(void*)&itoa},
{0x00459d9d,(void*)&ultoa},
{0x00459ded,(void*)&ltoa},
{0x00459e22,(void*)&_ltoa},
{0x00459e3c,(void*)&_toupper},
{0x00459e4a,(void*)&FUN_00459e4a},
{0x00459e6e,(void*)&FUN_00459e6e},
{0x00459e9d,(void*)&__CommonInit},
{0x00459ea8,(void*)&FUN_00459ea8},
{0x00459ebd,(void*)&nrealloc},
{0x00459f57,(void*)&FUN_00459f57},
{0x00459fc0,(void*)&FUN_00459fc0},
{0x00459ff5,(void*)&FUN_00459ff5},
};
int dd2_fnmap_n=sizeof(dd2_fnmap)/sizeof(dd2_fnmap[0]);
static void* g_lut[0x50000];
void dd2_relocate(void){
  int i; unsigned off;
  for(i=0;i<dd2_fnmap_n;i++){unsigned v=dd2_fnmap[i].va; if(v>=0x410000&&v<0x460000) g_lut[v-0x410000]=dd2_fnmap[i].fn;}
  for(off=0x60000; off+4<=0x578800; off+=4){
    unsigned w=*(unsigned*)(g_image+off);
    if(w>=0x410000&&w<0x460000){void*fn=g_lut[w-0x410000]; if(fn)*(void**)(g_image+off)=fn;}
  }
}
