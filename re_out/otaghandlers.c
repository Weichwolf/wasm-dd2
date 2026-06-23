/* ===== FUN_00410d50 ===== */

void FUN_00410d50(void)

{
                    /* START-> C:\PCMPE\libs\gfx.c: ? */
  return;
}


/* ===== FUN_00410d58 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00410d58(int param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  int iStack_40;
  int iStack_3c;
  int iStack_38;
  int iStack_34;
  int iStack_30;
  int iStack_2c;
  
  bVar1 = *(short *)(param_1 + 8) < 0;
  if (poly_clipx < *(int *)(param_1 + 6) >> 0x10) {
    bVar1 = 2;
  }
  bVar2 = *(short *)(param_1 + 0xc) < 0;
  if (poly_clipx < *(int *)(param_1 + 10) >> 0x10) {
    bVar2 = 2;
  }
  bVar3 = *(short *)(param_1 + 0x10) < 0;
  if (poly_clipx < *(int *)(param_1 + 0xe) >> 0x10) {
    bVar3 = 2;
  }
  if ((bVar1 & bVar2 & bVar3) == 0) {
    dth_clip = (uint)(bVar3 != 0 || (bVar1 != 0 || bVar2 != 0));
    iStack_40 = *(int *)(param_1 + 6) >> 0x10;
    iStack_38 = *(int *)(param_1 + 10) >> 0x10;
    iStack_30 = *(int *)(param_1 + 0xe) >> 0x10;
    iStack_3c = *(int *)(param_1 + 8) >> 0x10;
    iStack_34 = *(int *)(param_1 + 0xc) >> 0x10;
    iStack_2c = *(int *)(param_1 + 0x10) >> 0x10;
    _dth_shade = (uint)*(byte *)(param_1 + 4);
    FUN_0041243c(&iStack_40,draw_half);
  }
  return;
}


/* ===== FUN_00410e44 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00410e44(int param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  int iStack_40;
  int iStack_3c;
  int iStack_38;
  int iStack_34;
  int iStack_30;
  int iStack_2c;
  
  bVar1 = *(short *)(param_1 + 8) < 0;
  if (poly_clipx < *(int *)(param_1 + 6) >> 0x10) {
    bVar1 = 2;
  }
  bVar2 = *(short *)(param_1 + 0xc) < 0;
  if (poly_clipx < *(int *)(param_1 + 10) >> 0x10) {
    bVar2 = 2;
  }
  bVar3 = *(short *)(param_1 + 0x10) < 0;
  if (poly_clipx < *(int *)(param_1 + 0xe) >> 0x10) {
    bVar3 = 2;
  }
  bVar4 = *(short *)(param_1 + 0x14) < 0;
  if (poly_clipx < *(int *)(param_1 + 0x12) >> 0x10) {
    bVar4 = 2;
  }
  if ((bVar4 & bVar1 & bVar2 & bVar3) == 0) {
    dth_clip = (uint)(bVar4 != 0 || ((bVar1 != 0 || bVar2 != 0) || bVar3 != 0));
    iStack_40 = *(int *)(param_1 + 6) >> 0x10;
    iStack_38 = *(int *)(param_1 + 10) >> 0x10;
    iStack_30 = *(int *)(param_1 + 0xe) >> 0x10;
    iStack_3c = *(int *)(param_1 + 8) >> 0x10;
    iStack_34 = *(int *)(param_1 + 0xc) >> 0x10;
    iStack_2c = *(int *)(param_1 + 0x10) >> 0x10;
    _dth_shade = (uint)*(byte *)(param_1 + 4);
    FUN_0041243c(&iStack_40,draw_half);
    iStack_40 = *(int *)(param_1 + 0x12) >> 0x10;
    iStack_3c = *(int *)(param_1 + 0x14) >> 0x10;
    FUN_0041243c(&iStack_40,draw_half);
  }
  return;
}


/* ===== FUN_004114c0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004114c0(int param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  int iStack_40;
  int iStack_3c;
  int iStack_38;
  int iStack_34;
  int iStack_30;
  int iStack_2c;
  uint uStack_28;
  uint uStack_24;
  uint uStack_20;
  uint uStack_1c;
  uint uStack_18;
  uint uStack_14;
  
  bVar1 = *(short *)(param_1 + 8) < 0;
  if (poly_clipx < *(int *)(param_1 + 6) >> 0x10) {
    bVar1 = 2;
  }
  bVar2 = *(short *)(param_1 + 0x10) < 0;
  if (poly_clipx < *(int *)(param_1 + 0xe) >> 0x10) {
    bVar2 = 2;
  }
  bVar3 = *(short *)(param_1 + 0x18) < 0;
  if (poly_clipx < *(int *)(param_1 + 0x16) >> 0x10) {
    bVar3 = 2;
  }
  bVar4 = *(short *)(param_1 + 0x20) < 0;
  if (poly_clipx < *(int *)(param_1 + 0x1e) >> 0x10) {
    bVar4 = 2;
  }
  if ((bVar4 & bVar1 & bVar2 & bVar3) == 0) {
    dth_clip = (uint)(bVar4 != 0 || (bVar3 != 0 || (bVar1 != 0 || bVar2 != 0)));
    dth_clut = (uint)*(ushort *)(param_1 + 0xe);
    dth_tpage = (uint)*(ushort *)(param_1 + 0x16);
    iStack_40 = *(int *)(param_1 + 6) >> 0x10;
    iStack_38 = *(int *)(param_1 + 0xe) >> 0x10;
    iStack_30 = *(int *)(param_1 + 0x16) >> 0x10;
    iStack_3c = *(int *)(param_1 + 8) >> 0x10;
    iStack_34 = *(int *)(param_1 + 0x10) >> 0x10;
    iStack_2c = *(int *)(param_1 + 0x18) >> 0x10;
    uStack_28 = (uint)*(byte *)(param_1 + 0xc);
    uStack_20 = (uint)*(byte *)(param_1 + 0x14);
    uStack_18 = (uint)*(byte *)(param_1 + 0x1c);
    uStack_24 = (uint)*(byte *)(param_1 + 0xd);
    uStack_1c = (uint)*(byte *)(param_1 + 0x15);
    uStack_14 = (uint)*(byte *)(param_1 + 0x1d);
    _dth_shade = (int)(uint)*(byte *)(param_1 + 4) >> 4;
    FUN_00411ebc(&iStack_40,draw_text_half_trans);
    iStack_40 = *(int *)(param_1 + 0x1e) >> 0x10;
    iStack_3c = *(int *)(param_1 + 0x20) >> 0x10;
    uStack_28 = (uint)*(byte *)(param_1 + 0x24);
    uStack_24 = (uint)*(byte *)(param_1 + 0x25);
    FUN_00411ebc(&iStack_40,draw_text_half_trans);
  }
  return;
}


/* ===== FUN_00411654 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00411654(int param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  int iStack_40;
  int iStack_3c;
  int iStack_38;
  int iStack_34;
  int iStack_30;
  int iStack_2c;
  uint uStack_28;
  uint uStack_24;
  uint uStack_20;
  uint uStack_1c;
  uint uStack_18;
  uint uStack_14;
  
  bVar1 = *(short *)(param_1 + 8) < 0;
  if (poly_clipx < *(int *)(param_1 + 6) >> 0x10) {
    bVar1 = 2;
  }
  bVar2 = *(short *)(param_1 + 0x10) < 0;
  if (poly_clipx < *(int *)(param_1 + 0xe) >> 0x10) {
    bVar2 = 2;
  }
  bVar3 = *(short *)(param_1 + 0x18) < 0;
  if (poly_clipx < *(int *)(param_1 + 0x16) >> 0x10) {
    bVar3 = 2;
  }
  bVar4 = *(short *)(param_1 + 0x20) < 0;
  if (poly_clipx < *(int *)(param_1 + 0x1e) >> 0x10) {
    bVar4 = 2;
  }
  if ((bVar4 & bVar1 & bVar2 & bVar3) == 0) {
    dth_clip = (uint)(bVar4 != 0 || (bVar3 != 0 || (bVar1 != 0 || bVar2 != 0)));
    dth_clut = (uint)*(ushort *)(param_1 + 0xe);
    dth_tpage = (uint)*(ushort *)(param_1 + 0x16);
    iStack_40 = *(int *)(param_1 + 6) >> 0x10;
    iStack_38 = *(int *)(param_1 + 0xe) >> 0x10;
    iStack_30 = *(int *)(param_1 + 0x16) >> 0x10;
    iStack_3c = *(int *)(param_1 + 8) >> 0x10;
    iStack_34 = *(int *)(param_1 + 0x10) >> 0x10;
    iStack_2c = *(int *)(param_1 + 0x18) >> 0x10;
    uStack_28 = (uint)*(byte *)(param_1 + 0xc);
    uStack_20 = (uint)*(byte *)(param_1 + 0x14);
    uStack_18 = (uint)*(byte *)(param_1 + 0x1c);
    uStack_24 = (uint)*(byte *)(param_1 + 0xd);
    uStack_1c = (uint)*(byte *)(param_1 + 0x15);
    uStack_14 = (uint)*(byte *)(param_1 + 0x1d);
    _dth_shade = (int)(uint)*(byte *)(param_1 + 4) >> 4;
    FUN_00411ebc(&iStack_40,FUN_0041033a);
    iStack_40 = *(int *)(param_1 + 0x1e) >> 0x10;
    iStack_3c = *(int *)(param_1 + 0x20) >> 0x10;
    uStack_28 = (uint)*(byte *)(param_1 + 0x24);
    uStack_24 = (uint)*(byte *)(param_1 + 0x25);
    FUN_00411ebc(&iStack_40,FUN_0041033a);
  }
  return;
}


/* ===== FUN_004117e8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004117e8(int param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  int iStack_40;
  int iStack_3c;
  int iStack_38;
  int iStack_34;
  int iStack_30;
  int iStack_2c;
  
  bVar1 = *(short *)(param_1 + 8) < 0;
  if (poly_clipx < *(int *)(param_1 + 6) >> 0x10) {
    bVar1 = 2;
  }
  bVar2 = *(short *)(param_1 + 0x10) < 0;
  if (poly_clipx < *(int *)(param_1 + 0xe) >> 0x10) {
    bVar2 = 2;
  }
  bVar3 = *(short *)(param_1 + 0x18) < 0;
  if (poly_clipx < *(int *)(param_1 + 0x16) >> 0x10) {
    bVar3 = 2;
  }
  if ((bVar1 & bVar2 & bVar3) == 0) {
    dth_clip = (uint)(bVar3 != 0 || (bVar1 != 0 || bVar2 != 0));
    iStack_40 = *(int *)(param_1 + 6) >> 0x10;
    iStack_38 = *(int *)(param_1 + 0xe) >> 0x10;
    iStack_30 = *(int *)(param_1 + 0x16) >> 0x10;
    iStack_3c = *(int *)(param_1 + 8) >> 0x10;
    iStack_34 = *(int *)(param_1 + 0x10) >> 0x10;
    iStack_2c = *(int *)(param_1 + 0x18) >> 0x10;
    _dth_shade = (uint)*(byte *)(param_1 + 4);
    FUN_0041243c(&iStack_40,draw_half);
  }
  return;
}


/* ===== FUN_004118d4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004118d4(int param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  int iStack_40;
  int iStack_3c;
  int iStack_38;
  int iStack_34;
  int iStack_30;
  int iStack_2c;
  
  bVar1 = *(short *)(param_1 + 8) < 0;
  if (poly_clipx < *(int *)(param_1 + 6) >> 0x10) {
    bVar1 = 2;
  }
  bVar2 = *(short *)(param_1 + 0x10) < 0;
  if (poly_clipx < *(int *)(param_1 + 0xe) >> 0x10) {
    bVar2 = 2;
  }
  bVar3 = *(short *)(param_1 + 0x18) < 0;
  if (poly_clipx < *(int *)(param_1 + 0x16) >> 0x10) {
    bVar3 = 2;
  }
  bVar4 = *(short *)(param_1 + 0x20) < 0;
  if (poly_clipx < *(int *)(param_1 + 0x1e) >> 0x10) {
    bVar4 = 2;
  }
  if ((bVar4 & bVar1 & bVar2 & bVar3) == 0) {
    dth_clip = (uint)(bVar4 != 0 || ((bVar1 != 0 || bVar2 != 0) || bVar3 != 0));
    iStack_40 = *(int *)(param_1 + 6) >> 0x10;
    iStack_38 = *(int *)(param_1 + 0xe) >> 0x10;
    iStack_30 = *(int *)(param_1 + 0x16) >> 0x10;
    iStack_3c = *(int *)(param_1 + 8) >> 0x10;
    iStack_34 = *(int *)(param_1 + 0x10) >> 0x10;
    iStack_2c = *(int *)(param_1 + 0x18) >> 0x10;
    _dth_shade = (uint)*(byte *)(param_1 + 4);
    FUN_0041243c(&iStack_40,draw_half);
    iStack_40 = *(int *)(param_1 + 0x1e) >> 0x10;
    iStack_3c = *(int *)(param_1 + 0x20) >> 0x10;
    FUN_0041243c(&iStack_40,draw_half);
  }
  return;
}


