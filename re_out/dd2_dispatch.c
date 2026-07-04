#include <stdio.h>
/* indirect-call dispatch: VA->C-function map + startup relocation of fn-pointers in the image */
#include "ghidra_compat.h"
extern unsigned char* g_image;
extern int draw_text_half();
extern int FUN_0041033a();
extern int draw_half();
extern int FUN_0041080d();
extern int draw_text_half_trans();
extern int sub_410d50();
extern int sub_410d58();
extern int sub_410e44();
extern int FUN_00410f74();
extern int FUN_004110a4();
extern int FUN_004111e8();
extern int FUN_0041132c();
extern int sub_4114c0();
extern int sub_411654();
extern int sub_4117e8();
extern int sub_4118d4();
extern int FUN_00411a04();
extern int FUN_00411b34();
extern int FUN_00411c78();
extern int FUN_00411ebc();
extern int FUN_0041243c();
extern int FUN_00412694();
extern int ClearOTagR();
extern int DrawOTag();
extern int DrawPrim();
extern int FUN_004128e6();
extern int Init_Application();
extern int Close_Application();
extern int SetVideoMode();
extern int VSync();
extern int PutDrawEnv();
extern int PutDispEnv();
extern int SetPalette();
extern int VSyncCallback();
extern int FUN_00412ecc();
extern int FUN_00412edc();
extern int MoveImage();
extern int LoadImage();
extern int MoveImageClut();
extern int LoadImageClut();
extern int StoreImage();
extern int FUN_00413054();
extern int FUN_004130b0();
extern int FUN_004130f0();
extern int DDRelease();
extern int FUN_004132f0();
extern int FUN_00413488();
extern int Generate_Transparency_Tables();
extern int MulMatrix();
extern int gte_MulMatrix0();
extern int MulMatrix2();
extern int rsin();
extern int rcos();
extern int SquareRoot0_();
extern int GTERT();
extern int GTERPS();
extern int GTERPT();
extern int GTERPT4_();
extern int FUN_00413b8e();
extern int RotMatrix();
extern int ApplyMatrixLV();
extern int FUN_00413e08();
extern int FUN_00413f85();
extern int FUN_00414016();
extern int FUN_00414099();
extern int gte_dpcs();
extern int gte_ncds();
extern int SetFogNearFar();
extern int gte_SetFogFar();
extern int gte_SetFogNear();
extern int PushMatrix();
extern int PopMatrix();
extern int FUN_004143b0();
extern int VectorNormalS();
extern int VectorNormalSS();
extern int RotMatrixYXZ();
extern int RotMatrixX();
extern int RotMatrixY();
extern int RotMatrixZ();
extern int FUN_00414920();
extern int gte_SetRotMatrix();
extern int RotTrans();
extern int RotTransPers();
extern int RotAverageNclip4();
extern int OuterProduct0();
extern int OuterProduct12();
extern int Play_Movie();
extern int Load_Null();
extern int Load_Textures();
extern int sub_415000();
extern int Load_Texture2();
extern int Load_Cluts();
extern int Add_Buffer_Load_();
extern int FUN_004151b0();
extern int Read_Directory();
extern int File_Load();
extern int FUN_00415424();
extern int FUN_00415454();
extern int FUN_00415498();
extern int FUN_00415508();
extern int Decompress();
extern int DSLoadSoundBuffer();
extern int FUN_0041579c();
extern int DSGetWaveResource();
extern int FUN_00415838();
extern int FUN_004158e4();
extern int FUN_004159f8();
extern int Sound_Init();
extern int FUN_00415ae4();
extern int Sound_Remove();
extern int Master_CD_Volume();
extern int FUN_00415ba0();
extern int Sound_Stop();
extern int Sound_Pause_();
extern int Sound_Restart();
extern int Kill_Sound();
extern int Play_Sound();
extern int Modify_Sound();
extern int FUN_00415fb4();
extern int Unlock_Channel();
extern int Echo_Free();
extern int Set_Echo_Mode();
extern int Set_Echo_Depth();
extern int FUN_00416094();
extern int CD_Close();
extern int FUN_0041616c();
extern int Read_CD_Toc_();
extern int FUN_0041617c();
extern int Start_CD_Audio();
extern int Check_For_CD_Loop();
extern int FUN_004162b4();
extern int CD_Pause();
extern int CD_Restart();
extern int Sound_Timer_();
extern int FUN_00416404();
extern int FUN_0041648c();
extern int FUN_004164e4();
extern int FUN_00416524();
extern int FUN_004165f4();
extern int CD_Check();
extern int Load_Sprite_Info();
extern int Search_For_Sprite();
extern int FUN_00416714();
extern int Setup_Sprite();
extern int FUN_00416764();
extern int Modify_Sprite();
extern int FUN_00416a70();
extern int draw_face_3pt_flat();
extern int draw_face_3pt_flat_lit();
extern int draw_face_3pt_flat_dpq();
extern int draw_face_3pt_flat_dpq_lit();
extern int draw_face_4pt_flat();
extern int draw_face_4pt_flat_lit();
extern int draw_face_4pt_flat_dpq();
extern int draw_face_4pt_flat_dpq_lit();
extern int draw_face_3pt_text();
extern int draw_face_3pt_text_squash();
extern int draw_face_3pt_text_lit();
extern int draw_face_3pt_text_dpq();
extern int draw_face_3pt_text_dpq_squash();
extern int draw_face_3pt_text_dpq_lit();
extern int FUN_0041a3b4();
extern int draw_face_4pt_text();
extern int draw_face_4pt_text_squash();
extern int draw_face_4pt_text_lit();
extern int draw_face_4pt_text_dpq();
extern int draw_face_4pt_text_dpq_squash();
extern int draw_face_4pt_text_dpq_lit();
extern int draw_face_3pt_gour();
extern int draw_face_3pt_gour_lit();
extern int draw_face_3pt_gour_dpq();
extern int draw_face_3pt_gour_dpq_lit();
extern int draw_face_4pt_gour();
extern int draw_face_4pt_gour_lit();
extern int draw_face_4pt_gour_dpq();
extern int draw_face_4pt_gour_dpq_lit();
extern int draw_face_3pt_pict();
extern int draw_face_3pt_pict_lit();
extern int draw_face_3pt_pict_dpq();
extern int draw_face_3pt_pict_dpq_lit();
extern int draw_face_4pt_pict();
extern int draw_face_4pt_pict_lit();
extern int draw_face_4pt_pict_dpq();
extern int draw_face_4pt_pict_dpq_lit();
extern int setup_face_sprite();
extern int draw_face_sprite();
extern int draw_face_sprite_dpq();
extern int draw_face_tilt_sprite_dpq();
extern int FUN_0041f7d0();
extern int FUN_0041fcac();
extern int Create_Object();
extern int Set_Object();
extern int Remove_Object();
extern int Pre_Rotate();
extern int Draw_Subdiv_Object();
extern int FUN_0041ff98();
extern int Update_Object();
extern int FUN_00420080();
extern int FUN_0042016c();
extern int FUN_00420190();
extern int Set_Zclip();
extern int Set_Clip();
extern int Set_World_Position();
extern int Set_World_Matrix();
extern int Set_World_View();
extern int Set_Ambient_Light();
extern int Set_Depth_Cue();
extern int FUN_004203dc();
extern int FUN_004204d0();
extern int Calc_Object_MatrixYZX();
extern int Calc_Object_Angles();
extern int Calc_Object_AnglesYZX();
extern int FUN_00420708();
extern int Point_Camera();
extern int Set_Draw_Mode();
extern int FUN_00420c4c();
extern int Draw_All();
extern int Draw_Tile();
extern int Draw_Line();
extern int Allocate_OT_();
extern int Swap_Buffers();
extern int FUN_00420e20();
extern int Draw_Font_Poly();
extern int Init_Primitive_Buffer();
extern int Reset_Primitive_Buffer();
extern int Free_Primitive_Buffer();
extern int FUN_00420f9c();
extern int FUN_00421018();
extern int Allocate_Font_Buffers();
extern int Setup_Font();
extern int Duplicate_Font();
extern int Print_Locate();
extern int Print_Font();
extern int Print_Ink();
extern int Print_InkRGB();
extern int Print();
extern int Print_Draw();
extern int FUN_00422194();
extern int FUN_004221fc();
extern int FUN_00422258();
extern int FUN_004222b4();
extern int FUN_0042231c();
extern int FUN_00422488();
extern int FUN_00422678();
extern int FUN_00422a6c();
extern int Init_Controller_();
extern int Setup_Controller();
extern int Setup_Joystick();
extern int FUN_00422da4();
extern int Translate_Keypress();
extern int FUN_0042317c();
extern int InitCardSystem();
extern int FUN_00423340();
extern int SaveCardFile();
extern int DeleteFileMC();
extern int LoadCardFiles();
extern int LoadCardFile();
extern int FirstSavedGame();
extern int InitCardBlocks();
extern int DupFileCheck();
extern int FUN_0042366c();
extern int MPE_InitHeap();
extern int MPE_malloc();
extern int MPE_free();
extern int Debug_Stub();
extern int PC_Read_File();
extern int PC_Write_File();
extern int System_Error();
extern int FUN_004238f0();
extern int FUN_00423934();
extern int Profile_Init();
extern int Profile_Start();
extern int Profile_Stop();
extern int ddmain();
extern int Play_Game();
extern int Init_Debris_();
extern int Setup_Debris();
extern int Update_Debris();
extern int FUN_00424a60();
extern int Setup_Flying_Objects();
extern int Update_Flying_Objects();
extern int Zero_Flying_Object();
extern int Request_Flying_Object();
extern int FUN_00425444();
extern int Flying_Objects_Pos_Ang();
extern int FUN_00425bc8();
extern int FUN_00425cb8();
extern int InitialiseDenting();
extern int FUN_00425fa4();
extern int Generate_Surface_Normals();
extern int Mask_Point_In_Quad();
extern int FUN_00426be4();
extern int Track_Follow();
extern int Map_Height();
extern int FUN_00428678();
extern int Move_Forward_Strip();
extern int FUN_004288f0();
extern int Search_For_Strip();
extern int FUN_00428a00();
extern int FUN_00428a8c();
extern int FUN_00429714();
extern int Car_Camera();
extern int Camera_Pad_Control();
extern int Init_Pit_Camera_();
extern int FUN_00429ac4();
extern int Pit_Camera_Control();
extern int Init_The_Floaty_Camera();
extern int Do_The_Floaty_Camera_Thing();
extern int Init_Damage_Indicator();
extern int FUN_0042a608();
extern int Bonnet_Smoke();
extern int FUN_0042b720();
extern int Draw_Car();
extern int FUN_0042c2d4();
extern int Init_Car_Graphics();
extern int Init_Wild_Bill();
extern int Init_Rollercoaster();
extern int Init_CLUT_Animation_();
extern int Init_Texture_Animation();
extern int Update_Texture_Animation();
extern int Texture_Animation();
extern int Update_CLUT_Animation();
extern int CLUT_Animation();
extern int Update_Other_Objects();
extern int Draw_Other_Objects();
extern int Draw_Dynamic_Objects();
extern int Init_Flag();
extern int DrawFlagObject();
extern int UpdateFlag();
extern int ZoomFlag();
extern int Init_LensFlare();
extern int DrawLensFlare();
extern int Init_Overlays();
extern int Draw_Overlays();
extern int FUN_0042fb7c();
extern int Update_Race_CountDown();
extern int FUN_0042fe88();
extern int Display_Position_Pointers();
extern int Init_Scene();
extern int VVDraw_Object();
extern int Draw_Scene_Object();
extern int FUN_004307c8();
extern int Setup_Object_Block();
extern int FUN_004309e8();
extern int Decrunch_Object_Block();
extern int Object_Decompression();
extern int FUN_00430b60();
extern int Update_Scene_Objects();
extern int FUN_00430d0e();
extern int Init_Bowl_Objects();
extern int Init_Scene_Objects();
extern int Init_Track_Objects();
extern int Remove_Scene_Objects();
extern int Init_Sky();
extern int FUN_0043102c();
extern int Draw_Sky();
extern int AI_Com_Server();
extern int Determine_AI();
extern int Recommended_Acceleration();
extern int FUN_004330c8();
extern int Get_Car_Angle();
extern int Get_Direction_Cosines();
extern int Interpolate_Direction_Vectors_Left();
extern int Interpolate_Direction_Vectors_Right_();
extern int InitialiseAI();
extern int Obstacle_Ahead();
extern int Strip_Distance();
extern int FUN_00433ba0();
extern int CheckPointScoring();
extern int Barrier_Collision();
extern int Barrier_Corner_Collision();
extern int FUN_00435300();
extern int FUN_00435384();
extern int TransformWheels_();
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
extern int FUN_00436d34();
extern int FreeParticle();
extern int DrawParticles();
extern int FUN_00436e2c();
extern int FUN_00436f00();
extern int Start_Roll();
extern int FUN_004371cc();
extern int FUN_0043732c();
extern int Calc_Head_On_Clsn_Dynamics();
extern int Check_2D_Car_Collision();
extern int Check_Ground_Car_Collision();
extern int Check_Space_Car_Collision();
extern int Do_Car_Collisions();
extern int Init_Car_Cluts();
extern int Init_Car_Doors();
extern int FUN_0043b39c();
extern int Change_Bonnet_Clut();
extern int Change_Boot_Clut();
extern int Highlight_Area();
extern int FUN_0043b8ec();
extern int TextureDentHiCar();
extern int TextureDentMidCar();
extern int FUN_0043c618();
extern int FUN_0043c68c();
extern int FUN_0043c700();
extern int FUN_0043c868();
extern int Get_Corner_Positions();
extern int Ground_Collision();
extern int Find_Lowest_Corner();
extern int Make_Car_Fly();
extern int Car_Landed();
extern int Car_Landed_On_Corner();
extern int Car_Fly_Motion_3D();
extern int Car_Grounded_Motion_3D();
extern int Car_Rolled_Edge_Onto_Wheels_();
extern int FUN_0043dd08();
extern int Check_Quadrant();
extern int FUN_0043de30();
extern int Car_2pt_Motion_3D();
extern int Car_1pt_Motion_3D();
extern int Car_Drive_Motion_3D_();
extern int Sticky_Car_Motion_3D();
extern int FUN_00440ab0();
extern int FUN_00440bf4();
extern int FUN_00441090();
extern int Calc_Car_Tilt();
extern int Calc_Car_Angles_Square();
extern int FUN_004414c4();
extern int Car_Drive_Motion();
extern int Car_Drive_2pt_Motion();
extern int Car_Fly_Motion();
extern int FUN_0044295c();
extern int FUN_00442c38();
extern int Car_Movement();
extern int FUN_004431e8();
extern int Init_End_Race();
extern int Calc_Track_Positions();
extern int FUN_00443c40();
extern int Get_Race_Positions();
extern int Init_Track_Strip_Numbers();
extern int FUN_004440c0();
extern int FUN_00444178();
extern int FUN_00444a10();
extern int FUN_00444b90();
extern int Calc_Suspension_Right_Wheels();
extern int Calc_Null_Suspension();
extern int FUN_00444f6c();
extern int Boot_Lost_Geometry();
extern int FUN_004450fc();
extern int FUN_00445160();
extern int FUN_0044520c();
extern int Check_Bonnet_Removal();
extern int Check_Boot_Removal();
extern int Init_Sys();
extern int Init_Main();
extern int Set_Load_Textures();
extern int Modify_TDF();
extern int Init_Graphics();
extern int FUN_00445c70();
extern int FUN_00445ca8();
extern int Init_Game();
extern int Play_Intro();
extern int Play_Xtro();
extern int Initialise_Pause_Mode();
extern int Pause_Mode();
extern int FUN_00446d50();
extern int UndentCar();
extern int SetHighLight();
extern int FUN_00447300();
extern int Control_Car_Replay();
extern int FUN_004478e0();
extern int Record_Event();
extern int Terminate_Replay();
extern int Terminate_Replay_Bodge();
extern int FUN_00447aa0();
extern int FUN_00447bdc();
extern int Amplitude();
extern int DopplerFrequency();
extern int Allocate_Sound_Effect();
extern int FUN_004481fc();
extern int Load_Game_Vags();
extern int Clear_SoundFx();
extern int FUN_00448368();
extern int PitOut();
extern int PitIn();
extern int Pit_Stop1();
extern int Pit_Stop2();
extern int Strip_Trigger_Handler();
extern int Sparking();
extern int LoadSave();
extern int FUN_00449818();
extern int FUN_00449868();
extern int FUN_00449a68();
extern int FUN_00449a98();
extern int FUN_00449dc4();
extern int FUN_00449f48();
extern int FUN_0044a050();
extern int FUN_0044a314();
extern int FUN_0044a420();
extern int FUN_0044a49c();
extern int FUN_0044a510();
extern int FUN_0044a654();
extern int FUN_0044aa74();
extern int FUN_0044ab98();
extern int FUN_0044ac60();
extern int Duplicate_Results();
extern int Load_First_Config();
extern int Load_Card_File();
extern int FUN_0044ae0c();
extern int FUN_0044b038();
extern int Init_Front_End();
extern int View_Frontend_Replay();
extern int DemoMode();
extern int FUN_0044b730();
extern int Loading_Screen_From_Slab();
extern int FUN_0044b7f4();
extern int Load_Completion_Status();
extern int Run_Selection();
extern int Call_Loaded_Game();
extern int FUN_0044baec();
extern int FUN_0044bb3c();
extern int Setup_Pad();
extern int Init_Wrecking_Championship();
extern int Init_StockCar_Championship();
extern int FUN_0044bd48();
extern int Init_StockCar_MultiChamp();
extern int Championship();
extern int MultiChamp();
extern int Calculate_Finish();
extern int Calculate_Results();
extern int End_Of_Game_Sequence();
extern int Do_End_Of_Season_Stuff();
extern int Init_League_Info();
extern int Reset_League_Info();
extern int Sort_Leagues();
extern int Init_MultiLeague_Info();
extern int Setup_Driver_Names();
extern int FUN_0044c5d8();
extern int Add_Computer_Info();
extern int Update_League_Info();
extern int FUN_0044c6f8();
extern int Sort_MultiLeague();
extern int Sort_RacePos();
extern int Promote_And_Relegate();
extern int Check_League_Standing();
extern int Order_Cars();
extern int FUN_0044c9c0();
extern int FUN_0044caa8();
extern int View_Results_Replay_();
extern int FUN_0044cd08();
extern int View_Champ_Stats();
extern int FUN_0044ce78();
extern int View_Driver_Stats();
extern int FUN_0044d094();
extern int FUN_0044d0b0();
extern int FUN_0044d2a4();
extern int Start_New_Season_Stats();
extern int FUN_0044d594();
extern int Update_Track_Stats();
extern int FUN_0044d6f8();
extern int Update_Championship_Stats();
extern int FUN_0044d7f0();
extern int Get_Current_Recording_Season();
extern int FUN_0044d810();
extern int Update_Jimmy_Spunk_Times();
extern int View_Track_Stats();
extern int FUN_0044db34();
extern int FUN_0044dd30();
extern int Secret();
extern int FUN_0044e308();
extern int FUN_0044e330();
extern int Select_Car();
extern int FUN_0044e600();
extern int FUN_0044e7d8();
extern int FUN_0044e860();
extern int Select_ScreenPos();
extern int Configuration();
extern int FUN_0044ecdc();
extern int View_Credits();
extern int FUN_0044ee6c();
extern int FUN_0044f788();
extern int FUN_0044f81c();
extern int FUN_0044f870();
extern int FUN_0044f9d4();
extern int FUN_0044fd80();
extern int FUN_0044fe64();
extern int FUN_0044fea8();
extern int FUN_0044ff74();
extern int View_BestLaps();
extern int FUN_00450108();
extern int Front_End();
extern int FUN_00450800();
extern int Toggle_Track();
extern int Toggle_Car();
extern int FUN_00450b5c();
extern int FUN_00450e3c();
extern int FUN_004507f8(); extern int FUN_004509d8();   /* patch 775: FE Go! + mode-cycle */
extern int FUN_0045089c(); extern int FUN_00450840();
extern int FUN_004508d4(); extern int FUN_00450904(); extern int FUN_00450934();
extern int FUN_00450964(); extern int FUN_00450994();   /* patch 770: FE menu-screen handlers */
extern int Rotate_Slab_On();
extern int FUN_00450fe0();
extern int Draw_Screen_Polys();
extern int Setup_Screen_Text();
extern int Setup_Screen_Lines();
extern int Draw_Screen_Lines();
extern int Button_Pressed();
extern int Glow_Selector();
extern int Draw_Slab();
extern int Null_Routine(int);
extern int FUN_00451a6c();
extern int Draw_Semi_Trans_Poly();
extern int FUN_00451c40();
extern int Play_Click_FX();
extern int FUN_00451c80();
extern int FUN_00451ca0();
extern int Show_Information();
extern int FUN_00451f0c();
extern int FUN_00451f2c();
extern int FUN_0045219c();
extern int FUN_004521b0();
extern int FUN_004521ec();
extern int FUN_00452250();
extern int Enter_Driver_Names();
extern int FUN_00452760();
extern int FUN_00452950();
extern int FUN_004529b0();
extern int FUN_00452b94();
extern int FUN_00452bf4();
extern int FUN_00452bb4(); extern int FUN_00452bd4(); extern int FUN_00452c0c(); extern int FUN_00452c24();  /* patch 816: race-mode dialog */
extern int Practice_Over();
extern int FUN_00452ef0();
extern int FUN_00452f60();
extern int FUN_00452f80();
extern int FUN_00453164(); extern int FUN_004531a4(); extern int FUN_004531c4();  /* patch 817 */
extern int FUN_00453184();
extern int Select_Champ();
extern int Select_Multi();
extern int Select_ChampQS();
extern int Select_Pract();
extern int Select_TimeT();
extern int Select_Total();
extern int Select_DDPract();
extern int Select_RaceType_DD();
extern int Select_Track();
extern int FUN_00453740();
extern int FUN_00453780();
extern int Save_Game();
extern int FUN_00453b90();
extern int FUN_00453bb0();
extern int FUN_00453bd8();
extern int FUN_00453be0();
extern int FUN_00453d58();
extern int End_Of_Season();
extern int FUN_00454318();
extern int FUN_004543a8();
extern int FUN_00454438();
extern int FUN_004544c8();
extern int FUN_00454558();
extern int FUN_00454710();
extern int FUN_004549c4();
extern int View_MultiLeague();
extern int FUN_00454c30();
extern int Race_Over();
extern int FUN_0045505c();
extern int FUN_004550cc();
extern int FUN_00455370();
extern int FUN_00455420();
extern int Display_Season_Status();
extern int FUN_004558ec();
extern int FUN_00455b94();
extern int __open_flags();
extern int FUN_00455fa4();
extern int FUN_0045609b();
extern int FUN_004560fb();
extern int FUN_00456170();
extern int FUN_004561f4();
extern int __shutdown_stream();
extern int FUN_0045660e();
extern int FUN_0045661e();
extern int __CHP();
extern int nfree();
extern int __null_int23_exit();
extern int _exit();
extern int wstart2_();
extern int FUN_00456cb2();
extern int FUN_00456cf0();
extern int FUN_00456d27();
extern int FUN_00456db3();
extern int _tolower();
extern int FUN_00456e6e();
extern int __set_EDOM();
extern int __set_ERANGE();
extern int FUN_00456e9f();
extern int __set_doserrno();
extern int open();
extern int sopen();
extern int __allocfp();
extern int __freefp();
extern int __purgefp();
extern int __chktty();
extern int __threadid();
extern int FUN_004571fb();
extern int FUN_00457200();
extern int FUN_00457201();
extern int FUN_0045720f();
extern int __NTInit();
extern int __NTMainInit();
extern int __exit();
extern int FUN_004573a4();
extern int _lseek();
extern int FUN_004574b8();
extern int tell();
extern int __ioalloc();
extern int FUN_004575c8();
extern int FUN_00457731();
extern int getpid();
extern int FUN_004577f1();
extern int FUN_00457885();
extern int __init_8087_();
extern int _fpreset();
extern int nmalloc();
extern int __MemAllocator();
extern int __MemFree();
extern int FUN_00457ef9();
extern int FUN_00458044();
extern int FUN_004580a9();
extern int FUN_004580cf();
extern int FUN_00458100();
extern int FUN_0045815f();
extern int FUN_0045825c();
extern int FUN_00458277();
extern int FUN_004587a8();
extern int __qwrite();
extern int __WinMain();
extern int __NTAtMaxFiles();
extern int FUN_00458b24();
extern int __NTRemoveFileHandle();
extern int FUN_00458bf1();
extern int __NTGetFakeHandle();
extern int __GetNTAccessAttr();
extern int __GetNTShareAttr();
extern int _stricmp();
extern int FUN_00458d05();
extern int _dosret0();
extern int dosretax();
extern int FUN_00458d87();
extern int __set_errno_nt();
extern int isatty();
extern int __IOMode();
extern int FUN_00458e85();
extern int __sigfpe_handler();
extern int signal();
extern int raise();
extern int FUN_00458fef();
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
extern int FUN_00459a3f();
extern int flushall();
extern int __flushall();
extern int getche();
extern int unlink();
extern int FUN_00459b28();
extern int FUN_00459c48();
extern int __cnvs2d();
extern int FUN_00459cd9();
extern int __init_80x87();
extern int FUN_00459d11();
extern int FUN_00459d85();
extern int __ExpandDGROUP();
extern int FUN_00459e28();
extern int __nmemneed();
extern int utoa();
extern int _itoa();
extern int itoa();
extern int ultoa();
extern int ltoa();
extern int _ltoa();
extern int _toupper();
extern int FUN_0045a00a();
extern int FUN_0045a02e();
extern int __CommonInit();
extern int FUN_0045a068();
extern int nrealloc();
extern int FUN_0045a117();
extern int FUN_0045a180();
extern int FUN_0045a1b5();
extern int __RemoveThreadData();
extern int FUN_0045a2cb();
extern int __fatal_runtime_error();
extern int FUN_0045a334();
extern int __HasLeadingZero();
extern int FUN_0045a570();
extern int FUN_0045a613();
extern int FUN_0045a686();
extern int _FtoS();
extern int _nheapshrink();
extern int FUN_0045abb1();
extern int FUN_0045ac0a();
extern int FUN_0045ac5f();
extern int FUN_0045ac6c();
extern int __HeapManager_expand();
extern int nexpand();
extern int FUN_0045ae76();
extern int __initthread();
extern int __EnterWVIDEO();
extern int FUN_0045af27();
extern int FUN_0045af54();
extern int __NTConsoleInput();
extern int FUN_0045afcc();
extern int __Nan_Inf();
extern int FUN_0045b06a();
extern int IF_DLOG2();
extern int IF_DLOG10();
extern int log10();
extern int log2();
extern int floor();
extern int _Scale();
extern int __ZBuf2F();
extern int FUN_0045b599();
extern int __CBeginThread();
extern int FUN_0045b76e();
extern int FUN_0045b794();
extern int modf();
extern int __CmpBigInt_();
extern int FUN_0045b848();
extern int FUN_0045b8b3();
extern int FUN_0045b91d();
extern int FUN_0045b9d2();
extern int FUN_0045b9d4();
extern int FUN_0045bf9a();
extern int FUN_0045c058();
extern int frexp();
extern int __math1err();
extern int _set_matherr();
extern int __rterrmsg();
extern int _matherr();
extern int __matherr();
extern int __get_std_stream();
extern int FUN_0045c46b();
typedef struct{unsigned va;void*fn;}dd2_fnent;
extern int FUN_00417f00();
extern int FUN_0041867c();
extern int FUN_00418f30();
extern int FUN_0041bd3c();
extern int FUN_0041bd98();
extern int FUN_0041c514();
extern int FUN_0041c570();
extern int FUN_0041ce28();
extern int FUN_0041cf00();
extern int FUN_0041d964();
extern int FUN_0041da48();
dd2_fnent dd2_fnmap[]={
{0x00410010,(void*)&draw_text_half},
{0x0041033a,(void*)&FUN_0041033a},
{0x0041066a,(void*)&draw_half},
{0x0041080d,(void*)&FUN_0041080d},
{0x004109e8,(void*)&draw_text_half_trans},
{0x00410d50,(void*)&sub_410d50},
{0x00410d58,(void*)&sub_410d58},
{0x00410e44,(void*)&sub_410e44},
{0x00410f74,(void*)&FUN_00410f74},
{0x004110a4,(void*)&FUN_004110a4},
{0x004111e8,(void*)&FUN_004111e8},
{0x0041132c,(void*)&FUN_0041132c},
{0x004114c0,(void*)&sub_4114c0},
{0x00411654,(void*)&sub_411654},
{0x004117e8,(void*)&sub_4117e8},
{0x004118d4,(void*)&sub_4118d4},
{0x00411a04,(void*)&FUN_00411a04},
{0x00411b34,(void*)&FUN_00411b34},
{0x00411c78,(void*)&FUN_00411c78},
{0x00411ebc,(void*)&FUN_00411ebc},
{0x0041243c,(void*)&FUN_0041243c},
{0x00412694,(void*)&FUN_00412694},
{0x0041285c,(void*)&ClearOTagR},
{0x00412883,(void*)&DrawOTag},
{0x004128c5,(void*)&DrawPrim},
{0x004128e6,(void*)&FUN_004128e6},
{0x00412950,(void*)&Init_Application},
{0x00412ad4,(void*)&Close_Application},
{0x00412b6c,(void*)&SetVideoMode},
{0x00412bec,(void*)&VSync},
{0x00412bf4,(void*)&PutDrawEnv},
{0x00412ca0,(void*)&PutDispEnv},
{0x00412cf0,(void*)&SetPalette},
{0x00412ebc,(void*)&VSyncCallback},
{0x00412ecc,(void*)&FUN_00412ecc},
{0x00412edc,(void*)&FUN_00412edc},
{0x00412f1c,(void*)&MoveImage},
{0x00412f7c,(void*)&LoadImage},
{0x00412fe8,(void*)&MoveImageClut},
{0x00413044,(void*)&LoadImageClut},
{0x0041304c,(void*)&StoreImage},
{0x00413054,(void*)&FUN_00413054},
{0x004130b0,(void*)&FUN_004130b0},
{0x004130f0,(void*)&FUN_004130f0},
{0x00413248,(void*)&DDRelease},
{0x004132f0,(void*)&FUN_004132f0},
{0x00413488,(void*)&FUN_00413488},
{0x00413584,(void*)&Generate_Transparency_Tables},
{0x004137c0,(void*)&MulMatrix},
{0x004137ef,(void*)&gte_MulMatrix0},
{0x0041381e,(void*)&MulMatrix2},
{0x00413920,(void*)&rsin},
{0x00413939,(void*)&rcos},
{0x00413952,(void*)&SquareRoot0_},
{0x004139be,(void*)&GTERT},
{0x004139e7,(void*)&GTERPS},
{0x00413a29,(void*)&GTERPT},
{0x00413ac5,(void*)&GTERPT4_},
{0x00413b8e,(void*)&FUN_00413b8e},
{0x00413bc6,(void*)&RotMatrix},
{0x00413d4b,(void*)&ApplyMatrixLV},
{0x00413e08,(void*)&FUN_00413e08},
{0x00413f85,(void*)&FUN_00413f85},
{0x00414016,(void*)&FUN_00414016},
{0x00414099,(void*)&FUN_00414099},
{0x00414114,(void*)&gte_dpcs},
{0x00414178,(void*)&gte_ncds},
{0x0041429c,(void*)&SetFogNearFar},
{0x004142b4,(void*)&gte_SetFogFar},
{0x004142dc,(void*)&gte_SetFogNear},
{0x004142f8,(void*)&PushMatrix},
{0x00414354,(void*)&PopMatrix},
{0x004143b0,(void*)&FUN_004143b0},
{0x00414414,(void*)&VectorNormalS},
{0x0041447c,(void*)&VectorNormalSS},
{0x004144f0,(void*)&RotMatrixYXZ},
{0x00414590,(void*)&RotMatrixX},
{0x004146c0,(void*)&RotMatrixY},
{0x004147f0,(void*)&RotMatrixZ},
{0x00414920,(void*)&FUN_00414920},
{0x004149a0,(void*)&gte_SetRotMatrix},
{0x00414a0c,(void*)&RotTrans},
{0x00414a90,(void*)&RotTransPers},
{0x00414b1c,(void*)&RotAverageNclip4},
{0x00414d00,(void*)&OuterProduct0},
{0x00414db8,(void*)&OuterProduct12},
{0x00414e80,(void*)&Play_Movie},
{0x00414f80,(void*)&Load_Null},
{0x00414f88,(void*)&Load_Textures},
{0x00415000,(void*)&sub_415000},
{0x00415044,(void*)&Load_Texture2},
{0x0041506c,(void*)&Load_Cluts},
{0x004150f8,(void*)&Add_Buffer_Load_},
{0x004151b0,(void*)&FUN_004151b0},
{0x00415344,(void*)&Read_Directory},
{0x004153a4,(void*)&File_Load},
{0x00415424,(void*)&FUN_00415424},
{0x00415454,(void*)&FUN_00415454},
{0x00415498,(void*)&FUN_00415498},
{0x00415508,(void*)&FUN_00415508},
{0x004155a0,(void*)&Decompress},
{0x004156a8,(void*)&DSLoadSoundBuffer},
{0x0041579c,(void*)&FUN_0041579c},
{0x00415800,(void*)&DSGetWaveResource},
{0x00415838,(void*)&FUN_00415838},
{0x004158e4,(void*)&FUN_004158e4},
{0x004159f8,(void*)&FUN_004159f8},
{0x00415ad8,(void*)&Sound_Init},
{0x00415ae4,(void*)&FUN_00415ae4},
{0x00415b58,(void*)&Sound_Remove},
{0x00415b98,(void*)&Master_CD_Volume},
{0x00415ba0,(void*)&FUN_00415ba0},
{0x00415bc0,(void*)&Sound_Stop},
{0x00415bdc,(void*)&Sound_Pause_},
{0x00415c18,(void*)&Sound_Restart},
{0x00415c5c,(void*)&Kill_Sound},
{0x00415d28,(void*)&Play_Sound},
{0x00415f10,(void*)&Modify_Sound},
{0x00415fb4,(void*)&FUN_00415fb4},
{0x00416014,(void*)&Unlock_Channel},
{0x0041607c,(void*)&Echo_Free},
{0x00416084,(void*)&Set_Echo_Mode},
{0x0041608c,(void*)&Set_Echo_Depth},
{0x00416094,(void*)&FUN_00416094},
{0x0041613c,(void*)&CD_Close},
{0x0041616c,(void*)&FUN_0041616c},
{0x00416174,(void*)&Read_CD_Toc_},
{0x0041617c,(void*)&FUN_0041617c},
{0x004161ec,(void*)&Start_CD_Audio},
{0x00416228,(void*)&Check_For_CD_Loop},
{0x004162b4,(void*)&FUN_004162b4},
{0x004162e4,(void*)&CD_Pause},
{0x00416314,(void*)&CD_Restart},
{0x00416358,(void*)&Sound_Timer_},
{0x00416404,(void*)&FUN_00416404},
{0x0041648c,(void*)&FUN_0041648c},
{0x004164e4,(void*)&FUN_004164e4},
{0x00416524,(void*)&FUN_00416524},
{0x004165f4,(void*)&FUN_004165f4},
{0x00416670,(void*)&CD_Check},
{0x004166c0,(void*)&Load_Sprite_Info},
{0x004166d8,(void*)&Search_For_Sprite},
{0x00416714,(void*)&FUN_00416714},
{0x0041673c,(void*)&Setup_Sprite},
{0x00416764,(void*)&FUN_00416764},
{0x00416988,(void*)&Modify_Sprite},
{0x00416a70,(void*)&FUN_00416a70},
{0x00417f5c,(void*)&draw_face_3pt_flat},
{0x00418128,(void*)&draw_face_3pt_flat_lit},
{0x004182ec,(void*)&draw_face_3pt_flat_dpq},
{0x004184b8,(void*)&draw_face_3pt_flat_dpq_lit},
{0x004186d8,(void*)&draw_face_4pt_flat},
{0x004188ec,(void*)&draw_face_4pt_flat_lit},
{0x00418b04,(void*)&draw_face_4pt_flat_dpq},
{0x00418d18,(void*)&draw_face_4pt_flat_dpq_lit},
{0x00419040,(void*)&draw_face_3pt_text},
{0x0041920c,(void*)&draw_face_3pt_text_squash},
{0x00419738,(void*)&draw_face_3pt_text_lit},
{0x00419a40,(void*)&draw_face_3pt_text_dpq},
{0x00419c24,(void*)&draw_face_3pt_text_dpq_squash},
{0x0041a16c,(void*)&draw_face_3pt_text_dpq_lit},
{0x0041a3b4,(void*)&FUN_0041a3b4},
{0x0041a4cc,(void*)&draw_face_4pt_text},
{0x0041a72c,(void*)&draw_face_4pt_text_squash},
{0x0041adec,(void*)&draw_face_4pt_text_lit},
{0x0041b14c,(void*)&draw_face_4pt_text_dpq},
{0x0041b3d0,(void*)&draw_face_4pt_text_dpq_squash},
{0x0041baac,(void*)&draw_face_4pt_text_dpq_lit},
{0x0041bdf4,(void*)&draw_face_3pt_gour},
{0x0041bfc0,(void*)&draw_face_3pt_gour_lit},
{0x0041c184,(void*)&draw_face_3pt_gour_dpq},
{0x0041c350,(void*)&draw_face_3pt_gour_dpq_lit},
{0x0041c5cc,(void*)&draw_face_4pt_gour},
{0x0041c7e0,(void*)&draw_face_4pt_gour_lit},
{0x0041c9f8,(void*)&draw_face_4pt_gour_dpq},
{0x0041cc10,(void*)&draw_face_4pt_gour_dpq_lit},
{0x0041cfbc,(void*)&draw_face_3pt_pict},
{0x0041d188,(void*)&draw_face_3pt_pict_lit},
{0x0041d490,(void*)&draw_face_3pt_pict_dpq},
{0x0041d718,(void*)&draw_face_3pt_pict_dpq_lit},
{0x0041db10,(void*)&draw_face_4pt_pict},
{0x0041dd24,(void*)&draw_face_4pt_pict_lit},
{0x0041e084,(void*)&draw_face_4pt_pict_dpq},
{0x0041e2b4,(void*)&draw_face_4pt_pict_dpq_lit},
{0x0041e548,(void*)&setup_face_sprite},
{0x0041e624,(void*)&draw_face_sprite},
{0x0041ea78,(void*)&draw_face_sprite_dpq},
{0x0041f350,(void*)&draw_face_tilt_sprite_dpq},
{0x0041f7d0,(void*)&FUN_0041f7d0},
{0x0041fcac,(void*)&FUN_0041fcac},
{0x0041fd30,(void*)&Create_Object},
{0x0041fd8c,(void*)&Set_Object},
{0x0041fddc,(void*)&Remove_Object},
{0x0041fe2c,(void*)&Pre_Rotate},
{0x0041feec,(void*)&Draw_Subdiv_Object},
{0x0041ff98,(void*)&FUN_0041ff98},
{0x00420054,(void*)&Update_Object},
{0x00420080,(void*)&FUN_00420080},
{0x0042016c,(void*)&FUN_0042016c},
{0x00420190,(void*)&FUN_00420190},
{0x004201a4,(void*)&Set_Zclip},
{0x004201bc,(void*)&Set_Clip},
{0x004201ec,(void*)&Set_World_Position},
{0x0042020c,(void*)&Set_World_Matrix},
{0x0042024c,(void*)&Set_World_View},
{0x00420398,(void*)&Set_Ambient_Light},
{0x004203ac,(void*)&Set_Depth_Cue},
{0x004203dc,(void*)&FUN_004203dc},
{0x004204d0,(void*)&FUN_004204d0},
{0x00420544,(void*)&Calc_Object_MatrixYZX},
{0x004205b8,(void*)&Calc_Object_Angles},
{0x00420668,(void*)&Calc_Object_AnglesYZX},
{0x00420708,(void*)&FUN_00420708},
{0x004208dc,(void*)&Point_Camera},
{0x00420bd0,(void*)&Set_Draw_Mode},
{0x00420c4c,(void*)&FUN_00420c4c},
{0x00420c9c,(void*)&Draw_All},
{0x00420ce8,(void*)&Draw_Tile},
{0x00420d54,(void*)&Draw_Line},
{0x00420dc0,(void*)&Allocate_OT_},
{0x00420df4,(void*)&Swap_Buffers},
{0x00420e20,(void*)&FUN_00420e20},
{0x00420e74,(void*)&Draw_Font_Poly},
{0x00420ea0,(void*)&Init_Primitive_Buffer},
{0x00420f14,(void*)&Reset_Primitive_Buffer},
{0x00420f3c,(void*)&Free_Primitive_Buffer},
{0x00420f9c,(void*)&FUN_00420f9c},
{0x00421018,(void*)&FUN_00421018},
{0x004210f0,(void*)&Allocate_Font_Buffers},
{0x004211a8,(void*)&Setup_Font},
{0x004211f4,(void*)&Duplicate_Font},
{0x004212e8,(void*)&Print_Locate},
{0x00421334,(void*)&Print_Font},
{0x0042138c,(void*)&Print_Ink},
{0x004213cc,(void*)&Print_InkRGB},
{0x00421404,(void*)&Print},
{0x004220e0,(void*)&Print_Draw},
{0x00422194,(void*)&FUN_00422194},
{0x004221fc,(void*)&FUN_004221fc},
{0x00422258,(void*)&FUN_00422258},
{0x004222b4,(void*)&FUN_004222b4},
{0x0042231c,(void*)&FUN_0042231c},
{0x00422488,(void*)&FUN_00422488},
{0x00422678,(void*)&FUN_00422678},
{0x00422a6c,(void*)&FUN_00422a6c},
{0x00422cc0,(void*)&Init_Controller_},
{0x00422cd8,(void*)&Setup_Controller},
{0x00422cf0,(void*)&Setup_Joystick},
{0x00422da4,(void*)&FUN_00422da4},
{0x00423050,(void*)&Translate_Keypress},
{0x0042317c,(void*)&FUN_0042317c},
{0x00423220,(void*)&InitCardSystem},
{0x00423340,(void*)&FUN_00423340},
{0x0042335c,(void*)&SaveCardFile},
{0x00423428,(void*)&DeleteFileMC},
{0x004234c0,(void*)&LoadCardFiles},
{0x00423594,(void*)&LoadCardFile},
{0x004235f0,(void*)&FirstSavedGame},
{0x0042360c,(void*)&InitCardBlocks},
{0x00423620,(void*)&DupFileCheck},
{0x0042366c,(void*)&FUN_0042366c},
{0x004236f0,(void*)&MPE_InitHeap},
{0x00423714,(void*)&MPE_malloc},
{0x0042377c,(void*)&MPE_free},
{0x004237f0,(void*)&Debug_Stub},
{0x004237f8,(void*)&PC_Read_File},
{0x00423874,(void*)&PC_Write_File},
{0x004238cc,(void*)&System_Error},
{0x004238f0,(void*)&FUN_004238f0},
{0x00423934,(void*)&FUN_00423934},
{0x00423ac0,(void*)&Profile_Init},
{0x00423ac8,(void*)&Profile_Start},
{0x00423ad0,(void*)&Profile_Stop},
{0x00423b28,(void*)&ddmain},
{0x00423b50,(void*)&Play_Game},
{0x00424090,(void*)&Init_Debris_},
{0x0042453c,(void*)&Setup_Debris},
{0x00424640,(void*)&Update_Debris},
{0x00424a60,(void*)&FUN_00424a60},
{0x00424b88,(void*)&Setup_Flying_Objects},
{0x00425280,(void*)&Update_Flying_Objects},
{0x004253bc,(void*)&Zero_Flying_Object},
{0x00425400,(void*)&Request_Flying_Object},
{0x00425444,(void*)&FUN_00425444},
{0x00425b18,(void*)&Flying_Objects_Pos_Ang},
{0x00425bc8,(void*)&FUN_00425bc8},
{0x00425cb8,(void*)&FUN_00425cb8},
{0x00425ea0,(void*)&InitialiseDenting},
{0x00425fa4,(void*)&FUN_00425fa4},
{0x004268b8,(void*)&Generate_Surface_Normals},
{0x00426a94,(void*)&Mask_Point_In_Quad},
{0x00426be4,(void*)&FUN_00426be4},
{0x00426ff4,(void*)&Track_Follow},
{0x00427f70,(void*)&Map_Height},
{0x00428678,(void*)&FUN_00428678},
{0x0042885c,(void*)&Move_Forward_Strip},
{0x004288f0,(void*)&FUN_004288f0},
{0x00428980,(void*)&Search_For_Strip},
{0x00428a00,(void*)&FUN_00428a00},
{0x00428a8c,(void*)&FUN_00428a8c},
{0x00429714,(void*)&FUN_00429714},
{0x004297bc,(void*)&Car_Camera},
{0x00429868,(void*)&Camera_Pad_Control},
{0x00429a68,(void*)&Init_Pit_Camera_},
{0x00429ac4,(void*)&FUN_00429ac4},
{0x00429b48,(void*)&Pit_Camera_Control},
{0x00429ff0,(void*)&Init_The_Floaty_Camera},
{0x0042a040,(void*)&Do_The_Floaty_Camera_Thing},
{0x0042a4c8,(void*)&Init_Damage_Indicator},
{0x0042a608,(void*)&FUN_0042a608},
{0x0042aae0,(void*)&Bonnet_Smoke},
{0x0042b720,(void*)&FUN_0042b720},
{0x0042c15c,(void*)&Draw_Car},
{0x0042c2d4,(void*)&FUN_0042c2d4},
{0x0042c408,(void*)&Init_Car_Graphics},
{0x0042c640,(void*)&Init_Wild_Bill},
{0x0042c670,(void*)&Init_Rollercoaster},
{0x0042c77c,(void*)&Init_CLUT_Animation_},
{0x0042c8a4,(void*)&Init_Texture_Animation},
{0x0042ca20,(void*)&Update_Texture_Animation},
{0x0042ca64,(void*)&Texture_Animation},
{0x0042cb28,(void*)&Update_CLUT_Animation},
{0x0042cb6c,(void*)&CLUT_Animation},
{0x0042cbf8,(void*)&Update_Other_Objects},
{0x0042d008,(void*)&Draw_Other_Objects},
{0x0042d598,(void*)&Draw_Dynamic_Objects},
{0x0042d600,(void*)&Init_Flag},
{0x0042d758,(void*)&DrawFlagObject},
{0x0042dac8,(void*)&UpdateFlag},
{0x0042db28,(void*)&ZoomFlag},
{0x0042dc38,(void*)&Init_LensFlare},
{0x0042ddd8,(void*)&DrawLensFlare},
{0x0042e154,(void*)&Init_Overlays},
{0x0042e8b4,(void*)&Draw_Overlays},
{0x0042fb7c,(void*)&FUN_0042fb7c},
{0x0042fda0,(void*)&Update_Race_CountDown},
{0x0042fe88,(void*)&FUN_0042fe88},
{0x004300ac,(void*)&Display_Position_Pointers},
{0x0043036c,(void*)&Init_Scene},
{0x004304bc,(void*)&VVDraw_Object},
{0x00430594,(void*)&Draw_Scene_Object},
{0x004307c8,(void*)&FUN_004307c8},
{0x004308a8,(void*)&Setup_Object_Block},
{0x004309e8,(void*)&FUN_004309e8},
{0x00430a38,(void*)&Decrunch_Object_Block},
{0x00430aec,(void*)&Object_Decompression},
{0x00430b60,(void*)&FUN_00430b60},
{0x00430bc0,(void*)&Update_Scene_Objects},
{0x00430d0e,(void*)&FUN_00430d0e},
{0x00430d94,(void*)&Init_Bowl_Objects},
{0x00430ddc,(void*)&Init_Scene_Objects},
{0x00430de8,(void*)&Init_Track_Objects},
{0x00430ed8,(void*)&Remove_Scene_Objects},
{0x00430f38,(void*)&Init_Sky},
{0x0043102c,(void*)&FUN_0043102c},
{0x00431330,(void*)&Draw_Sky},
{0x004314a4,(void*)&AI_Com_Server},
{0x00432d7c,(void*)&Determine_AI},
{0x00433088,(void*)&Recommended_Acceleration},
{0x004330c8,(void*)&FUN_004330c8},
{0x00433108,(void*)&Get_Car_Angle},
{0x00433134,(void*)&Get_Direction_Cosines},
{0x004331a4,(void*)&Interpolate_Direction_Vectors_Left},
{0x0043321c,(void*)&Interpolate_Direction_Vectors_Right_},
{0x004332b8,(void*)&InitialiseAI},
{0x0043361c,(void*)&Obstacle_Ahead},
{0x00433b80,(void*)&Strip_Distance},
{0x00433ba0,(void*)&FUN_00433ba0},
{0x00433d34,(void*)&CheckPointScoring},
{0x00433f70,(void*)&Barrier_Collision},
{0x00434c7c,(void*)&Barrier_Corner_Collision},
{0x00435300,(void*)&FUN_00435300},
{0x00435384,(void*)&FUN_00435384},
{0x00435598,(void*)&TransformWheels_},
{0x00435870,(void*)&TransformEnemyWheels},
{0x00435990,(void*)&ApplyWheelOverlay},
{0x0043617c,(void*)&InitialiseParticleSystem},
{0x00436318,(void*)&Smoke},
{0x004363e0,(void*)&SmokeCtrl},
{0x00436578,(void*)&FireCtrl},
{0x004366fc,(void*)&Fire},
{0x004367ec,(void*)&Sparks},
{0x00436934,(void*)&SparksCtrl},
{0x00436ab0,(void*)&Steam},
{0x00436b9c,(void*)&SteamCtrl},
{0x00436d34,(void*)&FUN_00436d34},
{0x00436d64,(void*)&FreeParticle},
{0x00436dd4,(void*)&DrawParticles},
{0x00436e2c,(void*)&FUN_00436e2c},
{0x00436f00,(void*)&FUN_00436f00},
{0x00437090,(void*)&Start_Roll},
{0x004371cc,(void*)&FUN_004371cc},
{0x0043732c,(void*)&FUN_0043732c},
{0x0043853c,(void*)&Calc_Head_On_Clsn_Dynamics},
{0x004398bc,(void*)&Check_2D_Car_Collision},
{0x00439b34,(void*)&Check_Ground_Car_Collision},
{0x0043a230,(void*)&Check_Space_Car_Collision},
{0x0043a530,(void*)&Do_Car_Collisions},
{0x0043a7b4,(void*)&Init_Car_Cluts},
{0x0043b064,(void*)&Init_Car_Doors},
{0x0043b39c,(void*)&FUN_0043b39c},
{0x0043b4b0,(void*)&Change_Bonnet_Clut},
{0x0043b5e8,(void*)&Change_Boot_Clut},
{0x0043b6f0,(void*)&Highlight_Area},
{0x0043b8ec,(void*)&FUN_0043b8ec},
{0x0043ba70,(void*)&TextureDentHiCar},
{0x0043c05c,(void*)&TextureDentMidCar},
{0x0043c618,(void*)&FUN_0043c618},
{0x0043c68c,(void*)&FUN_0043c68c},
{0x0043c700,(void*)&FUN_0043c700},
{0x0043c868,(void*)&FUN_0043c868},
{0x0043ca40,(void*)&Get_Corner_Positions},
{0x0043d040,(void*)&Ground_Collision},
{0x0043d2c4,(void*)&Find_Lowest_Corner},
{0x0043d320,(void*)&Make_Car_Fly},
{0x0043d548,(void*)&Car_Landed},
{0x0043d704,(void*)&Car_Landed_On_Corner},
{0x0043d7bc,(void*)&Car_Fly_Motion_3D},
{0x0043da14,(void*)&Car_Grounded_Motion_3D},
{0x0043dc00,(void*)&Car_Rolled_Edge_Onto_Wheels_},
{0x0043dd08,(void*)&FUN_0043dd08},
{0x0043de10,(void*)&Check_Quadrant},
{0x0043de30,(void*)&FUN_0043de30},
{0x0043e634,(void*)&Car_2pt_Motion_3D},
{0x0043fc54,(void*)&Car_1pt_Motion_3D},
{0x004405bc,(void*)&Car_Drive_Motion_3D_},
{0x0044088c,(void*)&Sticky_Car_Motion_3D},
{0x00440ab0,(void*)&FUN_00440ab0},
{0x00440bf4,(void*)&FUN_00440bf4},
{0x00441090,(void*)&FUN_00441090},
{0x00441224,(void*)&Calc_Car_Tilt},
{0x004413a0,(void*)&Calc_Car_Angles_Square},
{0x004414c4,(void*)&FUN_004414c4},
{0x004414dc,(void*)&Car_Drive_Motion},
{0x0044229c,(void*)&Car_Drive_2pt_Motion},
{0x004428fc,(void*)&Car_Fly_Motion},
{0x0044295c,(void*)&FUN_0044295c},
{0x00442c38,(void*)&FUN_00442c38},
{0x00442e0c,(void*)&Car_Movement},
{0x004431e8,(void*)&FUN_004431e8},
{0x00443970,(void*)&Init_End_Race},
{0x00443990,(void*)&Calc_Track_Positions},
{0x00443c40,(void*)&FUN_00443c40},
{0x00443cbc,(void*)&Get_Race_Positions},
{0x00443ed4,(void*)&Init_Track_Strip_Numbers},
{0x004440c0,(void*)&FUN_004440c0},
{0x00444178,(void*)&FUN_00444178},
{0x00444a10,(void*)&FUN_00444a10},
{0x00444b90,(void*)&FUN_00444b90},
{0x00444d4c,(void*)&Calc_Suspension_Right_Wheels},
{0x00444f0c,(void*)&Calc_Null_Suspension},
{0x00444f6c,(void*)&FUN_00444f6c},
{0x00444fcc,(void*)&Boot_Lost_Geometry},
{0x004450fc,(void*)&FUN_004450fc},
{0x00445160,(void*)&FUN_00445160},
{0x0044520c,(void*)&FUN_0044520c},
{0x00445684,(void*)&Check_Bonnet_Removal},
{0x00445710,(void*)&Check_Boot_Removal},
{0x004457a8,(void*)&Init_Sys},
{0x00445814,(void*)&Init_Main},
{0x00445894,(void*)&Set_Load_Textures},
{0x00445918,(void*)&Modify_TDF},
{0x00445ba0,(void*)&Init_Graphics},
{0x00445c70,(void*)&FUN_00445c70},
{0x00445ca8,(void*)&FUN_00445ca8},
{0x00445da4,(void*)&Init_Game},
{0x00445ef0,(void*)&Play_Intro},
{0x00445f2c,(void*)&Play_Xtro},
{0x00445f70,(void*)&Initialise_Pause_Mode},
{0x00446390,(void*)&Pause_Mode},
{0x00446d50,(void*)&FUN_00446d50},
{0x00446d9c,(void*)&UndentCar},
{0x00447120,(void*)&SetHighLight},
{0x00447300,(void*)&FUN_00447300},
{0x004474a0,(void*)&Control_Car_Replay},
{0x004478e0,(void*)&FUN_004478e0},
{0x00447940,(void*)&Record_Event},
{0x00447a2c,(void*)&Terminate_Replay},
{0x00447a60,(void*)&Terminate_Replay_Bodge},
{0x00447aa0,(void*)&FUN_00447aa0},
{0x00447bdc,(void*)&FUN_00447bdc},
{0x00447f3c,(void*)&Amplitude},
{0x00447fe8,(void*)&DopplerFrequency},
{0x0044815c,(void*)&Allocate_Sound_Effect},
{0x004481fc,(void*)&FUN_004481fc},
{0x00448310,(void*)&Load_Game_Vags},
{0x00448364,(void*)&Clear_SoundFx},
{0x00448368,(void*)&FUN_00448368},
{0x00448e58,(void*)&PitOut},
{0x00448e88,(void*)&PitIn},
{0x00448f4c,(void*)&Pit_Stop1},
{0x00448f88,(void*)&Pit_Stop2},
{0x00448f8c,(void*)&Strip_Trigger_Handler},
{0x00448ff0,(void*)&Sparking},
{0x004494e0,(void*)&LoadSave},
{0x00449818,(void*)&FUN_00449818},
{0x00449868,(void*)&FUN_00449868},
{0x00449a68,(void*)&FUN_00449a68},
{0x00449a98,(void*)&FUN_00449a98},
{0x00449dc4,(void*)&FUN_00449dc4},
{0x00449f48,(void*)&FUN_00449f48},
{0x0044a050,(void*)&FUN_0044a050},
{0x0044a314,(void*)&FUN_0044a314},
{0x0044a420,(void*)&FUN_0044a420},
{0x0044a49c,(void*)&FUN_0044a49c},
{0x0044a510,(void*)&FUN_0044a510},
{0x0044a654,(void*)&FUN_0044a654},
{0x0044aa74,(void*)&FUN_0044aa74},
{0x0044ab98,(void*)&FUN_0044ab98},
{0x0044ac60,(void*)&FUN_0044ac60},
{0x0044ad74,(void*)&Duplicate_Results},
{0x0044ad98,(void*)&Load_First_Config},
{0x0044add8,(void*)&Load_Card_File},
{0x0044ae0c,(void*)&FUN_0044ae0c},
{0x0044b038,(void*)&FUN_0044b038},
{0x0044b2b8,(void*)&Init_Front_End},
{0x0044b62c,(void*)&View_Frontend_Replay},
{0x0044b650,(void*)&DemoMode},
{0x0044b730,(void*)&FUN_0044b730},
{0x0044b744,(void*)&Loading_Screen_From_Slab},
{0x0044b7f4,(void*)&FUN_0044b7f4},
{0x0044b824,(void*)&Load_Completion_Status},
{0x0044b978,(void*)&Run_Selection},
{0x0044bab4,(void*)&Call_Loaded_Game},
{0x0044baec,(void*)&FUN_0044baec},
{0x0044bb3c,(void*)&FUN_0044bb3c},
{0x0044bbe0,(void*)&Setup_Pad},
{0x0044bcc0,(void*)&Init_Wrecking_Championship},
{0x0044bd04,(void*)&Init_StockCar_Championship},
{0x0044bd48,(void*)&FUN_0044bd48},
{0x0044bd88,(void*)&Init_StockCar_MultiChamp},
{0x0044bdc8,(void*)&Championship},
{0x0044bec8,(void*)&MultiChamp},
{0x0044c018,(void*)&Calculate_Finish},
{0x0044c250,(void*)&Calculate_Results},
{0x0044c378,(void*)&End_Of_Game_Sequence},
{0x0044c394,(void*)&Do_End_Of_Season_Stuff},
{0x0044c430,(void*)&Init_League_Info},
{0x0044c498,(void*)&Reset_League_Info},
{0x0044c4b0,(void*)&Sort_Leagues},
{0x0044c504,(void*)&Init_MultiLeague_Info},
{0x0044c550,(void*)&Setup_Driver_Names},
{0x0044c5d8,(void*)&FUN_0044c5d8},
{0x0044c64c,(void*)&Add_Computer_Info},
{0x0044c6c8,(void*)&Update_League_Info},
{0x0044c6f8,(void*)&FUN_0044c6f8},
{0x0044c768,(void*)&Sort_MultiLeague},
{0x0044c7a0,(void*)&Sort_RacePos},
{0x0044c884,(void*)&Promote_And_Relegate},
{0x0044c92c,(void*)&Check_League_Standing},
{0x0044c970,(void*)&Order_Cars},
{0x0044c9c0,(void*)&FUN_0044c9c0},
{0x0044caa8,(void*)&FUN_0044caa8},
{0x0044ccd0,(void*)&View_Results_Replay_},
{0x0044cd08,(void*)&FUN_0044cd08},
{0x0044cd30,(void*)&View_Champ_Stats},
{0x0044ce78,(void*)&FUN_0044ce78},
{0x0044cf20,(void*)&View_Driver_Stats},
{0x0044d094,(void*)&FUN_0044d094},
{0x0044d0b0,(void*)&FUN_0044d0b0},
{0x0044d2a4,(void*)&FUN_0044d2a4},
{0x0044d550,(void*)&Start_New_Season_Stats},
{0x0044d594,(void*)&FUN_0044d594},
{0x0044d638,(void*)&Update_Track_Stats},
{0x0044d6f8,(void*)&FUN_0044d6f8},
{0x0044d798,(void*)&Update_Championship_Stats},
{0x0044d7f0,(void*)&FUN_0044d7f0},
{0x0044d808,(void*)&Get_Current_Recording_Season},
{0x0044d810,(void*)&FUN_0044d810},
{0x0044d85c,(void*)&Update_Jimmy_Spunk_Times},
{0x0044d9d0,(void*)&View_Track_Stats},
{0x0044db34,(void*)&FUN_0044db34},
{0x0044dd30,(void*)&FUN_0044dd30},
{0x0044debc,(void*)&Secret},
{0x0044e308,(void*)&FUN_0044e308},
{0x0044e330,(void*)&FUN_0044e330},
{0x0044e3fc,(void*)&Select_Car},
{0x0044e600,(void*)&FUN_0044e600},
{0x0044e7d8,(void*)&FUN_0044e7d8},
{0x0044e860,(void*)&FUN_0044e860},
{0x0044e950,(void*)&Select_ScreenPos},
{0x0044eae0,(void*)&Configuration},
{0x0044ecdc,(void*)&FUN_0044ecdc},
{0x0044ed50,(void*)&View_Credits},
{0x0044ee6c,(void*)&FUN_0044ee6c},
{0x0044f788,(void*)&FUN_0044f788},
{0x0044f81c,(void*)&FUN_0044f81c},
{0x0044f870,(void*)&FUN_0044f870},
{0x0044f9d4,(void*)&FUN_0044f9d4},
{0x0044fd80,(void*)&FUN_0044fd80},
{0x0044fe64,(void*)&FUN_0044fe64},
{0x0044fea8,(void*)&FUN_0044fea8},
{0x0044ff74,(void*)&FUN_0044ff74},
{0x0044ffc0,(void*)&View_BestLaps},
{0x00450108,(void*)&FUN_00450108},
{0x004502a8,(void*)&Front_End},
{0x00450800,(void*)&FUN_00450800},
{0x00450a7c,(void*)&Toggle_Track},
{0x00450b04,(void*)&Toggle_Car},
{0x00450b5c,(void*)&FUN_00450b5c},
{0x00450e3c,(void*)&FUN_00450e3c},
{0x004507f8,(void*)&FUN_004507f8},{0x004509d8,(void*)&FUN_004509d8},
{0x0045089c,(void*)&FUN_0045089c},{0x00450840,(void*)&FUN_00450840},
{0x004508d4,(void*)&FUN_004508d4},{0x00450904,(void*)&FUN_00450904},{0x00450934,(void*)&FUN_00450934},
{0x00450964,(void*)&FUN_00450964},{0x00450994,(void*)&FUN_00450994},
{0x00450ecc,(void*)&Rotate_Slab_On},
{0x00450fe0,(void*)&FUN_00450fe0},
{0x00451094,(void*)&Draw_Screen_Polys},
{0x00451448,(void*)&Setup_Screen_Text},
{0x004514b8,(void*)&Setup_Screen_Lines},
{0x004514c8,(void*)&Draw_Screen_Lines},
{0x0045186c,(void*)&Button_Pressed},
{0x00451900,(void*)&Glow_Selector},
{0x00451978,(void*)&Draw_Slab},
{0x00451a68,(void*)&Null_Routine},
{0x00451a6c,(void*)&FUN_00451a6c},
{0x00451ab8,(void*)&Draw_Semi_Trans_Poly},
{0x00451c40,(void*)&FUN_00451c40},
{0x00451c60,(void*)&Play_Click_FX},
{0x00451c80,(void*)&FUN_00451c80},
{0x00451ca0,(void*)&FUN_00451ca0},
{0x00451cc0,(void*)&Show_Information},
{0x00451f0c,(void*)&FUN_00451f0c},
{0x00451f2c,(void*)&FUN_00451f2c},
{0x0045219c,(void*)&FUN_0045219c},
{0x004521b0,(void*)&FUN_004521b0},
{0x004521ec,(void*)&FUN_004521ec},
{0x00452250,(void*)&FUN_00452250},
{0x004522b0,(void*)&Enter_Driver_Names},
{0x00452760,(void*)&FUN_00452760},
{0x00452950,(void*)&FUN_00452950},
{0x004529b0,(void*)&FUN_004529b0},
{0x00452b94,(void*)&FUN_00452b94},
{0x00452bf4,(void*)&FUN_00452bf4},
{0x00452bb4,(void*)&FUN_00452bb4},
{0x00452bd4,(void*)&FUN_00452bd4},
{0x00452c0c,(void*)&FUN_00452c0c},
{0x00452c24,(void*)&FUN_00452c24},
{0x00452c40,(void*)&Practice_Over},
{0x00452ef0,(void*)&FUN_00452ef0},
{0x00452f60,(void*)&FUN_00452f60},
{0x00452f80,(void*)&FUN_00452f80},
{0x00453164,(void*)&FUN_00453164},
{0x004531a4,(void*)&FUN_004531a4},
{0x004531c4,(void*)&FUN_004531c4},
{0x00453184,(void*)&FUN_00453184},
{0x00453204,(void*)&Select_Champ},
{0x00453240,(void*)&Select_Multi},
{0x0045327c,(void*)&Select_ChampQS},
{0x004532c0,(void*)&Select_Pract},
{0x004532fc,(void*)&Select_TimeT},
{0x00453330,(void*)&Select_Total},
{0x00453370,(void*)&Select_DDPract},
{0x004533b0,(void*)&Select_RaceType_DD},
{0x004535c0,(void*)&Select_Track},
{0x00453740,(void*)&FUN_00453740},
{0x00453780,(void*)&FUN_00453780},
{0x004538b0,(void*)&Save_Game},
{0x00453b90,(void*)&FUN_00453b90},
{0x00453bb0,(void*)&FUN_00453bb0},
{0x00453bd8,(void*)&FUN_00453bd8},
{0x00453be0,(void*)&FUN_00453be0},
{0x00453d58,(void*)&FUN_00453d58},
{0x00453f08,(void*)&End_Of_Season},
{0x00454318,(void*)&FUN_00454318},
{0x004543a8,(void*)&FUN_004543a8},
{0x00454438,(void*)&FUN_00454438},
{0x004544c8,(void*)&FUN_004544c8},
{0x00454558,(void*)&FUN_00454558},
{0x00454710,(void*)&FUN_00454710},
{0x004549c4,(void*)&FUN_004549c4},
{0x00454b80,(void*)&View_MultiLeague},
{0x00454c30,(void*)&FUN_00454c30},
{0x00454d28,(void*)&Race_Over},
{0x0045505c,(void*)&FUN_0045505c},
{0x004550cc,(void*)&FUN_004550cc},
{0x00455370,(void*)&FUN_00455370},
{0x00455420,(void*)&FUN_00455420},
{0x00455518,(void*)&Display_Season_Status},
{0x004558ec,(void*)&FUN_004558ec},
{0x00455b94,(void*)&FUN_00455b94},
{0x00455ed5,(void*)&__open_flags},
{0x00455fa4,(void*)&FUN_00455fa4},
{0x0045609b,(void*)&FUN_0045609b},
{0x004560fb,(void*)&FUN_004560fb},
{0x00456170,(void*)&FUN_00456170},
{0x004561f4,(void*)&FUN_004561f4},
{0x004565e8,(void*)&__shutdown_stream},
{0x0045660e,(void*)&FUN_0045660e},
{0x0045661e,(void*)&FUN_0045661e},
{0x0045681c,(void*)&__CHP},
{0x00456848,(void*)&nfree},
{0x00456bf2,(void*)&__null_int23_exit},
{0x00456c0d,(void*)&_exit},
{0x00456c34,(void*)&wstart2_},
{0x00456cb2,(void*)&FUN_00456cb2},
{0x00456cf0,(void*)&FUN_00456cf0},
{0x00456d27,(void*)&FUN_00456d27},
{0x00456db3,(void*)&FUN_00456db3},
{0x00456e60,(void*)&_tolower},
{0x00456e6e,(void*)&FUN_00456e6e},
{0x00456e80,(void*)&__set_EDOM},
{0x00456e8b,(void*)&__set_ERANGE},
{0x00456e9f,(void*)&FUN_00456e9f},
{0x00456ead,(void*)&__set_doserrno},
{0x00456ebb,(void*)&open},
{0x00456edd,(void*)&sopen},
{0x004570b2,(void*)&__allocfp},
{0x00457167,(void*)&__freefp},
{0x0045719e,(void*)&__purgefp},
{0x004571bc,(void*)&__chktty},
{0x004571ef,(void*)&__threadid},
{0x004571fb,(void*)&FUN_004571fb},
{0x00457200,(void*)&FUN_00457200},
{0x00457201,(void*)&FUN_00457201},
{0x0045720f,(void*)&FUN_0045720f},
{0x0045721d,(void*)&__NTInit},
{0x00457340,(void*)&__NTMainInit},
{0x00457383,(void*)&__exit},
{0x004573a4,(void*)&FUN_004573a4},
{0x0045749e,(void*)&_lseek},
{0x004574b8,(void*)&FUN_004574b8},
{0x00457506,(void*)&tell},
{0x00457548,(void*)&__ioalloc},
{0x004575c8,(void*)&FUN_004575c8},
{0x00457731,(void*)&FUN_00457731},
{0x004577ec,(void*)&getpid},
{0x004577f1,(void*)&FUN_004577f1},
{0x00457885,(void*)&FUN_00457885},
{0x004578af,(void*)&__init_8087_},
{0x004578de,(void*)&_fpreset},
{0x0045793d,(void*)&nmalloc},
{0x00457a3e,(void*)&__MemAllocator},
{0x00457ae6,(void*)&__MemFree},
{0x00457ef9,(void*)&FUN_00457ef9},
{0x00458044,(void*)&FUN_00458044},
{0x004580a9,(void*)&FUN_004580a9},
{0x004580cf,(void*)&FUN_004580cf},
{0x00458100,(void*)&FUN_00458100},
{0x0045815f,(void*)&FUN_0045815f},
{0x0045825c,(void*)&FUN_0045825c},
{0x00458277,(void*)&FUN_00458277},
{0x004587a8,(void*)&FUN_004587a8},
{0x004587c8,(void*)&__qwrite},
{0x00458980,(void*)&__WinMain},
{0x00458a5e,(void*)&__NTAtMaxFiles},
{0x00458b24,(void*)&FUN_00458b24},
{0x00458bcb,(void*)&__NTRemoveFileHandle},
{0x00458bf1,(void*)&FUN_00458bf1},
{0x00458c30,(void*)&__NTGetFakeHandle},
{0x00458c7e,(void*)&__GetNTAccessAttr},
{0x00458cb9,(void*)&__GetNTShareAttr},
{0x00458cf0,(void*)&_stricmp},
{0x00458d05,(void*)&FUN_00458d05},
{0x00458d50,(void*)&_dosret0},
{0x00458d6a,(void*)&dosretax},
{0x00458d87,(void*)&FUN_00458d87},
{0x00458dd9,(void*)&__set_errno_nt},
{0x00458de8,(void*)&isatty},
{0x00458e2f,(void*)&__IOMode},
{0x00458e85,(void*)&FUN_00458e85},
{0x00458ea6,(void*)&__sigfpe_handler},
{0x00458ef1,(void*)&signal},
{0x00458f70,(void*)&raise},
{0x00458fef,(void*)&FUN_00458fef},
{0x0045901c,(void*)&__SigFini},
{0x004593e8,(void*)&__NewExceptionHandler},
{0x00459428,(void*)&__DoneExceptionHandler},
{0x00459447,(void*)&__CloseSemaphore},
{0x00459471,(void*)&__AccessSemaphore},
{0x004594d4,(void*)&__ReleaseSemaphore},
{0x004595d0,(void*)&__InitThreadData},
{0x0045960e,(void*)&__NTThreadInit},
{0x00459656,(void*)&__NTAddThread},
{0x004596ad,(void*)&__NTRemoveThread},
{0x00459721,(void*)&__InitMultipleThread},
{0x00459916,(void*)&__InitRtns},
{0x00459961,(void*)&__FiniRtns},
{0x00459a25,(void*)&__full_io_exit},
{0x00459a3f,(void*)&FUN_00459a3f},
{0x00459a9f,(void*)&flushall},
{0x00459aaa,(void*)&__flushall},
{0x00459aed,(void*)&getche},
{0x00459b13,(void*)&unlink},
{0x00459b28,(void*)&FUN_00459b28},
{0x00459c48,(void*)&FUN_00459c48},
{0x00459cad,(void*)&__cnvs2d},
{0x00459cd9,(void*)&FUN_00459cd9},
{0x00459cea,(void*)&__init_80x87},
{0x00459d11,(void*)&FUN_00459d11},
{0x00459d85,(void*)&FUN_00459d85},
{0x00459e15,(void*)&__ExpandDGROUP},
{0x00459e28,(void*)&FUN_00459e28},
{0x00459e6f,(void*)&__nmemneed},
{0x00459e97,(void*)&utoa},
{0x00459ee9,(void*)&_itoa},
{0x00459f03,(void*)&itoa},
{0x00459f5d,(void*)&ultoa},
{0x00459fad,(void*)&ltoa},
{0x00459fe2,(void*)&_ltoa},
{0x00459ffc,(void*)&_toupper},
{0x0045a00a,(void*)&FUN_0045a00a},
{0x0045a02e,(void*)&FUN_0045a02e},
{0x0045a05d,(void*)&__CommonInit},
{0x0045a068,(void*)&FUN_0045a068},
{0x0045a07d,(void*)&nrealloc},
{0x0045a117,(void*)&FUN_0045a117},
{0x0045a180,(void*)&FUN_0045a180},
{0x0045a1b5,(void*)&FUN_0045a1b5},
{0x0045a221,(void*)&__RemoveThreadData},
{0x0045a2cb,(void*)&FUN_0045a2cb},
{0x0045a306,(void*)&__fatal_runtime_error},
{0x0045a334,(void*)&FUN_0045a334},
{0x0045a528,(void*)&__HasLeadingZero},
{0x0045a570,(void*)&FUN_0045a570},
{0x0045a613,(void*)&FUN_0045a613},
{0x0045a686,(void*)&FUN_0045a686},
{0x0045a6fc,(void*)&_FtoS},
{0x0045aba5,(void*)&_nheapshrink},
{0x0045abb1,(void*)&FUN_0045abb1},
{0x0045ac0a,(void*)&FUN_0045ac0a},
{0x0045ac5f,(void*)&FUN_0045ac5f},
{0x0045ac6c,(void*)&FUN_0045ac6c},
{0x0045ac81,(void*)&__HeapManager_expand},
{0x0045ae2c,(void*)&nexpand},
{0x0045ae76,(void*)&FUN_0045ae76},
{0x0045aea2,(void*)&__initthread},
{0x0045aefc,(void*)&__EnterWVIDEO},
{0x0045af27,(void*)&FUN_0045af27},
{0x0045af54,(void*)&FUN_0045af54},
{0x0045afc1,(void*)&__NTConsoleInput},
{0x0045afcc,(void*)&FUN_0045afcc},
{0x0045afd7,(void*)&__Nan_Inf},
{0x0045b06a,(void*)&FUN_0045b06a},
{0x0045b0b4,(void*)&IF_DLOG2},
{0x0045b0b8,(void*)&IF_DLOG10},
{0x0045b0cf,(void*)&log10},
{0x0045b0e2,(void*)&log2},
{0x0045b0f5,(void*)&floor},
{0x0045b13a,(void*)&_Scale},
{0x0045b54e,(void*)&__ZBuf2F},
{0x0045b599,(void*)&FUN_0045b599},
{0x0045b68a,(void*)&__CBeginThread},
{0x0045b76e,(void*)&FUN_0045b76e},
{0x0045b794,(void*)&FUN_0045b794},
{0x0045b7ee,(void*)&modf},
{0x0045b80e,(void*)&__CmpBigInt_},
{0x0045b848,(void*)&FUN_0045b848},
{0x0045b8b3,(void*)&FUN_0045b8b3},
{0x0045b91d,(void*)&FUN_0045b91d},
{0x0045b9d2,(void*)&FUN_0045b9d2},
{0x0045b9d4,(void*)&FUN_0045b9d4},
{0x0045bf9a,(void*)&FUN_0045bf9a},
{0x0045c058,(void*)&FUN_0045c058},
{0x0045c0f0,(void*)&frexp},
{0x0045c16b,(void*)&__math1err},
{0x0045c2f1,(void*)&_set_matherr},
{0x0045c2fb,(void*)&__rterrmsg},
{0x0045c348,(void*)&_matherr},
{0x0045c39c,(void*)&__matherr},
{0x0045c3a1,(void*)&__get_std_stream},
{0x0045c46b,(void*)&FUN_0045c46b},
{0x00417f00,(void*)&FUN_00417f00},
{0x0041867c,(void*)&FUN_0041867c},
{0x00418f30,(void*)&FUN_00418f30},
{0x0041bd3c,(void*)&FUN_0041bd3c},
{0x0041bd98,(void*)&FUN_0041bd98},
{0x0041c514,(void*)&FUN_0041c514},
{0x0041c570,(void*)&FUN_0041c570},
{0x0041ce28,(void*)&FUN_0041ce28},
{0x0041cf00,(void*)&FUN_0041cf00},
{0x0041d964,(void*)&FUN_0041d964},
{0x0041da48,(void*)&FUN_0041da48},
};
int dd2_fnmap_n=sizeof(dd2_fnmap)/sizeof(dd2_fnmap[0]);
static void* g_lut[0x50000];
void dd2_relocate(void){
  int i; unsigned off;
  for(i=0;i<dd2_fnmap_n;i++){unsigned v=dd2_fnmap[i].va; if(v>=0x410000&&v<0x460000) g_lut[v-0x410000]=dd2_fnmap[i].fn;}
  for(off=0x60000; off+4<=0x578800; off+=4){
    volatile unsigned* p = (volatile unsigned*)(g_image+off);
    unsigned w=*p;
    if(w>=0x410000&&w<0x460000){void*fn=g_lut[w-0x410000]; if(fn)*(void* volatile*)p=fn;}
  }
}
