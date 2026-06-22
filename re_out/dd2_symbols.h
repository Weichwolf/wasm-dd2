#ifndef DD2_SYMBOLS_H
#define DD2_SYMBOLS_H
#include "ghidra_compat.h"
extern unsigned char* g_image;
#define GIMG(va) (g_image + ((va)-0x400000))
#define AI_CommandListDEAD (*(int*)GIMG(0x46597c))
#define AI_CommandListGeneral_Steer_Left (*(int*)GIMG(0x4659b0))
#define AI_CommandListGeneral_Steer_Right (*(int*)GIMG(0x465998))
#define AI_CommandListPanic_Swerve_Right (*(int*)GIMG(0x4659f8))
#define AI_CommandListReDetermine (*(int*)GIMG(0x465990))
#define AI_CommandListSharp_Reverse_Left (*(int*)GIMG(0x4659e0))
#define AI_CommandListSharp_Reverse_Right (*(int*)GIMG(0x4659c8))
#define Anim (*(int*)GIMG(0x7402e0))
#define Arctan_Table (*(int*)GIMG(0x463054))
#define Attribute (*(int*)GIMG(0x7596d4))
#define BackPoly (*(int*)GIMG(0x9077e0))
#define BaseHandicaps (*(int*)GIMG(0x465a70))
#define CD_Audio_Player" (*(int*)GIMG(0x451db0))
#define CLUT_Anim_Casino (*(int*)GIMG(0x46518c))
#define CLUT_Anim_Dollar (*(int*)GIMG(0x465160))
#define CLUT_Anim_Safe (*(int*)GIMG(0x465174))
#define CLUT_Anim_Twist1 (*(int*)GIMG(0x4651d0))
#define CLUT_Anim_Twist2 (*(int*)GIMG(0x4651e4))
#define COSE (*(int*)GIMG(0x4604c8))
#define Calc_Object_AnglesYZX (*(int*)GIMG(0x420538))
#define Car_Friction (*(int*)GIMG(0x465a20))
#define Caravan_Track_Type (*(int*)GIMG(0x465d70))
#define Check_Quadrant (*(int*)GIMG(0x43dce0))
#define Clear_SoundFx (*(int*)GIMG(0x448224))
#define Collision_Counter (*(int*)GIMG(0x467068))
#define Configuration (*(int*)GIMG(0x44e920))
#define Debris (*(int*)GIMG(0x73c4e0))
#define Done (*(int*)GIMG(0x467070))
#define Draw_Line (*(int*)GIMG(0x420c24))
#define DummyPoly (*(int*)GIMG(0x74a6d0))
#define Duplicate_Results (*(int*)GIMG(0x44ac04))
#define Echo_Free (*(int*)GIMG(0x41602c))
#define End_Of_Game_Sequence (*(int*)GIMG(0x44c1b8))
#define FXPage (*(int*)GIMG(0x900ed0))
#define FaceArray (*(int*)GIMG(0x4685e4))
#define FacePolys (*(int*)GIMG(0x907880))
#define File_Func_List (*(int*)GIMG(0x462ce4))
#define FireCtrl (*(int*)GIMG(0x436448))
#define FireFrames (*(int*)GIMG(0x4665c0))
#define FirstTime (*(int*)GIMG(0x900eb0))
#define FlagPolys (*(int*)GIMG(0x749b90))
#define Flying_Objects (*(int*)GIMG(0x7432e0))
#define Forest_Track_Type (*(int*)GIMG(0x465da0))
#define Forest_Wheel_Locking_Speed (*(int*)GIMG(0x465d40))
#define Free_Primitive_Buffer (*(int*)GIMG(0x420e0c))
#define GTEFLAG" (*(int*)GIMG(0x414070))
#define Glow_Selector (*(int*)GIMG(0x451740))
#define IF@LOG (*(int*)GIMG(0x45aea8))
#define IF@LOG10 (*(int*)GIMG(0x45aef8))
#define Init_Bowl_Objects (*(int*)GIMG(0x430c64))
#define Init_Sys (*(int*)GIMG(0x445678))
#define Init_Track_Objects (*(int*)GIMG(0x430cb8))
#define Lap_Timer (*(int*)GIMG(0x75d9dc))
#define Last_Lap_Timer (*(int*)GIMG(0x75d9e8))
#define Level1_Personality (*(int*)GIMG(0x465b10))
#define Level2_Personality (*(int*)GIMG(0x465b60))
#define Level3_Personality (*(int*)GIMG(0x465bb0))
#define Level4_Personality (*(int*)GIMG(0x465c00))
#define Liberty_Track_Type (*(int*)GIMG(0x465da8))
#define LoadImageClut (*(int*)GIMG(0x413004))
#define Load_Graphics (*(int*)GIMG(0x445764))
#define Load_Null (*(int*)GIMG(0x414f30))
#define Load_Texture2 (*(int*)GIMG(0x414ff4))
#define Master_CD_Volume (*(int*)GIMG(0x415b48))
#define MoveImage (*(int*)GIMG(0x412edc))
#define Movie_Playing (*(int*)GIMG(0x462cd4))
#define MulMatrix (*(int*)GIMG(0x413780))
#define NameArray (*(int*)GIMG(0x468624))
#define NamePolys (*(int*)GIMG(0x9078d0))
#define Now_Timing_Lap (*(int*)GIMG(0x75d9d4))
#define Null_Routine (*(int*)GIMG(0x4518a8))
#define Object_Decompression (*(int*)GIMG(0x4309bc))
#define Old_Cam_Mode (*(int*)GIMG(0x464a68))
#define OuterProduct0 (*(int*)GIMG(0x414cb0))
#define OverPoly (*(int*)GIMG(0x74a6f0))
#define PC_Read_File (*(int*)GIMG(0x4236c8))
#define PIT_DONE (*(int*)GIMG(0x467058))
#define PIT_IN (*(int*)GIMG(0x46704c))
#define PIT_STOP (*(int*)GIMG(0x467054))
#define ParticleAvailabilityList (*(int*)GIMG(0x752058))
#define ParticlePositionList (*(int*)GIMG(0x751f90))
#define Pause (*(int*)GIMG(0x4686c4))
#define PitIn (*(int*)GIMG(0x448d48))
#define PitOut (*(int*)GIMG(0x448d18))
#define Pit_Stop1 (*(int*)GIMG(0x448e0c))
#define Pit_Stop2 (*(int*)GIMG(0x448e48))
#define Pit_Timer (*(int*)GIMG(0x46705c))
#define Pit_Timing_Delay (*(int*)GIMG(0x4653c4))
#define PointyBits (*(int*)GIMG(0x751e70))
#define Profile_Start (*(int*)GIMG(0x423998))
#define Profile_Stop (*(int*)GIMG(0x4239a0))
#define RacePoly (*(int*)GIMG(0x74a9b0))
#define Race_Personality (*(int*)GIMG(0x465c50))
#define Replay_Action_Repeat (*(int*)GIMG(0x46707c))
#define Replay_Invalid (*(int*)GIMG(0x900ec0))
#define Replay_Level (*(int*)GIMG(0x900ebc))
#define Replay_Script (*(int*)GIMG(0x8ff2b0))
#define Replay_Script_Ptr (*(int*)GIMG(0x900eb4))
#define Resultant_Direction" (*(int*)GIMG(0x433038))
#define RotAverageNclip4 (*(int*)GIMG(0x414acc))
#define RotMatrix (*(int*)GIMG(0x413b86))
#define Run_Selection (*(int*)GIMG(0x44b7fc))
#define SCA_Corner_Data" (*(int*)GIMG(0x465cf8))
#define SCA_Data (*(int*)GIMG(0x466da0))
#define SCA_Track_Type (*(int*)GIMG(0x465db8))
#define Select_AudioVol& (*(int*)GIMG(0x44df60))
#define Select_Car (*(int*)GIMG(0x44e23c))
#define Select_Multi (*(int*)GIMG(0x453080))
#define Select_RaceType_DD (*(int*)GIMG(0x4531f0))
#define Select_ScreenPos (*(int*)GIMG(0x44e790))
#define Select_Track (*(int*)GIMG(0x453400))
#define Set_Clip (*(int*)GIMG(0x42008c))
#define Set_Echo_Depth (*(int*)GIMG(0x41603c))
#define Set_Echo_Mode (*(int*)GIMG(0x416034))
#define Show_Information (*(int*)GIMG(0x451b00))
#define SmokeCtrl (*(int*)GIMG(0x4362b0))
#define Smoke_Animation1 (*(int*)GIMG(0x466294))
#define SparksCtrl (*(int*)GIMG(0x436804))
#define Speedway_Corner_Data (*(int*)GIMG(0x465ca0))
#define Speedway_Track_Type" (*(int*)GIMG(0x465d60))
#define Speedway_Wheel_Locking_Speed (*(int*)GIMG(0x465d00))
#define SpinTable (*(int*)GIMG(0x465e90))
#define SteamCtrl (*(int*)GIMG(0x436a6c))
#define Sticky_Car_Motion_3D (*(int*)GIMG(0x44075c))
#define StoreImage (*(int*)GIMG(0x41300c))
#define Street_Lighting" (*(int*)GIMG(0x42b09c))
#define Stunt_Corner_Data (*(int*)GIMG(0x465cf0))
#define Stunt_Wheel_Locking_Speed (*(int*)GIMG(0x465d50))
#define Template (*(int*)GIMG(0x73c308))
#define Timing_Delay (*(int*)GIMG(0x75d9f8))
#define TopHandicaps (*(int*)GIMG(0x465ac0))
#define Track_Records (*(int*)GIMG(0x466e3c))
#define Ultimate_Wheel_Locking_Speed (*(int*)GIMG(0x465d28))
#define Update_CLUT_Animation (*(int*)GIMG(0x42c9f8))
#define Update_Texture_Animation (*(int*)GIMG(0x42c8f0))
#define View_Champ_Stats (*(int*)GIMG(0x44cb70))
#define View_Credits (*(int*)GIMG(0x44eb90))
#define View_Driver_Stats (*(int*)GIMG(0x44cd60))
#define View_MultiLeague (*(int*)GIMG(0x4549c0))
#define View_Track_Stats (*(int*)GIMG(0x44d810))
#define ZValue (*(int*)GIMG(0x465250))
#define Z_DISTANCE (*(int*)GIMG(0x4604c2))
#define Zoom (*(int*)GIMG(0x46524c))
#define ZoomFlag (*(int*)GIMG(0x42d9f8))
#define _AccessFHeap (*(int*)GIMG(0x46c34c))
#define _AccessFList (*(int*)GIMG(0x46c360))
#define _AccessFileH (*(int*)GIMG(0x46c330))
#define _AccessIOB (*(int*)GIMG(0x46c340))
#define _AccessNHeap (*(int*)GIMG(0x46c348))
#define _AccessTDList (*(int*)GIMG(0x46c358))
#define _BigPow10Table (*(int*)GIMG(0x46fedc))
#define _Extender (*(int*)GIMG(0x46c3ee))
#define _ExtenderSubtype (*(int*)GIMG(0x46c3ef))
#define _FiniAccessH (*(int*)GIMG(0x46c33c))
#define _HugeValue (*(int*)GIMG(0x46fed4))
#define _InitAccessH (*(int*)GIMG(0x46c338))
#define _IsTable (*(int*)GIMG(0x46fdb0))
#define _LpCmdLine (*(int*)GIMG(0x46c3c0))
#define _LpDllName (*(int*)GIMG(0x46c3c8))
#define _LpPgmName (*(int*)GIMG(0x46c3c4))
#define _ReleaseFHeap (*(int*)GIMG(0x46c354))
#define _ReleaseFList (*(int*)GIMG(0x46c364))
#define _ReleaseFileH (*(int*)GIMG(0x46c334))
#define _ReleaseNHeap (*(int*)GIMG(0x46c350))
#define _ReleaseTDList (*(int*)GIMG(0x46c35c))
#define _STACKLOW (*(int*)GIMG(0x46c3d0))
#define _STACKTOP (*(int*)GIMG(0x46c3d4))
#define _SetMaxPrec" (*(int*)GIMG(0x45a3a6))
#define _ThreadExitRtn (*(int*)GIMG(0x46c368))
#define _WinMainProc (*(int*)GIMG(0x908588))
#define _WindowExitRtn (*(code**)GIMG(0x46c3b4))
#define _WindowsDestroyOnClose (*(int*)GIMG(0x46c390))
#define _WindowsGetch (*(code**)GIMG(0x46c3a8))
#define _WindowsGetche (*(int*)GIMG(0x46c3ac))
#define _WindowsIsWindowedHandle (*(code**)GIMG(0x46c374))
#define _WindowsKbhit (*(int*)GIMG(0x46c3a4))
#define _WindowsNewWindow (*(code**)GIMG(0x46c37c))
#define _WindowsPutch (*(code**)GIMG(0x46c3b0))
#define _WindowsRemoveWindowedHandle (*(code**)GIMG(0x46c378))
#define _WindowsSetAbout (*(int*)GIMG(0x46c384))
#define _WindowsShutDown (*(int*)GIMG(0x46c398))
#define _WindowsStdin (*(code**)GIMG(0x46c39c))
#define _WindowsStdout (*(code**)GIMG(0x46c3a0))
#define _WindowsYieldControl (*(int*)GIMG(0x46c394))
#define __8087 (*(int*)GIMG(0x46c104))
#define __8087cw (*(int*)GIMG(0x46c504))
#define __ASTACKPTR& (*(int*)GIMG(0x46c3dc))
#define __AccessFHeap (*(int*)GIMG(0x4593bb))
#define __AccessFList (*(int*)GIMG(0x4593e0))
#define __AccessFileH (*(int*)GIMG(0x459355))
#define __AccessNHeap (*(int*)GIMG(0x4593ad))
#define __AccessSema4 (*(int*)GIMG(0x46c4e8))
#define __AccessSema4Fini (*(int*)GIMG(0x46ff66))
#define __AccessTDList (*(int*)GIMG(0x4593cc))
#define __EFG_printf (*(int*)GIMG(0x46c4f4))
#define __ExceptionHandled (*(int*)GIMG(0x908594))
#define __FPE_handler (*(int*)GIMG(0x46c3fb))
#define __FPE_handler_exit (*(int*)GIMG(0x46c110))
#define __FiniAccessH (*(int*)GIMG(0x459387))
#define __FiniThreadProcessing (*(int*)GIMG(0x45a106))
#define __FirstThreadData (*(int*)GIMG(0x90857c))
#define __FreeThreadDataList (*(int*)GIMG(0x45a0b3))
#define __GetNTCreateAttr (*(int*)GIMG(0x458a80))
#define __GetThreadPtr (*(int*)GIMG(0x46c32c))
#define __InitAccessH (*(int*)GIMG(0x459379))
#define __Is_DLL (*(int*)GIMG(0x908578))
#define __LargestSizeB4MiniHeapRover (*(int*)GIMG(0x46c408))
#define __ModF (*(int*)GIMG(0x45b2fe))
#define __MultipleThread (*(int*)GIMG(0x4593f4))
#define __NFiles (*(int*)GIMG(0x46c428))
#define __NTThreadFini (*(int*)GIMG(0x459545))
#define __OpenStreams (*(int*)GIMG(0x908580))
#define __RegisterThreadData (*(int*)GIMG(0x45acf1))
#define __RegisterThreadDataSize (*(int*)GIMG(0x459e5c))
#define __ReleaseFHeap (*(int*)GIMG(0x4593c5))
#define __ReleaseFileH (*(int*)GIMG(0x459367))
#define __ReleaseIOB (*(int*)GIMG(0x459347))
#define __ReleaseNHeap (*(int*)GIMG(0x4593b4))
#define __ReleaseSema4 (*(int*)GIMG(0x46c4ec))
#define __Rest8087 (*(int*)GIMG(0x46c500))
#define __Save8087 (*(int*)GIMG(0x46c4fc))
#define __ThreadDataSize (*(int*)GIMG(0x46c508))
#define __WD_Present (*(int*)GIMG(0x46c52c))
#define ___ExceptionFilter (*(int*)GIMG(0x458e7b))
#define ___FPE_handler (*(int*)GIMG(0x46c3fb))
#define ___begtext (*(int*)GIMG(0x410003))
#define ___chipbug (*(int*)GIMG(0x46c538))
#define __atexit (*(int*)GIMG(0x46c108))
#define __chipbug (*(int*)GIMG(0x46c538))
#define __chk8087 (*(int*)GIMG(0x457728))
#define __fdiv_chk (*(int*)GIMG(0x45bded))
#define __fdiv_fpr (*(int*)GIMG(0x45b92b))
#define __fheap_clean (*(int*)GIMG(0x908585))
#define __heap_enabled (*(int*)GIMG(0x46c518))
#define __int23_exit (*(int*)GIMG(0x46c10c))
#define __iob (*(int*)GIMG(0x46c114))
#define __nheap_clean (*(int*)GIMG(0x908584))
#define __nheapbeg (*(int*)GIMG(0x46c400))
#define __no87 (*(int*)GIMG(0x46c3ec))
#define __nullarea (*(int*)GIMG(0x460000))
#define __real87 (*(int*)GIMG(0x46c105))
#define __set_EINVAL (*(int*)GIMG(0x456ccf))
#define __sig_fini_rtn (*(code**)GIMG(0x46c370))
#define __sig_init_rtn (*(int*)GIMG(0x46c36c))
#define __sigabort (*(int*)GIMG(0x458cdb))
#define __tmpfnext (*(int*)GIMG(0x46c31c))
#define __umaskval (*(int*)GIMG(0x46c410))
#define _amblksiz (*(int*)GIMG(0x46c51c))
#define _bcrgb (*(int*)GIMG(0x7142c4))
#define _cbyte (*(int*)GIMG(0x46c3e0))
#define _child (*(int*)GIMG(0x46c3e8))
#define _clutspace (*(int*)GIMG(0x7140d4))
#define _cmptr (*(int*)GIMG(0x7142c0))
#define _dosret0 (*(int*)GIMG(0x458b90))
#define _dosretax (*(int*)GIMG(0x458baa))
#define _far_fog (*(int*)GIMG(0x462cd0))
#define _fcrgb (*(int*)GIMG(0x7142ec))
#define _flg (*(int*)GIMG(0x7142dc))
#define _fmode (*(int*)GIMG(0x46c31d))
#define _globmat (*(int*)GIMG(0x7142f0))
#define _heapmin (*(int*)GIMG(0x45a9e5))
#define _lmptr (*(int*)GIMG(0x7142d8))
#define _near_fog (*(int*)GIMG(0x462ccc))
#define _nexpand (*(int*)GIMG(0x45ac6c))
#define _nfree (*(int*)GIMG(0x456688))
#define _nheapmin (*(int*)GIMG(0x45a9e5))
#define _nmalloc (*(int*)GIMG(0x45777d))
#define _nrealloc (*(int*)GIMG(0x459ebd))
#define _op0 (*(int*)GIMG(0x714180))
#define _opvr0 (*(int*)GIMG(0x714170))
#define _opvr1 (*(int*)GIMG(0x714160))
#define _opz (*(int*)GIMG(0x7142c8))
#define _osbuild (*(int*)GIMG(0x46c3f9))
#define _osmajor (*(int*)GIMG(0x46c3f7))
#define _osminor (*(int*)GIMG(0x46c3f8))
#define _otz (*(int*)GIMG(0x7142bc))
#define _pad_i (*(int*)GIMG(0x463041))
#define _pad_j (*(int*)GIMG(0x463042))
#define _pad_ldown (*(int*)GIMG(0x463044))
#define _pad_lleft (*(int*)GIMG(0x463045))
#define _pad_lup (*(int*)GIMG(0x463043))
#define _pad_rdown (*(int*)GIMG(0x46304c))
#define _pad_rleft (*(int*)GIMG(0x46304d))
#define _pad_rup (*(int*)GIMG(0x46304b))
#define _pad_start (*(int*)GIMG(0x463040))
#define _primfuncs (*(int*)GIMG(0x46002c))
#define _psp (*(int*)GIMG(0x46c3cc))
#define _rgb0 (*(int*)GIMG(0x7142e0))
#define _rgb1 (*(int*)GIMG(0x7142e4))
#define _rgb2 (*(int*)GIMG(0x7142e8))
#define _screenbuffer (*(int*)GIMG(0x700450))
#define _scrx (*(int*)GIMG(0x7142d0))
#define _scry (*(int*)GIMG(0x7142cc))
#define _set_matherr (*(int*)GIMG(0x45c131))
#define _start_TI (*(int*)GIMG(0x46ff6c))
#define _texturespace (*(int*)GIMG(0x7140cc))
#define _vr0 (*(int*)GIMG(0x714100))
#define _vr1 (*(int*)GIMG(0x714110))
#define _vr2 (*(int*)GIMG(0x7140e0))
#define _vr3 (*(int*)GIMG(0x7140f0))
#define _wstart2_ (*(int*)GIMG(0x456a74))
#define active_block_numbers (*(int*)GIMG(0x750ea0))
#define active_object_blocks (*(int*)GIMG(0x750f10))
#define actual_season_number (*(int*)GIMG(0x4682f4))
#define add_transparency_table (*(int*)GIMG(0x7140c8))
#define adjusted_music (*(int*)GIMG(0x8ff2a4))
#define adjusted_sfx (*(int*)GIMG(0x8ff2a0))
#define applause (*(int*)GIMG(0x901714))
#define applause_up" (*(int*)GIMG(0x901718))
#define audible_distance (*(int*)GIMG(0x900fc0))
#define bonnet_quad_list (*(int*)GIMG(0x466a48))
#define bonnet_quads (*(int*)GIMG(0x466a60))
#define bonnetoff_object (*(int*)GIMG(0x744260))
#define boot_objects_count (*(int*)GIMG(0x744348))
#define boot_quad_list (*(int*)GIMG(0x466a80))
#define boot_quads (*(int*)GIMG(0x466a88))
#define bootoff_index (*(int*)GIMG(0x744354))
#define bootoff_object (*(int*)GIMG(0x744180))
#define bowl_cars (*(int*)GIMG(0x751f64))
#define buffer_num (*(int*)GIMG(0x462fec))
#define cam_angles (*(int*)GIMG(0x464a98))
#define camera_car (*(int*)GIMG(0x463eec))
#define camera_collision (*(int*)GIMG(0x744b68))
#define camera_fd (*(int*)GIMG(0x744b10))
#define camera_fd_pt (*(int*)GIMG(0x744b70))
#define camera_offset (*(int*)GIMG(0x464a88))
#define camera_section (*(int*)GIMG(0x464a84))
#define camera_switch (*(int*)GIMG(0x464a94))
#define car0_being_obstructed (*(int*)GIMG(0x751f80))
#define car_colour_matrix (*(int*)GIMG(0x744e00))
#define car_edge_pos (*(int*)GIMG(0x464cd2))
#define car_fd (*(int*)GIMG(0x75a290))
#define car_handling (*(int*)GIMG(0x75a600))
#define car_info (*(int*)GIMG(0x75d840))
#define car_light_matrix (*(int*)GIMG(0x464cb4))
#define car_lookup (*(int*)GIMG(0x466a0c))
#define car_object (*(int*)GIMG(0x749018))
#define car_order (*(int*)GIMG(0x75d828))
#define car_position (*(int*)GIMG(0x74be7c))
#define car_speed (*(int*)GIMG(0x74be88))
#define car_vertices (*(int*)GIMG(0x745058))
#define car_wheel_fd (*(int*)GIMG(0x75c7e8))
#define card_data (*(int*)GIMG(0x73c064))
#define cars_in_crash (*(int*)GIMG(0x90171c))
#define carselect_object (*(int*)GIMG(0x907c6c))
#define casino_frame_count (*(int*)GIMG(0x4651b8))
#define cdb" (*(int*)GIMG(0x71bf80))
#define champ_info (*(int*)GIMG(0x905ae0))
#define cheese (*(int*)GIMG(0x716db0))
#define cigar_angles (*(int*)GIMG(0x464e54))
#define cigar_object (*(int*)GIMG(0x749b50))
#define cigar_position (*(int*)GIMG(0x464e5c))
#define coaster_car_object (*(int*)GIMG(0x749900))
#define commentating (*(int*)GIMG(0x901764))
#define cont_game (*(int*)GIMG(0x8fefc0))
#define corner_fd (*(int*)GIMG(0x75a0e0))
#define counter_thing (*(int*)GIMG(0x46706c))
#define crowd_volume (*(int*)GIMG(0x90176c))
#define current_frame (*(int*)GIMG(0x462ff0))
#define current_level (*(int*)GIMG(0x8febf4))
#define current_player (*(int*)GIMG(0x905acc))
#define current_player_car (*(int*)GIMG(0x905ad0))
#define current_race (*(int*)GIMG(0x905ac8))
#define current_season (*(int*)GIMG(0x905ac0))
#define d1 (*(int*)GIMG(0x4638cc))
#define d2 (*(int*)GIMG(0x463904))
#define d3 (*(int*)GIMG(0x46394c))
#define d4 (*(int*)GIMG(0x463984))
#define d5 (*(int*)GIMG(0x4639cc))
#define d6 (*(int*)GIMG(0x4639e4))
#define d7 (*(int*)GIMG(0x463a24))
#define d8 (*(int*)GIMG(0x463a6c))
#define d9 (*(int*)GIMG(0x463aac))
#define damage (*(int*)GIMG(0x74b2d0))
#define damage_car (*(int*)GIMG(0x744b80))
#define data (*(int*)GIMG(0x759810))
#define db (*(int*)GIMG(0x71be64))
#define ddmain (*(int*)GIMG(0x4239f8))
#define debris_cluts (*(int*)GIMG(0x73c2d0))
#define dec_info (*(int*)GIMG(0x750f48))
#define decrunch_block (*(int*)GIMG(0x751d60))
#define decrunch_flag (*(int*)GIMG(0x751d58))
#define demo_flash (*(int*)GIMG(0x4652a0))
#define demo_mode (*(int*)GIMG(0x46385c))
#define dent_area (*(int*)GIMG(0x463d3c))
#define dent_area_lookup (*(int*)GIMG(0x463d94))
#define depth_cue_far (*(int*)GIMG(0x462fdc))
#define depth_cue_near (*(int*)GIMG(0x462fd8))
#define dirbuf (*(int*)GIMG(0x714310))
#define disable_save_game (*(int*)GIMG(0x46a924))
#define dollar_frame_count (*(int*)GIMG(0x465170))
#define dpadbuttons_object (*(int*)GIMG(0x907cc0))
#define dpadlabels_object (*(int*)GIMG(0x907c50))
#define dr_modes (*(int*)GIMG(0x463004))
#define draw_face_3pt_flat (*(int*)GIMG(0x417efc))
#define draw_face_3pt_flat_dpq_lit (*(int*)GIMG(0x418458))
#define draw_face_3pt_flat_lit (*(int*)GIMG(0x4180c8))
#define draw_face_3pt_gour (*(int*)GIMG(0x41bcc4))
#define draw_face_3pt_gour_dpq_lit (*(int*)GIMG(0x41c220))
#define draw_face_3pt_gour_lit (*(int*)GIMG(0x41be90))
#define draw_face_3pt_pict (*(int*)GIMG(0x41ce8c))
#define draw_face_3pt_pict_lit (*(int*)GIMG(0x41d058))
#define draw_face_3pt_text (*(int*)GIMG(0x418fe0))
#define draw_face_3pt_text_dpq_lit (*(int*)GIMG(0x41a0ac))
#define draw_face_3pt_text_lit (*(int*)GIMG(0x4196a8))
#define draw_face_3pt_text_squash (*(int*)GIMG(0x4191ac))
#define draw_face_4pt_flat (*(int*)GIMG(0x418678))
#define draw_face_4pt_flat_dpq (*(int*)GIMG(0x418aa4))
#define draw_face_4pt_flat_dpq_lit (*(int*)GIMG(0x418cb8))
#define draw_face_4pt_flat_lit (*(int*)GIMG(0x41888c))
#define draw_face_4pt_gour (*(int*)GIMG(0x41c49c))
#define draw_face_4pt_gour_dpq (*(int*)GIMG(0x41c8c8))
#define draw_face_4pt_gour_dpq_lit (*(int*)GIMG(0x41cae0))
#define draw_face_4pt_gour_lit (*(int*)GIMG(0x41c6b0))
#define draw_face_4pt_pict (*(int*)GIMG(0x41d9e0))
#define draw_face_4pt_pict_lit (*(int*)GIMG(0x41dbf4))
#define draw_face_4pt_text (*(int*)GIMG(0x41a40c))
#define draw_face_4pt_text_dpq (*(int*)GIMG(0x41b054))
#define draw_face_4pt_text_dpq_lit (*(int*)GIMG(0x41b97c))
#define draw_face_4pt_text_dpq_squash (*(int*)GIMG(0x41b2d8))
#define draw_face_4pt_text_lit (*(int*)GIMG(0x41acf4))
#define draw_face_4pt_text_squash (*(int*)GIMG(0x41a66c))
#define draw_face_tilt_sprite_dpq (*(int*)GIMG(0x41f220))
#define draw_frame (*(int*)GIMG(0x73c2bc))
#define draw_text_half (*(int*)GIMG(0x410010))
#define drive_anim_frames (*(int*)GIMG(0x749530))
#define drive_frame_count (*(int*)GIMG(0x465074))
#define drive_textures (*(int*)GIMG(0x465064))
#define dth_clip (*(int*)GIMG(0x480010))
#define dth_clut (*(int*)GIMG(0x480014))
#define dth_delta1 (*(int*)GIMG(0x480040))
#define dth_delta2 (*(int*)GIMG(0x48003c))
#define dth_shade (*(int*)GIMG(0x480030))
#define dth_tpage (*(int*)GIMG(0x480044))
#define dth_u1 (*(int*)GIMG(0x48004c))
#define dth_v1 (*(int*)GIMG(0x480048))
#define dth_x1 (*(int*)GIMG(0x48002c))
#define dth_x2 (*(int*)GIMG(0x480028))
#define dth_y1 (*(int*)GIMG(0x480024))
#define dth_y2 (*(int*)GIMG(0x480020))
#define entry (*(int*)GIMG(0x456a74))
#define euphoria (*(int*)GIMG(0x901730))
#define exit_game (*(int*)GIMG(0x8ff0b0))
#define far_z_clip (*(int*)GIMG(0x462fc8))
#define fastest_laps (*(int*)GIMG(0x4680c0))
#define fi_clut" (*(int*)GIMG(0x75eb68))
#define fi_levdat (*(int*)GIMG(0x75eb60))
#define fi_texture (*(int*)GIMG(0x75eb70))
#define final_places (*(int*)GIMG(0x75d9fc))
#define flag (*(int*)GIMG(0x464a8c))
#define flag1_textures (*(int*)GIMG(0x4650a4))
#define flag1_wave_count (*(int*)GIMG(0x4650d4))
#define flag1_wave_frames (*(int*)GIMG(0x749500))
#define flag2_textures (*(int*)GIMG(0x4650bc))
#define flag2_wave_count (*(int*)GIMG(0x4650d8))
#define flag2_wave_frames (*(int*)GIMG(0x749518))
#define flame_anim_frames (*(int*)GIMG(0x7494c0))
#define flame_frame_count (*(int*)GIMG(0x749afc))
#define flame_textures (*(int*)GIMG(0x465148))
#define flare_images (*(int*)GIMG(0x74aa60))
#define flare_info (*(int*)GIMG(0x465254))
#define flash1_anim_frames (*(int*)GIMG(0x749ab4))
#define flash1_frame_count (*(int*)GIMG(0x749b00))
#define flash2_anim_frames (*(int*)GIMG(0x749a90))
#define flash2_frame_count (*(int*)GIMG(0x749b08))
#define flash3_anim_frames (*(int*)GIMG(0x749ad8))
#define flash3_frame_count (*(int*)GIMG(0x749b04))
#define floaty_camera_caprio (*(int*)GIMG(0x464438))
#define floaty_camera_fd (*(int*)GIMG(0x744b3c))
#define floaty_camera_forest (*(int*)GIMG(0x464288))
#define floaty_camera_liberty (*(int*)GIMG(0x464558))
#define floaty_camera_sca (*(int*)GIMG(0x464678))
#define floaty_camera_speedway (*(int*)GIMG(0x463f4c))
#define floaty_camera_ultimate (*(int*)GIMG(0x46484c))
#define fog_col" (*(int*)GIMG(0x46589e))
#define frame_rate (*(int*)GIMG(0x73c2c4))
#define frame_skip (*(int*)GIMG(0x73c2b8))
#define frame_wait (*(int*)GIMG(0x73c2a4))
#define frames_per_sec (*(int*)GIMG(0x462ffc))
#define free_mem (*(int*)GIMG(0x71bf9c))
#define fx (*(int*)GIMG(0x901734))
#define fx_1 (*(int*)GIMG(0x90174c))
#define fx_10 (*(int*)GIMG(0x90172c))
#define fx_11 (*(int*)GIMG(0x901728))
#define fx_15 (*(int*)GIMG(0x901724))
#define fx_17 (*(int*)GIMG(0x901758))
#define fx_18 (*(int*)GIMG(0x901774))
#define fx_2 (*(int*)GIMG(0x901754))
#define fx_21 (*(int*)GIMG(0x90175c))
#define fx_22 (*(int*)GIMG(0x901710))
#define fx_3 (*(int*)GIMG(0x901750))
#define fx_4 (*(int*)GIMG(0x901744))
#define fx_5 (*(int*)GIMG(0x901740))
#define fx_6 (*(int*)GIMG(0x901748))
#define fx_7 (*(int*)GIMG(0x901770))
#define fx_8 (*(int*)GIMG(0x901720))
#define fx_9 (*(int*)GIMG(0x90173c))
#define g_sprite_info (*(int*)GIMG(0x716da0))
#define gnormals (*(int*)GIMG(0x71bdd4))
#define goose_anim_frames (*(int*)GIMG(0x7494d8))
#define goose_frame_count (*(int*)GIMG(0x4650a0))
#define goose_textures (*(int*)GIMG(0x465078))
#define gpoly (*(int*)GIMG(0x71bdc8))
#define gprim1 (*(int*)GIMG(0x71bdd0))
#define gprim2 (*(int*)GIMG(0x71bdcc))
#define grounded_count (*(int*)GIMG(0x75a240))
#define gte_SetFogFar (*(int*)GIMG(0x414264))
#define gte_SetFogNear (*(int*)GIMG(0x41428c))
#define gtexture (*(int*)GIMG(0x71bddc))
#define gtexture_def (*(int*)GIMG(0x71bdf8))
#define h_norm (*(int*)GIMG(0x71bde8))
#define hanging_x (*(int*)GIMG(0x75e828))
#define hanging_y (*(int*)GIMG(0x75e690))
#define hanging_z (*(int*)GIMG(0x75e9c0))
#define high_car_vertices (*(int*)GIMG(0x744360))
#define highlight_colour (*(int*)GIMG(0x4699c8))
#define hlf_transparency_table (*(int*)GIMG(0x714050))
#define hold (*(int*)GIMG(0x759828))
#define image_info (*(int*)GIMG(0x74aa00))
#define in_640 (*(int*)GIMG(0x460480))
#define info_dmode (*(int*)GIMG(0x8fef70))
#define info_screen_dire_stats (*(int*)GIMG(0x469bb8))
#define info_screen_directions (*(int*)GIMG(0x469bac))
#define info_screen_text (*(int*)GIMG(0x469b0c))
#define info_tile (*(int*)GIMG(0x8ff1a0))
#define j_anim (*(int*)GIMG(0x749490))
#define j_frame_count (*(int*)GIMG(0x4651cc))
#define landing_data (*(int*)GIMG(0x466b50))
#define lap_name_entry (*(int*)GIMG(0x9063b0))
#define lap_num" (*(int*)GIMG(0x74be80))
#define last_time (*(int*)GIMG(0x73c2a0))
#define left_collision_offsets (*(int*)GIMG(0x465e30))
#define level_data (*(int*)GIMG(0x8febf0))
#define level_data_buffer (*(int*)GIMG(0x75ebf0))
#define light_matrix (*(int*)GIMG(0x71bdd8))
#define log2 (*(code**)GIMG(0x45af22))
#define madbase_object (*(int*)GIMG(0x907cf8))
#define madbut_object (*(int*)GIMG(0x907c34))
#define madjoy_object (*(int*)GIMG(0x907d84))
#define mem_size (*(int*)GIMG(0x73c294))
#define memory_card_clut (*(int*)GIMG(0x467370))
#define memory_card_icon (*(int*)GIMG(0x4672f0))
#define mid_car_vertices (*(int*)GIMG(0x7447a8))
#define move_key (*(int*)GIMG(0x8fee30))
#define move_txt (*(int*)GIMG(0x8fec50))
#define music_vol (*(int*)GIMG(0x8ff200))
#define music_volume (*(int*)GIMG(0x46740c))
#define negbut1_object (*(int*)GIMG(0x907cdc))
#define negbut2_object (*(int*)GIMG(0x907d14))
#define negbut3_object (*(int*)GIMG(0x907c88))
#define negleft_object (*(int*)GIMG(0x907da0))
#define no_txt (*(int*)GIMG(0x8feed0))
#define notches_music (*(int*)GIMG(0x8ff150))
#define num_cars (*(int*)GIMG(0x46765c))
#define num_of_ranks (*(int*)GIMG(0x75d9d0))
#define num_races (*(int*)GIMG(0x467654))
#define num_scene_objects (*(int*)GIMG(0x750ed8))
#define num_spies (*(int*)GIMG(0x716da4))
#define num_strips (*(int*)GIMG(0x744af0))
#define num_textures (*(int*)GIMG(0x462ce0))
#define offset_table (*(int*)GIMG(0x464a6c))
#define old_flying_index (*(int*)GIMG(0x744350))
#define old_index (*(int*)GIMG(0x74435c))
#define otsize (*(int*)GIMG(0x71be60))
#define pad_option (*(int*)GIMG(0x467414))
#define padmap (*(int*)GIMG(0x46302c))
#define pal_flag (*(int*)GIMG(0x462fe8))
#define particle (*(int*)GIMG(0x755340))
#define pause_tile (*(int*)GIMG(0x8ff1d0))
#define pause_txt (*(int*)GIMG(0x8ff010))
#define permission (*(int*)GIMG(0x75eb58))
#define playable_bowls (*(int*)GIMG(0x467408))
#define playable_tracks" (*(int*)GIMG(0x467404))
#define player_names (*(int*)GIMG(0x905f18))
#define point (*(int*)GIMG(0x4670a0))
#define poly_clipx (*(int*)GIMG(0x460024))
#define poly_clipy (*(int*)GIMG(0x460028))
#define polygon_angles (*(int*)GIMG(0x73c2f8))
#define prim_buf (*(int*)GIMG(0x71bf90))
#define prim_buf_size (*(int*)GIMG(0x463014))
#define quick_racetype_offset (*(int*)GIMG(0x467418))
#define quit_flag (*(int*)GIMG(0x73c2ac))
#define race_car (*(int*)GIMG(0x467400))
#define race_finished (*(int*)GIMG(0x75d9f4))
#define race_mode (*(int*)GIMG(0x4673f8))
#define race_points" (*(int*)GIMG(0x74be78))
#define race_track (*(int*)GIMG(0x4673fc))
#define race_type (*(int*)GIMG(0x4673f4))
#define recorded_pad_type (*(int*)GIMG(0x73c2a8))
#define recorded_strips" (*(int*)GIMG(0x74bea0))
#define restart_cd_audio (*(int*)GIMG(0x467420))
#define rgb_lookup (*(int*)GIMG(0x713050))
#define roller (*(int*)GIMG(0x749540))
#define rot_flags (*(int*)GIMG(0x71adc0))
#define rot_points (*(int*)GIMG(0x716dc0))
#define safe_anim (*(int*)GIMG(0x749a50))
#define safe_frame_count (*(int*)GIMG(0x465188))
#define sca_frame_count" (*(int*)GIMG(0x465060))
#define sca_textures (*(int*)GIMG(0x46501c))
#define scene_colour_matrix" (*(int*)GIMG(0x465834))
#define scene_colour_vectors (*(int*)GIMG(0x465922))
#define scene_light_matrix (*(int*)GIMG(0x465854))
#define scene_objects (*(int*)GIMG(0x74c1a0))
#define scene_position (*(int*)GIMG(0x74f2a0))
#define screen_centre_x. (*(int*)GIMG(0x462fd0))
#define screen_height (*(int*)GIMG(0x462ff8))
#define screen_line_list (*(int*)GIMG(0x907c08))
#define screen_poly_list (*(int*)GIMG(0x907c10))
#define screen_text_list (*(int*)GIMG(0x907c14))
#define screen_width (*(int*)GIMG(0x462ff4))
#define sdOverdata (*(int*)GIMG(0x46522e))
#define sdRacedata (*(int*)GIMG(0x465216))
#define sd_damage_botl (*(int*)GIMG(0x464c84))
#define sd_damage_botr (*(int*)GIMG(0x464c9c))
#define sd_damage_car (*(int*)GIMG(0x464c0c))
#define sd_damage_midl (*(int*)GIMG(0x464c54))
#define sd_damage_midr (*(int*)GIMG(0x464c6c))
#define sd_damage_topl (*(int*)GIMG(0x464c24))
#define sd_damage_topr (*(int*)GIMG(0x464c3c))
#define sd_music_vol (*(int*)GIMG(0x466efc))
#define sd_no_txt (*(int*)GIMG(0x466fbc))
#define sd_notches_music (*(int*)GIMG(0x466f14))
#define sd_notches_sfx (*(int*)GIMG(0x466f44))
#define sd_pause_txt (*(int*)GIMG(0x466ecc))
#define sd_select_txt (*(int*)GIMG(0x467034))
#define sd_sfx_vol (*(int*)GIMG(0x466f2c))
#define sd_shadow (*(int*)GIMG(0x466e94))
#define sd_smoke (*(int*)GIMG(0x466364))
#define sd_spark (*(int*)GIMG(0x4664e4))
#define sd_sure2_txt (*(int*)GIMG(0x466fd4))
#define sd_totitle_txt (*(int*)GIMG(0x466f74))
#define sd_x_key (*(int*)GIMG(0x46701c))
#define season_statistics (*(int*)GIMG(0x9063c0))
#define setup_face_sprite (*(int*)GIMG(0x41e418))
#define shadow (*(int*)GIMG(0x75da10))
#define sky_object (*(int*)GIMG(0x751d70))
#define sky_shape1 (*(int*)GIMG(0x751e60))
#define sky_shape2 (*(int*)GIMG(0x751e64))
#define sky_shape3 (*(int*)GIMG(0x751e68))
#define sky_shape4 (*(int*)GIMG(0x751e50))
#define sky_shape5 (*(int*)GIMG(0x751e54))
#define sky_shape6 (*(int*)GIMG(0x751e58))
#define sky_shape7 (*(int*)GIMG(0x751e5c))
#define sky_shape8 (*(int*)GIMG(0x751e6c))
#define slab_background& (*(int*)GIMG(0x907bf0))
#define slab_background_colour (*(int*)GIMG(0x4699c4))
#define slab_bounce_table (*(int*)GIMG(0x4699d0))
#define slab_light_matrix (*(int*)GIMG(0x469994))
#define slab_position (*(int*)GIMG(0x46995c))
#define sound_volume (*(int*)GIMG(0x467410))
#define sprite_matrix (*(int*)GIMG(0x71bdfc))
#define stats_recorded (*(int*)GIMG(0x46741c))
#define still_running (*(int*)GIMG(0x74be84))
#define strip_data (*(int*)GIMG(0x744af8))
#define strip_data_fi (*(int*)GIMG(0x744ae8))
#define strip_postincrements (*(int*)GIMG(0x463ebc))
#define strip_vertex (*(int*)GIMG(0x744af4))
#define sub_transparency_table (*(int*)GIMG(0x7140d0))
#define sun_images (*(int*)GIMG(0x74ace0))
#define sun_position (*(int*)GIMG(0x46526c))
#define sure2_txt (*(int*)GIMG(0x8feca0))
#define sure_txt (*(int*)GIMG(0x8ff060))
#define surface_friction_coeff (*(int*)GIMG(0x466d90))
#define surface_traction_coeff (*(int*)GIMG(0x466d98))
#define surfai (*(int*)GIMG(0x751f88))
#define switchdataD_004169d0 (*(int*)GIMG(0x4169d0))
#define switchdataD_00420ce0 (*(int*)GIMG(0x420ce0))
#define switchdataD_00425e60 (*(int*)GIMG(0x425e60))
#define switchdataD_00426e84 (*(int*)GIMG(0x426e84))
#define switchdataD_00427df8 (*(int*)GIMG(0x427df8))
#define switchdataD_00427e1c (*(int*)GIMG(0x427e1c))
#define switchdataD_00429670 (*(int*)GIMG(0x429670))
#define switchdataD_0042c748 (*(int*)GIMG(0x42c748))
#define switchdataD_0042ca9c (*(int*)GIMG(0x42ca9c))
#define switchdataD_0042ceac (*(int*)GIMG(0x42ceac))
#define switchdataD_00430210 (*(int*)GIMG(0x430210))
#define switchdataD_00431340 (*(int*)GIMG(0x431340))
#define switchdataD_00432c3c (*(int*)GIMG(0x432c3c))
#define switchdataD_0043315c (*(int*)GIMG(0x43315c))
#define switchdataD_0043316c (*(int*)GIMG(0x43316c))
#define switchdataD_00433e20 (*(int*)GIMG(0x433e20))
#define switchdataD_00433e30 (*(int*)GIMG(0x433e30))
#define switchdataD_004371dc (*(int*)GIMG(0x4371dc))
#define switchdataD_004371ec (*(int*)GIMG(0x4371ec))
#define switchdataD_004383ec (*(int*)GIMG(0x4383ec))
#define switchdataD_004383fc (*(int*)GIMG(0x4383fc))
#define switchdataD_0043b598 (*(int*)GIMG(0x43b598))
#define switchdataD_0043b914 (*(int*)GIMG(0x43b914))
#define switchdataD_0043bf00 (*(int*)GIMG(0x43bf00))
#define switchdataD_0043e4ec (*(int*)GIMG(0x43e4ec))
#define switchdataD_0043fb08 (*(int*)GIMG(0x43fb08))
#define switchdataD_0044044c (*(int*)GIMG(0x44044c))
#define switchdataD_00442cc4 (*(int*)GIMG(0x442cc4))
#define switchdataD_004430a0 (*(int*)GIMG(0x4430a0))
#define switchdataD_004450cc (*(int*)GIMG(0x4450cc))
#define switchdataD_0044624c (*(int*)GIMG(0x44624c))
#define switchdataD_00446c44 (*(int*)GIMG(0x446c44))
#define switchdataD_00446fb0 (*(int*)GIMG(0x446fb0))
#define switchdataD_00446fc8 (*(int*)GIMG(0x446fc8))
#define switchdataD_00449c30 (*(int*)GIMG(0x449c30))
#define switchdataD_00449dc8 (*(int*)GIMG(0x449dc8))
#define switchdataD_0044a18c (*(int*)GIMG(0x44a18c))
#define switchdataD_0044b7e8 (*(int*)GIMG(0x44b7e8))
#define switchdataD_0044c1c0 (*(int*)GIMG(0x44c1c0))
#define switchdataD_0044ec98 (*(int*)GIMG(0x44ec98))
#define switchdataD_0044f7fc (*(int*)GIMG(0x44f7fc))
#define switchdataD_0044fbac (*(int*)GIMG(0x44fbac))
#define switchdataD_00450ebc (*(int*)GIMG(0x450ebc))
#define switchdataD_00453b88 (*(int*)GIMG(0x453b88))
#define switchdataD_004547f0 (*(int*)GIMG(0x4547f0))
#define tilt_sprite_matrix (*(int*)GIMG(0x71be1c))
#define time_phase_shift (*(int*)GIMG(0x465248))
#define timing (*(int*)GIMG(0x463858))
#define tot_time (*(int*)GIMG(0x73c2b4))
#define total_collisions (*(int*)GIMG(0x901760))
#define total_dest_timer (*(int*)GIMG(0x74be8c))
#define total_time (*(int*)GIMG(0x73c2b0))
#define track_height (*(int*)GIMG(0x744b6c))
#define track_info (*(int*)GIMG(0x466df0))
#define track_lookup (*(int*)GIMG(0x467424))
#define track_object (*(int*)GIMG(0x907d68))
#define track_position (*(int*)GIMG(0x464e7c))
#define twist1_frame_count (*(int*)GIMG(0x4651e0))
#define twist2_frame_count (*(int*)GIMG(0x4651f4))
#define u (*(int*)GIMG(0x901738))
#define use_joystick (*(int*)GIMG(0x463028))
#define v_norm (*(int*)GIMG(0x71bdf0))
#define value (*(int*)GIMG(0x71bfa8))
#define wheel_object (*(int*)GIMG(0x7552d0))
#define wheeloff_index (*(int*)GIMG(0x744340))
#define wheeloff_object& (*(int*)GIMG(0x73c400))
#define whllck (*(int*)GIMG(0x751f84))
#define wild_bill_angles (*(int*)GIMG(0x464e4c))
#define wild_bill_position (*(int*)GIMG(0x464e6c))
#define world_angles (*(int*)GIMG(0x71bde0))
#define world_matrix (*(int*)GIMG(0x462fa4))
#define x_key (*(int*)GIMG(0x8fed90))
#define yes_quit (*(int*)GIMG(0x8ff2ac))
#define yes_retire (*(int*)GIMG(0x8ff2a8))
extern int DAT_00000186;
extern int DAT_00415644;
extern int DAT_00415648;
extern int DAT_0041564c;
extern int DAT_00415650;
extern int DAT_00415654;
extern int DAT_0042a380;
extern int DAT_0042a7e0;
extern int DAT_0042da30;
extern int DAT_0042daf0;
extern int DAT_0042e004;
extern int DAT_0042e00c;
extern int DAT_0042e010;
extern int DAT_0042e014;
extern int DAT_0042e018;
extern int DAT_0042e01c;
extern int DAT_00430de0;
extern int DAT_00443f70;
extern int DAT_00444860;
extern int DAT_004448a0;
extern int DAT_00449370;
extern int DAT_00449388;
extern int DAT_0044b130;
extern int DAT_0044b134;
extern int DAT_0044b138;
extern int DAT_0044b13c;
extern int DAT_0044b140;
extern int DAT_0044b144;
extern int DAT_0044c8d0;
extern int DAT_0044c8d4;
extern int DAT_0044c8d8;
extern int DAT_0044c8dc;
extern int DAT_0044c8e0;
extern int DAT_0044c8e4;
extern int DAT_00450070;
extern int DAT_00450088;
extern int DAT_004500a0;
extern int DAT_004500b8;
extern int DAT_004500d0;
extern int DAT_00450c20;
extern int DAT_00450c40;
extern int DAT_00450c5e;
extern int DAT_00450c62;
extern int DAT_00450c66;
extern int DAT_00453cd0;
extern int DAT_00453ce8;
extern int DAT_00453d00;
extern int DAT_00453d18;
extern int DAT_00453d30;
extern int DAT_00454af0;
extern int DAT_00454b08;
extern int DAT_00454b20;
extern int DAT_00454b38;
extern int DAT_00454b50;
extern int DAT_004552e0;
extern int DAT_004552f8;
extern int DAT_00455310;
extern int DAT_00455328;
extern int DAT_00455340;
extern int DAT_00459339;
extern int DAT_004593d6;
extern int DAT_004593ea;
extern int DAT_0045953b;
extern int DAT_00460004;
extern int DAT_0046000c;
extern int DAT_00460010;
extern int DAT_0046042c;
extern int DAT_00460434;
extern int* DAT_00460438;
extern int* DAT_0046043c;
extern int* DAT_00460440;
extern int* DAT_00460444;
extern int DAT_00460448;
extern int DAT_0046044c;
extern int DAT_00460474;
extern int DAT_00460478;
extern int DAT_0046047c;
extern int DAT_00460488;
extern int DAT_0046048c;
extern int DAT_00460490;
extern int DAT_004604a0;
extern int DAT_004604a4;
extern int DAT_004604a8;
extern int DAT_004604ac;
extern int DAT_004604b0;
extern int DAT_004604b2;
extern int DAT_004604b6;
extern int DAT_004604ba;
extern int DAT_004604be;
extern int DAT_00460cc8;
extern int DAT_00462cc8;
extern int DAT_00462cd8;
extern int DAT_00462cdc;
extern int DAT_00462d64;
extern int DAT_00462d68;
extern int DAT_00462d6c;
extern int DAT_00462d70;
extern int DAT_00462d74;
extern int DAT_00462d78;
extern int DAT_00462d80;
extern int DAT_00462d84;
extern int DAT_00462d88;
extern int DAT_00462d8c;
extern int DAT_00462d90;
extern int DAT_00462fa6;
extern int DAT_00462faa;
extern int DAT_00462fb6;
extern int DAT_00462fba;
extern int DAT_00462fbe;
extern int DAT_00462fc4;
extern int DAT_00462fcc;
extern int DAT_00463002;
extern int DAT_00463006;
extern int DAT_00463010;
extern int DAT_00463018;
extern int DAT_0046301c;
extern int DAT_00463020;
extern int DAT_00463024;
extern int DAT_0046302d;
extern int DAT_0046302e;
extern int DAT_0046302f;
extern int DAT_00463030;
extern int DAT_00463031;
extern int DAT_00463032;
extern int DAT_00463033;
extern int DAT_00463034;
extern int DAT_00463035;
extern int DAT_00463036;
extern int DAT_00463037;
extern int DAT_00463038;
extern int DAT_00463039;
extern int DAT_0046303e;
extern int DAT_0046303f;
extern int DAT_00463046;
extern int DAT_00463047;
extern int DAT_00463048;
extern int DAT_00463049;
extern int DAT_0046304a;
extern int DAT_0046304e;
extern int DAT_0046304f;
extern int DAT_00463050;
extern int DAT_00463052;
extern int DAT_00463860;
extern int DAT_00463864;
extern int DAT_00463868;
extern int DAT_0046386c;
extern int DAT_00463870;
extern int DAT_00463874;
extern int DAT_00463878;
extern int DAT_0046388a;
extern int DAT_0046388e;
extern int DAT_00463892;
extern int DAT_00463896;
extern int DAT_004638b8;
extern int DAT_004638bc;
extern int DAT_004638c0;
extern int DAT_004638c4;
extern int DAT_00463dcc;
extern int DAT_00463dd0;
extern int DAT_00463e2c;
extern int DAT_00463e30;
extern int DAT_00463e8c;
extern int DAT_00463ef0;
extern int DAT_00463ef8;
extern int DAT_00463efc;
extern int DAT_00463f00;
extern int DAT_00463f04;
extern int DAT_00463f08;
extern int DAT_00463f0c;
extern int DAT_00463f10;
extern int DAT_00463f14;
extern int DAT_00463f1c;
extern int DAT_00463f20;
extern int DAT_00463f24;
extern int DAT_00463f28;
extern int DAT_00463f2c;
extern int DAT_00464a70;
extern int DAT_00464a90;
extern int DAT_00464aa0;
extern int DAT_00464aa4;
extern int DAT_00464aa8;
extern int DAT_00464aac;
extern int DAT_00464ab0;
extern int DAT_00464bb0;
extern int DAT_00464bb4;
extern int DAT_00464bb8;
extern int DAT_00464bbc;
extern int DAT_00464bf0;
extern int DAT_00464c0e;
extern int DAT_00464c26;
extern int DAT_00464c3e;
extern int DAT_00464c56;
extern int DAT_00464c6e;
extern int DAT_00464c86;
extern int DAT_00464c9e;
extern int DAT_00464cb8;
extern int DAT_00464cd0;
extern int DAT_00464cd4;
extern int DAT_00464cf0;
extern int DAT_00464cf4;
extern int DAT_00464d1c;
extern int DAT_00464d2c;
extern int DAT_00464e50;
extern int DAT_00464e58;
extern int DAT_00464e80;
extern int DAT_00464e84;
extern int DAT_00464e88;
extern int DAT_00464e8c;
extern int DAT_00464e90;
extern int DAT_00464e94;
extern int DAT_00464e98;
extern int DAT_004651f8;
extern int DAT_0046520a;
extern int DAT_0046520e;
extern int DAT_00465212;
extern int DAT_00465255;
extern int DAT_00465256;
extern int DAT_0046526a;
extern int DAT_0046526e;
extern int DAT_004652a4;
extern int DAT_004652bc;
extern int DAT_004652d4;
extern int DAT_004652ec;
extern int DAT_00465304;
extern int DAT_0046531c;
extern int DAT_00465334;
extern int DAT_0046534c;
extern int DAT_00465364;
extern int DAT_0046537c;
extern int DAT_00465394;
extern int DAT_004653ac;
extern int DAT_004653c8;
extern int DAT_004653e0;
extern int DAT_004653f8;
extern int DAT_00465410;
extern int DAT_00465428;
extern int DAT_00465440;
extern int DAT_00465458;
extern int DAT_00465470;
extern int DAT_00465488;
extern int DAT_0046548a;
extern int DAT_004654a0;
extern int DAT_004654a2;
extern int DAT_004654b8;
extern int DAT_004654d0;
extern int DAT_004654e8;
extern int DAT_00465500;
extern int DAT_00465518;
extern int DAT_00465530;
extern int DAT_00465548;
extern int DAT_00465560;
extern int DAT_00465578;
extern int DAT_00465590;
extern int DAT_004655a8;
extern int DAT_004655c0;
extern int DAT_004655d8;
extern int DAT_004655f0;
extern int DAT_00465608;
extern int DAT_00465698;
extern int DAT_00465728;
extern int DAT_0046572a;
extern int DAT_004657b8;
extern int DAT_004657d0;
extern int DAT_004657e8;
extern int DAT_00465800;
extern int DAT_00465804;
extern int DAT_00465858;
extern int DAT_00465872;
extern int DAT_004658c2;
extern int DAT_004658c4;
extern int DAT_004658c6;
extern int DAT_0046591a;
extern int DAT_0046591c;
extern int DAT_0046591e;
extern int DAT_00465920;
extern int DAT_00465924;
extern int DAT_00465926;
extern int DAT_00465980;
extern int DAT_00465a0c;
extern int DAT_00465cb0;
extern int DAT_00465cc8;
extern int DAT_00465ce0;
extern int DAT_00465ce8;
extern int DAT_00465d10;
extern int DAT_00465d48;
extern int DAT_00465d58;
extern int DAT_00465d88;
extern int DAT_00465db0;
extern int DAT_00465dd0;
extern int DAT_00465dd4;
extern int DAT_00465e34;
extern int* DAT_00466290;
extern int DAT_004662f8;
extern int DAT_00466304;
extern int DAT_00466306;
extern int DAT_0046630a;
extern int DAT_0046630b;
extern int DAT_00466310;
extern int DAT_0046631c;
extern int DAT_0046631e;
extern int DAT_00466322;
extern int DAT_00466323;
extern int DAT_00466328;
extern int DAT_00466334;
extern int DAT_00466336;
extern int DAT_0046633a;
extern int DAT_0046633b;
extern int DAT_00466340;
extern int DAT_00466342;
extern int DAT_00466344;
extern int DAT_00466346;
extern int DAT_0046637c;
extern int DAT_00466394;
extern int DAT_004663ac;
extern int DAT_004663c4;
extern int DAT_004663dc;
extern int DAT_004663f4;
extern int DAT_0046640c;
extern int DAT_00466424;
extern int DAT_0046643c;
extern int DAT_00466454;
extern int DAT_0046646c;
extern int DAT_00466484;
extern int DAT_0046649c;
extern int DAT_004664b4;
extern int DAT_004664cc;
extern int DAT_004664fc;
extern int DAT_0046653e;
extern int DAT_00466626;
extern int DAT_0046662a;
extern int DAT_0046662e;
extern int DAT_004666f4;
extern int DAT_00466704;
extern int DAT_00466714;
extern int DAT_00466724;
extern int DAT_00466734;
extern int DAT_00466754;
extern int DAT_0046675d;
extern int DAT_00466772;
extern int DAT_0046677c;
extern int DAT_00466786;
extern int DAT_0046678f;
extern int DAT_00466796;
extern int DAT_0046679f;
extern int DAT_004667a8;
extern int DAT_004667b6;
extern int DAT_004667be;
extern int DAT_004667c4;
extern int DAT_004667c8;
extern int DAT_004667cc;
extern int DAT_004667cf;
extern int DAT_004667d6;
extern int DAT_004667d9;
extern int DAT_004667e5;
extern int DAT_004667e7;
extern int DAT_004667e9;
extern int DAT_004667ec;
extern int DAT_004667f0;
extern int DAT_004667f2;
extern int DAT_004667f5;
extern int DAT_004667fc;
extern int DAT_004667ff;
extern int DAT_0046680a;
extern int DAT_0046680c;
extern int DAT_00466810;
extern int DAT_00466814;
extern int DAT_00466817;
extern int DAT_00466834;
extern int DAT_0046688c;
extern int DAT_004668ec;
extern int DAT_00466934;
extern int DAT_004669b4;
extern int DAT_004669bc;
extern int DAT_00466a20;
extern int DAT_00466a21;
extern int DAT_00466a45;
extern int DAT_00466a4a;
extern int DAT_00466a7d;
extern int DAT_00466a96;
extern int DAT_00466a9a;
extern int DAT_00466a9e;
extern int DAT_00466aa2;
extern int DAT_00466aa6;
extern int DAT_00466aaa;
extern int DAT_00466aae;
extern int DAT_00466ab2;
extern int DAT_00466ab6;
extern int DAT_00466aba;
extern int DAT_00466abe;
extern int DAT_00466ac2;
extern int DAT_00466ac6;
extern int DAT_00466aca;
extern int DAT_00466ace;
extern int DAT_00466ad2;
extern int DAT_00466ad8;
extern int DAT_00466adc;
extern int DAT_00466ae0;
extern int DAT_00466ae4;
extern int DAT_00466b28;
extern int DAT_00466b2c;
extern int DAT_00466b54;
extern int DAT_00466d4e;
extern int DAT_00466d52;
extern int DAT_00466d56;
extern int DAT_00466d5a;
extern int DAT_00466d5e;
extern int DAT_00466d62;
extern int DAT_00466d66;
extern int DAT_00466d6a;
extern int DAT_00466d6e;
extern int DAT_00466d72;
extern int DAT_00466d76;
extern int DAT_00466d7a;
extern int DAT_00466d7e;
extern int DAT_00466d82;
extern int DAT_00466d86;
extern int DAT_00466d8a;
extern int DAT_00466dee;
extern int DAT_00466df2;
extern int DAT_00466df4;
extern int DAT_00466e38;
extern int DAT_00466e40;
extern int DAT_00466e44;
extern int DAT_00466e90;
extern int DAT_00466eac;
extern int DAT_00466ebc;
extern int DAT_00466ee4;
extern int DAT_00466f26;
extern int DAT_00466f27;
extern int DAT_00466f56;
extern int DAT_00466f57;
extern int DAT_00466f5c;
extern int DAT_00466f8c;
extern int DAT_00466fa4;
extern int DAT_00466fec;
extern int DAT_00467004;
extern int DAT_00467060;
extern int DAT_00467064;
extern int DAT_00467074;
extern int DAT_00467078;
extern int DAT_004670a8;
extern int DAT_004670ac;
extern int DAT_004670b0;
extern int DAT_004670b8;
extern int DAT_004670bc;
extern int DAT_004670e0;
extern int DAT_004670e4;
extern int DAT_00467168;
extern int DAT_00467180;
extern int DAT_00467198;
extern int DAT_004671b0;
extern int DAT_004671c8;
extern int DAT_00467224;
extern int DAT_00467390;
extern int DAT_00467394;
extern int DAT_004673ac;
extern int DAT_004673c4;
extern int DAT_004673dc;
extern int DAT_00467498;
extern int DAT_004674e8;
extern int DAT_00467534;
extern int DAT_00467564;
extern int DAT_00467568;
extern int DAT_0046757a;
extern int DAT_0046758c;
extern int DAT_00467658;
extern int DAT_00467660;
extern int DAT_00467794;
extern int DAT_00468094;
extern int DAT_004680ca;
extern int DAT_004680cc;
extern int DAT_004680ce;
extern int DAT_004680d0;
extern int DAT_004680e0;
extern int DAT_004680f0;
extern int DAT_00468100;
extern int DAT_004682f0;
extern int DAT_004685e0;
extern int DAT_00468664;
extern int DAT_0046867c;
extern int DAT_00468694;
extern int DAT_004686ac;
extern int DAT_00468728;
extern int DAT_00468818;
extern int DAT_00468ac8;
extern int DAT_00468ea4;
extern int DAT_00468eb4;
extern int DAT_00468eb8;
extern int DAT_00468ebc;
extern int DAT_00468edc;
extern int DAT_00469120;
extern int DAT_00469220;
extern int DAT_00469274;
extern int DAT_004692b0;
extern int DAT_00469348;
extern int DAT_004693a8;
extern int DAT_00469560;
extern int DAT_00469564;
extern int DAT_004696a4;
extern int DAT_0046974c;
extern int DAT_004697c4;
extern int DAT_004697c8;
extern int DAT_004697ca;
extern int DAT_004697d4;
extern int DAT_004698a4;
extern int DAT_004698a5;
extern int DAT_004698a6;
extern int DAT_004698a7;
extern int DAT_0046996c;
extern int DAT_00469974;
extern int DAT_004699b4;
extern int DAT_004699c9;
extern int DAT_004699ca;
extern int DAT_004699cc;
extern int DAT_004699d2;
extern int DAT_00469efc;
extern int DAT_00469f4c;
extern int DAT_00469f9c;
extern int DAT_0046a110;
extern int DAT_0046a17c;
extern int DAT_0046a1e4;
extern int DAT_0046a1e6;
extern int DAT_0046a208;
extern int DAT_0046a20a;
extern int DAT_0046a2ca;
extern int DAT_0046a308;
extern int DAT_0046a370;
extern int DAT_0046a372;
extern int DAT_0046a3a8;
extern int DAT_0046a3aa;
extern int DAT_0046a444;
extern int DAT_0046a4d0;
extern int DAT_0046a534;
extern int DAT_0046a538;
extern int DAT_0046a53a;
extern int DAT_0046a570;
extern int DAT_0046a571;
extern int DAT_0046a572;
extern int DAT_0046a573;
extern int DAT_0046a672;
extern int DAT_0046a6b8;
extern int DAT_0046a720;
extern int DAT_0046a722;
extern int DAT_0046a76c;
extern int DAT_0046a76e;
extern int DAT_0046a920;
extern int DAT_0046a9b8;
extern int DAT_0046aa3c;
extern int DAT_0046aaa4;
extern int DAT_0046aaa6;
extern int DAT_0046aacc;
extern int DAT_0046ad00;
extern int DAT_0046ae20;
extern int DAT_0046aef8;
extern int DAT_0046af4c;
extern int DAT_0046af4d;
extern int DAT_0046af4e;
extern int DAT_0046af70;
extern int DAT_0046af74;
extern int DAT_0046af76;
extern int DAT_0046afe8;
extern int DAT_0046afe9;
extern int DAT_0046afea;
extern int DAT_0046afeb;
extern int DAT_0046b000;
extern int DAT_0046b001;
extern int DAT_0046b002;
extern int DAT_0046b003;
extern int DAT_0046b6c0;
extern int DAT_0046b74c;
extern int DAT_0046b7a0;
extern int DAT_0046b7a1;
extern int DAT_0046b7a2;
extern int DAT_0046b7c4;
extern int DAT_0046b7c8;
extern int DAT_0046b7ca;
extern int DAT_0046b800;
extern int DAT_0046b801;
extern int DAT_0046b802;
extern int DAT_0046b803;
extern int DAT_0046b824;
extern int DAT_0046b8a8;
extern int DAT_0046be0a;
extern int DAT_0046bf14;
extern int DAT_0046bfe4;
extern int DAT_0046c038;
extern int DAT_0046c039;
extern int DAT_0046c03a;
extern int DAT_0046c05c;
extern int DAT_0046c060;
extern int DAT_0046c062;
extern int DAT_0046c0d4;
extern int DAT_0046c0d5;
extern int DAT_0046c0d6;
extern int DAT_0046c0d7;
extern int DAT_0046c0ec;
extern int DAT_0046c0ed;
extern int DAT_0046c0ee;
extern int DAT_0046c0ef;
extern int DAT_0046c12e;
extern int DAT_0046c162;
extern int DAT_0046c328;
extern code* DAT_0046c380;
extern int DAT_0046c3bc;
extern int DAT_0046c3d8;
extern int DAT_0046c404;
extern int DAT_0046c40c;
extern int DAT_0046c510;
extern int DAT_0046c530;
extern int DAT_0046c534;
extern int DAT_0046c7c4;
extern int DAT_0046c898;
extern int DAT_0046c89c;
extern int DAT_0046c8e8;
extern int DAT_0046c8f8;
extern int DAT_0046c920;
extern int DAT_0046c988;
extern int DAT_0046c98c;
extern int DAT_0046c990;
extern int DAT_0046c994;
extern int DAT_0046c998;
extern int DAT_0046c99c;
extern int DAT_0046c9a0;
extern int DAT_0046c9a4;
extern int DAT_0046c9a8;
extern int DAT_0046c9ac;
extern int DAT_0046c9b0;
extern int DAT_0046c9b4;
extern int DAT_0046c9b8;
extern int DAT_0046c9bc;
extern int DAT_0046cc58;
extern int DAT_0046cc60;
extern int DAT_0046cc90;
extern int DAT_0046cccc;
extern int DAT_0046ccd4;
extern int DAT_0046ccd8;
extern int DAT_0046ccdc;
extern int DAT_0046cd20;
extern int DAT_0046cd34;
extern int DAT_0046cd48;
extern int DAT_0046cdd4;
extern int DAT_0046cdd8;
extern int DAT_0046cddc;
extern int DAT_0046cde0;
extern int DAT_0046cde4;
extern int DAT_0046cef0;
extern int DAT_0046d02c;
extern int DAT_0046d030;
extern int DAT_0046d034;
extern int DAT_0046d03c;
extern int DAT_0046d044;
extern int DAT_0046d060;
extern int DAT_0046d0a4;
extern int DAT_0046d47c;
extern int DAT_0046d4cc;
extern int DAT_0046d594;
extern int DAT_0046d5b8;
extern int DAT_0046d7b8;
extern int DAT_0046da04;
extern int DAT_0046dcc4;
extern int DAT_0046dccc;
extern int DAT_0046dcd4;
extern int DAT_0046e46c;
extern int DAT_0046ecdc;
extern int DAT_0046f020;
extern int DAT_0046f2ac;
extern int DAT_0046f2b4;
extern int DAT_0046f33c;
extern int DAT_0046f658;
extern int DAT_0046f660;
extern int DAT_0046f6c4;
extern int DAT_0046f88c;
extern int DAT_0046f894;
extern int DAT_0046f930;
extern int DAT_0046fa08;
extern int DAT_0046fc94;
extern int DAT_0046fc98;
extern int DAT_0046fca4;
extern int DAT_0046fca8;
extern int DAT_0046fcac;
extern int DAT_0046fda9;
extern int DAT_0046fec4;
extern int DAT_0046fec8;
extern int DAT_0046fecc;
extern int DAT_0046fed0;
extern int DAT_0046fed8;
extern int DAT_0046fede;
extern int DAT_0046fee2;
extern int DAT_0046ff2c;
extern int DAT_0046ff48;
extern int DAT_0046ff60;
extern int DAT_00480018;
extern int DAT_0048001c;
extern int DAT_00480034;
extern int DAT_00480038;
extern int DAT_00700050;
extern int DAT_00713161;
extern int DAT_0071331d;
extern int DAT_00713850;
extern int DAT_0071391d;
extern int DAT_00714058;
extern int DAT_00716b18;
extern int DAT_00716d20;
extern int DAT_00716d24;
extern int DAT_00716d28;
extern int DAT_00716d2a;
extern int DAT_00716d2c;
extern int DAT_00716d2e;
extern int DAT_00716d30;
extern int DAT_00716d32;
extern int DAT_00716d9c;
extern int DAT_00716dc4;
extern int DAT_00716dc8;
extern int DAT_0071be4e;
extern int DAT_0071beee;
extern int DAT_0071bfa0;
extern int DAT_0071bfd0;
extern int DAT_0071c050;
extern int DAT_0071c051;
extern int DAT_0073c07a;
extern int DAT_0073c084;
extern int DAT_0073c086;
extern int DAT_0073c2c0;
extern int DAT_0073c2fa;
extern int DAT_0073c2fc;
extern int DAT_0073c348;
extern int DAT_00744b00;
extern int DAT_00744b37;
extern int DAT_00744b38;
extern int DAT_00744b3a;
extern int DAT_00744de0;
extern int DAT_0074505c;
extern int DAT_0074a6d7;
extern int DAT_0074be98;
extern int DAT_00750ea4;
extern int DAT_0075211c;
extern int DAT_0075211e;
extern int DAT_00752120;
extern int DAT_00752344;
extern int DAT_00752348;
extern int DAT_0075234c;
extern int DAT_00752350;
extern int DAT_00752390;
extern int DAT_00752392;
extern int DAT_00759794;
extern int DAT_007597f0;
extern int DAT_007597f8;
extern int DAT_0075a044;
extern int DAT_0075a0e4;
extern int DAT_0075a0e8;
extern int DAT_0075a0f4;
extern int DAT_0075a0fe;
extern int DAT_0075a10a;
extern int DAT_0075a10c;
extern int DAT_0075a136;
extern int DAT_0075a138;
extern int DAT_0075a162;
extern int DAT_0075a164;
extern int DAT_0075a18e;
extern int DAT_0075a190;
extern int DAT_0075a1ba;
extern int DAT_0075a1bc;
extern int DAT_0075a1e6;
extern int DAT_0075a1e8;
extern int DAT_0075a212;
extern int DAT_0075a214;
extern int DAT_0075a23e;
extern int DAT_0075a294;
extern int DAT_0075a2a4;
extern int DAT_0075a2b0;
extern int DAT_0075a610;
extern int DAT_0075a614;
extern int DAT_0075a618;
extern int DAT_0075a630;
extern int DAT_0075a638;
extern int DAT_0075a676;
extern int DAT_0075a67a;
extern int DAT_0075a686;
extern int DAT_0075a692;
extern int DAT_0075a6c6;
extern int DAT_0075a71a;
extern int DAT_0075a7a2;
extern int DAT_0075a7ae;
extern int DAT_0075d846;
extern int DAT_0075d852;
extern int DAT_0075d9d8;
extern int DAT_0075ec00;
extern int DAT_0075f026;
extern int DAT_00900ee4;
extern int DAT_00900f74;
extern int DAT_00901010;
extern int DAT_00901f10;
extern int DAT_00901f14;
extern int DAT_00901f9c;
extern int DAT_00901fc4;
extern int DAT_00901fc8;
extern int DAT_00901fc9;
extern int DAT_00901fca;
extern int DAT_00901fec;
extern int DAT_00902014;
extern int DAT_0090203c;
extern int DAT_00902064;
extern int DAT_00902090;
extern int DAT_009020a2;
extern int DAT_00903ca2;
extern int DAT_00905a74;
extern int DAT_00905a77;
extern int DAT_00905a98;
extern int DAT_00905a9b;
extern int DAT_00905af2;
extern int DAT_00905af4;
extern int DAT_009063b1;
extern int DAT_009063b2;
extern int DAT_009063b3;
extern int DAT_009063b4;
extern int DAT_009063b4_1;
extern int DAT_009063b8;
extern int DAT_009063b8_1;
extern int DAT_0090676c;
extern int DAT_00907930;
extern int DAT_00907940;
extern int DAT_00907990;
extern int DAT_00907993;
extern int DAT_00907995;
extern int DAT_00907998;
extern int DAT_00907999;
extern int DAT_0090799c;
extern int DAT_0090799d;
extern int DAT_00907c00;
extern int DAT_00908574;
extern int DAT_009085a8;
extern int DAT_009086f8;
extern int DAT_5af34e72;
extern int LAB_0041341c;
extern int LAB_004239b0;
extern int LAB_00445670;
extern int LAB_00453a40;
extern int LAB_00456bd3;
extern int LAB_004576de;
extern int LAB_004576e7;
extern int PTR_DAT_0044e200;
extern int PTR_DAT_0044e204;
extern int PTR_DAT_0044e208;
extern int PTR_DAT_0044e20c;
extern int PTR_DAT_0044e210;
extern int PTR_DAT_0044e214;
extern int PTR_DAT_0044e218;
extern int PTR_DAT_0044e21c;
extern int PTR_DAT_0044e220;
extern int PTR_DAT_0044e224;
extern int PTR_DAT_0044e228;
extern int PTR_DAT_0044e22c;
extern int PTR_DAT_0044e230;
extern int PTR_DAT_0044e234;
extern int PTR_DAT_0044e238;
extern int PTR_DAT_004651bc;
extern int PTR_DAT_00467234;
extern int PTR_DAT_00467248;
extern int PTR_DAT_0046725c;
extern int PTR_DAT_00467270;
extern int PTR_DAT_00467284;
extern int PTR_DAT_00467298;
extern int PTR_DAT_004672ac;
extern int PTR_DAT_004672c0;
extern int PTR_DAT_00467520;
extern int PTR_DAT_004683e8;
extern int PTR_DAT_00468b00;
extern int PTR_DAT_00468b14;
extern int PTR_DAT_00468ddc;
extern int PTR_DAT_00468df0;
extern int PTR_DAT_00469158;
extern int PTR_DAT_0046916c;
extern int PTR_DAT_004693e0;
extern int PTR_DAT_0046975c;
extern int PTR_DAT_00469770;
extern int PTR_DAT_00469784;
extern int PTR_DAT_00469798;
extern int PTR_DAT_004697ac;
extern int PTR_DAT_00469b44;
extern int PTR_DAT_00469b58;
extern int PTR_DAT_00469d64;
extern int PTR_DAT_00469d78;
extern int* PTR_DAT_00469fd4;
extern int* PTR_DAT_00469fe8;
extern int PTR_DAT_00469ffc;
extern int PTR_DAT_0046a010;
extern int PTR_DAT_0046a024;
extern int PTR_DAT_0046a1b4;
extern int PTR_DAT_0046a1c8;
extern int PTR_DAT_0046a340;
extern int PTR_DAT_0046a354;
extern int PTR_DAT_0046a508;
extern int PTR_DAT_0046a51c;
extern int PTR_DAT_0046a6f0;
extern int PTR_DAT_0046a704;
extern int PTR_DAT_0046a8f4;
extern int PTR_DAT_0046a908;
extern int PTR_DAT_0046aa74;
extern int PTR_DAT_0046aa88;
extern int PTR_DAT_0046ab80;
extern int PTR_DAT_0046af08;
extern int PTR_DAT_0046af1c;
extern int PTR_DAT_0046af30;
extern int PTR_DAT_0046af44;
extern int PTR_DAT_0046af58;
extern int PTR_DAT_0046b784;
extern int PTR_DAT_0046b798;
extern int PTR_DAT_0046b7ac;
extern int PTR_DAT_0046c01c;
extern int PTR_DAT_0046c030;
extern int PTR_DAT_0046c044;
extern int* PTR_DAT_0046c47c;
extern int PTR_FUN_004697cc;
extern int PTR_FUN_004697d0;
extern int PTR_FUN_0046a1e8;
extern int PTR_FUN_0046a1ec;
extern int PTR_FUN_0046a374;
extern int PTR_FUN_0046a378;
extern int PTR_FUN_0046a53c;
extern int PTR_FUN_0046a540;
extern int PTR_FUN_0046a724;
extern int PTR_FUN_0046aaa8;
extern int PTR_FUN_0046af78;
extern int PTR_FUN_0046af7c;
extern int PTR_FUN_0046b7cc;
extern int PTR_FUN_0046b7d0;
extern int PTR_FUN_0046c064;
extern int PTR_FUN_0046c068;
extern int PTR_FUN_0046c344;
extern int PTR_FUN_0046c4f8;
extern int PTR_FUN_0046c50c;
extern int PTR_FUN_0046c524;
extern int PTR_FUN_0046c528;
extern int PTR_LAB_00426740;
extern int PTR_LAB_00426764;
extern int PTR_LAB_0042a4c0;
extern int PTR_LAB_00462ef4;
extern int PTR_LAB_00467084;
extern int PTR_LAB_0046af90;
extern int PTR_LAB_0046c07c;
extern int PTR_LAB_0046c4f0;
extern int PTR_Select_Champ_0046a728;
extern int PTR___CBeginThread_0046c520;
extern int PTR___matherr_0046c598;
extern int PTR_draw_face_3pt_flat_00462d94;
extern int PTR_draw_face_3pt_flat_00462e44;
extern int PTR_hlf_transparency_table_00460014;
extern int PTR_s_CHAMPP_0046a6a8;
extern int PTR_s_CHAMP_0046a626;
extern int PTR_s_CHAMP_0046a698;
extern int PTR_s_DUMB1_004650dc;
extern int PTR_s_DUMB2_00465100;
extern int PTR_s_DUMB3_00465124;
extern int PTR_s_KEYBOARD_0046a136;
extern int PTR_s_KEYBOARD_0046a16c;
extern int PTR_s_KEYBOARP_0046a174;
extern int PTR_s_MEMLOADP_004672e4;
extern int PTR_s_MEMLOAD_004671ee;
extern int PTR_s_MEMLOAD_004672d8;
extern int PTR_s_MEMSAVE_004672dc;
extern int PTR_s_RACETYPE_004696b2;
extern int PTR_s_RACETYPE_00469864;
extern int PTR_s_RACTYPEP_00469884;
extern int PTR_s_RESULTSP_0046aee0;
extern int PTR_s_RESULTSP_0046bfcc;
extern int PTR_s_RESULTS_0046ae46;
extern int PTR_s_RESULTS_0046aec8;
extern int PTR_s_RESULTS_0046bf3a;
extern int PTR_s_RESULTS_0046bfb4;
extern int PTR_s_VIEWREPP_0046a4c4;
extern int PTR_s_VIEWREPP_0046b740;
extern int PTR_s_VIEWREP_0046a46a;
extern int PTR_s_VIEWREP_0046a4b8;
extern int PTR_s_VIEWREP_0046b6e6;
extern int PTR_s_VIEWREP_0046b734;
extern int PTR_s_WRECKINP_0046a2fc;
extern int PTR_s_WRECKIN_0046a28a;
extern int PTR_s_WRECKIN_0046a2f0;
extern int PTR_s__R_JC_T_Practice_004698d0;
extern int PTR_s__R_JC_T_Wrecking_Racing_004698c4;
extern int PTR_s__R_JL_T_Jug_00469df0;
extern int PTR_s__R_JL_T_Liberty_City_00469dc8;
extern int PTR_s__R_JL_T_Pine_Hills_Raceway_004682f8;
extern int PTR_s__R_JL_T_Pine_Hills_Raceway_00469314;
extern int PTR_s__R_JL_T_Pine_Hills_Raceway_00469bc4;
extern int PTR_s__R_JL_T_Pork_Sword_00469ddc;
extern int PTR_s__R_JL_T_Slapshot_00469c0c;
extern int SCA_Corner_Data_;
extern int Speedway_Track_Type_;
extern int UNK_00458895;
extern int _DAT_00460430;
extern int _DAT_00460450;
extern int _DAT_00462d7c;
extern int _DAT_00462fa8;
extern int _DAT_00462fac;
extern int _DAT_00462fae;
extern int _DAT_00462fb0;
extern int _DAT_00462fb2;
extern int _DAT_00462fb4;
extern int _DAT_00462fd4;
extern int _DAT_00463000;
extern int _DAT_0046303a;
extern int _DAT_0046303c;
extern int _DAT_00463ef4;
extern int _DAT_00465836;
extern int _DAT_00465838;
extern int _DAT_0046583a;
extern int _DAT_0046583c;
extern int _DAT_0046583e;
extern int _DAT_00465840;
extern int _DAT_00465842;
extern int _DAT_00465844;
extern int _DAT_0046585a;
extern int _DAT_0046585c;
extern int _DAT_0046585e;
extern int _DAT_00465860;
extern int _DAT_00465862;
extern int _DAT_00465864;
extern int _DAT_00467050;
extern int _DAT_00467078;
extern int _DAT_00467080;
extern int _DAT_00467150;
extern int _DAT_00467152;
extern int _DAT_0046716a;
extern int _DAT_00467182;
extern int _DAT_00467198;
extern int _DAT_0046719a;
extern int _DAT_004671b2;
extern int _DAT_004671ce;
extern int _DAT_004671da;
extern int _DAT_004672a4;
extern int _DAT_004672a6;
extern int _DAT_00467396;
extern int _DAT_004673ae;
extern int _DAT_004673c6;
extern int _DAT_004673dc;
extern int _DAT_004673de;
extern int _DAT_00467660;
extern int _DAT_004682f0;
extern int _DAT_00469568;
extern int _DAT_0046965c;
extern int _DAT_0046965e;
extern int _DAT_00469674;
extern int _DAT_00469676;
extern int _DAT_0046968c;
extern int _DAT_0046968e;
extern int _DAT_0046996e;
extern int _DAT_00469970;
extern int _DAT_00469f34;
extern int _DAT_00469f36;
extern int _DAT_0046a0e0;
extern int _DAT_0046a0e2;
extern int _DAT_0046a258;
extern int _DAT_0046a25a;
extern int _DAT_0046a3e4;
extern int _DAT_0046a3e6;
extern int _DAT_0046a3fc;
extern int _DAT_0046a3fe;
extern int _DAT_0046a5dc;
extern int _DAT_0046a5de;
extern int _DAT_0046a958;
extern int _DAT_0046a95a;
extern int _DAT_0046a970;
extern int _DAT_0046a972;
extern int _DAT_0046a9a0;
extern int _DAT_0046a9a2;
extern int _DAT_0046ad90;
extern int _DAT_0046ad92;
extern int _DAT_0046ada8;
extern int _DAT_0046adaa;
extern int _DAT_0046adf0;
extern int _DAT_0046adf2;
extern int _DAT_0046ae08;
extern int _DAT_0046ae0a;
extern int _DAT_0046ae26;
extern int _DAT_0046ae32;
extern int _DAT_0046b660;
extern int _DAT_0046b662;
extern int _DAT_0046b678;
extern int _DAT_0046b67a;
extern int _DAT_0046be0e;
extern int _DAT_0046be10;
extern int _DAT_0046be9c;
extern int _DAT_0046be9e;
extern int _DAT_0046beb4;
extern int _DAT_0046beb6;
extern int _DAT_0046befc;
extern int _DAT_0046befe;
extern int _DAT_0046c3f1;
extern int _DAT_0046c84c;
extern int _DAT_0046fc7c;
extern int _DAT_0046feb4;
extern int _DAT_0046febc;
extern int _DAT_0046ff84;
extern int _DAT_00714054;
extern int _DAT_00714058;
extern int _DAT_0071405c;
extern int _DAT_0071406c;
extern int _DAT_0071407c;
extern int _DAT_007140c0;
extern int _DAT_007140c4;
extern int _DAT_007140e4;
extern int _DAT_007140e8;
extern int _DAT_007140ec;
extern int _DAT_007140f4;
extern int _DAT_007140f8;
extern int _DAT_007140fc;
extern int _DAT_007140fe;
extern int _DAT_00714102;
extern int _DAT_00714104;
extern int _DAT_00714106;
extern int _DAT_00714108;
extern int _DAT_0071410c;
extern int _DAT_00714114;
extern int _DAT_00714118;
extern int _DAT_0071411c;
extern int _DAT_00714120;
extern int _DAT_00714124;
extern int _DAT_00714128;
extern int _DAT_0071412c;
extern int _DAT_00714130;
extern int _DAT_00714134;
extern int _DAT_00714138;
extern int _DAT_0071413c;
extern int _DAT_00714140;
extern int _DAT_00714144;
extern int _DAT_00714148;
extern int _DAT_0071414c;
extern int _DAT_00714150;
extern int _DAT_00714154;
extern int _DAT_00714158;
extern int _DAT_0071415c;
extern int _DAT_00714164;
extern int _DAT_00714168;
extern int _DAT_00714174;
extern int _DAT_00714178;
extern int _DAT_00714184;
extern int _DAT_00714188;
extern int _DAT_007142d4;
extern int _DAT_007142f2;
extern int _DAT_007142f4;
extern int _DAT_007142f6;
extern int _DAT_007142f8;
extern int _DAT_007142fa;
extern int _DAT_007142fc;
extern int _DAT_007142fe;
extern int _DAT_00714300;
extern int _DAT_00714302;
extern int _DAT_00714306;
extern int _DAT_0071430a;
extern int _DAT_00716d74;
extern int _DAT_00716d78;
extern int _DAT_00716d80;
extern int _DAT_00716d84;
extern int _DAT_00716d94;
extern int* _DAT_00716d9c;
extern int _DAT_0071bdc0;
extern int _DAT_0071bdc4;
extern int _DAT_0071bde6;
extern int _DAT_0071bdea;
extern int _DAT_0071bdec;
extern int _DAT_0071bdf2;
extern int _DAT_0071bdf4;
extern int _DAT_0071bdfe;
extern int _DAT_0071be00;
extern int _DAT_0071be02;
extern int _DAT_0071be04;
extern int _DAT_0071be06;
extern int _DAT_0071be08;
extern int _DAT_0071be0a;
extern int _DAT_0071be0c;
extern int _DAT_0071be1e;
extern int _DAT_0071be20;
extern int _DAT_0071be22;
extern int _DAT_0071be24;
extern int _DAT_0071be26;
extern int _DAT_0071be28;
extern int _DAT_0071be2a;
extern int _DAT_0071be2c;
extern int _DAT_0071be4e;
extern int _DAT_0071be52;
extern int _DAT_0071be56;
extern int _DAT_0071beee;
extern int _DAT_0071bf7c;
extern int _DAT_0071bf94;
extern int _DAT_0071bf98;
extern int _DAT_0071bfa0;
extern int _DAT_0071bfa4;
extern int _DAT_0071bfac;
extern int _DAT_0071bfb0;
extern int _DAT_0071bff8;
extern int _DAT_0071bffc;
extern int _DAT_0071c000;
extern int _DAT_0071c004;
extern int _DAT_0071c008;
extern int _DAT_0071c00c;
extern int _DAT_0071c030;
extern int _DAT_0071c034;
extern int _DAT_0071c038;
extern int _DAT_0071c03c;
extern int _DAT_0071c040;
extern int _DAT_0071c044;
extern int _DAT_0071c048;
extern int _DAT_0071c04a;
extern int _DAT_0071c04c;
extern int _DAT_0071c04e;
extern int _DAT_0071c050;
extern int _DAT_0073c060;
extern int _DAT_0073c280;
extern int* _DAT_0073c290;
extern int _DAT_0073c2c0;
extern int _DAT_0073c2fa;
extern int _DAT_0073c2fc;
extern int _DAT_0073c300;
extern int _DAT_0073c302;
extern int _DAT_0073c304;
extern int _DAT_0073c348;
extern int _DAT_0073c34a;
extern int _DAT_0073c34c;
extern int _DAT_0073c350;
extern int _DAT_0073c352;
extern int _DAT_0073c354;
extern int _DAT_0073c358;
extern int _DAT_0073c35a;
extern int _DAT_0073c35c;
extern int _DAT_0073c3c4;
extern int _DAT_0073c3c6;
extern int _DAT_0073c3c8;
extern int _DAT_0073c3cc;
extern int _DAT_0073c3ce;
extern int _DAT_0073c3d0;
extern int _DAT_0073c3d4;
extern int _DAT_0073c3d6;
extern int _DAT_0073c3d8;
extern int _DAT_00744344;
extern int _DAT_0074434c;
extern int _DAT_00744358;
extern int _DAT_00744ad8;
extern int _DAT_00744adc;
extern int _DAT_00744b00;
extern int _DAT_00744b04;
extern int _DAT_00744b08;
extern int _DAT_00744b0c;
extern int _DAT_00744b14;
extern int _DAT_00744b18;
extern int _DAT_00744b1c;
extern int _DAT_00744b24;
extern int _DAT_00744b34;
extern int _DAT_00744b40;
extern int _DAT_00744b44;
extern int _DAT_00744b74;
extern int _DAT_00744de0;
extern int _DAT_00744de4;
extern int _DAT_00744de8;
extern int _DAT_00744dec;
extern int _DAT_00744df0;
extern int _DAT_00744df4;
extern int _DAT_0074948c;
extern int _DAT_0074949c;
extern int _DAT_007494ac;
extern int _DAT_007494bc;
extern int _DAT_007494d4;
extern int _DAT_007494fc;
extern int _DAT_00749514;
extern int _DAT_0074952c;
extern int _DAT_0074953c;
extern int _DAT_00749a60;
extern int _DAT_00749a8c;
extern int _DAT_00749ab0;
extern int _DAT_00749ad4;
extern int _DAT_00749af8;
extern int _DAT_00749b4c;
extern int _DAT_0074a6d8;
extern int _DAT_0074a6da;
extern int _DAT_0074a6e0;
extern int _DAT_0074a6e2;
extern int _DAT_0074a6e6;
extern int _DAT_0074a6e8;
extern int _DAT_0074a6ea;
extern int _DAT_0074be90;
extern int _DAT_0074be94;
extern int _DAT_0074be98;
extern int _DAT_00750f4c;
extern int _DAT_00750f54;
extern int _DAT_00750f56;
extern int _DAT_00751d5c;
extern int _DAT_00751f60;
extern int* _DAT_00751f78;
extern int _DAT_00751f7c;
extern int _DAT_0075211c;
extern int _DAT_00752344;
extern int _DAT_00752348;
extern int _DAT_0075234c;
extern int _DAT_00752390;
extern int _DAT_00752392;
extern int _DAT_00759794;
extern int _DAT_00759798;
extern int _DAT_007597a4;
extern int _DAT_007597a8;
extern int _DAT_007597ae;
extern int _DAT_007597b4;
extern int _DAT_007597b6;
extern int _DAT_007597bc;
extern int _DAT_007597be;
extern int _DAT_007597c4;
extern int _DAT_007597c6;
extern int _DAT_007597cc;
extern int _DAT_007597ce;
extern int _DAT_007597d4;
extern int _DAT_007597d6;
extern int _DAT_007597dc;
extern int _DAT_007597de;
extern int _DAT_007597e4;
extern int _DAT_007597e6;
extern int _DAT_007597ec;
extern int _DAT_007597f0;
extern int _DAT_007597f8;
extern int _DAT_00759800;
extern int _DAT_00759808;
extern int _DAT_00759826;
extern int _DAT_0075a00e;
extern int _DAT_0075a038;
extern int _DAT_0075a040;
extern int _DAT_0075a044;
extern int _DAT_0075a048;
extern int _DAT_0075a04c;
extern int _DAT_0075a050;
extern int _DAT_0075a054;
extern int _DAT_0075a058;
extern int _DAT_0075a05c;
extern int _DAT_0075a060;
extern int _DAT_0075a064;
extern int _DAT_0075a068;
extern int _DAT_0075a06c;
extern int _DAT_0075a070;
extern int _DAT_0075a074;
extern int _DAT_0075a078;
extern int _DAT_0075a07c;
extern int _DAT_0075a080;
extern int _DAT_0075a084;
extern int _DAT_0075a088;
extern int _DAT_0075a08c;
extern int _DAT_0075a090;
extern int _DAT_0075a094;
extern int _DAT_0075a098;
extern int _DAT_0075a09c;
extern int _DAT_0075a0a0;
extern int _DAT_0075a0a4;
extern int _DAT_0075a0a8;
extern int _DAT_0075a0ac;
extern int _DAT_0075a0b0;
extern int _DAT_0075a0b4;
extern int _DAT_0075a0b8;
extern int _DAT_0075a0bc;
extern int _DAT_0075a0e4;
extern int _DAT_0075a0e8;
extern int _DAT_0075a0ec;
extern int _DAT_0075a0f4;
extern int _DAT_0075a0fe;
extern int _DAT_0075a10c;
extern int _DAT_0075a110;
extern int _DAT_0075a114;
extern int _DAT_0075a118;
extern int _DAT_0075a120;
extern int _DAT_0075a12a;
extern int _DAT_0075a138;
extern int _DAT_0075a13c;
extern int _DAT_0075a140;
extern int _DAT_0075a144;
extern int _DAT_0075a14c;
extern int _DAT_0075a156;
extern int _DAT_0075a164;
extern int _DAT_0075a168;
extern int _DAT_0075a16c;
extern int _DAT_0075a170;
extern int _DAT_0075a178;
extern int _DAT_0075a182;
extern int _DAT_0075a190;
extern int _DAT_0075a194;
extern int _DAT_0075a198;
extern int _DAT_0075a19c;
extern int _DAT_0075a1a4;
extern int _DAT_0075a1ae;
extern int _DAT_0075a1bc;
extern int _DAT_0075a1c0;
extern int _DAT_0075a1c4;
extern int _DAT_0075a1c8;
extern int _DAT_0075a1d0;
extern int _DAT_0075a1da;
extern int _DAT_0075a1e8;
extern int _DAT_0075a1ec;
extern int _DAT_0075a1f0;
extern int _DAT_0075a1f4;
extern int _DAT_0075a1fc;
extern int _DAT_0075a206;
extern int _DAT_0075a214;
extern int _DAT_0075a218;
extern int _DAT_0075a21c;
extern int _DAT_0075a220;
extern int _DAT_0075a228;
extern int _DAT_0075a232;
extern int _DAT_0075a2a4;
extern int _DAT_0075a2b0;
extern int _DAT_0075a610;
extern int _DAT_0075a614;
extern int _DAT_0075a618;
extern int _DAT_0075a630;
extern int _DAT_0075a638;
extern int _DAT_0075a676;
extern int _DAT_0075a67a;
extern int _DAT_0075a686;
extern int _DAT_0075a692;
extern int _DAT_0075a6c6;
extern int _DAT_0075a71a;
extern int _DAT_0075a7a2;
extern int _DAT_0075a7e2;
extern int _DAT_0075a8cc;
extern int _DAT_0075a99c;
extern int _DAT_0075d846;
extern int _DAT_0075d852;
extern int _DAT_0075d9d8;
extern int _DAT_0075d9e0;
extern int _DAT_0075d9e4;
extern int _DAT_0075d9ec;
extern int _DAT_0075d9f0;
extern int _DAT_00900eb8;
extern int* _DAT_00900ec4;
extern int _DAT_00900ee4;
extern int _DAT_00901768;
extern int _DAT_00901780;
extern int _DAT_00901784;
extern int _DAT_00901f08;
extern int _DAT_00901f0c;
extern int _DAT_00901f10;
extern int _DAT_00901f14;
extern int _DAT_00901f18;
extern int _DAT_00901f9c;
extern int _DAT_00901fc4;
extern int _DAT_00901fec;
extern int _DAT_00902014;
extern int _DAT_00902090;
extern int _DAT_00902092;
extern int _DAT_00902094;
extern int _DAT_00902096;
extern int _DAT_00902098;
extern int _DAT_0090209a;
extern int _DAT_0090209c;
extern int _DAT_0090209e;
extern int _DAT_009020a0;
extern int _DAT_00905a10;
extern int _DAT_00905a14;
extern int _DAT_00905a18;
extern int _DAT_00905a1c;
extern int _DAT_00905a78;
extern int _DAT_00905a7a;
extern int _DAT_00905a80;
extern int _DAT_00905a82;
extern int _DAT_00905a88;
extern int _DAT_00905a8a;
extern int _DAT_00905a90;
extern int _DAT_00905a92;
extern int _DAT_00905a9c;
extern int _DAT_00905a9e;
extern int _DAT_00905aa4;
extern int _DAT_00905aa6;
extern int _DAT_00905aac;
extern int _DAT_00905aae;
extern int _DAT_00905ab4;
extern int _DAT_00905ab6;
extern int _DAT_00905ac4;
extern int _DAT_00905af2;
extern int _DAT_00905af4;
extern int _DAT_009077f0;
extern int _DAT_00907800;
extern int _DAT_00907818;
extern int _DAT_00907828;
extern int _DAT_00907840;
extern int _DAT_00907850;
extern int _DAT_00907868;
extern int _DAT_00907878;
extern int _DAT_00907920;
extern int _DAT_00907940;
extern int _DAT_00907944;
extern int _DAT_00907948;
extern int _DAT_00907c00;
extern int _DAT_00907c04;
extern int _DAT_00907e20;
extern int _DAT_00908570;
extern int* _DAT_00908574;
extern int _DAT_0090858c;
extern int* _DAT_00908590;
extern int _DAT_009085a8;
extern int _DAT_009085ac;
extern int _DAT_009086f8;
extern int _DAT_009086fc;
extern int _DAT_00908700;
extern int _DAT_00908704;
extern int _DummyPoly;
extern int _FirstTime;
extern int _Lap_Timer;
extern int _Last_Lap_Timer;
extern int _Now_Timing_Lap;
extern int _Old_Cam_Mode;
extern int _Replay_Invalid;
extern int _Replay_Level;
extern int _Replay_Script;
extern int* _Replay_Script_Ptr;
extern int _Timing_Delay;
extern int _Z_DISTANCE;
extern int* __LpCmdLine;
extern int __LpDllName;
extern int __LpPgmName;
extern code* __WinMainProc;
extern int ___ASTACKPTR_;
extern int ___FirstThreadData;
extern int ___Is_DLL;
extern int ___OpenStreams;
extern int __bcrgb;
extern int __clutspace;
extern int __cmptr;
extern int __fcrgb;
extern int __flg;
extern int __globmat;
extern int* __lmptr;
extern int __op0;
extern int __opvr0;
extern int __opvr1;
extern int __opz;
extern int __otz;
extern int __rgb0;
extern int __scrx;
extern int __scry;
extern int __texturespace;
extern int __vr0;
extern int __vr1;
extern int __vr2;
extern int __vr3;
extern int _active_block_numbers;
extern int _actual_season_number;
extern int _add_transparency_table;
extern int _adjusted_music;
extern int _adjusted_sfx;
extern int _applause;
extern int _applause_up_;
extern int _boot_objects_count;
extern int _bootoff_index;
extern int _camera_collision;
extern int _camera_fd;
extern int _camera_fd_pt;
extern int _car0_being_obstructed;
extern int _car_info;
extern int _cars_in_crash;
extern int _cdb_;
extern int _commentating;
extern int _corner_fd;
extern int _crowd_volume;
extern int _current_level;
extern int _current_player;
extern int _current_player_car;
extern int _current_race;
extern int _current_season;
extern int _dec_info;
extern int _decrunch_block;
extern int _decrunch_flag;
extern int _draw_frame;
extern int _dth_shade;
extern int _euphoria;
extern int* _fi_levdat;
extern int _flag;
extern int _flame_frame_count;
extern int _flash1_frame_count;
extern int _flash2_frame_count;
extern int _flash3_frame_count;
extern int _floaty_camera_fd;
extern int _frame_skip;
extern int _frame_wait;
extern int _frames_per_sec;
extern int _free_mem;
extern int _fx;
extern int _fx_1;
extern int _fx_10;
extern int _fx_11;
extern int _fx_15;
extern int _fx_17;
extern int _fx_18;
extern int _fx_2;
extern int _fx_21;
extern int _fx_22;
extern int _fx_3;
extern int _fx_4;
extern int _fx_5;
extern int _fx_6;
extern int _fx_7;
extern int _fx_8;
extern int _fx_9;
extern int _g_sprite_info;
extern int _gnormals;
extern int* _gpoly;
extern int* _gprim1;
extern int _gprim2;
extern int _grounded_count;
extern int _gtexture;
extern int _gtexture_def;
extern int _h_norm;
extern int _highlight_colour;
extern int _hlf_transparency_table;
extern int _last_time;
extern int* _level_data;
extern int _light_matrix;
extern int _local_1e;
extern int _local_24;
extern int _local_26;
extern int _mem_size;
extern int _num_races;
extern int _num_spies;
extern int _num_strips;
extern int _old_flying_index;
extern int _otsize;
extern int _pad_option;
extern int _permission;
extern int _playable_bowls;
extern int _playable_tracks_;
extern int _polygon_angles;
extern int* _prim_buf;
extern int _quit_flag;
extern int _race_finished;
extern int _recorded_pad_type;
extern int _scene_colour_matrix_;
extern int _screen_centre_x_;
extern int* _screen_line_list;
extern int _screen_poly_list;
extern int _sky_shape1;
extern int _sky_shape2;
extern int _sky_shape3;
extern int _sky_shape4;
extern int _sky_shape5;
extern int _sky_shape6;
extern int _sky_shape7;
extern int _sky_shape8;
extern int _sound_volume;
extern int _sprite_matrix;
extern int _stats_recorded;
extern int _strip_data;
extern int _strip_vertex;
extern int _sub_transparency_table;
extern int _surfai;
extern int _tilt_sprite_matrix;
extern int _tot_time;
extern int _total_collisions;
extern int _total_dest_timer;
extern int _track_height;
extern int _u;
extern int _v_norm;
extern int _value;
extern int _wheeloff_index;
extern int _whllck;
extern int _yes_quit;
extern int _yes_retire;
extern int fog_col_;
extern int iRam00749038;
extern int pHVar1;
extern int pHVar6;
extern int playable_tracks_;
extern int recorded_strips_;
extern int sRam00465932;
extern int sRam00465934;
extern int sRam00465936;
extern int sRam00465942;
extern int sRam00465944;
extern int sRam00465946;
extern int sRam0046956a;
extern int sRam0046be12;
extern int sRam0071410a;
extern int sRam007597b8;
extern int sRam007597c0;
extern int sRam007597c8;
extern int sRam007597d0;
extern int sRam007597d8;
extern int sRam007597e0;
extern int sRam007597e8;
extern int* s_ABCDEFGHIJKLMNOPQRSTUVWXYZ012345_004692ec;
extern int s_ABNORMAL_TERMINATION_0046fc3c;
extern int s_AIDAN_0046da74;
extern int s_AIDCRED_0046daec;
extern int s_Add_Buffer_Load__0046c77c;
extern int* s_Anonymous_0046ecf0;
extern int s_BACKD1_0046db94;
extern int s_BACKD2_0046db9c;
extern int s_BACKSPCE_0046d42c;
extern int s_BKWN88A_0046ce68;
extern int s_BKWN88B_0046ce70;
extern int s_BKWN88C_0046ce78;
extern int s_BKWN88D_0046ce80;
extern int s_BKWN88E_0046ce88;
extern int s_BON88A_0046c934;
extern int s_BON88A_0046ce90;
extern int s_BON88B_0046c93c;
extern int s_BON88B_0046ce98;
extern int s_BON88C_0046cea0;
extern int s_BOOT88A_0046ced8;
extern int s_BOOT88B_0046cee0;
extern int s_BOOT88C_0046cee8;
extern int s_BUMP88A_0046ceb0;
extern int s_BUMP88B_0046ceb8;
extern int s_BUMP88C_0046cec0;
extern int s_BUMP88D_0046cec8;
extern int s_BUMP88E_0046ced0;
extern int s_Buffer_Load__0046c7b4;
extern int s_CAMCORDR_0046d540;
extern int s_CARDCROS_0046d2f0;
extern int s_CARDCROS_0046e778;
extern int s_CARDCROS_0046f47c;
extern int s_CARDCROS_0046f724;
extern int s_CARDCROS_0046f9d8;
extern int s_CARDCURS_0046d440;
extern int s_CARDTEXT_0046d420;
extern int s_CARDTICK_0046d2e4;
extern int s_CARDTICK_0046e76c;
extern int s_CARDTICK_0046f470;
extern int s_CARDTICK_0046f718;
extern int s_CARDTICK_0046f9cc;
extern int s_CARD_ERR_0046d174;
extern int s_CARD_ERR_0046e758;
extern int s_CARD_ERR_0046f45c;
extern int s_CARD_ERR_0046f704;
extern int s_CARD_ERR_0046f9b8;
extern int s_CARD_YN_0046d180;
extern int s_CARD_YN_0046e764;
extern int s_CARD_YN_0046f468;
extern int s_CARD_YN_0046f710;
extern int s_CARD_YN_0046f9c4;
extern int s_CFONT2A_0046d4a4;
extern int s_CFONT2A_0046d5d8;
extern int s_CFONT2_0046d49c;
extern int s_CFONT2_0046d5d0;
extern int s_CFONT_0046d494;
extern int s_CFONT_0046d5c0;
extern int s_CLT_02dB2_0046c95c;
extern int s_CLT_02dB3_0046c950;
extern int s_CLT_02d_s_0046cde8;
extern int s_CLUT01B_0046c968;
extern int s_CLUT_02dA_0046ce08;
extern int s_CLUT_02dB_0046c944;
extern int s_CLUT_02dB_0046cdfc;
extern int s_CLUT_02dC_0046ce20;
extern int s_CLUT_02dD_0046ce14;
extern int s_CLUT_02dE_0046ce2c;
extern int s_CONTINUE_0046cff4;
extern int s_CREDITZ__0046ed08;
extern int s_Created_Primary_Surface_0046c69c;
extern int s_Creating_Palette_0046c618;
extern int s_DIRINFO_0046cf68;
extern int s_DRAWDIST_0046d000;
extern int s_DRIVER_d_0046d79c;
extern int s_DR_02dA_0046ce50;
extern int s_DR_02dB_0046ce58;
extern int s_DR_02dC_0046ce60;
extern int s_DUST1_0046cd8c;
extern int s_DUST2_0046cd94;
extern int s_DUST3_0046cd9c;
extern int s_DUST4_0046cda4;
extern int s_DUST5_0046cdac;
extern int s_DUST6_0046cdb4;
extern int s_DUST7_0046cdbc;
extern int s_DUST8_0046cdc4;
extern int s_DirectDraw_Back_Count____d_0046c660;
extern int s_DirectDraw_Palette_Count____d_0046c640;
extern int s_DirectDraw_Palette_Count____d_0046c6b8;
extern int s_DirectDraw_Primary_Count____d_0046c67c;
extern int s_DirectDraw_Primary_Count____d_0046c6d8;
extern int s_ENGINE_0046cea8;
extern int s_FASTLAP_0046cd2c;
extern int s_FILE_NOT_FOUND__0046c7c8;
extern int s_FLARE_0_0046cc88;
extern int s_FLARE_1_0046cc78;
extern int s_FLARE_2_0046cc68;
extern int s_FLARE_3_0046cc80;
extern int s_FLARE_4_0046cc70;
extern int s_FONT2_0046d484;
extern int s_FONT2_0046d5c8;
extern int s_FONT3_0046d48c;
extern int s_FONT3_0046d5e0;
extern int s_FRNT88A_0046cef8;
extern int s_FRNT88B_0046cf00;
extern int s_FRNT88C_0046cf08;
extern int s_FRNT88D_0046cf10;
extern int s_FRNT88E_0046cf18;
extern int s_FRWN88A_0046cf20;
extern int s_FRWN88B_0046cf28;
extern int s_FRWN88C_0046cf30;
extern int s_FRWN88D_0046cf38;
extern int s_FRWN88E_0046cf40;
extern int s_Floating_point_support_not_loade_0046fc18;
extern int s_GREENDIM_0046ccc0;
extern int s_GRNLIGHT_0046ccb4;
extern int s_HELMET_0046d0d0;
extern int s_Had_to_Restore_Primary_Surface__0046c5d0;
extern int s_Had_to_Restore_Primary_Surface__0046c5f4;
extern int s_INTRO_AVI_0046cfd4;
extern int s_INVALID_FILE_TYPE_0046c768;
extern int s_Init_Primitive_Buffer_0046c880;
extern int s_Invalid_Channel_0046c7e8;
extern int s_Invalid_Channels_0046c820;
extern int s_Kill_Sound_0046c7f8;
extern int s_LAPNUM_0046cd04;
extern int s_LAPTIME_0046cd24;
extern int s_LEV0_COPYRIGH_BMP_0046d56c;
extern int s_LEV0_FONT_BNK_0046d46c;
extern int s_LEV0_FONT_BNK_0046d5a8;
extern int s_LEV0_LEVEL_SPR_0046d45c;
extern int s_LEV0_LOADING_BMP_0046d580;
extern int s_LEVC_LEVEL_SPR_0046db84;
extern int s_LEVF_LEVEL_SPR_0046d598;
extern int s_LEV_X_LEVEL_CLT_0046cfa4;
extern int s_LEV_X_LEVEL_DAT_0046cfc4;
extern int s_LEV_X_LEVEL_ECL_0046cfb4;
extern int s_LEV_X_LEVEL_PAL_0046cf94;
extern int s_LEV_X_LEVEL_TX0_0046cf70;
extern int s_LEV_X_LEVEL_TX_d_0046cf80;
extern int s_LIGHTSUR_0046cc94;
extern int s_Lock_Channel_0046c834;
extern int s_MACSrPOO_0046ed14;
extern int s_MAD_I_0046d050;
extern int s_Modify_Sound_0046c810;
extern int s_Modifying_Palette_0046c62c;
extern int s_NEG_I_0046d048;
extern int s_NOTCHES_0046d01c;
extern int s_NO_FILES_IN_LIST_0046c7a0;
extern int s_OUTRO_AVI_0046cfe0;
extern int s_OUT_OF_BANKS_0046c8c8;
extern int s_OUT_OF_MEMORY_0046c790;
extern int s_Out_of_heap_space_0046c86c;
extern int s_P1D1T_d_0046cdf4;
extern int s_PAUSED_0046cfec;
extern int s_PC_DD2_0046c59c;
extern int s_PC_Write_File__0046c924;
extern int s_POSNUM_0046cd0c;
extern int s_Play_Sound_0046c804;
extern int s_Player_0046eff8;
extern int s_Player__d_0046ecfc;
extern int s_Popped_too_many_matrices_0046c730;
extern int s_Print__0046c8c0;
extern int s_PushMatrix_0046c724;
extern int s_Pushed_too_many_matrices_0046c708;
extern int s_RACEPOIN_0046ccf8;
extern int s_REDDIM_0046ccac;
extern int s_REDLIGHT_0046cca0;
extern int s_RETIRE_0046d014;
extern int s_RMPEClass_00460454;
extern int s_ROOF88A_0046cf48;
extern int s_ROOF88B_0046cf50;
extern int s_ROOF88C_0046cf58;
extern int s_RSURE_0046d024;
extern int s_SFXVOL_0046d00c;
extern int s_SHADOW_0046cf60;
extern int s_SMALLNUM_0046cd14;
extern int s_SMALRING_0046d2fc;
extern int s_SMALRING_0046e784;
extern int s_SMALRING_0046f488;
extern int s_SMALRING_0046f730;
extern int s_SMALRING_0046f9e4;
extern int s_SMCL_02dA_0046ce44;
extern int s_SMCL_02dB_0046ce38;
extern int s_SMOKE1_0046cd4c;
extern int s_SMOKE2_0046cd54;
extern int s_SMOKE3_0046cd5c;
extern int s_SMOKE4_0046cd64;
extern int s_SMOKE5_0046cd6c;
extern int s_SMOKE6_0046cd74;
extern int s_SMOKE7_0046cd7c;
extern int s_SMOKE8_0046cd84;
extern int s_SPARK_0046cdcc;
extern int s_SPEEDIND_0046cce0;
extern int s_STILLRUN_0046ccec;
extern int s_SaveGames_0046c8ec;
extern int s_TICK2_0046d438;
extern int s_TMOVE_0046d068;
extern int s_TOO_MUCH_TEXT__0046c8b0;
extern int s_TRACK_02dL_0046f030;
extern int s_TRACK_02d_0046f024;
extern int s_TSELECT_0046d058;
extern int s_Text_functions__0046c8d8;
extern int s_Thread_has_no_thread_specific_da_0046fc54;
extern int s_VAGS_BANK1_SBK_0046d070;
extern int s_WHEEL2_0046cd38;
extern int s_WHEEL3_0046cd40;
extern int s__R_JC_T_Cannot_delete_0046d1b0;
extern int s__R_JC_T_Cannot_format_0046d198;
extern int s__R_JC_T_Cannot_load_0046d1c8;
extern int s__R_JC_T_Cannot_save_0046d1dc;
extern int s__R_JC_T_Championship_0046ef88;
extern int s__R_JC_T_Enter_your_name_0046a03c;
extern int s__R_JC_T_Fastest_Lap_0046a054;
extern int s__R_JC_T_File_Options_0046f380;
extern int s__R_JC_T_File_already_exists_0046d224;
extern int s__R_JC_T_Formatting_0046d290;
extern int s__R_JC_T_Keyboard_0046ed80;
extern int s__R_JC_T_Loading_0046d2b4;
extern int s__R_JC_T_Memory_card_full_0046d1f0;
extern int s__R_JC_T_Memory_card_unformatted_0046d258;
extern int s__R_JC_T_No_Memory_card_0046d20c;
extern int s__R_JC_T_Not_a_DD2_file_0046d240;
extern int s__R_JC_T_Please_Wait___0046d278;
extern int s__R_JC_T_Practice_0046efa0;
extern int s__R_JC_T_Reach_division_1_to_unlo_0046f0dc;
extern int s__R_JC_T_Reach_division_1_to_unlo_0046f104;
extern int s__R_JC_T_Reach_division_2_to_unlo_0046f08c;
extern int s__R_JC_T_Reach_division_2_to_unlo_0046f0b4;
extern int s__R_JC_T_Reach_division_3_to_unlo_0046f03c;
extern int s__R_JC_T_Reach_division_3_to_unlo_0046f064;
extern int s__R_JC_T_Reading_Memory_cards_0046d2c4;
extern int s__R_JC_T_Save_Game_0046f1a0;
extern int s__R_JC_T_Save_Replay_0046f1b4;
extern int s__R_JC_T_Saving_0046d2a4;
extern int s__R_JC_T_Select_Control_Method_0046dd8c;
extern int s__R_JC_T_Select_Control_Method_0046ddac;
extern int s__R_JC_T_Sound_Effects_0046dc1c;
extern int s__R_JC_T_View_Lap_Times_0046e82c;
extern int s__R_JC_T_View_League_0046f358;
extern int s__R_JC_T_View_Replay_0046eea8;
extern int s__R_JC_T_View_Replay_0046f36c;
extern int s__R_JC_T_View_Replay_0046f6c8;
extern int s__R_JC_T_View_Results_0046f340;
extern int s__R_JC_T_View_Results_0046f934;
extern int s__R_JC_T_View_Statistics_0046e814;
extern int s__R_JC_T_View_Statistics_0046f398;
extern int s__R_JC_T_Wrecking_Racing_0046ee00;
extern int s__R_JL_T_Amateur_0046dca8;
extern int s__R_JL_T_Delete_File_0046d3c0;
extern int s__R_JL_T_Delete_File__0046d308;
extern int s__R_JL_T_Division_1_0046f25c;
extern int s__R_JL_T_Division_2_0046f270;
extern int s__R_JL_T_Division_3_0046f284;
extern int s__R_JL_T_Division_4_0046f298;
extern int s__R_JL_T_ERROR_0046d188;
extern int s__R_JL_T_Format__0046d338;
extern int s__R_JL_T_Jug_0046ebc0;
extern int s__R_JL_T_Load_0046d348;
extern int s__R_JL_T_Load_File_0046d358;
extern int s__R_JL_T_Overwrite_File__0046d320;
extern int s__R_JL_T_Play_0046ec40;
extern int s__R_JL_T_Prev__Track_0046ec2c;
extern int s__R_JL_T_Pro_0046dcb8;
extern int s__R_JL_T_Promoted_0046f4c4;
extern int s__R_JL_T_Quit_DD2__0046e790;
extern int s__R_JL_T_Quit_Season__0046f494;
extern int s__R_JL_T_Quit_Season__0046f73c;
extern int s__R_JL_T_Quit_Season__0046f9f0;
extern int s__R_JL_T_Relegated_0046f4d8;
extern int s__R_JL_T_Relegated_Out_0046f4ec;
extern int s__R_JL_T_Rookie_0046dc98;
extern int s__R_JL_T_Save_0046d36c;
extern int s__R_JL_T_Save_Configuration_0046d37c;
extern int s__R_JL_T_Save_Game_0046d398;
extern int s__R_JL_T_Save_Replay_0046d3ac;
extern int s__R_JL_T_Season_Results_0046f4ac;
extern int s__R_JL_T_Select_Block_To_Save_To_0046d0f4;
extern int s__R_JL_T_Select_Block_To_Save_To_0046d114;
extern int s__R_JL_T_Select_File_To_Delete_0046d134;
extern int s__R_JL_T_Select_File_To_Delete_0046d154;
extern int s__R_JL_T_Select_File_To_Load_0046d0d8;
extern int s__R_JL_T_Tuscan_0046ec8c;
extern int s__R_JL_T_Winner___0046f504;
extern int s___JL__T_Left_0046e264;
extern int s___JL__T_Right_0046e274;
extern int s___JL__T_Space_0046e254;
extern int s___JL__T__c_0046e248;
extern int s___R__JC__T_Car__02d_0046d7cc;
extern int s___R__JC__T__d_0046da44;
extern int s___R__JC__T__s_0046d44c;
extern int s___R__JC__T__s_to_race_next___0046d54c;
extern int s___R__JL__T_File__DD2____s_0046d3d4;
extern int s___R__JL__T_File__EMPTY_0046d408;
extern int s___R__JL__T_File__USED_0046d3f0;
extern int s___R__JL__T_Season__d_0046d724;
extern int s___R__JL__T_Track__d___0046ec74;
extern int s___R__JL__T__d_0046d7a8;
extern int s___R__JL__T__d_0046da64;
extern int s___R__JL__T__d__02d__02d_0046e3ec;
extern int s___R__JL__T__s_0046d714;
extern int s___R__JL__T__s_0046d7bc;
extern int s___R__JL__T__s_0046da54;
extern int s___R__JL__T__s_0046e3dc;
extern int s__s_player__d_0046ece0;
extern int s_avivideo_0046c74c;
extern int s_cdaudio_0046c844;
extern int s_conin__0046fc84;
extern int s_conout__0046fc8b;
extern int sca_frame_count_;
extern int scene_colour_matrix_;
extern int slab_background_;
extern int stack0x00000008;
extern int stack0x0000000c;
extern int stack0xfffffbd4;
extern int stack0xfffffbd5;
extern int stack0xfffffbd6;
extern int stack0xffffff78;
extern int stack0xffffffa0;
extern int stack0xffffffbf;
extern int stack0xfffffff4;
extern int stack0xfffffffc;
extern int uRam00464e52;
extern int uRam00464e5a;
extern int uRam0073c2f4;
extern int uRam00744b36;
extern int uRam007598a0;
extern int uRam007598a2;
extern int uRam007598a4;
extern int uRam007598a6;
extern int uRam007598a8;
extern int uRam007598ae;
extern int uVar1;
extern int wheeloff_object_;
void FUN_0041033a(void);
void draw_half(void);
void FUN_0041080d(void);
void draw_text_half_trans(void);
void __cdecl FUN_00410f74(int param_1);
void __cdecl FUN_004110a4(int param_1);
void __cdecl FUN_004111e8(int param_1);
void __cdecl FUN_0041132c(int param_1);
void __cdecl FUN_00411a04(int param_1);
void __cdecl FUN_00411b34(int param_1);
void __cdecl FUN_00411c78(int param_1);
void __cdecl FUN_00411ebc(int *param_1,undefined *param_2);
void __cdecl FUN_0041243c(int *param_1,undefined *param_2);
void __cdecl FUN_00412694(int param_1);
void __cdecl ClearOTagR(undefined4 *param_1,int param_2);
void __cdecl DrawOTag(int *param_1);
void __cdecl DrawPrim(int param_1);
void __cdecl FUN_004128a6(undefined4 *param_1,uint param_2,int param_3);
undefined2 __cdecl Init_Application(HINSTANCE param_1);
void __cdecl Close_Application(LPCSTR param_1,LPCSTR param_2);
void __cdecl SetVideoMode(undefined4 param_1,undefined4 param_2,int param_3);
void VSync(void);
void PutDrawEnv(void);
void PutDispEnv(void);
void __cdecl SetPalette(byte *param_1);
void __cdecl VSyncCallback(undefined4 param_1);
void FUN_00412e8c(void);
void __cdecl FUN_00412e9c(int param_1);
void __cdecl LoadImage(int *param_1,undefined4 *param_2);
void __cdecl MoveImageClut(int *param_1,int param_2,int param_3);
void FUN_00413014(void);
undefined4 FUN_00413070(void);
undefined4 __cdecl FUN_004130b0(undefined4 param_1,undefined4 param_2,int param_3);
void DDRelease(void);
LRESULT FUN_004132b0(HWND param_1,uint param_2,uint param_3,uint param_4);
void __cdecl FUN_00413448(int param_1);
void __cdecl Generate_Transparency_Tables(int param_1);
void __cdecl gte_MulMatrix0(undefined4 *param_1,short *param_2);
void __cdecl MulMatrix2(undefined4 *param_1,short *param_2);
int __cdecl rsin(uint param_1);
int __cdecl rcos(uint param_1);
uint __cdecl SquareRoot0_(uint param_1);
void GTERT(void);
void GTERPS(void);
void GTERPT(void);
void GTERPT4_(void);
void FUN_00413b4e(void);
void __cdecl ApplyMatrixLV(short *param_1,int *param_2,uint *param_3);
void __fastcall FUN_00413dc8(short *param_1);
void FUN_00413f45(void);
void FUN_00413fd2(void);
void FUN_00414055(void);
void gte_dpcs(void);
void gte_ncds(void);
void __cdecl SetFogNearFar(undefined4 param_1,undefined4 param_2);
void PushMatrix(void);
void PopMatrix(void);
uint __cdecl FUN_00414360(int *param_1,int *param_2);
uint __cdecl VectorNormalS(int *param_1,undefined2 *param_2);
uint __cdecl VectorNormalSS(int *param_1,undefined2 *param_2);
void __cdecl RotMatrixYXZ(int *param_1,short *param_2);
void __cdecl RotMatrixX(uint param_1,undefined4 *param_2);
void __cdecl RotMatrixY(uint param_1,short *param_2);
void __cdecl RotMatrixZ(uint param_1,short *param_2);
void __cdecl FUN_004148d0(int param_1,int param_2,int param_3,int param_4);
void __cdecl gte_SetRotMatrix(undefined2 *param_1);
void __cdecl RotTrans(int *param_1,int *param_2,undefined4 *param_3);
void __cdecl RotTransPers(int *param_1,int *param_2,undefined4 *param_3,undefined4 *param_4);
void __cdecl OuterProduct12(int *param_1,int *param_2,int *param_3);
void __cdecl Play_Movie(undefined4 param_1);
int * __cdecl Load_Textures(undefined4 *param_1,int *param_2,int param_3);
undefined4 __cdecl Load_Cluts(int *param_1);
void __cdecl Add_Buffer_Load_(char *param_1,uint param_2,uint param_3);
void __cdecl FUN_00415160(void *param_1,undefined *param_2);
void __cdecl Read_Directory(char *param_1);
int __cdecl File_Load(char *param_1,void *param_2);
void __cdecl FUN_004153d4(undefined4 *param_1);
undefined4 __cdecl FUN_00415404(char *param_1);
void __cdecl FUN_00415448(char *param_1,uint *param_2);
undefined4 __cdecl FUN_004154b8(int param_1);
void __cdecl Decompress(undefined4 *param_1);
int * __cdecl DSLoadSoundBuffer(int *param_1,HGLOBAL param_2);
undefined4 __cdecl FUN_0041574c(int *param_1,HGLOBAL param_2);
bool __cdecl DSGetWaveResource(HGLOBAL param_1,int *param_2,int *param_3,uint *param_4);
undefined4 __cdecl FUN_004157e8(int *param_1,undefined4 *param_2,int param_3);
undefined4 __cdecl FUN_00415894(int *param_1,int *param_2,int *param_3,uint *param_4);
void __cdecl FUN_004159a8(undefined4 param_1);
void Sound_Init(void);
void FUN_00415a94(void);
void Sound_Remove(void);
void __cdecl FUN_00415b50(uint param_1);
void Sound_Stop(void);
void Sound_Pause_(void);
void Sound_Restart(void);
void __cdecl Kill_Sound(int param_1);
int __cdecl Play_Sound(int param_1,int param_2,int param_3,uint param_4,undefined4 param_5,int param_6);
void __cdecl Modify_Sound(int param_1,uint param_2,undefined4 param_3,int param_4);
void __cdecl FUN_00415f64(int param_1,int param_2);
void __cdecl Unlock_Channel(int param_1,int param_2);
undefined4 FUN_00416044(void);
void CD_Close(void);
void FUN_0041611c(void);
void Read_CD_Toc_(void);
void __cdecl FUN_0041612c(char param_1,undefined4 param_2);
void Start_CD_Audio(void);
void Check_For_CD_Loop(void);
void FUN_00416264(void);
void CD_Pause(void);
void CD_Restart(void);
void Sound_Timer_(void);
int __cdecl FUN_004163b4(int param_1);
void FUN_0041643c(void);
int FUN_00416494(void);
undefined4 __cdecl FUN_004164d4(int param_1,int param_2);
int __cdecl FUN_004165a4(uint param_1);
undefined4 CD_Check(void);
void __cdecl Load_Sprite_Info(undefined4 *param_1);
int __cdecl Search_For_Sprite(char *param_1);
void __cdecl FUN_004166c4(int param_1,char *param_2,ushort *param_3);
void __cdecl Setup_Sprite(int param_1,char *param_2,ushort *param_3);
void __cdecl FUN_00416714(int param_1,char *param_2,ushort *param_3);
void __cdecl Modify_Sprite(int param_1,int param_2,char param_3,char param_4,short param_5,short param_6);
undefined4 __cdecl FUN_00416a10(int param_1);
void __cdecl draw_face_3pt_flat_dpq(int param_1);
void __cdecl draw_face_3pt_text_dpq(int param_1);
void __cdecl draw_face_3pt_text_dpq_squash(int param_1);
void __cdecl FUN_0041a2f4(int param_1);
void __cdecl draw_face_3pt_gour_dpq(int param_1);
void __cdecl draw_face_3pt_pict_dpq(int param_1);
void __cdecl draw_face_3pt_pict_dpq_lit(int param_1);
void __cdecl draw_face_4pt_pict_dpq(int param_1);
void __cdecl draw_face_4pt_pict_dpq_lit(int param_1);
void __cdecl draw_face_sprite(int param_1);
void __cdecl draw_face_sprite_dpq(int param_1);
void __cdecl FUN_0041f6a0(int param_1);
int __cdecl FUN_0041fb7c(undefined4 *param_1);
void __cdecl Create_Object(undefined4 *param_1,int param_2);
void __cdecl Set_Object(undefined4 *param_1,int param_2);
void __cdecl Remove_Object(int param_1,uint param_2);
void __cdecl Pre_Rotate(int param_1);
void __cdecl Draw_Subdiv_Object(undefined4 *param_1);
void __cdecl FUN_0041fe68(undefined4 *param_1,int param_2);
void __cdecl Update_Object(undefined4 *param_1);
void __cdecl FUN_0041ff50(undefined4 param_1);
void __cdecl FUN_0042003c(undefined4 param_1,undefined4 param_2);
void __cdecl FUN_00420060(undefined4 param_1);
void __cdecl Set_Zclip(undefined4 param_1,undefined4 param_2);
void __cdecl Set_World_Position(undefined4 *param_1);
void __cdecl Set_World_Matrix(int param_1);
void __cdecl Set_World_View(int *param_1);
void __cdecl Set_Ambient_Light(byte *param_1);
void __cdecl Set_Depth_Cue(undefined4 param_1,undefined4 param_2,byte *param_3);
void __cdecl FUN_004202ac(short *param_1,int *param_2);
void __cdecl FUN_004203a0(short *param_1,int *param_2);
void __cdecl Calc_Object_MatrixYZX(short *param_1,int *param_2);
void __cdecl Calc_Object_Angles(short *param_1,short *param_2);
void __cdecl FUN_004205d8(undefined4 param_1,int *param_2,uint param_3);
void __cdecl Point_Camera(int *param_1,uint param_2);
void __cdecl Set_Draw_Mode(int param_1);
void FUN_00420b1c(void);
void __cdecl Draw_All(int param_1);
void __cdecl Draw_Tile(undefined4 *param_1,int param_2,undefined2 param_3,undefined2 param_4,undefined2 param_5, undefined2 param_6,byte *param_7);
void __cdecl Allocate_OT_(int param_1);
void Swap_Buffers(void);
void __cdecl FUN_00420cf0(int param_1,ushort param_2,undefined2 param_3,undefined1 *param_4,undefined4 param_5);
void __cdecl Draw_Font_Poly(undefined4 *param_1,int param_2);
void __cdecl Init_Primitive_Buffer(uint param_1);
void Reset_Primitive_Buffer(void);
int __cdecl FUN_00420e6c(int param_1);
void __cdecl FUN_00420ee8(uint param_1);
void __cdecl Allocate_Font_Buffers(int param_1,int param_2,int param_3);
void __cdecl Setup_Font(char *param_1,int param_2,undefined2 param_3);
void __cdecl Duplicate_Font(int param_1,int param_2,char *param_3);
void __cdecl Print_Locate(undefined4 param_1,undefined4 param_2,undefined4 param_3);
void __cdecl Print_Font(int param_1);
void __cdecl Print_Ink(undefined1 *param_1);
void __cdecl Print_InkRGB(undefined1 param_1,undefined1 param_2,undefined1 param_3);
uint __cdecl Print(undefined2 param_1,int param_2);
void Print_Draw(void);
void FUN_00422064(void);
int __cdecl FUN_004220cc(undefined1 *param_1,int param_2,int param_3);
int __cdecl FUN_00422128(int *param_1,int param_2,int param_3);
int __cdecl FUN_00422184(int *param_1,int param_2,int param_3);
void FUN_004221ec(void);
void __cdecl FUN_00422358(int param_1,int param_2,int param_3);
void __cdecl FUN_00422548(int param_1,int param_2,int param_3);
void __cdecl FUN_0042293c(int param_1,int param_2,int param_3);
void Init_Controller_(void);
void Setup_Controller(void);
void Setup_Joystick(void);
void FUN_00422c74(void);
void __cdecl Translate_Keypress(uint param_1,uint param_2);
void __cdecl FUN_0042304c(byte *param_1);
void InitCardSystem(void);
void FUN_00423210(void);
undefined4 __cdecl SaveCardFile(char *param_1,undefined4 param_2,undefined4 *param_3);
undefined4 __cdecl DeleteFileMC(char *param_1);
void LoadCardFiles(void);
undefined4 __cdecl LoadCardFile(char *param_1,undefined4 param_2,undefined4 *param_3);
int FirstSavedGame(void);
void InitCardBlocks(void);
undefined4 __cdecl DupFileCheck(undefined4 param_1,char *param_2,int param_3);
int FUN_0042353c(void);
void __cdecl MPE_InitHeap(int param_1,uint param_2);
undefined4 * __cdecl MPE_malloc(int param_1);
void __cdecl MPE_free(int param_1);
void Debug_Stub(void);
void __cdecl PC_Write_File(char *param_1,void *param_2,size_t param_3);
void System_Error(LPCSTR param_1,LPCSTR param_2);
int __cdecl FUN_004237c0(int param_1);
int __cdecl FUN_00423804(int param_1,int param_2);
void Profile_Init(void);
undefined4 Play_Game(void);
void Init_Debris_(void);
void __cdecl Setup_Debris(undefined4 *param_1,int *param_2,uint param_3,int param_4);
void Update_Debris(void);
void FUN_00424930(void);
void __cdecl Setup_Flying_Objects(int param_1,int *param_2,int param_3,int param_4);
void Update_Flying_Objects(void);
void __cdecl Zero_Flying_Object(int param_1);
undefined4 __cdecl Request_Flying_Object(int param_1);
void __cdecl FUN_00425314(int param_1);
void __cdecl Flying_Objects_Pos_Ang(int param_1);
void __cdecl FUN_00425a98(int param_1);
undefined4 __cdecl FUN_00425b88(int param_1);
void InitialiseDenting(void);
void __cdecl FUN_00425e74(int param_1,int param_2,undefined4 param_3,int param_4,int param_5);
void __cdecl Generate_Surface_Normals(byte *param_1);
uint __cdecl Mask_Point_In_Quad(int *param_1,int *param_2,int *param_3,int *param_4,int *param_5,byte param_6);
void FUN_00426ab4(void);
void __cdecl Track_Follow(int *param_1);
void __cdecl Map_Height(int *param_1);
void __cdecl FUN_00428548(int param_1,int *param_2,int param_3);
void __cdecl Move_Forward_Strip(int param_1,int param_2);
void __cdecl FUN_004287c0(int param_1,int param_2);
undefined4 __cdecl Search_For_Strip(int param_1);
void __cdecl FUN_004288d0(int param_1);
void __cdecl FUN_0042895c(int param_1);
void __cdecl FUN_004295e4(int param_1);
void __cdecl Car_Camera(int param_1);
void __cdecl Camera_Pad_Control(int param_1);
void Init_Pit_Camera_(void);
void FUN_00429994(void);
void __cdecl Pit_Camera_Control(int param_1);
void __cdecl Init_The_Floaty_Camera(int param_1);
int __cdecl Do_The_Floaty_Camera_Thing(int *param_1,int param_2);
void Init_Damage_Indicator(void);
void __cdecl FUN_0042a4d8(int param_1);
void __cdecl Bonnet_Smoke(int param_1);
void __cdecl FUN_0042b5f0(int param_1);
void __cdecl Draw_Car(int param_1);
void __cdecl FUN_0042c1a4(int param_1);
void Init_Car_Graphics(void);
void Init_Wild_Bill(void);
void Init_Rollercoaster(void);
void Init_CLUT_Animation_(void);
void Init_Texture_Animation(void);
void __cdecl Texture_Animation(undefined4 param_1,undefined4 *param_2,int *param_3);
void __cdecl CLUT_Animation(undefined4 param_1,int *param_2,int *param_3);
int Update_Other_Objects(void);
void Draw_Other_Objects(void);
void __cdecl Draw_Dynamic_Objects(undefined4 *param_1,int *param_2,int *param_3);
void Init_Flag(void);
void DrawFlagObject(void);
void UpdateFlag(void);
void Init_LensFlare(void);
void DrawLensFlare(void);
void Init_Overlays(void);
void __cdecl Draw_Overlays(int param_1);
void FUN_0042fa4c(void);
void Update_Race_CountDown(void);
void FUN_0042fd58(void);
void Display_Position_Pointers(void);
void Init_Scene(void);
undefined4 __cdecl VVDraw_Object(undefined4 *param_1);
void __cdecl Draw_Scene_Object(undefined4 *param_1,int *param_2);
void FUN_00430698(void);
void __cdecl Setup_Object_Block(undefined4 *param_1,int param_2);
void FUN_004308b8(void);
void __cdecl Decrunch_Object_Block(int param_1,int param_2);
void __cdecl FUN_00430a30(int param_1);
void Update_Scene_Objects(void);
void __fastcall FUN_00430bde(int param_1);
void Init_Scene_Objects(void);
void Remove_Scene_Objects(void);
void Init_Sky(void);
void __cdecl FUN_00430efc(int param_1,short param_2,short param_3,short param_4,int param_5);
void Draw_Sky(void);
void __cdecl AI_Com_Server(byte param_1);
void __cdecl Determine_AI(int param_1,int param_2,int param_3,int param_4,int param_5,int param_6,int param_7, int param_8);
int __cdecl Recommended_Acceleration(int param_1,int param_2,int param_3);
undefined4 __cdecl FUN_00432f98(int param_1);
int __cdecl Get_Car_Angle(int param_1);
void __cdecl Get_Direction_Cosines(uint *param_1,int *param_2);
void __cdecl Interpolate_Direction_Vectors_Left(int param_1,int param_2,int param_3);
void __cdecl Interpolate_Direction_Vectors_Right_(int param_1,int param_2,int param_3);
void __cdecl InitialiseAI(byte param_1);
int __cdecl Obstacle_Ahead(byte param_1,byte param_2,byte param_3,byte param_4,byte param_5);
int __cdecl Strip_Distance(int param_1,int param_2);
void FUN_00433a70(void);
void CheckPointScoring(void);
int __cdecl Barrier_Collision(int param_1,int param_2);
int __cdecl Barrier_Corner_Collision(int param_1,int param_2);
void FUN_004351d0(void);
void __cdecl FUN_00435254(int param_1);
void __cdecl TransformWheels_(int param_1,int param_2,int param_3);
void __cdecl TransformEnemyWheels(int param_1);
void __cdecl ApplyWheelOverlay(int param_1,uint param_2);
void InitialiseParticleSystem(void);
void __cdecl Smoke(undefined4 *param_1,int param_2,undefined4 param_3,undefined4 param_4,int param_5, undefined4 param_6,undefined4 param_7,undefined4 param_8,undefined4 param_9,undefined4 param_10 ,undefined4 param_11,undefined4 param_12,undefined4 param_13,undefined4 param_14, undefined4 param_15,undefined1 param_16);
void __cdecl Fire(int *param_1,int param_2,undefined4 param_3,undefined4 param_4,int param_5,undefined4 param_6, undefined4 param_7,undefined4 param_8,undefined4 param_9,undefined4 param_10,undefined4 param_11 ,undefined4 param_12,undefined4 param_13,undefined4 param_14,undefined4 param_15);
void __cdecl Sparks(int *param_1,int *param_2,int param_3,undefined4 param_4,undefined4 param_5,int param_6, undefined4 param_7,undefined4 param_8,undefined4 param_9,undefined4 param_10, undefined4 param_11,undefined4 param_12,undefined4 param_13,undefined4 param_14, undefined4 param_15,undefined4 param_16);
void __cdecl Steam(int *param_1,undefined4 param_2,int param_3,undefined4 param_4,undefined4 param_5,int param_6, undefined4 param_7,undefined4 param_8,undefined4 param_9,undefined4 param_10, undefined4 param_11,undefined4 param_12,undefined4 param_13,undefined4 param_14, undefined4 param_15,undefined4 param_16,undefined1 param_17);
int FUN_00436c04(void);
void __cdecl FreeParticle(int param_1);
void DrawParticles(void);
void FUN_00436cfc(void);
void __cdecl FUN_00436dd0(int param_1);
void __cdecl Start_Roll(int param_1,int param_2,int param_3);
void __cdecl FUN_0043709c(int param_1,int param_2,int param_3);
void __cdecl FUN_004371fc(int param_1,int param_2,int param_3,int param_4,short param_5,int param_6,uint param_7, undefined4 param_8);
void __cdecl Calc_Head_On_Clsn_Dynamics(int param_1,int param_2,int param_3);
uint __cdecl Check_2D_Car_Collision(undefined4 param_1,undefined4 param_2,int *param_3,uint param_4);
void __cdecl Check_Ground_Car_Collision(int param_1,int param_2);
void __cdecl Check_Space_Car_Collision(int param_1,int param_2);
void Do_Car_Collisions(void);
void Init_Car_Cluts(void);
void Init_Car_Doors(void);
void __cdecl FUN_0043b26c(int param_1,int param_2,int param_3);
void __cdecl Change_Bonnet_Clut(int param_1,int param_2);
void __cdecl Change_Boot_Clut(int param_1,int param_2);
void __cdecl Highlight_Area(int param_1,undefined4 param_2,undefined1 param_3);
void __cdecl FUN_0043b7bc(int param_1,int param_2,int param_3,int param_4,int param_5);
void __cdecl TextureDentHiCar(int param_1,uint param_2,int param_3,int param_4);
void __cdecl TextureDentMidCar(int param_1,uint param_2,int param_3,int param_4);
void __cdecl FUN_0043c4e8(int param_1,int param_2,int *param_3,int param_4);
void __cdecl FUN_0043c55c(int param_1,int param_2,int *param_3,int param_4);
void __cdecl FUN_0043c5d0(int param_1,int param_2,char *param_3,int param_4,int param_5);
void __cdecl FUN_0043c738(int param_1,int param_2,char *param_3,int param_4,int param_5);
void __cdecl Get_Corner_Positions(int param_1);
void __cdecl Ground_Collision(int param_1,int param_2);
int __cdecl Find_Lowest_Corner(undefined4 param_1,int param_2,int param_3);
void __cdecl Make_Car_Fly(int param_1,int param_2);
void __cdecl Car_Landed(int param_1);
void __cdecl Car_Landed_On_Corner(int param_1,int param_2);
void __cdecl Car_Fly_Motion_3D(int param_1);
void __cdecl Car_Grounded_Motion_3D(int param_1);
void __cdecl Car_Rolled_Edge_Onto_Wheels_(int param_1);
void __cdecl FUN_0043dbd8(int param_1);
void __cdecl FUN_0043dd00(int param_1);
void __cdecl Car_2pt_Motion_3D(int param_1);
void __cdecl Car_1pt_Motion_3D(int param_1);
void __cdecl Car_Drive_Motion_3D_(int param_1);
void FUN_00440980(void);
void __cdecl FUN_00440ac4(int param_1);
void __cdecl FUN_00440f60(int param_1);
int __cdecl Calc_Car_Tilt(int param_1,int param_2);
void __cdecl Calc_Car_Angles_Square (int param_1,int *param_2,int *param_3,int *param_4,int *param_5,short param_6, short param_7);
void __cdecl FUN_00441394(int param_1,int param_2);
void __cdecl Car_Drive_Motion(int param_1);
void __cdecl Car_Drive_2pt_Motion(int param_1);
void __cdecl Car_Fly_Motion(int param_1);
void __cdecl FUN_0044282c(int param_1);
void __cdecl FUN_00442b08(int param_1);
void __cdecl Car_Movement(int param_1);
void FUN_004430b8(void);
void Init_End_Race(void);
void Calc_Track_Positions(void);
void FUN_00443b10(void);
void Get_Race_Positions(void);
void Init_Track_Strip_Numbers(void);
void FUN_00443f90(void);
void __cdecl FUN_00444048(int param_1);
void __cdecl FUN_004448e0(int param_1);
void __cdecl FUN_00444a60(int param_1);
void __cdecl Calc_Suspension_Right_Wheels(int param_1);
void __cdecl Calc_Null_Suspension(int param_1);
void __cdecl FUN_00444e3c(int param_1);
void __cdecl Boot_Lost_Geometry(int param_1);
void __cdecl FUN_00444fcc(int param_1,int param_2);
void __cdecl FUN_00445030(int param_1,int param_2);
void __cdecl FUN_004450dc(int param_1,uint param_2);
void __cdecl Check_Bonnet_Removal(int param_1);
void __cdecl Check_Boot_Removal(int param_1);
void Init_Main(void);
void Set_Load_Textures(void);
void Modify_TDF(void);
void Init_Graphics(void);
void FUN_00445b40(void);
void FUN_00445b78(void);
void Init_Game(void);
void Play_Intro(void);
void Play_Xtro(void);
void Initialise_Pause_Mode(void);
undefined4 Pause_Mode(void);
void FUN_00446c10(void);
void __cdecl UndentCar(int param_1,undefined4 param_2);
void __cdecl SetHighLight(undefined4 param_1,int param_2);
void __cdecl FUN_004471c0(int param_1);
void __cdecl Control_Car_Replay(int param_1,int param_2);
void FUN_004477a0(void);
void __cdecl Record_Event(undefined4 param_1,int param_2);
void __cdecl Terminate_Replay(int param_1);
void __cdecl Terminate_Replay_Bodge(int param_1);
void FUN_00447960(void);
void FUN_00447a9c(void);
uint __cdecl Amplitude(int param_1);
int __cdecl DopplerFrequency(int param_1,int *param_2);
void __cdecl Allocate_Sound_Effect(int param_1,undefined4 param_2,undefined4 *param_3);
int __cdecl FUN_004480bc(int param_1);
void Load_Game_Vags(void);
void FUN_00448228(void);
void Strip_Trigger_Handler(void);
void __cdecl Sparking(uint param_1,int param_2);
undefined4 __cdecl LoadSave(uint param_1,undefined4 *param_2);
void FUN_004496d8(void);
undefined4 __cdecl FUN_00449728(uint param_1);
void FUN_00449928(void);
void FUN_00449958(void);
void __cdecl FUN_00449c54(undefined4 param_1);
void __cdecl FUN_00449dd8(undefined4 param_1);
undefined4 __cdecl FUN_00449ee0(uint param_1);
void __cdecl FUN_0044a1a4(undefined4 param_1,int param_2);
void __cdecl FUN_0044a2b0(undefined4 param_1,int param_2);
undefined4 __cdecl FUN_0044a32c(undefined4 param_1,int param_2,undefined4 *param_3);
undefined4 __cdecl FUN_0044a3a0(undefined4 param_1,int param_2,undefined4 *param_3);
undefined4 __cdecl FUN_0044a4e4(char *param_1);
void __cdecl FUN_0044a904(int param_1);
void FUN_0044aa28(void);
void FUN_0044aaf0(void);
void Load_First_Config(void);
void Load_Card_File(void);
void __cdecl FUN_0044ac9c(undefined2 *param_1,undefined2 param_2);
void __cdecl FUN_0044aec8(int *param_1);
void Init_Front_End(void);
undefined4 View_Frontend_Replay(void);
undefined4 DemoMode(void);
void FUN_0044b5c0(void);
void __cdecl Loading_Screen_From_Slab(int param_1);
void FUN_0044b684(void);
void __cdecl Load_Completion_Status(int param_1);
void Call_Loaded_Game(void);
void FUN_0044b970(void);
void __cdecl FUN_0044b9c0(char *param_1);
void __cdecl Setup_Pad(int param_1);
void Init_Wrecking_Championship(void);
void Init_StockCar_Championship(void);
void FUN_0044bb88(void);
void Init_StockCar_MultiChamp(void);
void Championship(void);
void MultiChamp(void);
void __cdecl Calculate_Finish(int param_1);
void Calculate_Results(void);
undefined4 Do_End_Of_Season_Stuff(void);
void Init_League_Info(void);
void Reset_League_Info(void);
void Sort_Leagues(void);
void Init_MultiLeague_Info(void);
void Setup_Driver_Names(void);
void __cdecl FUN_0044c418(int param_1,int param_2);
void __cdecl Add_Computer_Info(int param_1);
void Update_League_Info(void);
void __cdecl FUN_0044c538(int param_1,int param_2);
void Sort_MultiLeague(void);
void Sort_RacePos(void);
void Promote_And_Relegate(void);
undefined4 Check_League_Standing(void);
void Order_Cars(void);
void __cdecl FUN_0044c800(int *param_1,int param_2);
void FUN_0044c8e8(void);
undefined4 View_Results_Replay_(void);
void FUN_0044cb48(void);
void FUN_0044ccb8(void);
void FUN_0044ced4(void);
void FUN_0044cef0(void);
void FUN_0044d0e4(void);
void Start_New_Season_Stats(void);
void FUN_0044d3d4(void);
void __cdecl Update_Track_Stats(int param_1);
void FUN_0044d538(void);
void Update_Championship_Stats(void);
int FUN_0044d630(void);
int Get_Current_Recording_Season(void);
void FUN_0044d650(void);
void __cdecl Update_Jimmy_Spunk_Times(int param_1,int param_2);
void FUN_0044d974(void);
void FUN_0044db70(void);
void Secret(void);
void FUN_0044e148(void);
int FUN_0044e170(void);
void __cdecl FUN_0044e440(int *param_1);
void FUN_0044e618(void);
void FUN_0044e6a0(void);
void FUN_0044eb1c(void);
undefined4 __cdecl FUN_0044ecac(undefined4 param_1);
void __cdecl FUN_0044f5c8(int param_1,int param_2,int param_3);
void __cdecl FUN_0044f65c(undefined4 param_1,undefined4 param_2,int param_3);
undefined4 FUN_0044f6b0(void);
void __cdecl FUN_0044f814(undefined4 param_1);
undefined4 __cdecl FUN_0044fbc0(undefined4 param_1,uint param_2);
uint FUN_0044fca4(void);
void __cdecl FUN_0044fce8(undefined1 *param_1,int param_2);
void __cdecl FUN_0044fdb4(undefined4 param_1,undefined4 param_2,int param_3);
undefined4 View_BestLaps(void);
void FUN_0044ff48(void);
void Front_End(void);
void FUN_00450640(void);
void __cdecl Toggle_Track(int param_1);
void __cdecl Toggle_Car(int param_1);
undefined4 FUN_0045099c(void);
void __cdecl FUN_00450c7c(short *param_1);
void __cdecl Rotate_Slab_On(short *param_1);
void __cdecl FUN_00450e20(int param_1);
void Draw_Screen_Polys(void);
void __cdecl Setup_Screen_Text(int *param_1);
void __cdecl Setup_Screen_Lines(undefined4 param_1);
void Draw_Screen_Lines(void);
void __cdecl Button_Pressed(int param_1,short *param_2);
void Draw_Slab(void);
void FUN_004518ac(void);
void __cdecl Draw_Semi_Trans_Poly(short param_1,short param_2,short param_3,short param_4);
void FUN_00451a80(void);
void Play_Click_FX(void);
void FUN_00451ac0(void);
void FUN_00451ae0(void);
void FUN_00451d4c(void);
void FUN_00451d6c(void);
void FUN_00451fdc(void);
void FUN_00451ff0(void);
undefined4 FUN_0045202c(void);
void FUN_00452090(void);
int __cdecl Enter_Driver_Names(int param_1,int param_2);
undefined4 FUN_004525a0(void);
void FUN_00452790(void);
undefined4 FUN_004527f0(void);
void FUN_004529d4(void);
bool FUN_00452a34(void);
undefined4 Practice_Over(void);
void FUN_00452d30(void);
undefined4 FUN_00452da0(void);
undefined4 FUN_00452dc0(void);
void FUN_00452fa4(void);
void FUN_00452fc4(void);
int Select_Champ(void);
undefined4 Select_ChampQS(void);
undefined4 Select_Pract(void);
undefined4 Select_TimeT(void);
undefined4 Select_Total(void);
undefined4 Select_DDPract(void);
void FUN_00453580(void);
void __cdecl FUN_004535c0(int param_1);
undefined4 Save_Game(void);
void FUN_004539d0(void);
void FUN_004539f0(void);
undefined4 FUN_00453a18(void);
undefined4 FUN_00453a20(void);
void FUN_00453b98(void);
undefined4 End_Of_Season(void);
void FUN_00454158(void);
void FUN_004541e8(void);
void FUN_00454278(void);
void FUN_00454308(void);
void FUN_00454398(void);
undefined4 FUN_00454550(void);
void FUN_00454804(void);
void FUN_00454a70(void);
undefined4 Race_Over(void);
void FUN_00454e9c(void);
undefined4 FUN_00454f0c(void);
undefined4 FUN_004551b0(void);
void FUN_00455260(void);
undefined4 Display_Season_Status(void);
void FUN_0045572c(void);
undefined4 FUN_004559d4(void);
uint __cdecl __open_flags(byte *param_1);
int * __cdecl FUN_00455de4(char *param_1,byte param_2,uint param_3,int param_4,int *param_5);
int * __cdecl FUN_00455edb(char *param_1,byte *param_2,int param_3);
FILE * __cdecl FUN_00455f3b(FILE *param_1);
int * __cdecl FUN_00455fb0(char *param_1,byte *param_2,FILE *param_3);
undefined4 __cdecl FUN_00456034(int param_1,int *param_2);
undefined4 __cdecl FUN_0045607b(int *param_1,LONG param_2,uint param_3);
uint __cdecl __shutdown_stream(FILE *param_1,int param_2);
int __cdecl FUN_0045644e(int param_1);
void __cdecl FUN_0045645e(undefined1 *param_1,uint param_2);
uint __cdecl __doclose(FILE *param_1,int param_2);
float10 __CHP(void);
void __cdecl nfree(uint param_1);
void __cdecl FUN_00456717(int *param_1,undefined1 param_2);
void __cdecl FUN_0045672e(int param_1,byte *param_2);
void __null_int23_exit(void);
void __cdecl _exit(int _Code);
void __cdecl wstart2_(void);
int FUN_00456af2(void);
void __fastcall FUN_00456b30(uint param_1,uint param_2);
void __fastcall FUN_00456b67(uint param_1,undefined4 param_2);
undefined4 __cdecl FUN_00456bf3(undefined4 *param_1,byte *param_2,int *param_3);
int __cdecl _tolower(int _C);
int __cdecl FUN_00456cae(int param_1);
void __set_EDOM(void);
void __set_ERANGE(void);
void __cdecl FUN_00456cdf(undefined4 param_1);
errno_t __cdecl __set_doserrno(ulong _Value);
int __cdecl open(char *_Filename,int _OpenFlag,...);
int __cdecl sopen(char *_Filename,int _OpenFlag,int _ShareFlag,...);
undefined4 * __allocfp(void);
void __cdecl __freefp(int param_1);
void __purgefp(void);
void __cdecl __chktty(int param_1);
ulong __cdecl __threadid(void);
undefined4 FUN_0045703b(void);
void FUN_00457040(void);
void __cdecl FUN_00457041(int param_1);
void __cdecl FUN_0045704f(int param_1);
undefined4 __cdecl __NTInit(int param_1,void *param_2,HMODULE param_3);
void __cdecl __NTMainInit(undefined4 param_1,void *param_2);
void __exit(void);
int __cdecl FUN_004571e4(undefined4 *param_1);
long __cdecl _lseek(int _FileHandle,long _Offset,int _Origin);
DWORD __cdecl FUN_004572f8(int param_1,LONG param_2,DWORD param_3);
long __cdecl tell(int _FileHandle);
void __cdecl __ioalloc(undefined4 *param_1);
DWORD __cdecl FUN_00457408(int param_1,LPVOID param_2,DWORD param_3);
int __cdecl __filbuf(FILE *_File);
undefined4 __cdecl FUN_00457571(undefined4 *param_1);
DWORD getpid(void);
undefined4 __cdecl FUN_00457631(int param_1);
void FUN_004576c5(void);
void __init_8087_(void);
void __cdecl _fpreset(void);
uint * __cdecl nmalloc(uint param_1);
uint * __MemAllocator(void);
void __MemFree(void);
undefined4 __cdecl __prtf(undefined4 param_1,byte *param_2,int *param_3,undefined *param_4);
byte * __cdecl FUN_00457d39(char *param_1,int *param_2,int param_3);
char * __cdecl FUN_00457e84(char *param_1,int param_2);
int __cdecl FUN_00457ee9(char *param_1,undefined4 param_2,int param_3);
int __cdecl FUN_00457f0f(short *param_1,undefined4 param_2,int param_3);
void __cdecl FUN_00457f40(int param_1,char *param_2,int param_3);
void __cdecl FUN_00457f9f(char *param_1,uint param_2,int param_3);
int __cdecl FUN_0045809c(undefined4 param_1,undefined4 param_2,undefined4 param_3);
undefined8 __thiscall FUN_004580b7(void *this,ushort *param_1,int *param_2,int param_3,ushort *param_4);
void __cdecl FUN_004585e8(byte *param_1);
DWORD __cdecl __qwrite(uint param_1,LPCVOID param_2,DWORD param_3);
void __WinMain(void);
undefined4 __NTAtMaxFiles(void);
int __cdecl __NTAddFileHandle(int param_1);
void __cdecl FUN_00458964(HANDLE param_1,uint param_2);
void __cdecl __NTRemoveFileHandle(int param_1);
void FUN_00458a31(void);
int __NTGetFakeHandle(void);
void __cdecl __GetNTAccessAttr(int param_1,undefined4 *param_2,undefined4 *param_3);
void __cdecl __GetNTShareAttr(int param_1,undefined4 *param_2);
int __cdecl _stricmp(char *_Str1,char *_Str2);
int __cdecl FUN_00458b45(byte *param_1,byte *param_2);
uint __cdecl dosretax(uint param_1,int param_2);
undefined4 __cdecl FUN_00458bc7(uint param_1);
int __set_errno_nt(void);
int __cdecl isatty(int _FileHandle);
undefined4 __cdecl __IOMode(uint param_1);
void __cdecl FUN_00458cc5(int param_1,uint param_2);
undefined4 __cdecl __sigfpe_handler(undefined4 param_1);
void signal(int param_1);
int __cdecl raise(int _SigNum);
undefined2 FUN_00458e2f(void);
void __SigFini(void);
void __cdecl __NewExceptionHandler(undefined4 param_1);
void __DoneExceptionHandler(void);
void __cdecl __CloseSemaphore(undefined4 *param_1);
void __cdecl __AccessSemaphore(undefined4 *param_1);
void __cdecl __ReleaseSemaphore(undefined4 *param_1);
void * __cdecl __InitThreadData(void *param_1);
bool __NTThreadInit(void);
undefined4 __cdecl __NTAddThread(void *param_1);
void __cdecl __NTRemoveThread(int param_1);
void __InitMultipleThread(void);
void __InitRtns(void);
void __fastcall __FiniRtns(undefined4 param_1,byte param_2);
void __full_io_exit(void);
int __cdecl FUN_0045987f(int param_1);
int __cdecl flushall(void);
int __cdecl __flushall(void);
int __cdecl getche(void);
int __cdecl unlink(char *_Filename);
undefined8 __cdecl FUN_00459968(char *param_1,int *param_2,int param_3);
void __cdecl FUN_00459a88(char *param_1,int param_2);
float10 __cdecl __cnvs2d(char *param_1,undefined4 *param_2);
void FUN_00459b19(void);
ushort __init_80x87(void);
int * __cdecl FUN_00459b51(int *param_1);
undefined4 __cdecl FUN_00459bc5(uint param_1);
int __cdecl __ExpandDGROUP(uint param_1);
bool __cdecl FUN_00459c68(uint *param_1);
undefined4 __nmemneed(void);
char * __cdecl utoa(uint param_1,char *param_2,uint param_3);
char * __cdecl _itoa(int _Value,char *_Dest,int _Radix);
char * __cdecl itoa(int _Val,char *_DstBuf,int _Radix);
char * __cdecl ultoa(ulong _Val,char *_Dstbuf,int _Radix);
char * __cdecl ltoa(long _Val,char *_DstBuf,int _Radix);
char * __cdecl _ltoa(long _Value,char *_Dest,int _Radix);
int __cdecl _toupper(int _C);
int __cdecl FUN_00459e4a(int param_1);
undefined1 * FUN_00459e6e(void);
void __CommonInit(void);
void __cdecl FUN_00459ea8(uint *param_1,uint param_2);
uint * __cdecl nrealloc(uint *param_1,uint param_2);
void __cdecl abort(void);
void FUN_00459f57(void);
uint __cdecl _control87(uint _NewValue,uint _Mask);
LPVOID FUN_00459fc0(void);
undefined4 __cdecl FUN_00459ff5(undefined4 param_1,int param_2);
void __cdecl __RemoveThreadData(int param_1);
void FUN_0045a10b(char *param_1);
void __cdecl __fatal_runtime_error(char *param_1);
uint __cdecl FUN_0045a174(HANDLE param_1);
int __cdecl getch(void);
int __cdecl putch(int _Ch);
undefined4 __cdecl __HasLeadingZero(double *param_1);
int __cdecl FUN_0045a3b0(char *param_1,int param_2,int param_3,int param_4);
char * __cdecl FUN_0045a453(char *param_1,char *param_2,int param_3,int param_4);
void __cdecl FUN_0045a4c6(undefined1 *param_1,size_t param_2,size_t param_3);
void __cdecl _FtoS(char *param_1,double *param_2,int param_3,char param_4,size_t param_5,int param_6,int param_7, char param_8,byte param_9);
int __cdecl _nheapshrink(void);
undefined4 __cdecl FUN_0045a9f1(HLOCAL param_1);
void __cdecl FUN_0045aa4a(HLOCAL param_1);
int __cdecl FUN_0045aa9f(int param_1);
void __cdecl FUN_0045aaac(uint param_1,uint param_2);
undefined4 __cdecl __HeapManager_expand(short param_1,uint param_2,uint param_3,uint *param_4);
uint __cdecl nexpand(uint param_1,uint param_2);
undefined4 FUN_0045acb6(void);
uintptr_t __cdecl _beginthread(_StartAddress *_StartAddress,uint _StackSize,void *_ArgList);
void __cdecl _endthread(void);
int __cdecl __initthread(undefined4 param_1);
undefined4 __cdecl __EnterWVIDEO(undefined4 param_1,undefined2 param_2);
undefined4 __cdecl FUN_0045ad67(short *param_1);
void FUN_0045ad94(void);
undefined4 __NTConsoleInput(void);
undefined4 FUN_0045ae0c(void);
undefined4 __cdecl __Nan_Inf(int param_1,uint param_2,undefined4 *param_3);
undefined8 __fastcall FUN_0045aeaa(undefined4 param_1,undefined4 param_2);
undefined8 __fastcall IF_DLOG2(undefined4 param_1,undefined4 param_2);
undefined8 __fastcall IF_DLOG10(undefined4 param_1,undefined4 param_2);
double __cdecl log10(double _X);
double __cdecl floor(double _X);
void __thiscall _Scale(void *this,int param_1,undefined4 param_2,undefined4 param_3,int param_4,uint *param_5);
double __cdecl _Scale10V(double param_1,int param_2);
char * __cdecl __cvt(double param_1,int param_2,int *param_3,undefined4 *param_4,int param_5,char *param_6);
void __fastcall __ZBuf2F(undefined4 param_1,undefined8 *param_2);
void __fastcall FUN_0045b3d9(undefined4 param_1,uint param_2);
DWORD __cdecl __CBeginThread(undefined4 param_1,undefined4 param_2,int param_3,undefined4 param_4);
void FUN_0045b5ae(void);
undefined8 __cdecl FUN_0045b5d4(int param_1,uint param_2,uint param_3);
double __cdecl modf(double _X,double *_Y);
int __CmpBigInt_(void);
void __fastcall FUN_0045b688(undefined4 param_1,uint *param_2);
uint __fastcall FUN_0045b6f3(undefined4 param_1,int param_2);
int FUN_0045b75d(void);
void FUN_0045b812(void);
uint __cdecl FUN_0045b814(int param_1,uint param_2,ushort param_3,undefined4 param_4,uint param_5,uint param_6);
void __fdivp_sti_st(void);
void FUN_0045bdda(void);
undefined4 __fdiv_m32(uint param_1);
undefined4 __fdiv_m64(undefined4 param_1,uint param_2);
undefined4 FUN_0045be98(uint param_1);
double __cdecl frexp(double _X,int *_Y);
undefined8 __cdecl __math1err(uint param_1,undefined4 *param_2);
undefined8 __cdecl __math2err(uint param_1,undefined4 *param_2,undefined4 *param_3);
void __cdecl __rterrmsg(int param_1,char *param_2);
int __cdecl _matherr(_exception *_Except);
undefined4 __matherr(void);
undefined * __cdecl __get_std_stream(uint param_1);
undefined4 FUN_0045c2ab(void);
void ExitThread(DWORD dwExitCode);
HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter,DWORD dwCreationFlags, LPDWORD lpThreadId);
HANDLE CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes,BOOL bManualReset,BOOL bInitialState, LPCSTR lpName);
HANDLE GetCurrentThread(void);
BOOL SetEvent(HANDLE hEvent);
HLOCAL LocalFree(HLOCAL hMem);
BOOL WriteConsoleA(HANDLE hConsoleOutput,void *lpBuffer,DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten,LPVOID lpReserved);
BOOL SetConsoleMode(HANDLE hConsoleHandle,DWORD dwMode);
BOOL GetConsoleMode(HANDLE hConsoleHandle,LPDWORD lpMode);
BOOL ReadConsoleInputA(HANDLE hConsoleInput,PINPUT_RECORD lpBuffer,DWORD nLength, LPDWORD lpNumberOfEventsRead);
HLOCAL LocalAlloc(UINT uFlags,SIZE_T uBytes);
BOOL DeleteFileA(LPCSTR lpFileName);
BOOL TlsFree(DWORD dwTlsIndex);
BOOL TlsSetValue(DWORD dwTlsIndex,LPVOID lpTlsValue);
DWORD TlsAlloc(void);
LPVOID TlsGetValue(DWORD dwTlsIndex);
BOOL ReleaseMutex(HANDLE hMutex);
DWORD WaitForSingleObject(HANDLE hHandle,DWORD dwMilliseconds);
HANDLE CreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes,BOOL bInitialOwner,LPCSTR lpName);
DWORD GetCurrentThreadId(void);
DWORD GetFileType(HANDLE hFile);
DWORD GetLastError(void);
HANDLE GetStdHandle(DWORD nStdHandle);
BOOL SetStdHandle(DWORD nStdHandle,HANDLE hHandle);
BOOL WriteFile(HANDLE hFile,LPCVOID lpBuffer,DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten,LPOVERLAPPED lpOverlapped);
BOOL CloseHandle(HANDLE hObject);
DWORD GetCurrentProcessId(void);
BOOL ReadFile(HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
DWORD SetFilePointer(HANDLE hFile,LONG lDistanceToMove,PLONG lpDistanceToMoveHigh,DWORD dwMoveMethod );
HMODULE GetModuleHandleA(LPCSTR lpModuleName);
DWORD GetVersion(void);
LPSTR GetCommandLineA(void);
DWORD GetModuleFileNameA(HMODULE hModule,LPSTR lpFilename,DWORD nSize);
LPCH GetEnvironmentStrings(void);
void ExitProcess(UINT uExitCode);
HANDLE CreateFileA(LPCSTR lpFileName,DWORD dwDesiredAccess,DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,HANDLE hTemplateFile);
MMRESULT joyGetDevCapsA(UINT_PTR uJoyID,LPJOYCAPSA pjc,UINT cbjc);
MMRESULT joyGetPos(UINT uJoyID,LPJOYINFO pji);
int DirectSoundCreate();
MCIERROR mciSendCommandA(MCIDEVICEID mciId,UINT uMsg,DWORD_PTR dwParam1,DWORD_PTR dwParam2);
int DirectDrawCreate();
MMRESULT timeKillEvent(UINT uTimerID);
MMRESULT timeEndPeriod(UINT uPeriod);
MMRESULT timeSetEvent(UINT uDelay,UINT uResolution,LPTIMECALLBACK fptc,DWORD_PTR dwUser,UINT fuEvent );
MMRESULT timeBeginPeriod(UINT uPeriod);
extern int extraout_DL;
extern int extraout_DL_00;
extern int extraout_EAX;
extern int extraout_ECX;
extern int extraout_ECX_00;
extern int extraout_ECX_01;
extern int extraout_EDX;
extern int extraout_ST0;
extern int extraout_ST0_00;
extern int extraout_ST1;
extern int extraout_var;
extern int in_AF;
extern int in_AL;
extern int in_DS;
extern int in_EAX;
extern int in_ECX;
extern int in_EDX;
extern int in_FPUControlWord;
extern int in_FPUStatusWord;
extern int in_ST0;
extern int in_ST1;
extern int unaff_BH;
extern int unaff_EBP;
extern int unaff_EBX;
extern int unaff_EDI;
extern int unaff_ESI;
extern int unaff_FS_OFFSET;
extern int unaff_retaddr;
#endif
