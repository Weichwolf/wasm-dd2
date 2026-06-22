#ifndef DD2_SYMBOLS_H
#define DD2_SYMBOLS_H
#include "ghidra_compat.h"
extern unsigned char* g_image;
#define GIMG(va) ((unsigned char*)(uintptr_t)(va))
#define _DAT_00716c18 (*(int*)GIMG(0x716c18))
#define s__R_JL_T__0046ad0c ((char*)GIMG(0x46ad0c))
#define s__R_JL_T__0046be14 ((char*)GIMG(0x46be14))
#define s__R_JL_T__0046b620 ((char*)GIMG(0x46b620))
#define DAT_00467bf4 (*(int*)GIMG(0x467bf4))
#define DAT_00467eec (*(int*)GIMG(0x467eec))
#define PTR_DAT_00464bf4 (*(int*)GIMG(0x464bf4))
extern void draw_text_half(void);
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
#define CLUT_Anim_Casino (*(int*)GIMG(0x46518c))
#define CLUT_Anim_Dollar (*(int*)GIMG(0x465160))
#define CLUT_Anim_Safe (*(int*)GIMG(0x465174))
#define CLUT_Anim_Twist1 (*(int*)GIMG(0x4651d0))
#define CLUT_Anim_Twist2 (*(int*)GIMG(0x4651e4))
#define COSE (*(int*)GIMG(0x4604c8))
#define Car_Friction (*(int*)GIMG(0x465a20))
#define Caravan_Track_Type (*(int*)GIMG(0x465d70))
#define Collision_Counter (*(int*)GIMG(0x467068))
#define DAT_00460004 (*(int*)GIMG(0x460004))
#define DAT_0046000c (*(int*)GIMG(0x46000c))
#define DAT_00460010 (*(int*)GIMG(0x460010))
#define DAT_0046042c (*(int*)GIMG(0x46042c))
#define DAT_00460434 (*(int*)GIMG(0x460434))
#define DAT_00460438 (*(int**)GIMG(0x460438))
#define DAT_0046043c (*(int**)GIMG(0x46043c))
#define DAT_00460440 (*(int**)GIMG(0x460440))
#define DAT_00460444 (*(int**)GIMG(0x460444))
#define DAT_00460448 (*(int*)GIMG(0x460448))
#define DAT_0046044c (*(int*)GIMG(0x46044c))
#define DAT_00460474 (*(int*)GIMG(0x460474))
#define DAT_00460478 (*(int*)GIMG(0x460478))
#define DAT_0046047c (*(int*)GIMG(0x46047c))
#define DAT_00460488 (*(int*)GIMG(0x460488))
#define DAT_0046048c (*(int*)GIMG(0x46048c))
#define DAT_00460490 (*(int*)GIMG(0x460490))
#define DAT_004604a0 (*(int*)GIMG(0x4604a0))
#define DAT_004604a4 (*(int*)GIMG(0x4604a4))
#define DAT_004604a8 (*(int*)GIMG(0x4604a8))
#define DAT_004604ac (*(int*)GIMG(0x4604ac))
#define DAT_004604b0 (*(int*)GIMG(0x4604b0))
#define DAT_004604b2 (*(int*)GIMG(0x4604b2))
#define DAT_004604b6 (*(int*)GIMG(0x4604b6))
#define DAT_004604ba (*(int*)GIMG(0x4604ba))
#define DAT_004604be (*(int*)GIMG(0x4604be))
#define DAT_00460cc8 (*(int*)GIMG(0x460cc8))
#define DAT_00462cc8 (*(int*)GIMG(0x462cc8))
#define DAT_00462cd8 (*(int*)GIMG(0x462cd8))
#define DAT_00462cdc (*(int*)GIMG(0x462cdc))
#define DAT_00462d64 (*(int*)GIMG(0x462d64))
#define DAT_00462d68 (*(int*)GIMG(0x462d68))
#define DAT_00462d6c (*(int*)GIMG(0x462d6c))
#define DAT_00462d70 (*(int*)GIMG(0x462d70))
#define DAT_00462d74 (*(int*)GIMG(0x462d74))
#define DAT_00462d78 (*(int*)GIMG(0x462d78))
#define DAT_00462d80 (*(int*)GIMG(0x462d80))
#define DAT_00462d84 (*(int*)GIMG(0x462d84))
#define DAT_00462d88 (*(int*)GIMG(0x462d88))
#define DAT_00462d8c (*(int*)GIMG(0x462d8c))
#define DAT_00462d90 (*(int*)GIMG(0x462d90))
#define DAT_00462fa6 (*(int*)GIMG(0x462fa6))
#define DAT_00462faa (*(int*)GIMG(0x462faa))
#define DAT_00462fb6 (*(int*)GIMG(0x462fb6))
#define DAT_00462fba (*(int*)GIMG(0x462fba))
#define DAT_00462fbe (*(int*)GIMG(0x462fbe))
#define DAT_00462fc4 (*(int*)GIMG(0x462fc4))
#define DAT_00462fcc (*(int*)GIMG(0x462fcc))
#define DAT_00463002 (*(int*)GIMG(0x463002))
#define DAT_00463006 (*(int*)GIMG(0x463006))
#define DAT_00463010 (*(int*)GIMG(0x463010))
#define DAT_00463018 (*(int*)GIMG(0x463018))
#define DAT_0046301c (*(int*)GIMG(0x46301c))
#define DAT_00463020 (*(int*)GIMG(0x463020))
#define DAT_00463024 (*(int*)GIMG(0x463024))
#define DAT_0046302d (*(int*)GIMG(0x46302d))
#define DAT_0046302e (*(int*)GIMG(0x46302e))
#define DAT_0046302f (*(int*)GIMG(0x46302f))
#define DAT_00463030 (*(int*)GIMG(0x463030))
#define DAT_00463031 (*(int*)GIMG(0x463031))
#define DAT_00463032 (*(int*)GIMG(0x463032))
#define DAT_00463033 (*(int*)GIMG(0x463033))
#define DAT_00463034 (*(int*)GIMG(0x463034))
#define DAT_00463035 (*(int*)GIMG(0x463035))
#define DAT_00463036 (*(int*)GIMG(0x463036))
#define DAT_00463037 (*(int*)GIMG(0x463037))
#define DAT_00463038 (*(int*)GIMG(0x463038))
#define DAT_00463039 (*(int*)GIMG(0x463039))
#define DAT_0046303e (*(int*)GIMG(0x46303e))
#define DAT_0046303f (*(int*)GIMG(0x46303f))
#define DAT_00463046 (*(int*)GIMG(0x463046))
#define DAT_00463047 (*(int*)GIMG(0x463047))
#define DAT_00463048 (*(int*)GIMG(0x463048))
#define DAT_00463049 (*(int*)GIMG(0x463049))
#define DAT_0046304a (*(int*)GIMG(0x46304a))
#define DAT_0046304e (*(int*)GIMG(0x46304e))
#define DAT_0046304f (*(int*)GIMG(0x46304f))
#define DAT_00463050 (*(int*)GIMG(0x463050))
#define DAT_00463052 (*(int*)GIMG(0x463052))
#define DAT_00463860 (*(int*)GIMG(0x463860))
#define DAT_00463864 (*(int*)GIMG(0x463864))
#define DAT_00463868 (*(int*)GIMG(0x463868))
#define DAT_0046386c (*(int*)GIMG(0x46386c))
#define DAT_00463870 (*(int*)GIMG(0x463870))
#define DAT_00463874 (*(int*)GIMG(0x463874))
#define DAT_00463878 (*(int*)GIMG(0x463878))
#define DAT_0046388a (*(int*)GIMG(0x46388a))
#define DAT_0046388e (*(int*)GIMG(0x46388e))
#define DAT_00463892 (*(int*)GIMG(0x463892))
#define DAT_00463896 (*(int*)GIMG(0x463896))
#define DAT_004638b8 (*(int*)GIMG(0x4638b8))
#define DAT_004638bc (*(int*)GIMG(0x4638bc))
#define DAT_004638c0 (*(int*)GIMG(0x4638c0))
#define DAT_004638c4 (*(int*)GIMG(0x4638c4))
#define DAT_00463dcc (*(int*)GIMG(0x463dcc))
#define DAT_00463dd0 (*(int*)GIMG(0x463dd0))
#define DAT_00463e2c (*(int*)GIMG(0x463e2c))
#define DAT_00463e30 (*(int*)GIMG(0x463e30))
#define DAT_00463e8c (*(int*)GIMG(0x463e8c))
#define DAT_00463ef0 (*(int*)GIMG(0x463ef0))
#define DAT_00463ef8 (*(int*)GIMG(0x463ef8))
#define DAT_00463efc (*(int*)GIMG(0x463efc))
#define DAT_00463f00 (*(int*)GIMG(0x463f00))
#define DAT_00463f04 (*(int*)GIMG(0x463f04))
#define DAT_00463f08 (*(int*)GIMG(0x463f08))
#define DAT_00463f0c (*(int*)GIMG(0x463f0c))
#define DAT_00463f10 (*(int*)GIMG(0x463f10))
#define DAT_00463f14 (*(int*)GIMG(0x463f14))
#define DAT_00463f1c (*(int*)GIMG(0x463f1c))
#define DAT_00463f20 (*(int*)GIMG(0x463f20))
#define DAT_00463f24 (*(int*)GIMG(0x463f24))
#define DAT_00463f28 (*(int*)GIMG(0x463f28))
#define DAT_00463f2c (*(int*)GIMG(0x463f2c))
#define DAT_00464a70 (*(int*)GIMG(0x464a70))
#define DAT_00464a90 (*(int*)GIMG(0x464a90))
#define DAT_00464aa0 (*(int*)GIMG(0x464aa0))
#define DAT_00464aa4 (*(int*)GIMG(0x464aa4))
#define DAT_00464aa8 (*(int*)GIMG(0x464aa8))
#define DAT_00464aac (*(int*)GIMG(0x464aac))
#define DAT_00464ab0 (*(int*)GIMG(0x464ab0))
#define DAT_00464bb0 (*(int*)GIMG(0x464bb0))
#define DAT_00464bb4 (*(int*)GIMG(0x464bb4))
#define DAT_00464bb8 (*(int*)GIMG(0x464bb8))
#define DAT_00464bbc (*(int*)GIMG(0x464bbc))
#define DAT_00464bf0 (*(int*)GIMG(0x464bf0))
#define DAT_00464c0e (*(int*)GIMG(0x464c0e))
#define DAT_00464c26 (*(int*)GIMG(0x464c26))
#define DAT_00464c3e (*(int*)GIMG(0x464c3e))
#define DAT_00464c56 (*(int*)GIMG(0x464c56))
#define DAT_00464c6e (*(int*)GIMG(0x464c6e))
#define DAT_00464c86 (*(int*)GIMG(0x464c86))
#define DAT_00464c9e (*(int*)GIMG(0x464c9e))
#define DAT_00464cb8 (*(int*)GIMG(0x464cb8))
#define DAT_00464cd0 (*(int*)GIMG(0x464cd0))
#define DAT_00464cd4 (*(int*)GIMG(0x464cd4))
#define DAT_00464cf0 (*(int*)GIMG(0x464cf0))
#define DAT_00464cf4 (*(int*)GIMG(0x464cf4))
#define DAT_00464d1c (*(int*)GIMG(0x464d1c))
#define DAT_00464d2c (*(int*)GIMG(0x464d2c))
#define DAT_00464e50 (*(int*)GIMG(0x464e50))
#define DAT_00464e58 (*(int*)GIMG(0x464e58))
#define DAT_00464e80 (*(int*)GIMG(0x464e80))
#define DAT_00464e84 (*(int*)GIMG(0x464e84))
#define DAT_00464e88 (*(int*)GIMG(0x464e88))
#define DAT_00464e8c (*(int*)GIMG(0x464e8c))
#define DAT_00464e90 (*(int*)GIMG(0x464e90))
#define DAT_00464e94 (*(int*)GIMG(0x464e94))
#define DAT_00464e98 (*(int*)GIMG(0x464e98))
#define DAT_004651f8 (*(int*)GIMG(0x4651f8))
#define DAT_0046520a (*(int*)GIMG(0x46520a))
#define DAT_0046520e (*(int*)GIMG(0x46520e))
#define DAT_00465212 (*(int*)GIMG(0x465212))
#define DAT_00465255 (*(int*)GIMG(0x465255))
#define DAT_00465256 (*(int*)GIMG(0x465256))
#define DAT_0046526a (*(int*)GIMG(0x46526a))
#define DAT_0046526e (*(int*)GIMG(0x46526e))
#define DAT_004652a4 (*(int*)GIMG(0x4652a4))
#define DAT_004652bc (*(int*)GIMG(0x4652bc))
#define DAT_004652d4 (*(int*)GIMG(0x4652d4))
#define DAT_004652ec (*(int*)GIMG(0x4652ec))
#define DAT_00465304 (*(int*)GIMG(0x465304))
#define DAT_0046531c (*(int*)GIMG(0x46531c))
#define DAT_00465334 (*(int*)GIMG(0x465334))
#define DAT_0046534c (*(int*)GIMG(0x46534c))
#define DAT_00465364 (*(int*)GIMG(0x465364))
#define DAT_0046537c (*(int*)GIMG(0x46537c))
#define DAT_00465394 (*(int*)GIMG(0x465394))
#define DAT_004653ac (*(int*)GIMG(0x4653ac))
#define DAT_004653c8 (*(int*)GIMG(0x4653c8))
#define DAT_004653e0 (*(int*)GIMG(0x4653e0))
#define DAT_004653f8 (*(int*)GIMG(0x4653f8))
#define DAT_00465410 (*(int*)GIMG(0x465410))
#define DAT_00465428 (*(int*)GIMG(0x465428))
#define DAT_00465440 (*(int*)GIMG(0x465440))
#define DAT_00465458 (*(int*)GIMG(0x465458))
#define DAT_00465470 (*(int*)GIMG(0x465470))
#define DAT_00465488 (*(int*)GIMG(0x465488))
#define DAT_0046548a (*(int*)GIMG(0x46548a))
#define DAT_004654a0 (*(int*)GIMG(0x4654a0))
#define DAT_004654a2 (*(int*)GIMG(0x4654a2))
#define DAT_004654b8 (*(int*)GIMG(0x4654b8))
#define DAT_004654d0 (*(int*)GIMG(0x4654d0))
#define DAT_004654e8 (*(int*)GIMG(0x4654e8))
#define DAT_00465500 (*(int*)GIMG(0x465500))
#define DAT_00465518 (*(int*)GIMG(0x465518))
#define DAT_00465530 (*(int*)GIMG(0x465530))
#define DAT_00465548 (*(int*)GIMG(0x465548))
#define DAT_00465560 (*(int*)GIMG(0x465560))
#define DAT_00465578 (*(int*)GIMG(0x465578))
#define DAT_00465590 (*(int*)GIMG(0x465590))
#define DAT_004655a8 (*(int*)GIMG(0x4655a8))
#define DAT_004655c0 (*(int*)GIMG(0x4655c0))
#define DAT_004655d8 (*(int*)GIMG(0x4655d8))
#define DAT_004655f0 (*(int*)GIMG(0x4655f0))
#define DAT_00465608 (*(int*)GIMG(0x465608))
#define DAT_00465698 (*(int*)GIMG(0x465698))
#define DAT_00465728 (*(int*)GIMG(0x465728))
#define DAT_0046572a (*(int*)GIMG(0x46572a))
#define DAT_004657b8 (*(int*)GIMG(0x4657b8))
#define DAT_004657d0 (*(int*)GIMG(0x4657d0))
#define DAT_004657e8 (*(int*)GIMG(0x4657e8))
#define DAT_00465800 (*(int*)GIMG(0x465800))
#define DAT_00465804 (*(int*)GIMG(0x465804))
#define DAT_00465858 (*(int*)GIMG(0x465858))
#define DAT_00465872 (*(int*)GIMG(0x465872))
#define DAT_004658c2 (*(int*)GIMG(0x4658c2))
#define DAT_004658c4 (*(int*)GIMG(0x4658c4))
#define DAT_004658c6 (*(int*)GIMG(0x4658c6))
#define DAT_0046591a (*(int*)GIMG(0x46591a))
#define DAT_0046591c (*(int*)GIMG(0x46591c))
#define DAT_0046591e (*(int*)GIMG(0x46591e))
#define DAT_00465920 (*(int*)GIMG(0x465920))
#define DAT_00465924 (*(int*)GIMG(0x465924))
#define DAT_00465926 (*(int*)GIMG(0x465926))
#define DAT_00465980 (*(int*)GIMG(0x465980))
#define DAT_00465a0c (*(int*)GIMG(0x465a0c))
#define DAT_00465cb0 (*(int*)GIMG(0x465cb0))
#define DAT_00465cc8 (*(int*)GIMG(0x465cc8))
#define DAT_00465ce0 (*(int*)GIMG(0x465ce0))
#define DAT_00465ce8 (*(int*)GIMG(0x465ce8))
#define DAT_00465d10 (*(int*)GIMG(0x465d10))
#define DAT_00465d48 (*(int*)GIMG(0x465d48))
#define DAT_00465d58 (*(int*)GIMG(0x465d58))
#define DAT_00465d88 (*(int*)GIMG(0x465d88))
#define DAT_00465db0 (*(int*)GIMG(0x465db0))
#define DAT_00465dd0 (*(int*)GIMG(0x465dd0))
#define DAT_00465dd4 (*(int*)GIMG(0x465dd4))
#define DAT_00465e34 (*(int*)GIMG(0x465e34))
#define DAT_00466290 (*(int**)GIMG(0x466290))
#define DAT_004662f8 (*(int*)GIMG(0x4662f8))
#define DAT_00466304 (*(int*)GIMG(0x466304))
#define DAT_00466306 (*(int*)GIMG(0x466306))
#define DAT_0046630a (*(int*)GIMG(0x46630a))
#define DAT_0046630b (*(int*)GIMG(0x46630b))
#define DAT_00466310 (*(int*)GIMG(0x466310))
#define DAT_0046631c (*(int*)GIMG(0x46631c))
#define DAT_0046631e (*(int*)GIMG(0x46631e))
#define DAT_00466322 (*(int*)GIMG(0x466322))
#define DAT_00466323 (*(int*)GIMG(0x466323))
#define DAT_00466328 (*(int*)GIMG(0x466328))
#define DAT_00466334 (*(int*)GIMG(0x466334))
#define DAT_00466336 (*(int*)GIMG(0x466336))
#define DAT_0046633a (*(int*)GIMG(0x46633a))
#define DAT_0046633b (*(int*)GIMG(0x46633b))
#define DAT_00466340 (*(int*)GIMG(0x466340))
#define DAT_00466342 (*(int*)GIMG(0x466342))
#define DAT_00466344 (*(int*)GIMG(0x466344))
#define DAT_00466346 (*(int*)GIMG(0x466346))
#define DAT_0046637c (*(int*)GIMG(0x46637c))
#define DAT_00466394 (*(int*)GIMG(0x466394))
#define DAT_004663ac (*(int*)GIMG(0x4663ac))
#define DAT_004663c4 (*(int*)GIMG(0x4663c4))
#define DAT_004663dc (*(int*)GIMG(0x4663dc))
#define DAT_004663f4 (*(int*)GIMG(0x4663f4))
#define DAT_0046640c (*(int*)GIMG(0x46640c))
#define DAT_00466424 (*(int*)GIMG(0x466424))
#define DAT_0046643c (*(int*)GIMG(0x46643c))
#define DAT_00466454 (*(int*)GIMG(0x466454))
#define DAT_0046646c (*(int*)GIMG(0x46646c))
#define DAT_00466484 (*(int*)GIMG(0x466484))
#define DAT_0046649c (*(int*)GIMG(0x46649c))
#define DAT_004664b4 (*(int*)GIMG(0x4664b4))
#define DAT_004664cc (*(int*)GIMG(0x4664cc))
#define DAT_004664fc (*(int*)GIMG(0x4664fc))
#define DAT_0046653e (*(int*)GIMG(0x46653e))
#define DAT_00466626 (*(int*)GIMG(0x466626))
#define DAT_0046662a (*(int*)GIMG(0x46662a))
#define DAT_0046662e (*(int*)GIMG(0x46662e))
#define DAT_004666f4 (*(int*)GIMG(0x4666f4))
#define DAT_00466704 (*(int*)GIMG(0x466704))
#define DAT_00466714 (*(int*)GIMG(0x466714))
#define DAT_00466724 (*(int*)GIMG(0x466724))
#define DAT_00466734 (*(int*)GIMG(0x466734))
#define DAT_00466754 (*(int*)GIMG(0x466754))
#define DAT_0046675d (*(int*)GIMG(0x46675d))
#define DAT_00466772 (*(int*)GIMG(0x466772))
#define DAT_0046677c (*(int*)GIMG(0x46677c))
#define DAT_00466786 (*(int*)GIMG(0x466786))
#define DAT_0046678f (*(int*)GIMG(0x46678f))
#define DAT_00466796 (*(int*)GIMG(0x466796))
#define DAT_0046679f (*(int*)GIMG(0x46679f))
#define DAT_004667a8 (*(int*)GIMG(0x4667a8))
#define DAT_004667b6 (*(int*)GIMG(0x4667b6))
#define DAT_004667be (*(int*)GIMG(0x4667be))
#define DAT_004667c4 (*(int*)GIMG(0x4667c4))
#define DAT_004667c8 (*(int*)GIMG(0x4667c8))
#define DAT_004667cc (*(int*)GIMG(0x4667cc))
#define DAT_004667cf (*(int*)GIMG(0x4667cf))
#define DAT_004667d6 (*(int*)GIMG(0x4667d6))
#define DAT_004667d9 (*(int*)GIMG(0x4667d9))
#define DAT_004667e5 (*(int*)GIMG(0x4667e5))
#define DAT_004667e7 (*(int*)GIMG(0x4667e7))
#define DAT_004667e9 (*(int*)GIMG(0x4667e9))
#define DAT_004667ec (*(int*)GIMG(0x4667ec))
#define DAT_004667f0 (*(int*)GIMG(0x4667f0))
#define DAT_004667f2 (*(int*)GIMG(0x4667f2))
#define DAT_004667f5 (*(int*)GIMG(0x4667f5))
#define DAT_004667fc (*(int*)GIMG(0x4667fc))
#define DAT_004667ff (*(int*)GIMG(0x4667ff))
#define DAT_0046680a (*(int*)GIMG(0x46680a))
#define DAT_0046680c (*(int*)GIMG(0x46680c))
#define DAT_00466810 (*(int*)GIMG(0x466810))
#define DAT_00466814 (*(int*)GIMG(0x466814))
#define DAT_00466817 (*(int*)GIMG(0x466817))
#define DAT_00466834 (*(int*)GIMG(0x466834))
#define DAT_0046688c (*(int*)GIMG(0x46688c))
#define DAT_004668ec (*(int*)GIMG(0x4668ec))
#define DAT_00466934 (*(int*)GIMG(0x466934))
#define DAT_004669b4 (*(int*)GIMG(0x4669b4))
#define DAT_004669bc (*(int*)GIMG(0x4669bc))
#define DAT_00466a20 (*(int*)GIMG(0x466a20))
#define DAT_00466a21 (*(int*)GIMG(0x466a21))
#define DAT_00466a45 (*(int*)GIMG(0x466a45))
#define DAT_00466a4a (*(int*)GIMG(0x466a4a))
#define DAT_00466a7d (*(int*)GIMG(0x466a7d))
#define DAT_00466a96 (*(int*)GIMG(0x466a96))
#define DAT_00466a9a (*(int*)GIMG(0x466a9a))
#define DAT_00466a9e (*(int*)GIMG(0x466a9e))
#define DAT_00466aa2 (*(int*)GIMG(0x466aa2))
#define DAT_00466aa6 (*(int*)GIMG(0x466aa6))
#define DAT_00466aaa (*(int*)GIMG(0x466aaa))
#define DAT_00466aae (*(int*)GIMG(0x466aae))
#define DAT_00466ab2 (*(int*)GIMG(0x466ab2))
#define DAT_00466ab6 (*(int*)GIMG(0x466ab6))
#define DAT_00466aba (*(int*)GIMG(0x466aba))
#define DAT_00466abe (*(int*)GIMG(0x466abe))
#define DAT_00466ac2 (*(int*)GIMG(0x466ac2))
#define DAT_00466ac6 (*(int*)GIMG(0x466ac6))
#define DAT_00466aca (*(int*)GIMG(0x466aca))
#define DAT_00466ace (*(int*)GIMG(0x466ace))
#define DAT_00466ad2 (*(int*)GIMG(0x466ad2))
#define DAT_00466ad8 (*(int*)GIMG(0x466ad8))
#define DAT_00466adc (*(int*)GIMG(0x466adc))
#define DAT_00466ae0 (*(int*)GIMG(0x466ae0))
#define DAT_00466ae4 (*(int*)GIMG(0x466ae4))
#define DAT_00466b28 (*(int*)GIMG(0x466b28))
#define DAT_00466b2c (*(int*)GIMG(0x466b2c))
#define DAT_00466b54 (*(int*)GIMG(0x466b54))
#define DAT_00466d4e (*(int*)GIMG(0x466d4e))
#define DAT_00466d52 (*(int*)GIMG(0x466d52))
#define DAT_00466d56 (*(int*)GIMG(0x466d56))
#define DAT_00466d5a (*(int*)GIMG(0x466d5a))
#define DAT_00466d5e (*(int*)GIMG(0x466d5e))
#define DAT_00466d62 (*(int*)GIMG(0x466d62))
#define DAT_00466d66 (*(int*)GIMG(0x466d66))
#define DAT_00466d6a (*(int*)GIMG(0x466d6a))
#define DAT_00466d6e (*(int*)GIMG(0x466d6e))
#define DAT_00466d72 (*(int*)GIMG(0x466d72))
#define DAT_00466d76 (*(int*)GIMG(0x466d76))
#define DAT_00466d7a (*(int*)GIMG(0x466d7a))
#define DAT_00466d7e (*(int*)GIMG(0x466d7e))
#define DAT_00466d82 (*(int*)GIMG(0x466d82))
#define DAT_00466d86 (*(int*)GIMG(0x466d86))
#define DAT_00466d8a (*(int*)GIMG(0x466d8a))
#define DAT_00466dee (*(int*)GIMG(0x466dee))
#define DAT_00466df2 (*(int*)GIMG(0x466df2))
#define DAT_00466df4 (*(int*)GIMG(0x466df4))
#define DAT_00466e38 (*(int*)GIMG(0x466e38))
#define DAT_00466e40 (*(int*)GIMG(0x466e40))
#define DAT_00466e44 (*(int*)GIMG(0x466e44))
#define DAT_00466e90 (*(int*)GIMG(0x466e90))
#define DAT_00466eac (*(int*)GIMG(0x466eac))
#define DAT_00466ebc (*(int*)GIMG(0x466ebc))
#define DAT_00466ee4 (*(int*)GIMG(0x466ee4))
#define DAT_00466f26 (*(int*)GIMG(0x466f26))
#define DAT_00466f27 (*(int*)GIMG(0x466f27))
#define DAT_00466f56 (*(int*)GIMG(0x466f56))
#define DAT_00466f57 (*(int*)GIMG(0x466f57))
#define DAT_00466f5c (*(int*)GIMG(0x466f5c))
#define DAT_00466f8c (*(int*)GIMG(0x466f8c))
#define DAT_00466fa4 (*(int*)GIMG(0x466fa4))
#define DAT_00466fec (*(int*)GIMG(0x466fec))
#define DAT_00467004 (*(int*)GIMG(0x467004))
#define DAT_00467060 (*(int*)GIMG(0x467060))
#define DAT_00467064 (*(int*)GIMG(0x467064))
#define DAT_00467074 (*(int*)GIMG(0x467074))
#define DAT_00467078 (*(int*)GIMG(0x467078))
#define DAT_004670a8 (*(int*)GIMG(0x4670a8))
#define DAT_004670ac (*(int*)GIMG(0x4670ac))
#define DAT_004670b0 (*(int*)GIMG(0x4670b0))
#define DAT_004670b8 (*(int*)GIMG(0x4670b8))
#define DAT_004670bc (*(int*)GIMG(0x4670bc))
#define DAT_004670e0 (*(int*)GIMG(0x4670e0))
#define DAT_004670e4 (*(int*)GIMG(0x4670e4))
#define DAT_00467168 (*(int*)GIMG(0x467168))
#define DAT_00467180 (*(int*)GIMG(0x467180))
#define DAT_00467198 (*(int*)GIMG(0x467198))
#define DAT_004671b0 (*(int*)GIMG(0x4671b0))
#define DAT_004671c8 (*(int*)GIMG(0x4671c8))
#define DAT_00467224 (*(int*)GIMG(0x467224))
#define DAT_00467390 (*(int*)GIMG(0x467390))
#define DAT_00467394 (*(int*)GIMG(0x467394))
#define DAT_004673ac (*(int*)GIMG(0x4673ac))
#define DAT_004673c4 (*(int*)GIMG(0x4673c4))
#define DAT_004673dc (*(int*)GIMG(0x4673dc))
#define DAT_00467498 (*(int*)GIMG(0x467498))
#define DAT_004674e8 (*(int*)GIMG(0x4674e8))
#define DAT_00467534 (*(int*)GIMG(0x467534))
#define DAT_00467564 (*(int*)GIMG(0x467564))
#define DAT_00467568 (*(int*)GIMG(0x467568))
#define DAT_0046757a (*(int*)GIMG(0x46757a))
#define DAT_0046758c (*(int*)GIMG(0x46758c))
#define DAT_00467658 (*(int*)GIMG(0x467658))
#define DAT_00467660 (*(int*)GIMG(0x467660))
#define DAT_00467794 (*(int*)GIMG(0x467794))
#define DAT_00468094 (*(int*)GIMG(0x468094))
#define DAT_004680ca (*(int*)GIMG(0x4680ca))
#define DAT_004680cc (*(int*)GIMG(0x4680cc))
#define DAT_004680ce (*(int*)GIMG(0x4680ce))
#define DAT_004680d0 (*(int*)GIMG(0x4680d0))
#define DAT_004680e0 (*(int*)GIMG(0x4680e0))
#define DAT_004680f0 (*(int*)GIMG(0x4680f0))
#define DAT_00468100 (*(int*)GIMG(0x468100))
#define DAT_004682f0 (*(int*)GIMG(0x4682f0))
#define DAT_004685e0 (*(int*)GIMG(0x4685e0))
#define DAT_00468664 (*(int*)GIMG(0x468664))
#define DAT_0046867c (*(int*)GIMG(0x46867c))
#define DAT_00468694 (*(int*)GIMG(0x468694))
#define DAT_004686ac (*(int*)GIMG(0x4686ac))
#define DAT_00468728 (*(int*)GIMG(0x468728))
#define DAT_00468818 (*(int*)GIMG(0x468818))
#define DAT_00468ac8 (*(int*)GIMG(0x468ac8))
#define DAT_00468ea4 (*(int*)GIMG(0x468ea4))
#define DAT_00468eb4 (*(int*)GIMG(0x468eb4))
#define DAT_00468eb8 (*(int*)GIMG(0x468eb8))
#define DAT_00468ebc (*(int*)GIMG(0x468ebc))
#define DAT_00468edc (*(int*)GIMG(0x468edc))
#define DAT_00469120 (*(int*)GIMG(0x469120))
#define DAT_00469220 (*(int*)GIMG(0x469220))
#define DAT_00469274 (*(int*)GIMG(0x469274))
#define DAT_004692b0 (*(int*)GIMG(0x4692b0))
#define DAT_00469348 (*(int*)GIMG(0x469348))
#define DAT_004693a8 (*(int*)GIMG(0x4693a8))
#define DAT_00469560 (*(int*)GIMG(0x469560))
#define DAT_00469564 (*(int*)GIMG(0x469564))
#define DAT_004696a4 (*(int*)GIMG(0x4696a4))
#define DAT_0046974c (*(int*)GIMG(0x46974c))
#define DAT_004697c4 (*(int*)GIMG(0x4697c4))
#define DAT_004697c8 (*(int*)GIMG(0x4697c8))
#define DAT_004697ca (*(int*)GIMG(0x4697ca))
#define DAT_004697d4 (*(int*)GIMG(0x4697d4))
#define DAT_004698a4 (*(int*)GIMG(0x4698a4))
#define DAT_004698a5 (*(int*)GIMG(0x4698a5))
#define DAT_004698a6 (*(int*)GIMG(0x4698a6))
#define DAT_004698a7 (*(int*)GIMG(0x4698a7))
#define DAT_0046996c (*(int*)GIMG(0x46996c))
#define DAT_00469974 (*(int*)GIMG(0x469974))
#define DAT_004699b4 (*(int*)GIMG(0x4699b4))
#define DAT_004699c9 (*(int*)GIMG(0x4699c9))
#define DAT_004699ca (*(int*)GIMG(0x4699ca))
#define DAT_004699cc (*(int*)GIMG(0x4699cc))
#define DAT_004699d2 (*(int*)GIMG(0x4699d2))
#define DAT_00469efc (*(int*)GIMG(0x469efc))
#define DAT_00469f4c (*(int*)GIMG(0x469f4c))
#define DAT_00469f9c (*(int*)GIMG(0x469f9c))
#define DAT_0046a110 (*(int*)GIMG(0x46a110))
#define DAT_0046a17c (*(int*)GIMG(0x46a17c))
#define DAT_0046a1e4 (*(int*)GIMG(0x46a1e4))
#define DAT_0046a1e6 (*(int*)GIMG(0x46a1e6))
#define DAT_0046a208 (*(int*)GIMG(0x46a208))
#define DAT_0046a20a (*(int*)GIMG(0x46a20a))
#define DAT_0046a2ca (*(int*)GIMG(0x46a2ca))
#define DAT_0046a308 (*(int*)GIMG(0x46a308))
#define DAT_0046a370 (*(int*)GIMG(0x46a370))
#define DAT_0046a372 (*(int*)GIMG(0x46a372))
#define DAT_0046a3a8 (*(int*)GIMG(0x46a3a8))
#define DAT_0046a3aa (*(int*)GIMG(0x46a3aa))
#define DAT_0046a444 (*(int*)GIMG(0x46a444))
#define DAT_0046a4d0 (*(int*)GIMG(0x46a4d0))
#define DAT_0046a534 (*(int*)GIMG(0x46a534))
#define DAT_0046a538 (*(int*)GIMG(0x46a538))
#define DAT_0046a53a (*(int*)GIMG(0x46a53a))
#define DAT_0046a570 (*(int*)GIMG(0x46a570))
#define DAT_0046a571 (*(int*)GIMG(0x46a571))
#define DAT_0046a572 (*(int*)GIMG(0x46a572))
#define DAT_0046a573 (*(int*)GIMG(0x46a573))
#define DAT_0046a672 (*(int*)GIMG(0x46a672))
#define DAT_0046a6b8 (*(int*)GIMG(0x46a6b8))
#define DAT_0046a720 (*(int*)GIMG(0x46a720))
#define DAT_0046a722 (*(int*)GIMG(0x46a722))
#define DAT_0046a76c (*(int*)GIMG(0x46a76c))
#define DAT_0046a76e (*(int*)GIMG(0x46a76e))
#define DAT_0046a920 (*(int*)GIMG(0x46a920))
#define DAT_0046a9b8 (*(int*)GIMG(0x46a9b8))
#define DAT_0046aa3c (*(int*)GIMG(0x46aa3c))
#define DAT_0046aaa4 (*(int*)GIMG(0x46aaa4))
#define DAT_0046aaa6 (*(int*)GIMG(0x46aaa6))
#define DAT_0046aacc (*(int*)GIMG(0x46aacc))
#define DAT_0046ad00 (*(int*)GIMG(0x46ad00))
#define DAT_0046ae20 (*(int*)GIMG(0x46ae20))
#define DAT_0046aef8 (*(int*)GIMG(0x46aef8))
#define DAT_0046af4c (*(int*)GIMG(0x46af4c))
#define DAT_0046af4d (*(int*)GIMG(0x46af4d))
#define DAT_0046af4e (*(int*)GIMG(0x46af4e))
#define DAT_0046af70 (*(int*)GIMG(0x46af70))
#define DAT_0046af74 (*(int*)GIMG(0x46af74))
#define DAT_0046af76 (*(int*)GIMG(0x46af76))
#define DAT_0046afe8 (*(int*)GIMG(0x46afe8))
#define DAT_0046afe9 (*(int*)GIMG(0x46afe9))
#define DAT_0046afea (*(int*)GIMG(0x46afea))
#define DAT_0046afeb (*(int*)GIMG(0x46afeb))
#define DAT_0046b000 (*(int*)GIMG(0x46b000))
#define DAT_0046b001 (*(int*)GIMG(0x46b001))
#define DAT_0046b002 (*(int*)GIMG(0x46b002))
#define DAT_0046b003 (*(int*)GIMG(0x46b003))
#define DAT_0046b6c0 (*(int*)GIMG(0x46b6c0))
#define DAT_0046b74c (*(int*)GIMG(0x46b74c))
#define DAT_0046b7a0 (*(int*)GIMG(0x46b7a0))
#define DAT_0046b7a1 (*(int*)GIMG(0x46b7a1))
#define DAT_0046b7a2 (*(int*)GIMG(0x46b7a2))
#define DAT_0046b7c4 (*(int*)GIMG(0x46b7c4))
#define DAT_0046b7c8 (*(int*)GIMG(0x46b7c8))
#define DAT_0046b7ca (*(int*)GIMG(0x46b7ca))
#define DAT_0046b800 (*(int*)GIMG(0x46b800))
#define DAT_0046b801 (*(int*)GIMG(0x46b801))
#define DAT_0046b802 (*(int*)GIMG(0x46b802))
#define DAT_0046b803 (*(int*)GIMG(0x46b803))
#define DAT_0046b824 (*(int*)GIMG(0x46b824))
#define DAT_0046b8a8 (*(int*)GIMG(0x46b8a8))
#define DAT_0046be0a (*(int*)GIMG(0x46be0a))
#define DAT_0046bf14 (*(int*)GIMG(0x46bf14))
#define DAT_0046bfe4 (*(int*)GIMG(0x46bfe4))
#define DAT_0046c038 (*(int*)GIMG(0x46c038))
#define DAT_0046c039 (*(int*)GIMG(0x46c039))
#define DAT_0046c03a (*(int*)GIMG(0x46c03a))
#define DAT_0046c05c (*(int*)GIMG(0x46c05c))
#define DAT_0046c060 (*(int*)GIMG(0x46c060))
#define DAT_0046c062 (*(int*)GIMG(0x46c062))
#define DAT_0046c0d4 (*(int*)GIMG(0x46c0d4))
#define DAT_0046c0d5 (*(int*)GIMG(0x46c0d5))
#define DAT_0046c0d6 (*(int*)GIMG(0x46c0d6))
#define DAT_0046c0d7 (*(int*)GIMG(0x46c0d7))
#define DAT_0046c0ec (*(int*)GIMG(0x46c0ec))
#define DAT_0046c0ed (*(int*)GIMG(0x46c0ed))
#define DAT_0046c0ee (*(int*)GIMG(0x46c0ee))
#define DAT_0046c0ef (*(int*)GIMG(0x46c0ef))
#define DAT_0046c12e (*(int*)GIMG(0x46c12e))
#define DAT_0046c162 (*(int*)GIMG(0x46c162))
#define DAT_0046c328 (*(int*)GIMG(0x46c328))
#define DAT_0046c380 (*(code**)GIMG(0x46c380))
#define DAT_0046c3bc (*(int*)GIMG(0x46c3bc))
#define DAT_0046c3d8 (*(int*)GIMG(0x46c3d8))
#define DAT_0046c404 (*(int*)GIMG(0x46c404))
#define DAT_0046c40c (*(int*)GIMG(0x46c40c))
#define DAT_0046c510 (*(int*)GIMG(0x46c510))
#define DAT_0046c530 (*(int*)GIMG(0x46c530))
#define DAT_0046c534 (*(int*)GIMG(0x46c534))
#define DAT_0046c7c4 (*(int*)GIMG(0x46c7c4))
#define DAT_0046c898 (*(int*)GIMG(0x46c898))
#define DAT_0046c89c (*(int*)GIMG(0x46c89c))
#define DAT_0046c8e8 (*(int*)GIMG(0x46c8e8))
#define DAT_0046c8f8 (*(int*)GIMG(0x46c8f8))
#define DAT_0046c920 (*(int*)GIMG(0x46c920))
#define DAT_0046c988 (*(int*)GIMG(0x46c988))
#define DAT_0046c98c (*(int*)GIMG(0x46c98c))
#define DAT_0046c990 (*(int*)GIMG(0x46c990))
#define DAT_0046c994 (*(int*)GIMG(0x46c994))
#define DAT_0046c998 (*(int*)GIMG(0x46c998))
#define DAT_0046c99c (*(int*)GIMG(0x46c99c))
#define DAT_0046c9a0 (*(int*)GIMG(0x46c9a0))
#define DAT_0046c9a4 (*(int*)GIMG(0x46c9a4))
#define DAT_0046c9a8 (*(int*)GIMG(0x46c9a8))
#define DAT_0046c9ac (*(int*)GIMG(0x46c9ac))
#define DAT_0046c9b0 (*(int*)GIMG(0x46c9b0))
#define DAT_0046c9b4 (*(int*)GIMG(0x46c9b4))
#define DAT_0046c9b8 (*(int*)GIMG(0x46c9b8))
#define DAT_0046c9bc (*(int*)GIMG(0x46c9bc))
#define DAT_0046cc58 (*(int*)GIMG(0x46cc58))
#define DAT_0046cc60 (*(int*)GIMG(0x46cc60))
#define DAT_0046cc90 (*(int*)GIMG(0x46cc90))
#define DAT_0046cccc (*(int*)GIMG(0x46cccc))
#define DAT_0046ccd4 (*(int*)GIMG(0x46ccd4))
#define DAT_0046ccd8 (*(int*)GIMG(0x46ccd8))
#define DAT_0046ccdc (*(int*)GIMG(0x46ccdc))
#define DAT_0046cd20 (*(int*)GIMG(0x46cd20))
#define DAT_0046cd34 (*(int*)GIMG(0x46cd34))
#define DAT_0046cd48 (*(int*)GIMG(0x46cd48))
#define DAT_0046cdd4 (*(int*)GIMG(0x46cdd4))
#define DAT_0046cdd8 (*(int*)GIMG(0x46cdd8))
#define DAT_0046cddc (*(int*)GIMG(0x46cddc))
#define DAT_0046cde0 (*(int*)GIMG(0x46cde0))
#define DAT_0046cde4 (*(int*)GIMG(0x46cde4))
#define DAT_0046cef0 (*(int*)GIMG(0x46cef0))
#define DAT_0046d02c (*(int*)GIMG(0x46d02c))
#define DAT_0046d030 (*(int*)GIMG(0x46d030))
#define DAT_0046d034 (*(int*)GIMG(0x46d034))
#define DAT_0046d03c (*(int*)GIMG(0x46d03c))
#define DAT_0046d044 (*(int*)GIMG(0x46d044))
#define DAT_0046d060 (*(int*)GIMG(0x46d060))
#define DAT_0046d0a4 (*(int*)GIMG(0x46d0a4))
#define DAT_0046d47c (*(int*)GIMG(0x46d47c))
#define DAT_0046d4cc (*(int*)GIMG(0x46d4cc))
#define DAT_0046d594 (*(int*)GIMG(0x46d594))
#define DAT_0046d5b8 (*(int*)GIMG(0x46d5b8))
#define DAT_0046d7b8 (*(int*)GIMG(0x46d7b8))
#define DAT_0046da04 (*(int*)GIMG(0x46da04))
#define DAT_0046dcc4 (*(int*)GIMG(0x46dcc4))
#define DAT_0046dccc (*(int*)GIMG(0x46dccc))
#define DAT_0046dcd4 (*(int*)GIMG(0x46dcd4))
#define DAT_0046e46c (*(int*)GIMG(0x46e46c))
#define DAT_0046ecdc (*(int*)GIMG(0x46ecdc))
#define DAT_0046f020 (*(int*)GIMG(0x46f020))
#define DAT_0046f2ac (*(int*)GIMG(0x46f2ac))
#define DAT_0046f2b4 (*(int*)GIMG(0x46f2b4))
#define DAT_0046f33c (*(int*)GIMG(0x46f33c))
#define DAT_0046f658 (*(int*)GIMG(0x46f658))
#define DAT_0046f660 (*(int*)GIMG(0x46f660))
#define DAT_0046f6c4 (*(int*)GIMG(0x46f6c4))
#define DAT_0046f88c (*(int*)GIMG(0x46f88c))
#define DAT_0046f894 (*(int*)GIMG(0x46f894))
#define DAT_0046f930 (*(int*)GIMG(0x46f930))
#define DAT_0046fa08 (*(int*)GIMG(0x46fa08))
#define DAT_0046fc94 (*(int*)GIMG(0x46fc94))
#define DAT_0046fc98 (*(int*)GIMG(0x46fc98))
#define DAT_0046fca4 (*(int*)GIMG(0x46fca4))
#define DAT_0046fca8 (*(int*)GIMG(0x46fca8))
#define DAT_0046fcac (*(int*)GIMG(0x46fcac))
#define DAT_0046fda9 (*(int*)GIMG(0x46fda9))
#define DAT_0046fec4 (*(int*)GIMG(0x46fec4))
#define DAT_0046fec8 (*(int*)GIMG(0x46fec8))
#define DAT_0046fecc (*(int*)GIMG(0x46fecc))
#define DAT_0046fed0 (*(int*)GIMG(0x46fed0))
#define DAT_0046fed8 (*(int*)GIMG(0x46fed8))
#define DAT_0046fede (*(int*)GIMG(0x46fede))
#define DAT_0046fee2 (*(int*)GIMG(0x46fee2))
#define DAT_0046ff2c (*(int*)GIMG(0x46ff2c))
#define DAT_0046ff48 (*(int*)GIMG(0x46ff48))
#define DAT_0046ff60 (*(int*)GIMG(0x46ff60))
#define DAT_00480018 (*(int*)GIMG(0x480018))
#define DAT_0048001c (*(int*)GIMG(0x48001c))
#define DAT_00480034 (*(int*)GIMG(0x480034))
#define DAT_00480038 (*(int*)GIMG(0x480038))
#define DAT_00700050 (*(int*)GIMG(0x700050))
#define DAT_00713161 (*(int*)GIMG(0x713161))
#define DAT_0071331d (*(int*)GIMG(0x71331d))
#define DAT_00713850 (*(int*)GIMG(0x713850))
#define DAT_0071391d (*(int*)GIMG(0x71391d))
#define DAT_00714058 (*(int*)GIMG(0x714058))
#define DAT_00716b18 (*(int*)GIMG(0x716b18))
#define DAT_00716d20 (*(int*)GIMG(0x716d20))
#define DAT_00716d24 (*(int*)GIMG(0x716d24))
#define DAT_00716d28 (*(int*)GIMG(0x716d28))
#define DAT_00716d2a (*(int*)GIMG(0x716d2a))
#define DAT_00716d2c (*(int*)GIMG(0x716d2c))
#define DAT_00716d2e (*(int*)GIMG(0x716d2e))
#define DAT_00716d30 (*(int*)GIMG(0x716d30))
#define DAT_00716d32 (*(int*)GIMG(0x716d32))
#define DAT_00716d9c (*(int*)GIMG(0x716d9c))
#define DAT_00716dc4 (*(int*)GIMG(0x716dc4))
#define DAT_00716dc8 (*(int*)GIMG(0x716dc8))
#define DAT_0071be4e (*(int*)GIMG(0x71be4e))
#define DAT_0071beee (*(int*)GIMG(0x71beee))
#define DAT_0071bfa0 (*(int*)GIMG(0x71bfa0))
#define DAT_0071bfd0 (*(int*)GIMG(0x71bfd0))
#define DAT_0071c050 (*(int*)GIMG(0x71c050))
#define DAT_0071c051 (*(int*)GIMG(0x71c051))
#define DAT_0073c07a (*(int*)GIMG(0x73c07a))
#define DAT_0073c084 (*(int*)GIMG(0x73c084))
#define DAT_0073c086 (*(int*)GIMG(0x73c086))
#define DAT_0073c2c0 (*(int*)GIMG(0x73c2c0))
#define DAT_0073c2fa (*(int*)GIMG(0x73c2fa))
#define DAT_0073c2fc (*(int*)GIMG(0x73c2fc))
#define DAT_0073c348 (*(int*)GIMG(0x73c348))
#define DAT_00744b00 (*(int*)GIMG(0x744b00))
#define DAT_00744b37 (*(int*)GIMG(0x744b37))
#define DAT_00744b38 (*(int*)GIMG(0x744b38))
#define DAT_00744b3a (*(int*)GIMG(0x744b3a))
#define DAT_00744de0 (*(int*)GIMG(0x744de0))
#define DAT_0074505c (*(int*)GIMG(0x74505c))
#define DAT_0074a6d7 (*(int*)GIMG(0x74a6d7))
#define DAT_0074be98 (*(int*)GIMG(0x74be98))
#define DAT_00750ea4 (*(int*)GIMG(0x750ea4))
#define DAT_0075211c (*(int*)GIMG(0x75211c))
#define DAT_0075211e (*(int*)GIMG(0x75211e))
#define DAT_00752120 (*(int*)GIMG(0x752120))
#define DAT_00752344 (*(int*)GIMG(0x752344))
#define DAT_00752348 (*(int*)GIMG(0x752348))
#define DAT_0075234c (*(int*)GIMG(0x75234c))
#define DAT_00752350 (*(int*)GIMG(0x752350))
#define DAT_00752390 (*(int*)GIMG(0x752390))
#define DAT_00752392 (*(int*)GIMG(0x752392))
#define DAT_00759794 (*(int*)GIMG(0x759794))
#define DAT_007597f0 (*(int*)GIMG(0x7597f0))
#define DAT_007597f8 (*(int*)GIMG(0x7597f8))
#define DAT_0075a044 (*(int*)GIMG(0x75a044))
#define DAT_0075a0e4 (*(int*)GIMG(0x75a0e4))
#define DAT_0075a0e8 (*(int*)GIMG(0x75a0e8))
#define DAT_0075a0f4 (*(int*)GIMG(0x75a0f4))
#define DAT_0075a0fe (*(int*)GIMG(0x75a0fe))
#define DAT_0075a10a (*(int*)GIMG(0x75a10a))
#define DAT_0075a10c (*(int*)GIMG(0x75a10c))
#define DAT_0075a136 (*(int*)GIMG(0x75a136))
#define DAT_0075a138 (*(int*)GIMG(0x75a138))
#define DAT_0075a162 (*(int*)GIMG(0x75a162))
#define DAT_0075a164 (*(int*)GIMG(0x75a164))
#define DAT_0075a18e (*(int*)GIMG(0x75a18e))
#define DAT_0075a190 (*(int*)GIMG(0x75a190))
#define DAT_0075a1ba (*(int*)GIMG(0x75a1ba))
#define DAT_0075a1bc (*(int*)GIMG(0x75a1bc))
#define DAT_0075a1e6 (*(int*)GIMG(0x75a1e6))
#define DAT_0075a1e8 (*(int*)GIMG(0x75a1e8))
#define DAT_0075a212 (*(int*)GIMG(0x75a212))
#define DAT_0075a214 (*(int*)GIMG(0x75a214))
#define DAT_0075a23e (*(int*)GIMG(0x75a23e))
#define DAT_0075a294 (*(int*)GIMG(0x75a294))
#define DAT_0075a2a4 (*(int*)GIMG(0x75a2a4))
#define DAT_0075a2b0 (*(int*)GIMG(0x75a2b0))
#define DAT_0075a610 (*(int*)GIMG(0x75a610))
#define DAT_0075a614 (*(int*)GIMG(0x75a614))
#define DAT_0075a618 (*(int*)GIMG(0x75a618))
#define DAT_0075a630 (*(int*)GIMG(0x75a630))
#define DAT_0075a638 (*(int*)GIMG(0x75a638))
#define DAT_0075a676 (*(int*)GIMG(0x75a676))
#define DAT_0075a67a (*(int*)GIMG(0x75a67a))
#define DAT_0075a686 (*(int*)GIMG(0x75a686))
#define DAT_0075a692 (*(int*)GIMG(0x75a692))
#define DAT_0075a6c6 (*(int*)GIMG(0x75a6c6))
#define DAT_0075a71a (*(int*)GIMG(0x75a71a))
#define DAT_0075a7a2 (*(int*)GIMG(0x75a7a2))
#define DAT_0075a7ae (*(int*)GIMG(0x75a7ae))
#define DAT_0075d846 (*(int*)GIMG(0x75d846))
#define DAT_0075d852 (*(int*)GIMG(0x75d852))
#define DAT_0075d9d8 (*(int*)GIMG(0x75d9d8))
#define DAT_0075ec00 (*(int*)GIMG(0x75ec00))
#define DAT_0075f026 (*(int*)GIMG(0x75f026))
#define DAT_00900ee4 (*(int*)GIMG(0x900ee4))
#define DAT_00900f74 (*(int*)GIMG(0x900f74))
#define DAT_00901010 (*(int*)GIMG(0x901010))
#define DAT_00901f10 (*(int*)GIMG(0x901f10))
#define DAT_00901f14 (*(int*)GIMG(0x901f14))
#define DAT_00901f9c (*(int*)GIMG(0x901f9c))
#define DAT_00901fc4 (*(int*)GIMG(0x901fc4))
#define DAT_00901fc8 (*(int*)GIMG(0x901fc8))
#define DAT_00901fc9 (*(int*)GIMG(0x901fc9))
#define DAT_00901fca (*(int*)GIMG(0x901fca))
#define DAT_00901fec (*(int*)GIMG(0x901fec))
#define DAT_00902014 (*(int*)GIMG(0x902014))
#define DAT_0090203c (*(int*)GIMG(0x90203c))
#define DAT_00902064 (*(int*)GIMG(0x902064))
#define DAT_00902090 (*(int*)GIMG(0x902090))
#define DAT_009020a2 (*(int*)GIMG(0x9020a2))
#define DAT_00903ca2 (*(int*)GIMG(0x903ca2))
#define DAT_00905a74 (*(int*)GIMG(0x905a74))
#define DAT_00905a77 (*(int*)GIMG(0x905a77))
#define DAT_00905a98 (*(int*)GIMG(0x905a98))
#define DAT_00905a9b (*(int*)GIMG(0x905a9b))
#define DAT_00905af2 (*(int*)GIMG(0x905af2))
#define DAT_00905af4 (*(int*)GIMG(0x905af4))
#define DAT_009063b1 (*(int*)GIMG(0x9063b1))
#define DAT_009063b2 (*(int*)GIMG(0x9063b2))
#define DAT_009063b3 (*(int*)GIMG(0x9063b3))
#define DAT_009063b4 (*(int*)GIMG(0x9063b4))
#define DAT_009063b8 (*(int*)GIMG(0x9063b8))
#define DAT_0090676c (*(int*)GIMG(0x90676c))
#define DAT_00907930 (*(int*)GIMG(0x907930))
#define DAT_00907940 (*(int*)GIMG(0x907940))
#define DAT_00907990 (*(int*)GIMG(0x907990))
#define DAT_00907993 (*(int*)GIMG(0x907993))
#define DAT_00907995 (*(int*)GIMG(0x907995))
#define DAT_00907998 (*(int*)GIMG(0x907998))
#define DAT_00907999 (*(int*)GIMG(0x907999))
#define DAT_0090799c (*(int*)GIMG(0x90799c))
#define DAT_0090799d (*(int*)GIMG(0x90799d))
#define DAT_00907c00 (*(int*)GIMG(0x907c00))
#define DAT_00908574 (*(int*)GIMG(0x908574))
#define DAT_009085a8 (*(int*)GIMG(0x9085a8))
#define DAT_009086f8 (*(int*)GIMG(0x9086f8))
#define Debris (*(int*)GIMG(0x73c4e0))
#define Done (*(int*)GIMG(0x467070))
#define DummyPoly (*(int*)GIMG(0x74a6d0))
#define FXPage (*(int*)GIMG(0x900ed0))
#define FaceArray (*(int*)GIMG(0x4685e4))
#define FacePolys (*(int*)GIMG(0x907880))
#define File_Func_List (*(int*)GIMG(0x462ce4))
#define FireFrames (*(int*)GIMG(0x4665c0))
#define FirstTime (*(int*)GIMG(0x900eb0))
#define FlagPolys (*(int*)GIMG(0x749b90))
#define Flying_Objects (*(int*)GIMG(0x7432e0))
#define Forest_Track_Type (*(int*)GIMG(0x465da0))
#define Forest_Wheel_Locking_Speed (*(int*)GIMG(0x465d40))
#define Lap_Timer (*(int*)GIMG(0x75d9dc))
#define Last_Lap_Timer (*(int*)GIMG(0x75d9e8))
#define Level1_Personality (*(int*)GIMG(0x465b10))
#define Level2_Personality (*(int*)GIMG(0x465b60))
#define Level3_Personality (*(int*)GIMG(0x465bb0))
#define Level4_Personality (*(int*)GIMG(0x465c00))
#define Liberty_Track_Type (*(int*)GIMG(0x465da8))
#define Movie_Playing (*(int*)GIMG(0x462cd4))
#define NameArray (*(int*)GIMG(0x468624))
#define NamePolys (*(int*)GIMG(0x9078d0))
#define Now_Timing_Lap (*(int*)GIMG(0x75d9d4))
#define Old_Cam_Mode (*(int*)GIMG(0x464a68))
#define OverPoly (*(int*)GIMG(0x74a6f0))
#define PIT_DONE (*(int*)GIMG(0x467058))
#define PIT_IN (*(int*)GIMG(0x46704c))
#define PIT_STOP (*(int*)GIMG(0x467054))
#define PTR_DAT_004651bc (*(int*)GIMG(0x4651bc))
#define PTR_DAT_00467234 (*(int*)GIMG(0x467234))
#define PTR_DAT_00467248 (*(int*)GIMG(0x467248))
#define PTR_DAT_0046725c (*(int*)GIMG(0x46725c))
#define PTR_DAT_00467270 (*(int*)GIMG(0x467270))
#define PTR_DAT_00467284 (*(int*)GIMG(0x467284))
#define PTR_DAT_00467298 (*(int*)GIMG(0x467298))
#define PTR_DAT_004672ac (*(int*)GIMG(0x4672ac))
#define PTR_DAT_004672c0 (*(int*)GIMG(0x4672c0))
#define PTR_DAT_00467520 (*(int*)GIMG(0x467520))
#define PTR_DAT_004683e8 (*(int*)GIMG(0x4683e8))
#define PTR_DAT_00468b00 (*(int*)GIMG(0x468b00))
#define PTR_DAT_00468b14 (*(int*)GIMG(0x468b14))
#define PTR_DAT_00468ddc (*(int*)GIMG(0x468ddc))
#define PTR_DAT_00468df0 (*(int*)GIMG(0x468df0))
#define PTR_DAT_00469158 (*(int*)GIMG(0x469158))
#define PTR_DAT_0046916c (*(int*)GIMG(0x46916c))
#define PTR_DAT_004693e0 (*(int*)GIMG(0x4693e0))
#define PTR_DAT_0046975c (*(int*)GIMG(0x46975c))
#define PTR_DAT_00469770 (*(int*)GIMG(0x469770))
#define PTR_DAT_00469784 (*(int*)GIMG(0x469784))
#define PTR_DAT_00469798 (*(int*)GIMG(0x469798))
#define PTR_DAT_004697ac (*(int*)GIMG(0x4697ac))
#define PTR_DAT_00469b44 (*(int*)GIMG(0x469b44))
#define PTR_DAT_00469b58 (*(int*)GIMG(0x469b58))
#define PTR_DAT_00469d64 (*(int*)GIMG(0x469d64))
#define PTR_DAT_00469d78 (*(int*)GIMG(0x469d78))
#define PTR_DAT_00469fd4 (*(int**)GIMG(0x469fd4))
#define PTR_DAT_00469fe8 (*(int**)GIMG(0x469fe8))
#define PTR_DAT_00469ffc (*(int*)GIMG(0x469ffc))
#define PTR_DAT_0046a010 (*(int*)GIMG(0x46a010))
#define PTR_DAT_0046a024 (*(int*)GIMG(0x46a024))
#define PTR_DAT_0046a1b4 (*(int*)GIMG(0x46a1b4))
#define PTR_DAT_0046a1c8 (*(int*)GIMG(0x46a1c8))
#define PTR_DAT_0046a340 (*(int*)GIMG(0x46a340))
#define PTR_DAT_0046a354 (*(int*)GIMG(0x46a354))
#define PTR_DAT_0046a508 (*(int*)GIMG(0x46a508))
#define PTR_DAT_0046a51c (*(int*)GIMG(0x46a51c))
#define PTR_DAT_0046a6f0 (*(int*)GIMG(0x46a6f0))
#define PTR_DAT_0046a704 (*(int*)GIMG(0x46a704))
#define PTR_DAT_0046a8f4 (*(int*)GIMG(0x46a8f4))
#define PTR_DAT_0046a908 (*(int*)GIMG(0x46a908))
#define PTR_DAT_0046aa74 (*(int*)GIMG(0x46aa74))
#define PTR_DAT_0046aa88 (*(int*)GIMG(0x46aa88))
#define PTR_DAT_0046ab80 (*(int*)GIMG(0x46ab80))
#define PTR_DAT_0046af08 (*(int*)GIMG(0x46af08))
#define PTR_DAT_0046af1c (*(int*)GIMG(0x46af1c))
#define PTR_DAT_0046af30 (*(int*)GIMG(0x46af30))
#define PTR_DAT_0046af44 (*(int*)GIMG(0x46af44))
#define PTR_DAT_0046af58 (*(int*)GIMG(0x46af58))
#define PTR_DAT_0046b784 (*(int*)GIMG(0x46b784))
#define PTR_DAT_0046b798 (*(int*)GIMG(0x46b798))
#define PTR_DAT_0046b7ac (*(int*)GIMG(0x46b7ac))
#define PTR_DAT_0046c01c (*(int*)GIMG(0x46c01c))
#define PTR_DAT_0046c030 (*(int*)GIMG(0x46c030))
#define PTR_DAT_0046c044 (*(int*)GIMG(0x46c044))
#define PTR_DAT_0046c47c (*(int**)GIMG(0x46c47c))
#define PTR_FUN_004697cc (*(int*)GIMG(0x4697cc))
#define PTR_FUN_004697d0 (*(int*)GIMG(0x4697d0))
#define PTR_FUN_0046a1e8 (*(int*)GIMG(0x46a1e8))
#define PTR_FUN_0046a1ec (*(int*)GIMG(0x46a1ec))
#define PTR_FUN_0046a374 (*(int*)GIMG(0x46a374))
#define PTR_FUN_0046a378 (*(int*)GIMG(0x46a378))
#define PTR_FUN_0046a53c (*(int*)GIMG(0x46a53c))
#define PTR_FUN_0046a540 (*(int*)GIMG(0x46a540))
#define PTR_FUN_0046a724 (*(int*)GIMG(0x46a724))
#define PTR_FUN_0046aaa8 (*(int*)GIMG(0x46aaa8))
#define PTR_FUN_0046af78 (*(int*)GIMG(0x46af78))
#define PTR_FUN_0046af7c (*(int*)GIMG(0x46af7c))
#define PTR_FUN_0046b7cc (*(int*)GIMG(0x46b7cc))
#define PTR_FUN_0046b7d0 (*(int*)GIMG(0x46b7d0))
#define PTR_FUN_0046c064 (*(int*)GIMG(0x46c064))
#define PTR_FUN_0046c068 (*(int*)GIMG(0x46c068))
#define PTR_FUN_0046c344 (*(int*)GIMG(0x46c344))
#define PTR_FUN_0046c4f8 (*(int*)GIMG(0x46c4f8))
#define PTR_FUN_0046c50c (*(int*)GIMG(0x46c50c))
#define PTR_FUN_0046c524 (*(int*)GIMG(0x46c524))
#define PTR_FUN_0046c528 (*(int*)GIMG(0x46c528))
#define PTR_LAB_00462ef4 (*(int*)GIMG(0x462ef4))
#define PTR_LAB_00467084 (*(int*)GIMG(0x467084))
#define PTR_LAB_0046af90 (*(int*)GIMG(0x46af90))
#define PTR_LAB_0046c07c (*(int*)GIMG(0x46c07c))
#define PTR_LAB_0046c4f0 (*(int*)GIMG(0x46c4f0))
#define PTR_Select_Champ_0046a728 (*(int*)GIMG(0x46a728))
#define PTR___CBeginThread_0046c520 (*(int*)GIMG(0x46c520))
#define PTR___matherr_0046c598 (*(int*)GIMG(0x46c598))
#define PTR_draw_face_3pt_flat_00462d94 (*(int*)GIMG(0x462d94))
#define PTR_draw_face_3pt_flat_00462e44 (*(int*)GIMG(0x462e44))
#define PTR_hlf_transparency_table_00460014 (*(int*)GIMG(0x460014))
#define PTR_s_CHAMPP_0046a6a8 (*(int*)GIMG(0x46a6a8))
#define PTR_s_CHAMP_0046a626 (*(int*)GIMG(0x46a626))
#define PTR_s_CHAMP_0046a698 (*(int*)GIMG(0x46a698))
#define PTR_s_DUMB1_004650dc (*(int*)GIMG(0x4650dc))
#define PTR_s_DUMB2_00465100 (*(int*)GIMG(0x465100))
#define PTR_s_DUMB3_00465124 (*(int*)GIMG(0x465124))
#define PTR_s_KEYBOARD_0046a136 (*(int*)GIMG(0x46a136))
#define PTR_s_KEYBOARD_0046a16c (*(int*)GIMG(0x46a16c))
#define PTR_s_KEYBOARP_0046a174 (*(int*)GIMG(0x46a174))
#define PTR_s_MEMLOADP_004672e4 (*(int*)GIMG(0x4672e4))
#define PTR_s_MEMLOAD_004671ee (*(int*)GIMG(0x4671ee))
#define PTR_s_MEMLOAD_004672d8 (*(int*)GIMG(0x4672d8))
#define PTR_s_MEMSAVE_004672dc (*(int*)GIMG(0x4672dc))
#define PTR_s_RACETYPE_004696b2 (*(int*)GIMG(0x4696b2))
#define PTR_s_RACETYPE_00469864 (*(int*)GIMG(0x469864))
#define PTR_s_RACTYPEP_00469884 (*(int*)GIMG(0x469884))
#define PTR_s_RESULTSP_0046aee0 (*(int*)GIMG(0x46aee0))
#define PTR_s_RESULTSP_0046bfcc (*(int*)GIMG(0x46bfcc))
#define PTR_s_RESULTS_0046ae46 (*(int*)GIMG(0x46ae46))
#define PTR_s_RESULTS_0046aec8 (*(int*)GIMG(0x46aec8))
#define PTR_s_RESULTS_0046bf3a (*(int*)GIMG(0x46bf3a))
#define PTR_s_RESULTS_0046bfb4 (*(int*)GIMG(0x46bfb4))
#define PTR_s_VIEWREPP_0046a4c4 (*(int*)GIMG(0x46a4c4))
#define PTR_s_VIEWREPP_0046b740 (*(int*)GIMG(0x46b740))
#define PTR_s_VIEWREP_0046a46a (*(int*)GIMG(0x46a46a))
#define PTR_s_VIEWREP_0046a4b8 (*(int*)GIMG(0x46a4b8))
#define PTR_s_VIEWREP_0046b6e6 (*(int*)GIMG(0x46b6e6))
#define PTR_s_VIEWREP_0046b734 (*(int*)GIMG(0x46b734))
#define PTR_s_WRECKINP_0046a2fc (*(int*)GIMG(0x46a2fc))
#define PTR_s_WRECKIN_0046a28a (*(int*)GIMG(0x46a28a))
#define PTR_s_WRECKIN_0046a2f0 (*(int*)GIMG(0x46a2f0))
#define PTR_s__R_JC_T_Practice_004698d0 (*(int*)GIMG(0x4698d0))
#define PTR_s__R_JC_T_Wrecking_Racing_004698c4 (*(int*)GIMG(0x4698c4))
#define PTR_s__R_JL_T_Jug_00469df0 (*(int*)GIMG(0x469df0))
#define PTR_s__R_JL_T_Liberty_City_00469dc8 (*(int*)GIMG(0x469dc8))
#define PTR_s__R_JL_T_Pine_Hills_Raceway_004682f8 (*(int*)GIMG(0x4682f8))
#define PTR_s__R_JL_T_Pine_Hills_Raceway_00469314 (*(int*)GIMG(0x469314))
#define PTR_s__R_JL_T_Pine_Hills_Raceway_00469bc4 (*(int*)GIMG(0x469bc4))
#define PTR_s__R_JL_T_Pork_Sword_00469ddc (*(int*)GIMG(0x469ddc))
#define PTR_s__R_JL_T_Slapshot_00469c0c (*(int*)GIMG(0x469c0c))
#define ParticleAvailabilityList (*(int*)GIMG(0x752058))
#define ParticlePositionList (*(int*)GIMG(0x751f90))
#define Pause (*(int*)GIMG(0x4686c4))
#define Pit_Timer (*(int*)GIMG(0x46705c))
#define Pit_Timing_Delay (*(int*)GIMG(0x4653c4))
#define PointyBits (*(int*)GIMG(0x751e70))
#define RacePoly (*(int*)GIMG(0x74a9b0))
#define Race_Personality (*(int*)GIMG(0x465c50))
#define Replay_Action_Repeat (*(int*)GIMG(0x46707c))
#define Replay_Invalid (*(int*)GIMG(0x900ec0))
#define Replay_Level (*(int*)GIMG(0x900ebc))
#define Replay_Script (*(int*)GIMG(0x8ff2b0))
#define Replay_Script_Ptr (*(int*)GIMG(0x900eb4))
#define SCA_Corner_Data" (*(int*)GIMG(0x465cf8))
#define SCA_Data (*(int*)GIMG(0x466da0))
#define SCA_Track_Type (*(int*)GIMG(0x465db8))
#define Smoke_Animation1 (*(int*)GIMG(0x466294))
#define Speedway_Corner_Data (*(int*)GIMG(0x465ca0))
#define Speedway_Track_Type" (*(int*)GIMG(0x465d60))
#define Speedway_Wheel_Locking_Speed (*(int*)GIMG(0x465d00))
#define SpinTable (*(int*)GIMG(0x465e90))
#define Stunt_Corner_Data (*(int*)GIMG(0x465cf0))
#define Stunt_Wheel_Locking_Speed (*(int*)GIMG(0x465d50))
#define Template (*(int*)GIMG(0x73c308))
#define Timing_Delay (*(int*)GIMG(0x75d9f8))
#define TopHandicaps (*(int*)GIMG(0x465ac0))
#define Track_Records (*(int*)GIMG(0x466e3c))
#define Ultimate_Wheel_Locking_Speed (*(int*)GIMG(0x465d28))
#define ZValue (*(int*)GIMG(0x465250))
#define Z_DISTANCE (*(int*)GIMG(0x4604c2))
#define Zoom (*(int*)GIMG(0x46524c))
#define _AccessFHeap (*(int*)GIMG(0x46c34c))
#define _AccessFList (*(int*)GIMG(0x46c360))
#define _AccessFileH (*(int*)GIMG(0x46c330))
#define _AccessIOB (*(int*)GIMG(0x46c340))
#define _AccessNHeap (*(int*)GIMG(0x46c348))
#define _AccessTDList (*(int*)GIMG(0x46c358))
#define _BigPow10Table (*(int*)GIMG(0x46fedc))
#define _DAT_00460430 (*(int*)GIMG(0x460430))
#define _DAT_00460450 (*(int*)GIMG(0x460450))
#define _DAT_00462d7c (*(int*)GIMG(0x462d7c))
#define _DAT_00462fa8 (*(int*)GIMG(0x462fa8))
#define _DAT_00462fac (*(int*)GIMG(0x462fac))
#define _DAT_00462fae (*(int*)GIMG(0x462fae))
#define _DAT_00462fb0 (*(int*)GIMG(0x462fb0))
#define _DAT_00462fb2 (*(int*)GIMG(0x462fb2))
#define _DAT_00462fb4 (*(int*)GIMG(0x462fb4))
#define _DAT_00462fd4 (*(int*)GIMG(0x462fd4))
#define _DAT_00463000 (*(int*)GIMG(0x463000))
#define _DAT_0046303a (*(int*)GIMG(0x46303a))
#define _DAT_0046303c (*(int*)GIMG(0x46303c))
#define _DAT_00463ef4 (*(int*)GIMG(0x463ef4))
#define _DAT_00465836 (*(int*)GIMG(0x465836))
#define _DAT_00465838 (*(int*)GIMG(0x465838))
#define _DAT_0046583a (*(int*)GIMG(0x46583a))
#define _DAT_0046583c (*(int*)GIMG(0x46583c))
#define _DAT_0046583e (*(int*)GIMG(0x46583e))
#define _DAT_00465840 (*(int*)GIMG(0x465840))
#define _DAT_00465842 (*(int*)GIMG(0x465842))
#define _DAT_00465844 (*(int*)GIMG(0x465844))
#define _DAT_0046585a (*(int*)GIMG(0x46585a))
#define _DAT_0046585c (*(int*)GIMG(0x46585c))
#define _DAT_0046585e (*(int*)GIMG(0x46585e))
#define _DAT_00465860 (*(int*)GIMG(0x465860))
#define _DAT_00465862 (*(int*)GIMG(0x465862))
#define _DAT_00465864 (*(int*)GIMG(0x465864))
#define _DAT_00467050 (*(int*)GIMG(0x467050))
#define _DAT_00467078 (*(int*)GIMG(0x467078))
#define _DAT_00467080 (*(int*)GIMG(0x467080))
#define _DAT_00467150 (*(int*)GIMG(0x467150))
#define _DAT_00467152 (*(int*)GIMG(0x467152))
#define _DAT_0046716a (*(int*)GIMG(0x46716a))
#define _DAT_00467182 (*(int*)GIMG(0x467182))
#define _DAT_00467198 (*(int*)GIMG(0x467198))
#define _DAT_0046719a (*(int*)GIMG(0x46719a))
#define _DAT_004671b2 (*(int*)GIMG(0x4671b2))
#define _DAT_004671ce (*(int*)GIMG(0x4671ce))
#define _DAT_004671da (*(int*)GIMG(0x4671da))
#define _DAT_004672a4 (*(int*)GIMG(0x4672a4))
#define _DAT_004672a6 (*(int*)GIMG(0x4672a6))
#define _DAT_00467396 (*(int*)GIMG(0x467396))
#define _DAT_004673ae (*(int*)GIMG(0x4673ae))
#define _DAT_004673c6 (*(int*)GIMG(0x4673c6))
#define _DAT_004673dc (*(int*)GIMG(0x4673dc))
#define _DAT_004673de (*(int*)GIMG(0x4673de))
#define _DAT_00467660 (*(int*)GIMG(0x467660))
#define _DAT_004682f0 (*(int*)GIMG(0x4682f0))
#define _DAT_00469568 (*(int*)GIMG(0x469568))
#define _DAT_0046965c (*(int*)GIMG(0x46965c))
#define _DAT_0046965e (*(int*)GIMG(0x46965e))
#define _DAT_00469674 (*(int*)GIMG(0x469674))
#define _DAT_00469676 (*(int*)GIMG(0x469676))
#define _DAT_0046968c (*(int*)GIMG(0x46968c))
#define _DAT_0046968e (*(int*)GIMG(0x46968e))
#define _DAT_0046996e (*(int*)GIMG(0x46996e))
#define _DAT_00469970 (*(int*)GIMG(0x469970))
#define _DAT_00469f34 (*(int*)GIMG(0x469f34))
#define _DAT_00469f36 (*(int*)GIMG(0x469f36))
#define _DAT_0046a0e0 (*(int*)GIMG(0x46a0e0))
#define _DAT_0046a0e2 (*(int*)GIMG(0x46a0e2))
#define _DAT_0046a258 (*(int*)GIMG(0x46a258))
#define _DAT_0046a25a (*(int*)GIMG(0x46a25a))
#define _DAT_0046a3e4 (*(int*)GIMG(0x46a3e4))
#define _DAT_0046a3e6 (*(int*)GIMG(0x46a3e6))
#define _DAT_0046a3fc (*(int*)GIMG(0x46a3fc))
#define _DAT_0046a3fe (*(int*)GIMG(0x46a3fe))
#define _DAT_0046a5dc (*(int*)GIMG(0x46a5dc))
#define _DAT_0046a5de (*(int*)GIMG(0x46a5de))
#define _DAT_0046a958 (*(int*)GIMG(0x46a958))
#define _DAT_0046a95a (*(int*)GIMG(0x46a95a))
#define _DAT_0046a970 (*(int*)GIMG(0x46a970))
#define _DAT_0046a972 (*(int*)GIMG(0x46a972))
#define _DAT_0046a9a0 (*(int*)GIMG(0x46a9a0))
#define _DAT_0046a9a2 (*(int*)GIMG(0x46a9a2))
#define _DAT_0046ad90 (*(int*)GIMG(0x46ad90))
#define _DAT_0046ad92 (*(int*)GIMG(0x46ad92))
#define _DAT_0046ada8 (*(int*)GIMG(0x46ada8))
#define _DAT_0046adaa (*(int*)GIMG(0x46adaa))
#define _DAT_0046adf0 (*(int*)GIMG(0x46adf0))
#define _DAT_0046adf2 (*(int*)GIMG(0x46adf2))
#define _DAT_0046ae08 (*(int*)GIMG(0x46ae08))
#define _DAT_0046ae0a (*(int*)GIMG(0x46ae0a))
#define _DAT_0046ae26 (*(int*)GIMG(0x46ae26))
#define _DAT_0046ae32 (*(int*)GIMG(0x46ae32))
#define _DAT_0046b660 (*(int*)GIMG(0x46b660))
#define _DAT_0046b662 (*(int*)GIMG(0x46b662))
#define _DAT_0046b678 (*(int*)GIMG(0x46b678))
#define _DAT_0046b67a (*(int*)GIMG(0x46b67a))
#define _DAT_0046be0e (*(int*)GIMG(0x46be0e))
#define _DAT_0046be10 (*(int*)GIMG(0x46be10))
#define _DAT_0046be9c (*(int*)GIMG(0x46be9c))
#define _DAT_0046be9e (*(int*)GIMG(0x46be9e))
#define _DAT_0046beb4 (*(int*)GIMG(0x46beb4))
#define _DAT_0046beb6 (*(int*)GIMG(0x46beb6))
#define _DAT_0046befc (*(int*)GIMG(0x46befc))
#define _DAT_0046befe (*(int*)GIMG(0x46befe))
#define _DAT_0046c3f1 (*(int*)GIMG(0x46c3f1))
#define _DAT_0046c84c (*(int*)GIMG(0x46c84c))
#define _DAT_0046fc7c (*(int*)GIMG(0x46fc7c))
#define _DAT_0046feb4 (*(int*)GIMG(0x46feb4))
#define _DAT_0046febc (*(int*)GIMG(0x46febc))
#define _DAT_0046ff84 (*(int*)GIMG(0x46ff84))
#define _DAT_00714054 (*(int*)GIMG(0x714054))
#define _DAT_00714058 (*(int*)GIMG(0x714058))
#define _DAT_0071405c (*(int*)GIMG(0x71405c))
#define _DAT_0071406c (*(int*)GIMG(0x71406c))
#define _DAT_0071407c (*(int*)GIMG(0x71407c))
#define _DAT_007140c0 (*(int*)GIMG(0x7140c0))
#define _DAT_007140c4 (*(int*)GIMG(0x7140c4))
#define _DAT_007140e4 (*(int*)GIMG(0x7140e4))
#define _DAT_007140e8 (*(int*)GIMG(0x7140e8))
#define _DAT_007140ec (*(int*)GIMG(0x7140ec))
#define _DAT_007140f4 (*(int*)GIMG(0x7140f4))
#define _DAT_007140f8 (*(int*)GIMG(0x7140f8))
#define _DAT_007140fc (*(int*)GIMG(0x7140fc))
#define _DAT_007140fe (*(int*)GIMG(0x7140fe))
#define _DAT_00714102 (*(int*)GIMG(0x714102))
#define _DAT_00714104 (*(int*)GIMG(0x714104))
#define _DAT_00714106 (*(int*)GIMG(0x714106))
#define _DAT_00714108 (*(int*)GIMG(0x714108))
#define _DAT_0071410c (*(int*)GIMG(0x71410c))
#define _DAT_00714114 (*(int*)GIMG(0x714114))
#define _DAT_00714118 (*(int*)GIMG(0x714118))
#define _DAT_0071411c (*(int*)GIMG(0x71411c))
#define _DAT_00714120 (*(int*)GIMG(0x714120))
#define _DAT_00714124 (*(int*)GIMG(0x714124))
#define _DAT_00714128 (*(int*)GIMG(0x714128))
#define _DAT_0071412c (*(int*)GIMG(0x71412c))
#define _DAT_00714130 (*(int*)GIMG(0x714130))
#define _DAT_00714134 (*(int*)GIMG(0x714134))
#define _DAT_00714138 (*(int*)GIMG(0x714138))
#define _DAT_0071413c (*(int*)GIMG(0x71413c))
#define _DAT_00714140 (*(int*)GIMG(0x714140))
#define _DAT_00714144 (*(int*)GIMG(0x714144))
#define _DAT_00714148 (*(int*)GIMG(0x714148))
#define _DAT_0071414c (*(int*)GIMG(0x71414c))
#define _DAT_00714150 (*(int*)GIMG(0x714150))
#define _DAT_00714154 (*(int*)GIMG(0x714154))
#define _DAT_00714158 (*(int*)GIMG(0x714158))
#define _DAT_0071415c (*(int*)GIMG(0x71415c))
#define _DAT_00714164 (*(int*)GIMG(0x714164))
#define _DAT_00714168 (*(int*)GIMG(0x714168))
#define _DAT_00714174 (*(int*)GIMG(0x714174))
#define _DAT_00714178 (*(int*)GIMG(0x714178))
#define _DAT_00714184 (*(int*)GIMG(0x714184))
#define _DAT_00714188 (*(int*)GIMG(0x714188))
#define _DAT_007142d4 (*(int*)GIMG(0x7142d4))
#define _DAT_007142f2 (*(int*)GIMG(0x7142f2))
#define _DAT_007142f4 (*(int*)GIMG(0x7142f4))
#define _DAT_007142f6 (*(int*)GIMG(0x7142f6))
#define _DAT_007142f8 (*(int*)GIMG(0x7142f8))
#define _DAT_007142fa (*(int*)GIMG(0x7142fa))
#define _DAT_007142fc (*(int*)GIMG(0x7142fc))
#define _DAT_007142fe (*(int*)GIMG(0x7142fe))
#define _DAT_00714300 (*(int*)GIMG(0x714300))
#define _DAT_00714302 (*(int*)GIMG(0x714302))
#define _DAT_00714306 (*(int*)GIMG(0x714306))
#define _DAT_0071430a (*(int*)GIMG(0x71430a))
#define _DAT_00716d74 (*(int*)GIMG(0x716d74))
#define _DAT_00716d78 (*(int*)GIMG(0x716d78))
#define _DAT_00716d80 (*(int*)GIMG(0x716d80))
#define _DAT_00716d84 (*(int*)GIMG(0x716d84))
#define _DAT_00716d94 (*(int*)GIMG(0x716d94))
#define _DAT_00716d9c (*(int**)GIMG(0x716d9c))
#define _DAT_0071bdc0 (*(int*)GIMG(0x71bdc0))
#define _DAT_0071bdc4 (*(int*)GIMG(0x71bdc4))
#define _DAT_0071bde6 (*(int*)GIMG(0x71bde6))
#define _DAT_0071bdea (*(int*)GIMG(0x71bdea))
#define _DAT_0071bdec (*(int*)GIMG(0x71bdec))
#define _DAT_0071bdf2 (*(int*)GIMG(0x71bdf2))
#define _DAT_0071bdf4 (*(int*)GIMG(0x71bdf4))
#define _DAT_0071bdfe (*(int*)GIMG(0x71bdfe))
#define _DAT_0071be00 (*(int*)GIMG(0x71be00))
#define _DAT_0071be02 (*(int*)GIMG(0x71be02))
#define _DAT_0071be04 (*(int*)GIMG(0x71be04))
#define _DAT_0071be06 (*(int*)GIMG(0x71be06))
#define _DAT_0071be08 (*(int*)GIMG(0x71be08))
#define _DAT_0071be0a (*(int*)GIMG(0x71be0a))
#define _DAT_0071be0c (*(int*)GIMG(0x71be0c))
#define _DAT_0071be1e (*(int*)GIMG(0x71be1e))
#define _DAT_0071be20 (*(int*)GIMG(0x71be20))
#define _DAT_0071be22 (*(int*)GIMG(0x71be22))
#define _DAT_0071be24 (*(int*)GIMG(0x71be24))
#define _DAT_0071be26 (*(int*)GIMG(0x71be26))
#define _DAT_0071be28 (*(int*)GIMG(0x71be28))
#define _DAT_0071be2a (*(int*)GIMG(0x71be2a))
#define _DAT_0071be2c (*(int*)GIMG(0x71be2c))
#define _DAT_0071be4e (*(int*)GIMG(0x71be4e))
#define _DAT_0071be52 (*(int*)GIMG(0x71be52))
#define _DAT_0071be56 (*(int*)GIMG(0x71be56))
#define _DAT_0071beee (*(int*)GIMG(0x71beee))
#define _DAT_0071bf7c (*(int*)GIMG(0x71bf7c))
#define _DAT_0071bf94 (*(int*)GIMG(0x71bf94))
#define _DAT_0071bf98 (*(int*)GIMG(0x71bf98))
#define _DAT_0071bfa0 (*(int*)GIMG(0x71bfa0))
#define _DAT_0071bfa4 (*(int*)GIMG(0x71bfa4))
#define _DAT_0071bfac (*(int*)GIMG(0x71bfac))
#define _DAT_0071bfb0 (*(int*)GIMG(0x71bfb0))
#define _DAT_0071bff8 (*(int*)GIMG(0x71bff8))
#define _DAT_0071bffc (*(int*)GIMG(0x71bffc))
#define _DAT_0071c000 (*(int*)GIMG(0x71c000))
#define _DAT_0071c004 (*(int*)GIMG(0x71c004))
#define _DAT_0071c008 (*(int*)GIMG(0x71c008))
#define _DAT_0071c00c (*(int*)GIMG(0x71c00c))
#define _DAT_0071c030 (*(int*)GIMG(0x71c030))
#define _DAT_0071c034 (*(int*)GIMG(0x71c034))
#define _DAT_0071c038 (*(int*)GIMG(0x71c038))
#define _DAT_0071c03c (*(int*)GIMG(0x71c03c))
#define _DAT_0071c040 (*(int*)GIMG(0x71c040))
#define _DAT_0071c044 (*(int*)GIMG(0x71c044))
#define _DAT_0071c048 (*(int*)GIMG(0x71c048))
#define _DAT_0071c04a (*(int*)GIMG(0x71c04a))
#define _DAT_0071c04c (*(int*)GIMG(0x71c04c))
#define _DAT_0071c04e (*(int*)GIMG(0x71c04e))
#define _DAT_0071c050 (*(int*)GIMG(0x71c050))
#define _DAT_0073c060 (*(int*)GIMG(0x73c060))
#define _DAT_0073c280 (*(int*)GIMG(0x73c280))
#define _DAT_0073c290 (*(int**)GIMG(0x73c290))
#define _DAT_0073c2c0 (*(int*)GIMG(0x73c2c0))
#define _DAT_0073c2fa (*(int*)GIMG(0x73c2fa))
#define _DAT_0073c2fc (*(int*)GIMG(0x73c2fc))
#define _DAT_0073c300 (*(int*)GIMG(0x73c300))
#define _DAT_0073c302 (*(int*)GIMG(0x73c302))
#define _DAT_0073c304 (*(int*)GIMG(0x73c304))
#define _DAT_0073c348 (*(int*)GIMG(0x73c348))
#define _DAT_0073c34a (*(int*)GIMG(0x73c34a))
#define _DAT_0073c34c (*(int*)GIMG(0x73c34c))
#define _DAT_0073c350 (*(int*)GIMG(0x73c350))
#define _DAT_0073c352 (*(int*)GIMG(0x73c352))
#define _DAT_0073c354 (*(int*)GIMG(0x73c354))
#define _DAT_0073c358 (*(int*)GIMG(0x73c358))
#define _DAT_0073c35a (*(int*)GIMG(0x73c35a))
#define _DAT_0073c35c (*(int*)GIMG(0x73c35c))
#define _DAT_0073c3c4 (*(int*)GIMG(0x73c3c4))
#define _DAT_0073c3c6 (*(int*)GIMG(0x73c3c6))
#define _DAT_0073c3c8 (*(int*)GIMG(0x73c3c8))
#define _DAT_0073c3cc (*(int*)GIMG(0x73c3cc))
#define _DAT_0073c3ce (*(int*)GIMG(0x73c3ce))
#define _DAT_0073c3d0 (*(int*)GIMG(0x73c3d0))
#define _DAT_0073c3d4 (*(int*)GIMG(0x73c3d4))
#define _DAT_0073c3d6 (*(int*)GIMG(0x73c3d6))
#define _DAT_0073c3d8 (*(int*)GIMG(0x73c3d8))
#define _DAT_00744344 (*(int*)GIMG(0x744344))
#define _DAT_0074434c (*(int*)GIMG(0x74434c))
#define _DAT_00744358 (*(int*)GIMG(0x744358))
#define _DAT_00744ad8 (*(int*)GIMG(0x744ad8))
#define _DAT_00744adc (*(int*)GIMG(0x744adc))
#define _DAT_00744b00 (*(int*)GIMG(0x744b00))
#define _DAT_00744b04 (*(int*)GIMG(0x744b04))
#define _DAT_00744b08 (*(int*)GIMG(0x744b08))
#define _DAT_00744b0c (*(int*)GIMG(0x744b0c))
#define _DAT_00744b14 (*(int*)GIMG(0x744b14))
#define _DAT_00744b18 (*(int*)GIMG(0x744b18))
#define _DAT_00744b1c (*(int*)GIMG(0x744b1c))
#define _DAT_00744b24 (*(int*)GIMG(0x744b24))
#define _DAT_00744b34 (*(int*)GIMG(0x744b34))
#define _DAT_00744b40 (*(int*)GIMG(0x744b40))
#define _DAT_00744b44 (*(int*)GIMG(0x744b44))
#define _DAT_00744b74 (*(int*)GIMG(0x744b74))
#define _DAT_00744de0 (*(int*)GIMG(0x744de0))
#define _DAT_00744de4 (*(int*)GIMG(0x744de4))
#define _DAT_00744de8 (*(int*)GIMG(0x744de8))
#define _DAT_00744dec (*(int*)GIMG(0x744dec))
#define _DAT_00744df0 (*(int*)GIMG(0x744df0))
#define _DAT_00744df4 (*(int*)GIMG(0x744df4))
#define _DAT_0074948c (*(int*)GIMG(0x74948c))
#define _DAT_0074949c (*(int*)GIMG(0x74949c))
#define _DAT_007494ac (*(int*)GIMG(0x7494ac))
#define _DAT_007494bc (*(int*)GIMG(0x7494bc))
#define _DAT_007494d4 (*(int*)GIMG(0x7494d4))
#define _DAT_007494fc (*(int*)GIMG(0x7494fc))
#define _DAT_00749514 (*(int*)GIMG(0x749514))
#define _DAT_0074952c (*(int*)GIMG(0x74952c))
#define _DAT_0074953c (*(int*)GIMG(0x74953c))
#define _DAT_00749a60 (*(int*)GIMG(0x749a60))
#define _DAT_00749a8c (*(int*)GIMG(0x749a8c))
#define _DAT_00749ab0 (*(int*)GIMG(0x749ab0))
#define _DAT_00749ad4 (*(int*)GIMG(0x749ad4))
#define _DAT_00749af8 (*(int*)GIMG(0x749af8))
#define _DAT_00749b4c (*(int*)GIMG(0x749b4c))
#define _DAT_0074a6d8 (*(int*)GIMG(0x74a6d8))
#define _DAT_0074a6da (*(int*)GIMG(0x74a6da))
#define _DAT_0074a6e0 (*(int*)GIMG(0x74a6e0))
#define _DAT_0074a6e2 (*(int*)GIMG(0x74a6e2))
#define _DAT_0074a6e6 (*(int*)GIMG(0x74a6e6))
#define _DAT_0074a6e8 (*(int*)GIMG(0x74a6e8))
#define _DAT_0074a6ea (*(int*)GIMG(0x74a6ea))
#define _DAT_0074be90 (*(int*)GIMG(0x74be90))
#define _DAT_0074be94 (*(int*)GIMG(0x74be94))
#define _DAT_0074be98 (*(int*)GIMG(0x74be98))
#define _DAT_00750f4c (*(int*)GIMG(0x750f4c))
#define _DAT_00750f54 (*(int*)GIMG(0x750f54))
#define _DAT_00750f56 (*(int*)GIMG(0x750f56))
#define _DAT_00751d5c (*(int*)GIMG(0x751d5c))
#define _DAT_00751f60 (*(int*)GIMG(0x751f60))
#define _DAT_00751f78 (*(int**)GIMG(0x751f78))
#define _DAT_00751f7c (*(int*)GIMG(0x751f7c))
#define _DAT_0075211c (*(int*)GIMG(0x75211c))
#define _DAT_00752344 (*(int*)GIMG(0x752344))
#define _DAT_00752348 (*(int*)GIMG(0x752348))
#define _DAT_0075234c (*(int*)GIMG(0x75234c))
#define _DAT_00752390 (*(int*)GIMG(0x752390))
#define _DAT_00752392 (*(int*)GIMG(0x752392))
#define _DAT_00759794 (*(int*)GIMG(0x759794))
#define _DAT_00759798 (*(int*)GIMG(0x759798))
#define _DAT_007597a4 (*(int*)GIMG(0x7597a4))
#define _DAT_007597a8 (*(int*)GIMG(0x7597a8))
#define _DAT_007597ae (*(int*)GIMG(0x7597ae))
#define _DAT_007597b4 (*(int*)GIMG(0x7597b4))
#define _DAT_007597b6 (*(int*)GIMG(0x7597b6))
#define _DAT_007597bc (*(int*)GIMG(0x7597bc))
#define _DAT_007597be (*(int*)GIMG(0x7597be))
#define _DAT_007597c4 (*(int*)GIMG(0x7597c4))
#define _DAT_007597c6 (*(int*)GIMG(0x7597c6))
#define _DAT_007597cc (*(int*)GIMG(0x7597cc))
#define _DAT_007597ce (*(int*)GIMG(0x7597ce))
#define _DAT_007597d4 (*(int*)GIMG(0x7597d4))
#define _DAT_007597d6 (*(int*)GIMG(0x7597d6))
#define _DAT_007597dc (*(int*)GIMG(0x7597dc))
#define _DAT_007597de (*(int*)GIMG(0x7597de))
#define _DAT_007597e4 (*(int*)GIMG(0x7597e4))
#define _DAT_007597e6 (*(int*)GIMG(0x7597e6))
#define _DAT_007597ec (*(int*)GIMG(0x7597ec))
#define _DAT_007597f0 (*(int*)GIMG(0x7597f0))
#define _DAT_007597f8 (*(int*)GIMG(0x7597f8))
#define _DAT_00759800 (*(int*)GIMG(0x759800))
#define _DAT_00759808 (*(int*)GIMG(0x759808))
#define _DAT_00759826 (*(int*)GIMG(0x759826))
#define _DAT_0075a00e (*(int*)GIMG(0x75a00e))
#define _DAT_0075a038 (*(int*)GIMG(0x75a038))
#define _DAT_0075a040 (*(int*)GIMG(0x75a040))
#define _DAT_0075a044 (*(int*)GIMG(0x75a044))
#define _DAT_0075a048 (*(int*)GIMG(0x75a048))
#define _DAT_0075a04c (*(int*)GIMG(0x75a04c))
#define _DAT_0075a050 (*(int*)GIMG(0x75a050))
#define _DAT_0075a054 (*(int*)GIMG(0x75a054))
#define _DAT_0075a058 (*(int*)GIMG(0x75a058))
#define _DAT_0075a05c (*(int*)GIMG(0x75a05c))
#define _DAT_0075a060 (*(int*)GIMG(0x75a060))
#define _DAT_0075a064 (*(int*)GIMG(0x75a064))
#define _DAT_0075a068 (*(int*)GIMG(0x75a068))
#define _DAT_0075a06c (*(int*)GIMG(0x75a06c))
#define _DAT_0075a070 (*(int*)GIMG(0x75a070))
#define _DAT_0075a074 (*(int*)GIMG(0x75a074))
#define _DAT_0075a078 (*(int*)GIMG(0x75a078))
#define _DAT_0075a07c (*(int*)GIMG(0x75a07c))
#define _DAT_0075a080 (*(int*)GIMG(0x75a080))
#define _DAT_0075a084 (*(int*)GIMG(0x75a084))
#define _DAT_0075a088 (*(int*)GIMG(0x75a088))
#define _DAT_0075a08c (*(int*)GIMG(0x75a08c))
#define _DAT_0075a090 (*(int*)GIMG(0x75a090))
#define _DAT_0075a094 (*(int*)GIMG(0x75a094))
#define _DAT_0075a098 (*(int*)GIMG(0x75a098))
#define _DAT_0075a09c (*(int*)GIMG(0x75a09c))
#define _DAT_0075a0a0 (*(int*)GIMG(0x75a0a0))
#define _DAT_0075a0a4 (*(int*)GIMG(0x75a0a4))
#define _DAT_0075a0a8 (*(int*)GIMG(0x75a0a8))
#define _DAT_0075a0ac (*(int*)GIMG(0x75a0ac))
#define _DAT_0075a0b0 (*(int*)GIMG(0x75a0b0))
#define _DAT_0075a0b4 (*(int*)GIMG(0x75a0b4))
#define _DAT_0075a0b8 (*(int*)GIMG(0x75a0b8))
#define _DAT_0075a0bc (*(int*)GIMG(0x75a0bc))
#define _DAT_0075a0e4 (*(int*)GIMG(0x75a0e4))
#define _DAT_0075a0e8 (*(int*)GIMG(0x75a0e8))
#define _DAT_0075a0ec (*(int*)GIMG(0x75a0ec))
#define _DAT_0075a0f4 (*(int*)GIMG(0x75a0f4))
#define _DAT_0075a0fe (*(int*)GIMG(0x75a0fe))
#define _DAT_0075a10c (*(int*)GIMG(0x75a10c))
#define _DAT_0075a110 (*(int*)GIMG(0x75a110))
#define _DAT_0075a114 (*(int*)GIMG(0x75a114))
#define _DAT_0075a118 (*(int*)GIMG(0x75a118))
#define _DAT_0075a120 (*(int*)GIMG(0x75a120))
#define _DAT_0075a12a (*(int*)GIMG(0x75a12a))
#define _DAT_0075a138 (*(int*)GIMG(0x75a138))
#define _DAT_0075a13c (*(int*)GIMG(0x75a13c))
#define _DAT_0075a140 (*(int*)GIMG(0x75a140))
#define _DAT_0075a144 (*(int*)GIMG(0x75a144))
#define _DAT_0075a14c (*(int*)GIMG(0x75a14c))
#define _DAT_0075a156 (*(int*)GIMG(0x75a156))
#define _DAT_0075a164 (*(int*)GIMG(0x75a164))
#define _DAT_0075a168 (*(int*)GIMG(0x75a168))
#define _DAT_0075a16c (*(int*)GIMG(0x75a16c))
#define _DAT_0075a170 (*(int*)GIMG(0x75a170))
#define _DAT_0075a178 (*(int*)GIMG(0x75a178))
#define _DAT_0075a182 (*(int*)GIMG(0x75a182))
#define _DAT_0075a190 (*(int*)GIMG(0x75a190))
#define _DAT_0075a194 (*(int*)GIMG(0x75a194))
#define _DAT_0075a198 (*(int*)GIMG(0x75a198))
#define _DAT_0075a19c (*(int*)GIMG(0x75a19c))
#define _DAT_0075a1a4 (*(int*)GIMG(0x75a1a4))
#define _DAT_0075a1ae (*(int*)GIMG(0x75a1ae))
#define _DAT_0075a1bc (*(int*)GIMG(0x75a1bc))
#define _DAT_0075a1c0 (*(int*)GIMG(0x75a1c0))
#define _DAT_0075a1c4 (*(int*)GIMG(0x75a1c4))
#define _DAT_0075a1c8 (*(int*)GIMG(0x75a1c8))
#define _DAT_0075a1d0 (*(int*)GIMG(0x75a1d0))
#define _DAT_0075a1da (*(int*)GIMG(0x75a1da))
#define _DAT_0075a1e8 (*(int*)GIMG(0x75a1e8))
#define _DAT_0075a1ec (*(int*)GIMG(0x75a1ec))
#define _DAT_0075a1f0 (*(int*)GIMG(0x75a1f0))
#define _DAT_0075a1f4 (*(int*)GIMG(0x75a1f4))
#define _DAT_0075a1fc (*(int*)GIMG(0x75a1fc))
#define _DAT_0075a206 (*(int*)GIMG(0x75a206))
#define _DAT_0075a214 (*(int*)GIMG(0x75a214))
#define _DAT_0075a218 (*(int*)GIMG(0x75a218))
#define _DAT_0075a21c (*(int*)GIMG(0x75a21c))
#define _DAT_0075a220 (*(int*)GIMG(0x75a220))
#define _DAT_0075a228 (*(int*)GIMG(0x75a228))
#define _DAT_0075a232 (*(int*)GIMG(0x75a232))
#define _DAT_0075a2a4 (*(int*)GIMG(0x75a2a4))
#define _DAT_0075a2b0 (*(int*)GIMG(0x75a2b0))
#define _DAT_0075a610 (*(int*)GIMG(0x75a610))
#define _DAT_0075a614 (*(int*)GIMG(0x75a614))
#define _DAT_0075a618 (*(int*)GIMG(0x75a618))
#define _DAT_0075a630 (*(int*)GIMG(0x75a630))
#define _DAT_0075a638 (*(int*)GIMG(0x75a638))
#define _DAT_0075a676 (*(int*)GIMG(0x75a676))
#define _DAT_0075a67a (*(int*)GIMG(0x75a67a))
#define _DAT_0075a686 (*(int*)GIMG(0x75a686))
#define _DAT_0075a692 (*(int*)GIMG(0x75a692))
#define _DAT_0075a6c6 (*(int*)GIMG(0x75a6c6))
#define _DAT_0075a71a (*(int*)GIMG(0x75a71a))
#define _DAT_0075a7a2 (*(int*)GIMG(0x75a7a2))
#define _DAT_0075a7e2 (*(int*)GIMG(0x75a7e2))
#define _DAT_0075a8cc (*(int*)GIMG(0x75a8cc))
#define _DAT_0075a99c (*(int*)GIMG(0x75a99c))
#define _DAT_0075d846 (*(int*)GIMG(0x75d846))
#define _DAT_0075d852 (*(int*)GIMG(0x75d852))
#define _DAT_0075d9d8 (*(int*)GIMG(0x75d9d8))
#define _DAT_0075d9e0 (*(int*)GIMG(0x75d9e0))
#define _DAT_0075d9e4 (*(int*)GIMG(0x75d9e4))
#define _DAT_0075d9ec (*(int*)GIMG(0x75d9ec))
#define _DAT_0075d9f0 (*(int*)GIMG(0x75d9f0))
#define _DAT_00900eb8 (*(int*)GIMG(0x900eb8))
#define _DAT_00900ec4 (*(int**)GIMG(0x900ec4))
#define _DAT_00900ee4 (*(int*)GIMG(0x900ee4))
#define _DAT_00901768 (*(int*)GIMG(0x901768))
#define _DAT_00901780 (*(int*)GIMG(0x901780))
#define _DAT_00901784 (*(int*)GIMG(0x901784))
#define _DAT_00901f08 (*(int*)GIMG(0x901f08))
#define _DAT_00901f0c (*(int*)GIMG(0x901f0c))
#define _DAT_00901f10 (*(int*)GIMG(0x901f10))
#define _DAT_00901f14 (*(int*)GIMG(0x901f14))
#define _DAT_00901f18 (*(int*)GIMG(0x901f18))
#define _DAT_00901f9c (*(int*)GIMG(0x901f9c))
#define _DAT_00901fc4 (*(int*)GIMG(0x901fc4))
#define _DAT_00901fec (*(int*)GIMG(0x901fec))
#define _DAT_00902014 (*(int*)GIMG(0x902014))
#define _DAT_00902090 (*(int*)GIMG(0x902090))
#define _DAT_00902092 (*(int*)GIMG(0x902092))
#define _DAT_00902094 (*(int*)GIMG(0x902094))
#define _DAT_00902096 (*(int*)GIMG(0x902096))
#define _DAT_00902098 (*(int*)GIMG(0x902098))
#define _DAT_0090209a (*(int*)GIMG(0x90209a))
#define _DAT_0090209c (*(int*)GIMG(0x90209c))
#define _DAT_0090209e (*(int*)GIMG(0x90209e))
#define _DAT_009020a0 (*(int*)GIMG(0x9020a0))
#define _DAT_00905a10 (*(int*)GIMG(0x905a10))
#define _DAT_00905a14 (*(int*)GIMG(0x905a14))
#define _DAT_00905a18 (*(int*)GIMG(0x905a18))
#define _DAT_00905a1c (*(int*)GIMG(0x905a1c))
#define _DAT_00905a78 (*(int*)GIMG(0x905a78))
#define _DAT_00905a7a (*(int*)GIMG(0x905a7a))
#define _DAT_00905a80 (*(int*)GIMG(0x905a80))
#define _DAT_00905a82 (*(int*)GIMG(0x905a82))
#define _DAT_00905a88 (*(int*)GIMG(0x905a88))
#define _DAT_00905a8a (*(int*)GIMG(0x905a8a))
#define _DAT_00905a90 (*(int*)GIMG(0x905a90))
#define _DAT_00905a92 (*(int*)GIMG(0x905a92))
#define _DAT_00905a9c (*(int*)GIMG(0x905a9c))
#define _DAT_00905a9e (*(int*)GIMG(0x905a9e))
#define _DAT_00905aa4 (*(int*)GIMG(0x905aa4))
#define _DAT_00905aa6 (*(int*)GIMG(0x905aa6))
#define _DAT_00905aac (*(int*)GIMG(0x905aac))
#define _DAT_00905aae (*(int*)GIMG(0x905aae))
#define _DAT_00905ab4 (*(int*)GIMG(0x905ab4))
#define _DAT_00905ab6 (*(int*)GIMG(0x905ab6))
#define _DAT_00905ac4 (*(int*)GIMG(0x905ac4))
#define _DAT_00905af2 (*(int*)GIMG(0x905af2))
#define _DAT_00905af4 (*(int*)GIMG(0x905af4))
#define _DAT_009077f0 (*(int*)GIMG(0x9077f0))
#define _DAT_00907800 (*(int*)GIMG(0x907800))
#define _DAT_00907818 (*(int*)GIMG(0x907818))
#define _DAT_00907828 (*(int*)GIMG(0x907828))
#define _DAT_00907840 (*(int*)GIMG(0x907840))
#define _DAT_00907850 (*(int*)GIMG(0x907850))
#define _DAT_00907868 (*(int*)GIMG(0x907868))
#define _DAT_00907878 (*(int*)GIMG(0x907878))
#define _DAT_00907920 (*(int*)GIMG(0x907920))
#define _DAT_00907940 (*(int*)GIMG(0x907940))
#define _DAT_00907944 (*(int*)GIMG(0x907944))
#define _DAT_00907948 (*(int*)GIMG(0x907948))
#define _DAT_00907c00 (*(int*)GIMG(0x907c00))
#define _DAT_00907c04 (*(int*)GIMG(0x907c04))
#define _DAT_00907e20 (*(int*)GIMG(0x907e20))
#define _DAT_00908570 (*(int*)GIMG(0x908570))
#define _DAT_00908574 (*(int**)GIMG(0x908574))
#define _DAT_0090858c (*(int*)GIMG(0x90858c))
#define _DAT_00908590 (*(int**)GIMG(0x908590))
#define _DAT_009085a8 (*(int*)GIMG(0x9085a8))
#define _DAT_009085ac (*(int*)GIMG(0x9085ac))
#define _DAT_009086f8 (*(int*)GIMG(0x9086f8))
#define _DAT_009086fc (*(int*)GIMG(0x9086fc))
#define _DAT_00908700 (*(int*)GIMG(0x908700))
#define _DAT_00908704 (*(int*)GIMG(0x908704))
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
#define __AccessSema4 (*(int*)GIMG(0x46c4e8))
#define __AccessSema4Fini (*(int*)GIMG(0x46ff66))
#define __EFG_printf (*(int*)GIMG(0x46c4f4))
#define __ExceptionHandled (*(int*)GIMG(0x908594))
#define __FPE_handler (*(int*)GIMG(0x46c3fb))
#define __FPE_handler_exit (*(int*)GIMG(0x46c110))
#define __FirstThreadData (*(int*)GIMG(0x90857c))
#define __GetThreadPtr (*(int*)GIMG(0x46c32c))
#define __Is_DLL (*(int*)GIMG(0x908578))
#define __LargestSizeB4MiniHeapRover (*(int*)GIMG(0x46c408))
#define __NFiles (*(int*)GIMG(0x46c428))
#define __OpenStreams (*(int*)GIMG(0x908580))
#define __ReleaseSema4 (*(int*)GIMG(0x46c4ec))
#define __Rest8087 (*(int*)GIMG(0x46c500))
#define __Save8087 (*(int*)GIMG(0x46c4fc))
#define __ThreadDataSize (*(int*)GIMG(0x46c508))
#define __WD_Present (*(int*)GIMG(0x46c52c))
#define ___FPE_handler (*(int*)GIMG(0x46c3fb))
#define ___chipbug (*(int*)GIMG(0x46c538))
#define __atexit (*(int*)GIMG(0x46c108))
#define __chipbug (*(int*)GIMG(0x46c538))
#define __fheap_clean (*(int*)GIMG(0x908585))
#define __heap_enabled (*(int*)GIMG(0x46c518))
#define __int23_exit (*(int*)GIMG(0x46c10c))
#define __iob (*(int*)GIMG(0x46c114))
#define __nheap_clean (*(int*)GIMG(0x908584))
#define __nheapbeg (*(int*)GIMG(0x46c400))
#define __no87 (*(int*)GIMG(0x46c3ec))
#define __nullarea (*(int*)GIMG(0x460000))
#define __real87 (*(int*)GIMG(0x46c105))
#define __sig_fini_rtn (*(code**)GIMG(0x46c370))
#define __sig_init_rtn (*(int*)GIMG(0x46c36c))
#define __tmpfnext (*(int*)GIMG(0x46c31c))
#define __umaskval (*(int*)GIMG(0x46c410))
#define _amblksiz (*(int*)GIMG(0x46c51c))
#define _bcrgb (*(int*)GIMG(0x7142c4))
#define _cbyte (*(int*)GIMG(0x46c3e0))
#define _child (*(int*)GIMG(0x46c3e8))
#define _clutspace (*(int*)GIMG(0x7140d4))
#define _cmptr (*(int*)GIMG(0x7142c0))
#define _far_fog (*(int*)GIMG(0x462cd0))
#define _fcrgb (*(int*)GIMG(0x7142ec))
#define _flg (*(int*)GIMG(0x7142dc))
#define _fmode (*(int*)GIMG(0x46c31d))
#define _globmat (*(int*)GIMG(0x7142f0))
#define _lmptr (*(int*)GIMG(0x7142d8))
#define _near_fog (*(int*)GIMG(0x462ccc))
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
#define _start_TI (*(int*)GIMG(0x46ff6c))
#define _texturespace (*(int*)GIMG(0x7140cc))
#define _vr0 (*(int*)GIMG(0x714100))
#define _vr1 (*(int*)GIMG(0x714110))
#define _vr2 (*(int*)GIMG(0x7140e0))
#define _vr3 (*(int*)GIMG(0x7140f0))
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
#define draw_frame (*(int*)GIMG(0x73c2bc))
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
#define s_ABCDEFGHIJKLMNOPQRSTUVWXYZ012345_004692ec (*(int**)GIMG(0x4692ec))
#define s_ABNORMAL_TERMINATION_0046fc3c ((char*)GIMG(0x46fc3c))
#define s_AIDAN_0046da74 ((char*)GIMG(0x46da74))
#define s_AIDCRED_0046daec ((char*)GIMG(0x46daec))
#define s_Add_Buffer_Load__0046c77c ((char*)GIMG(0x46c77c))
#define s_Anonymous_0046ecf0 (*(int**)GIMG(0x46ecf0))
#define s_BACKD1_0046db94 ((char*)GIMG(0x46db94))
#define s_BACKD2_0046db9c ((char*)GIMG(0x46db9c))
#define s_BACKSPCE_0046d42c ((char*)GIMG(0x46d42c))
#define s_BKWN88A_0046ce68 ((char*)GIMG(0x46ce68))
#define s_BKWN88B_0046ce70 ((char*)GIMG(0x46ce70))
#define s_BKWN88C_0046ce78 ((char*)GIMG(0x46ce78))
#define s_BKWN88D_0046ce80 ((char*)GIMG(0x46ce80))
#define s_BKWN88E_0046ce88 ((char*)GIMG(0x46ce88))
#define s_BON88A_0046c934 ((char*)GIMG(0x46c934))
#define s_BON88A_0046ce90 ((char*)GIMG(0x46ce90))
#define s_BON88B_0046c93c ((char*)GIMG(0x46c93c))
#define s_BON88B_0046ce98 ((char*)GIMG(0x46ce98))
#define s_BON88C_0046cea0 ((char*)GIMG(0x46cea0))
#define s_BOOT88A_0046ced8 ((char*)GIMG(0x46ced8))
#define s_BOOT88B_0046cee0 ((char*)GIMG(0x46cee0))
#define s_BOOT88C_0046cee8 ((char*)GIMG(0x46cee8))
#define s_BUMP88A_0046ceb0 ((char*)GIMG(0x46ceb0))
#define s_BUMP88B_0046ceb8 ((char*)GIMG(0x46ceb8))
#define s_BUMP88C_0046cec0 ((char*)GIMG(0x46cec0))
#define s_BUMP88D_0046cec8 ((char*)GIMG(0x46cec8))
#define s_BUMP88E_0046ced0 ((char*)GIMG(0x46ced0))
#define s_Buffer_Load__0046c7b4 ((char*)GIMG(0x46c7b4))
#define s_CAMCORDR_0046d540 ((char*)GIMG(0x46d540))
#define s_CARDCROS_0046d2f0 ((char*)GIMG(0x46d2f0))
#define s_CARDCROS_0046e778 ((char*)GIMG(0x46e778))
#define s_CARDCROS_0046f47c ((char*)GIMG(0x46f47c))
#define s_CARDCROS_0046f724 ((char*)GIMG(0x46f724))
#define s_CARDCROS_0046f9d8 ((char*)GIMG(0x46f9d8))
#define s_CARDCURS_0046d440 ((char*)GIMG(0x46d440))
#define s_CARDTEXT_0046d420 ((char*)GIMG(0x46d420))
#define s_CARDTICK_0046d2e4 ((char*)GIMG(0x46d2e4))
#define s_CARDTICK_0046e76c ((char*)GIMG(0x46e76c))
#define s_CARDTICK_0046f470 ((char*)GIMG(0x46f470))
#define s_CARDTICK_0046f718 ((char*)GIMG(0x46f718))
#define s_CARDTICK_0046f9cc ((char*)GIMG(0x46f9cc))
#define s_CARD_ERR_0046d174 ((char*)GIMG(0x46d174))
#define s_CARD_ERR_0046e758 ((char*)GIMG(0x46e758))
#define s_CARD_ERR_0046f45c ((char*)GIMG(0x46f45c))
#define s_CARD_ERR_0046f704 ((char*)GIMG(0x46f704))
#define s_CARD_ERR_0046f9b8 ((char*)GIMG(0x46f9b8))
#define s_CARD_YN_0046d180 ((char*)GIMG(0x46d180))
#define s_CARD_YN_0046e764 ((char*)GIMG(0x46e764))
#define s_CARD_YN_0046f468 ((char*)GIMG(0x46f468))
#define s_CARD_YN_0046f710 ((char*)GIMG(0x46f710))
#define s_CARD_YN_0046f9c4 ((char*)GIMG(0x46f9c4))
#define s_CFONT2A_0046d4a4 ((char*)GIMG(0x46d4a4))
#define s_CFONT2A_0046d5d8 ((char*)GIMG(0x46d5d8))
#define s_CFONT2_0046d49c ((char*)GIMG(0x46d49c))
#define s_CFONT2_0046d5d0 ((char*)GIMG(0x46d5d0))
#define s_CFONT_0046d494 ((char*)GIMG(0x46d494))
#define s_CFONT_0046d5c0 ((char*)GIMG(0x46d5c0))
#define s_CLT_02dB2_0046c95c ((char*)GIMG(0x46c95c))
#define s_CLT_02dB3_0046c950 ((char*)GIMG(0x46c950))
#define s_CLT_02d_s_0046cde8 ((char*)GIMG(0x46cde8))
#define s_CLUT01B_0046c968 ((char*)GIMG(0x46c968))
#define s_CLUT_02dA_0046ce08 ((char*)GIMG(0x46ce08))
#define s_CLUT_02dB_0046c944 ((char*)GIMG(0x46c944))
#define s_CLUT_02dB_0046cdfc ((char*)GIMG(0x46cdfc))
#define s_CLUT_02dC_0046ce20 ((char*)GIMG(0x46ce20))
#define s_CLUT_02dD_0046ce14 ((char*)GIMG(0x46ce14))
#define s_CLUT_02dE_0046ce2c ((char*)GIMG(0x46ce2c))
#define s_CONTINUE_0046cff4 ((char*)GIMG(0x46cff4))
#define s_CREDITZ__0046ed08 ((char*)GIMG(0x46ed08))
#define s_Created_Primary_Surface_0046c69c ((char*)GIMG(0x46c69c))
#define s_Creating_Palette_0046c618 ((char*)GIMG(0x46c618))
#define s_DIRINFO_0046cf68 ((char*)GIMG(0x46cf68))
#define s_DRAWDIST_0046d000 ((char*)GIMG(0x46d000))
#define s_DRIVER_d_0046d79c ((char*)GIMG(0x46d79c))
#define s_DR_02dA_0046ce50 ((char*)GIMG(0x46ce50))
#define s_DR_02dB_0046ce58 ((char*)GIMG(0x46ce58))
#define s_DR_02dC_0046ce60 ((char*)GIMG(0x46ce60))
#define s_DUST1_0046cd8c ((char*)GIMG(0x46cd8c))
#define s_DUST2_0046cd94 ((char*)GIMG(0x46cd94))
#define s_DUST3_0046cd9c ((char*)GIMG(0x46cd9c))
#define s_DUST4_0046cda4 ((char*)GIMG(0x46cda4))
#define s_DUST5_0046cdac ((char*)GIMG(0x46cdac))
#define s_DUST6_0046cdb4 ((char*)GIMG(0x46cdb4))
#define s_DUST7_0046cdbc ((char*)GIMG(0x46cdbc))
#define s_DUST8_0046cdc4 ((char*)GIMG(0x46cdc4))
#define s_DirectDraw_Back_Count____d_0046c660 ((char*)GIMG(0x46c660))
#define s_DirectDraw_Palette_Count____d_0046c640 ((char*)GIMG(0x46c640))
#define s_DirectDraw_Palette_Count____d_0046c6b8 ((char*)GIMG(0x46c6b8))
#define s_DirectDraw_Primary_Count____d_0046c67c ((char*)GIMG(0x46c67c))
#define s_DirectDraw_Primary_Count____d_0046c6d8 ((char*)GIMG(0x46c6d8))
#define s_ENGINE_0046cea8 ((char*)GIMG(0x46cea8))
#define s_FASTLAP_0046cd2c ((char*)GIMG(0x46cd2c))
#define s_FILE_NOT_FOUND__0046c7c8 ((char*)GIMG(0x46c7c8))
#define s_FLARE_0_0046cc88 ((char*)GIMG(0x46cc88))
#define s_FLARE_1_0046cc78 ((char*)GIMG(0x46cc78))
#define s_FLARE_2_0046cc68 ((char*)GIMG(0x46cc68))
#define s_FLARE_3_0046cc80 ((char*)GIMG(0x46cc80))
#define s_FLARE_4_0046cc70 ((char*)GIMG(0x46cc70))
#define s_FONT2_0046d484 ((char*)GIMG(0x46d484))
#define s_FONT2_0046d5c8 ((char*)GIMG(0x46d5c8))
#define s_FONT3_0046d48c ((char*)GIMG(0x46d48c))
#define s_FONT3_0046d5e0 ((char*)GIMG(0x46d5e0))
#define s_FRNT88A_0046cef8 ((char*)GIMG(0x46cef8))
#define s_FRNT88B_0046cf00 ((char*)GIMG(0x46cf00))
#define s_FRNT88C_0046cf08 ((char*)GIMG(0x46cf08))
#define s_FRNT88D_0046cf10 ((char*)GIMG(0x46cf10))
#define s_FRNT88E_0046cf18 ((char*)GIMG(0x46cf18))
#define s_FRWN88A_0046cf20 ((char*)GIMG(0x46cf20))
#define s_FRWN88B_0046cf28 ((char*)GIMG(0x46cf28))
#define s_FRWN88C_0046cf30 ((char*)GIMG(0x46cf30))
#define s_FRWN88D_0046cf38 ((char*)GIMG(0x46cf38))
#define s_FRWN88E_0046cf40 ((char*)GIMG(0x46cf40))
#define s_Floating_point_support_not_loade_0046fc18 ((char*)GIMG(0x46fc18))
#define s_GREENDIM_0046ccc0 ((char*)GIMG(0x46ccc0))
#define s_GRNLIGHT_0046ccb4 ((char*)GIMG(0x46ccb4))
#define s_HELMET_0046d0d0 ((char*)GIMG(0x46d0d0))
#define s_Had_to_Restore_Primary_Surface__0046c5d0 ((char*)GIMG(0x46c5d0))
#define s_Had_to_Restore_Primary_Surface__0046c5f4 ((char*)GIMG(0x46c5f4))
#define s_INTRO_AVI_0046cfd4 ((char*)GIMG(0x46cfd4))
#define s_INVALID_FILE_TYPE_0046c768 ((char*)GIMG(0x46c768))
#define s_Init_Primitive_Buffer_0046c880 ((char*)GIMG(0x46c880))
#define s_Invalid_Channel_0046c7e8 ((char*)GIMG(0x46c7e8))
#define s_Invalid_Channels_0046c820 ((char*)GIMG(0x46c820))
#define s_Kill_Sound_0046c7f8 ((char*)GIMG(0x46c7f8))
#define s_LAPNUM_0046cd04 ((char*)GIMG(0x46cd04))
#define s_LAPTIME_0046cd24 ((char*)GIMG(0x46cd24))
#define s_LEV0_COPYRIGH_BMP_0046d56c ((char*)GIMG(0x46d56c))
#define s_LEV0_FONT_BNK_0046d46c ((char*)GIMG(0x46d46c))
#define s_LEV0_FONT_BNK_0046d5a8 ((char*)GIMG(0x46d5a8))
#define s_LEV0_LEVEL_SPR_0046d45c ((char*)GIMG(0x46d45c))
#define s_LEV0_LOADING_BMP_0046d580 ((char*)GIMG(0x46d580))
#define s_LEVC_LEVEL_SPR_0046db84 ((char*)GIMG(0x46db84))
#define s_LEVF_LEVEL_SPR_0046d598 ((char*)GIMG(0x46d598))
#define s_LEV_X_LEVEL_CLT_0046cfa4 ((char*)GIMG(0x46cfa4))
#define s_LEV_X_LEVEL_DAT_0046cfc4 ((char*)GIMG(0x46cfc4))
#define s_LEV_X_LEVEL_ECL_0046cfb4 ((char*)GIMG(0x46cfb4))
#define s_LEV_X_LEVEL_PAL_0046cf94 ((char*)GIMG(0x46cf94))
#define s_LEV_X_LEVEL_TX0_0046cf70 ((char*)GIMG(0x46cf70))
#define s_LEV_X_LEVEL_TX_d_0046cf80 ((char*)GIMG(0x46cf80))
#define s_LIGHTSUR_0046cc94 ((char*)GIMG(0x46cc94))
#define s_Lock_Channel_0046c834 ((char*)GIMG(0x46c834))
#define s_MACSrPOO_0046ed14 ((char*)GIMG(0x46ed14))
#define s_MAD_I_0046d050 ((char*)GIMG(0x46d050))
#define s_Modify_Sound_0046c810 ((char*)GIMG(0x46c810))
#define s_Modifying_Palette_0046c62c ((char*)GIMG(0x46c62c))
#define s_NEG_I_0046d048 ((char*)GIMG(0x46d048))
#define s_NOTCHES_0046d01c ((char*)GIMG(0x46d01c))
#define s_NO_FILES_IN_LIST_0046c7a0 ((char*)GIMG(0x46c7a0))
#define s_OUTRO_AVI_0046cfe0 ((char*)GIMG(0x46cfe0))
#define s_OUT_OF_BANKS_0046c8c8 ((char*)GIMG(0x46c8c8))
#define s_OUT_OF_MEMORY_0046c790 ((char*)GIMG(0x46c790))
#define s_Out_of_heap_space_0046c86c ((char*)GIMG(0x46c86c))
#define s_P1D1T_d_0046cdf4 ((char*)GIMG(0x46cdf4))
#define s_PAUSED_0046cfec ((char*)GIMG(0x46cfec))
#define s_PC_DD2_0046c59c ((char*)GIMG(0x46c59c))
#define s_PC_Write_File__0046c924 ((char*)GIMG(0x46c924))
#define s_POSNUM_0046cd0c ((char*)GIMG(0x46cd0c))
#define s_Play_Sound_0046c804 ((char*)GIMG(0x46c804))
#define s_Player_0046eff8 ((char*)GIMG(0x46eff8))
#define s_Player__d_0046ecfc ((char*)GIMG(0x46ecfc))
#define s_Popped_too_many_matrices_0046c730 ((char*)GIMG(0x46c730))
#define s_Print__0046c8c0 ((char*)GIMG(0x46c8c0))
#define s_PushMatrix_0046c724 ((char*)GIMG(0x46c724))
#define s_Pushed_too_many_matrices_0046c708 ((char*)GIMG(0x46c708))
#define s_RACEPOIN_0046ccf8 ((char*)GIMG(0x46ccf8))
#define s_REDDIM_0046ccac ((char*)GIMG(0x46ccac))
#define s_REDLIGHT_0046cca0 ((char*)GIMG(0x46cca0))
#define s_RETIRE_0046d014 ((char*)GIMG(0x46d014))
#define s_RMPEClass_00460454 ((char*)GIMG(0x460454))
#define s_ROOF88A_0046cf48 ((char*)GIMG(0x46cf48))
#define s_ROOF88B_0046cf50 ((char*)GIMG(0x46cf50))
#define s_ROOF88C_0046cf58 ((char*)GIMG(0x46cf58))
#define s_RSURE_0046d024 ((char*)GIMG(0x46d024))
#define s_SFXVOL_0046d00c ((char*)GIMG(0x46d00c))
#define s_SHADOW_0046cf60 ((char*)GIMG(0x46cf60))
#define s_SMALLNUM_0046cd14 ((char*)GIMG(0x46cd14))
#define s_SMALRING_0046d2fc ((char*)GIMG(0x46d2fc))
#define s_SMALRING_0046e784 ((char*)GIMG(0x46e784))
#define s_SMALRING_0046f488 ((char*)GIMG(0x46f488))
#define s_SMALRING_0046f730 ((char*)GIMG(0x46f730))
#define s_SMALRING_0046f9e4 ((char*)GIMG(0x46f9e4))
#define s_SMCL_02dA_0046ce44 ((char*)GIMG(0x46ce44))
#define s_SMCL_02dB_0046ce38 ((char*)GIMG(0x46ce38))
#define s_SMOKE1_0046cd4c ((char*)GIMG(0x46cd4c))
#define s_SMOKE2_0046cd54 ((char*)GIMG(0x46cd54))
#define s_SMOKE3_0046cd5c ((char*)GIMG(0x46cd5c))
#define s_SMOKE4_0046cd64 ((char*)GIMG(0x46cd64))
#define s_SMOKE5_0046cd6c ((char*)GIMG(0x46cd6c))
#define s_SMOKE6_0046cd74 ((char*)GIMG(0x46cd74))
#define s_SMOKE7_0046cd7c ((char*)GIMG(0x46cd7c))
#define s_SMOKE8_0046cd84 ((char*)GIMG(0x46cd84))
#define s_SPARK_0046cdcc ((char*)GIMG(0x46cdcc))
#define s_SPEEDIND_0046cce0 ((char*)GIMG(0x46cce0))
#define s_STILLRUN_0046ccec ((char*)GIMG(0x46ccec))
#define s_SaveGames_0046c8ec ((char*)GIMG(0x46c8ec))
#define s_TICK2_0046d438 ((char*)GIMG(0x46d438))
#define s_TMOVE_0046d068 ((char*)GIMG(0x46d068))
#define s_TOO_MUCH_TEXT__0046c8b0 ((char*)GIMG(0x46c8b0))
#define s_TRACK_02dL_0046f030 ((char*)GIMG(0x46f030))
#define s_TRACK_02d_0046f024 ((char*)GIMG(0x46f024))
#define s_TSELECT_0046d058 ((char*)GIMG(0x46d058))
#define s_Text_functions__0046c8d8 ((char*)GIMG(0x46c8d8))
#define s_Thread_has_no_thread_specific_da_0046fc54 ((char*)GIMG(0x46fc54))
#define s_VAGS_BANK1_SBK_0046d070 ((char*)GIMG(0x46d070))
#define s_WHEEL2_0046cd38 ((char*)GIMG(0x46cd38))
#define s_WHEEL3_0046cd40 ((char*)GIMG(0x46cd40))
#define s__R_JC_T_Cannot_delete_0046d1b0 ((char*)GIMG(0x46d1b0))
#define s__R_JC_T_Cannot_format_0046d198 ((char*)GIMG(0x46d198))
#define s__R_JC_T_Cannot_load_0046d1c8 ((char*)GIMG(0x46d1c8))
#define s__R_JC_T_Cannot_save_0046d1dc ((char*)GIMG(0x46d1dc))
#define s__R_JC_T_Championship_0046ef88 ((char*)GIMG(0x46ef88))
#define s__R_JC_T_Enter_your_name_0046a03c ((char*)GIMG(0x46a03c))
#define s__R_JC_T_Fastest_Lap_0046a054 ((char*)GIMG(0x46a054))
#define s__R_JC_T_File_Options_0046f380 ((char*)GIMG(0x46f380))
#define s__R_JC_T_File_already_exists_0046d224 ((char*)GIMG(0x46d224))
#define s__R_JC_T_Formatting_0046d290 ((char*)GIMG(0x46d290))
#define s__R_JC_T_Keyboard_0046ed80 ((char*)GIMG(0x46ed80))
#define s__R_JC_T_Loading_0046d2b4 ((char*)GIMG(0x46d2b4))
#define s__R_JC_T_Memory_card_full_0046d1f0 ((char*)GIMG(0x46d1f0))
#define s__R_JC_T_Memory_card_unformatted_0046d258 ((char*)GIMG(0x46d258))
#define s__R_JC_T_No_Memory_card_0046d20c ((char*)GIMG(0x46d20c))
#define s__R_JC_T_Not_a_DD2_file_0046d240 ((char*)GIMG(0x46d240))
#define s__R_JC_T_Please_Wait___0046d278 ((char*)GIMG(0x46d278))
#define s__R_JC_T_Practice_0046efa0 ((char*)GIMG(0x46efa0))
#define s__R_JC_T_Reach_division_1_to_unlo_0046f0dc ((char*)GIMG(0x46f0dc))
#define s__R_JC_T_Reach_division_1_to_unlo_0046f104 ((char*)GIMG(0x46f104))
#define s__R_JC_T_Reach_division_2_to_unlo_0046f08c ((char*)GIMG(0x46f08c))
#define s__R_JC_T_Reach_division_2_to_unlo_0046f0b4 ((char*)GIMG(0x46f0b4))
#define s__R_JC_T_Reach_division_3_to_unlo_0046f03c ((char*)GIMG(0x46f03c))
#define s__R_JC_T_Reach_division_3_to_unlo_0046f064 ((char*)GIMG(0x46f064))
#define s__R_JC_T_Reading_Memory_cards_0046d2c4 ((char*)GIMG(0x46d2c4))
#define s__R_JC_T_Save_Game_0046f1a0 ((char*)GIMG(0x46f1a0))
#define s__R_JC_T_Save_Replay_0046f1b4 ((char*)GIMG(0x46f1b4))
#define s__R_JC_T_Saving_0046d2a4 ((char*)GIMG(0x46d2a4))
#define s__R_JC_T_Select_Control_Method_0046dd8c ((char*)GIMG(0x46dd8c))
#define s__R_JC_T_Select_Control_Method_0046ddac ((char*)GIMG(0x46ddac))
#define s__R_JC_T_Sound_Effects_0046dc1c ((char*)GIMG(0x46dc1c))
#define s__R_JC_T_View_Lap_Times_0046e82c ((char*)GIMG(0x46e82c))
#define s__R_JC_T_View_League_0046f358 ((char*)GIMG(0x46f358))
#define s__R_JC_T_View_Replay_0046eea8 ((char*)GIMG(0x46eea8))
#define s__R_JC_T_View_Replay_0046f36c ((char*)GIMG(0x46f36c))
#define s__R_JC_T_View_Replay_0046f6c8 ((char*)GIMG(0x46f6c8))
#define s__R_JC_T_View_Results_0046f340 ((char*)GIMG(0x46f340))
#define s__R_JC_T_View_Results_0046f934 ((char*)GIMG(0x46f934))
#define s__R_JC_T_View_Statistics_0046e814 ((char*)GIMG(0x46e814))
#define s__R_JC_T_View_Statistics_0046f398 ((char*)GIMG(0x46f398))
#define s__R_JC_T_Wrecking_Racing_0046ee00 ((char*)GIMG(0x46ee00))
#define s__R_JL_T_Amateur_0046dca8 ((char*)GIMG(0x46dca8))
#define s__R_JL_T_Delete_File_0046d3c0 ((char*)GIMG(0x46d3c0))
#define s__R_JL_T_Delete_File__0046d308 ((char*)GIMG(0x46d308))
#define s__R_JL_T_Division_1_0046f25c ((char*)GIMG(0x46f25c))
#define s__R_JL_T_Division_2_0046f270 ((char*)GIMG(0x46f270))
#define s__R_JL_T_Division_3_0046f284 ((char*)GIMG(0x46f284))
#define s__R_JL_T_Division_4_0046f298 ((char*)GIMG(0x46f298))
#define s__R_JL_T_ERROR_0046d188 ((char*)GIMG(0x46d188))
#define s__R_JL_T_Format__0046d338 ((char*)GIMG(0x46d338))
#define s__R_JL_T_Jug_0046ebc0 ((char*)GIMG(0x46ebc0))
#define s__R_JL_T_Load_0046d348 ((char*)GIMG(0x46d348))
#define s__R_JL_T_Load_File_0046d358 ((char*)GIMG(0x46d358))
#define s__R_JL_T_Overwrite_File__0046d320 ((char*)GIMG(0x46d320))
#define s__R_JL_T_Play_0046ec40 ((char*)GIMG(0x46ec40))
#define s__R_JL_T_Prev__Track_0046ec2c ((char*)GIMG(0x46ec2c))
#define s__R_JL_T_Pro_0046dcb8 ((char*)GIMG(0x46dcb8))
#define s__R_JL_T_Promoted_0046f4c4 ((char*)GIMG(0x46f4c4))
#define s__R_JL_T_Quit_DD2__0046e790 ((char*)GIMG(0x46e790))
#define s__R_JL_T_Quit_Season__0046f494 ((char*)GIMG(0x46f494))
#define s__R_JL_T_Quit_Season__0046f73c ((char*)GIMG(0x46f73c))
#define s__R_JL_T_Quit_Season__0046f9f0 ((char*)GIMG(0x46f9f0))
#define s__R_JL_T_Relegated_0046f4d8 ((char*)GIMG(0x46f4d8))
#define s__R_JL_T_Relegated_Out_0046f4ec ((char*)GIMG(0x46f4ec))
#define s__R_JL_T_Rookie_0046dc98 ((char*)GIMG(0x46dc98))
#define s__R_JL_T_Save_0046d36c ((char*)GIMG(0x46d36c))
#define s__R_JL_T_Save_Configuration_0046d37c ((char*)GIMG(0x46d37c))
#define s__R_JL_T_Save_Game_0046d398 ((char*)GIMG(0x46d398))
#define s__R_JL_T_Save_Replay_0046d3ac ((char*)GIMG(0x46d3ac))
#define s__R_JL_T_Season_Results_0046f4ac ((char*)GIMG(0x46f4ac))
#define s__R_JL_T_Select_Block_To_Save_To_0046d0f4 ((char*)GIMG(0x46d0f4))
#define s__R_JL_T_Select_Block_To_Save_To_0046d114 ((char*)GIMG(0x46d114))
#define s__R_JL_T_Select_File_To_Delete_0046d134 ((char*)GIMG(0x46d134))
#define s__R_JL_T_Select_File_To_Delete_0046d154 ((char*)GIMG(0x46d154))
#define s__R_JL_T_Select_File_To_Load_0046d0d8 ((char*)GIMG(0x46d0d8))
#define s__R_JL_T_Tuscan_0046ec8c ((char*)GIMG(0x46ec8c))
#define s__R_JL_T_Winner___0046f504 ((char*)GIMG(0x46f504))
#define s___JL__T_Left_0046e264 ((char*)GIMG(0x46e264))
#define s___JL__T_Right_0046e274 ((char*)GIMG(0x46e274))
#define s___JL__T_Space_0046e254 ((char*)GIMG(0x46e254))
#define s___JL__T__c_0046e248 ((char*)GIMG(0x46e248))
#define s___R__JC__T_Car__02d_0046d7cc ((char*)GIMG(0x46d7cc))
#define s___R__JC__T__d_0046da44 ((char*)GIMG(0x46da44))
#define s___R__JC__T__s_0046d44c ((char*)GIMG(0x46d44c))
#define s___R__JC__T__s_to_race_next___0046d54c ((char*)GIMG(0x46d54c))
#define s___R__JL__T_File__DD2____s_0046d3d4 ((char*)GIMG(0x46d3d4))
#define s___R__JL__T_File__EMPTY_0046d408 ((char*)GIMG(0x46d408))
#define s___R__JL__T_File__USED_0046d3f0 ((char*)GIMG(0x46d3f0))
#define s___R__JL__T_Season__d_0046d724 ((char*)GIMG(0x46d724))
#define s___R__JL__T_Track__d___0046ec74 ((char*)GIMG(0x46ec74))
#define s___R__JL__T__d_0046d7a8 ((char*)GIMG(0x46d7a8))
#define s___R__JL__T__d_0046da64 ((char*)GIMG(0x46da64))
#define s___R__JL__T__d__02d__02d_0046e3ec ((char*)GIMG(0x46e3ec))
#define s___R__JL__T__s_0046d714 ((char*)GIMG(0x46d714))
#define s___R__JL__T__s_0046d7bc ((char*)GIMG(0x46d7bc))
#define s___R__JL__T__s_0046da54 ((char*)GIMG(0x46da54))
#define s___R__JL__T__s_0046e3dc ((char*)GIMG(0x46e3dc))
#define s__s_player__d_0046ece0 ((char*)GIMG(0x46ece0))
#define s_avivideo_0046c74c ((char*)GIMG(0x46c74c))
#define s_cdaudio_0046c844 ((char*)GIMG(0x46c844))
#define s_conin__0046fc84 ((char*)GIMG(0x46fc84))
#define s_conout__0046fc8b ((char*)GIMG(0x46fc8b))
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
extern int DAT_009063b4_1;
extern int DAT_009063b8_1;
extern int DAT_5af34e72;
extern int FireCtrl;
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
extern int PTR_LAB_00426740;
extern int PTR_LAB_00426764;
extern int PTR_LAB_0042a4c0;
extern int SCA_Corner_Data_;
extern int SmokeCtrl;
extern int SparksCtrl;
extern int Speedway_Track_Type_;
extern int SteamCtrl;
extern int UNK_00458895;
extern int View_MultiLeague;
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
extern int __AccessFHeap;
extern int __AccessFList;
extern int __AccessFileH;
extern int __AccessNHeap;
extern int __AccessTDList;
extern int __FiniAccessH;
extern int __InitAccessH;
extern int* __LpCmdLine;
extern int __LpDllName;
extern int __LpPgmName;
extern int __MultipleThread;
extern int __ReleaseFHeap;
extern int __ReleaseFileH;
extern int __ReleaseIOB;
extern int __ReleaseNHeap;
extern code* __WinMainProc;
extern int ___ASTACKPTR_;
extern int ___ExceptionFilter;
extern int ___FirstThreadData;
extern int ___Is_DLL;
extern int ___OpenStreams;
extern int __bcrgb;
extern unsigned char* __clutspace;
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
extern int __sigabort;
extern unsigned char* __texturespace;
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
int FUN_0045672e(char* buf, const char* fmt, ...);
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
