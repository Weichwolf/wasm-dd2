/* ===== Load_Null @ 00414f30 ===== */

undefined4 Load_Null(undefined4 param_1,undefined4 param_2)

{
                    /* START-> C:\PcMpe\system\file.C: ? */
  return param_2;
}

/* ===== FUN_00414fb0 @ 00414fb0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_00414fb0(int *param_1,int param_2)

{
  num_textures = *param_1;
  _DAT_00716c18 =
       Load_Textures(param_1 + num_textures * 4 + 1,param_1 + 1,param_2 - (num_textures * 0x10 + 4))
  ;
  return num_textures * 0x10 + 4;
}

/* ===== Load_Texture2 @ 00414ff4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 Load_Texture2(undefined4 *param_1,int param_2)

{
  _DAT_00716c18 = Load_Textures(param_1,_DAT_00716c18,param_2);
  return 0;
}

