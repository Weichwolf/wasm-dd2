/* ===== FUN_00448d10 ===== */

void FUN_00448d10(void)

{
  return;
}


/* ===== FUN_00448d18 ===== */

void PitOut(int param_1)

{
  if (((param_1 == 0) && (PIT_IN != 0)) && (PIT_IN = param_1, race_mode == 1)) {
    PIT_DONE = param_1;
  }
  return;
}


/* ===== FUN_00448d48 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void PitIn(int param_1)

{
  byte bVar1;
  int iVar2;
  int iStack_18;
  uint uStack_14;
  
  bVar1 = *(byte *)(_strip_data + *(int *)(&DAT_0075a2a4 + param_1 * 0x2c) + 9);
  iVar2 = Get_Car_Angle((int)(short)(_DAT_00752392 & 0xfff));
  uStack_14 = (iVar2 + *(int *)(param_1 * 0x1b2 + 0x75a682) & 0xfffU) -
              ((uint)bVar1 * 0x10 + 0x400 & 0xfff);
  Get_Direction_Cosines(&uStack_14,&iStack_18);
  if ((((int)uStack_14 < 0x200) || (0xe00 < (int)uStack_14)) && (param_1 == 0)) {
    PIT_IN = 1;
  }
  return;
}


/* ===== FUN_00448e0c ===== */

void Pit_Stop1(int param_1)

{
  if (((param_1 == 0) && (PIT_DONE == 0)) && (PIT_IN != 0)) {
    Pit_Timer = param_1;
    DAT_00467060 = 5;
    PIT_STOP = 1;
  }
  return;
}


/* ===== FUN_00448e48 ===== */

void Pit_Stop2(void)

{
  return;
}


