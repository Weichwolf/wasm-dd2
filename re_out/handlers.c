/* ===== FUN_00417ea0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00417ea0(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
                    /* START-> C:\PCMPE\graphics\object.C: ? */
  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {
    *(undefined1 *)(_gprim1 + 7) = 0x20;
    *(undefined1 *)(_gprim2 + 7) = 0x20;
    uVar1 = *(undefined4 *)(_gpoly + 4);
    *(undefined4 *)(_gprim2 + 4) = uVar1;
    *(undefined4 *)(_gprim1 + 4) = uVar1;
    *(undefined1 *)(_gprim2 + 4) = 0;
    _gpoly = _gpoly + 0x10;
    *(undefined1 *)(_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);
    _gprim2 = _gprim2 + 0x14;
    _gprim1 = _gprim1 + 0x14;
  }
  return;
}


/* ===== FUN_0041861c ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0041861c(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {
    *(undefined1 *)(_gprim1 + 7) = 0x28;
    *(undefined1 *)(_gprim2 + 7) = 0x28;
    uVar1 = *(undefined4 *)(_gpoly + 4);
    *(undefined4 *)(_gprim2 + 4) = uVar1;
    *(undefined4 *)(_gprim1 + 4) = uVar1;
    *(undefined1 *)(_gprim2 + 4) = 0;
    _gpoly = _gpoly + 0x10;
    *(undefined1 *)(_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);
    _gprim2 = _gprim2 + 0x18;
    _gprim1 = _gprim1 + 0x18;
  }
  return;
}


/* ===== FUN_00418ed0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00418ed0(int param_1)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  int iStack_18;
  undefined4 *puStack_14;
  
  puStack_14 = _gprim2;
  for (iStack_18 = 0; iStack_18 < param_1; iStack_18 = iStack_18 + 1) {
    *(undefined1 *)((int)_gprim1 + 7) = 0x24;
    _gprim1[1] = *(undefined4 *)(_gpoly + 4);
    iVar1 = *(int *)(_gpoly + 6) >> 0x10;
    *(undefined2 *)((int)_gprim1 + 0x16) = *(undefined2 *)(_gtexture + iVar1 * 0xc);
    *(undefined2 *)((int)_gprim1 + 0xe) = *(undefined2 *)(_gpoly + 10);
    if (((*(byte *)((int)_gprim1 + 0x17) & 0x80) != 0) &&
       (*(char *)(__clutspace + 0x800 +
                 ((int)(uint)(ushort)((*(ushort *)((int)_gprim1 + 0x16) >> 8 & 0x7f) << 8) >> 4) +
                 (uint)*(ushort *)((int)_gprim1 + 0xe) * 0x1000) == '\0')) {
      *(char *)((int)_gprim1 + 7) = *(char *)((int)_gprim1 + 7) + -0x10;
    }
    iVar1 = _gtexture + iVar1 * 0xc;
    *(undefined2 *)(_gprim1 + 3) = *(undefined2 *)(iVar1 + 4);
    *(undefined2 *)(_gprim1 + 5) = *(undefined2 *)(iVar1 + 6);
    *(undefined2 *)(_gprim1 + 7) = *(undefined2 *)(iVar1 + 8);
    puVar2 = _gprim1;
    puVar3 = puStack_14;
    for (iVar1 = 8; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar3 = puVar3 + 1;
    }
    for (iVar1 = 0; iVar1 != 0; iVar1 = iVar1 + -1) {
      *(undefined1 *)puVar3 = *(undefined1 *)puVar2;
      puVar2 = (undefined4 *)((int)puVar2 + 1);
      puVar3 = (undefined4 *)((int)puVar3 + 1);
    }
    puStack_14 = puStack_14 + 8;
    _gpoly = _gpoly + 0x14;
    _gprim1 = _gprim1 + 8;
  }
  _gprim2 = puStack_14;
  return;
}


/* ===== FUN_0041bc0c ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0041bc0c(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {
    *(undefined1 *)(_gprim1 + 7) = 0x30;
    *(undefined1 *)(_gprim2 + 7) = 0x30;
    uVar1 = *(undefined4 *)(_gpoly + 4);
    *(undefined4 *)(_gprim2 + 4) = uVar1;
    *(undefined4 *)(_gprim1 + 4) = uVar1;
    *(undefined1 *)(_gprim2 + 4) = 0;
    _gpoly = _gpoly + 0x18;
    *(undefined1 *)(_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);
    _gprim2 = _gprim2 + 0x1c;
    _gprim1 = _gprim1 + 0x1c;
  }
  return;
}


/* ===== FUN_0041bc68 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0041bc68(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {
    *(undefined1 *)(_gprim1 + 7) = 0x30;
    *(undefined1 *)(_gprim2 + 7) = 0x30;
    uVar1 = *(undefined4 *)(_gpoly + 4);
    *(undefined4 *)(_gprim2 + 4) = uVar1;
                    /* END-> C:\PCMPE\sound\sound.C: ? */
    *(undefined4 *)(_gprim1 + 4) = uVar1;
    *(undefined1 *)(_gprim2 + 4) = 0;
    _gpoly = _gpoly + 0x14;
    *(undefined1 *)(_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);
    _gprim2 = _gprim2 + 0x1c;
    _gprim1 = _gprim1 + 0x1c;
  }
  return;
}


/* ===== FUN_0041c3e4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0041c3e4(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {
    *(undefined1 *)(_gprim1 + 7) = 0x38;
    *(undefined1 *)(_gprim2 + 7) = 0x38;
    uVar1 = *(undefined4 *)(_gpoly + 4);
    *(undefined4 *)(_gprim2 + 4) = uVar1;
    *(undefined4 *)(_gprim1 + 4) = uVar1;
    *(undefined1 *)(_gprim2 + 4) = 0;
    _gpoly = _gpoly + 0x1c;
    *(undefined1 *)(_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);
    _gprim2 = _gprim2 + 0x24;
    _gprim1 = _gprim1 + 0x24;
  }
  return;
}


/* ===== FUN_0041c440 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0041c440(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  for (iVar2 = 0; iVar2 < param_1; iVar2 = iVar2 + 1) {
    *(undefined1 *)(_gprim1 + 7) = 0x38;
    *(undefined1 *)(_gprim2 + 7) = 0x38;
    uVar1 = *(undefined4 *)(_gpoly + 4);
    *(undefined4 *)(_gprim2 + 4) = uVar1;
    *(undefined4 *)(_gprim1 + 4) = uVar1;
    *(undefined1 *)(_gprim2 + 4) = 0;
    _gpoly = _gpoly + 0x18;
    *(undefined1 *)(_gprim1 + 4) = *(undefined1 *)(_gprim2 + 4);
    _gprim2 = _gprim2 + 0x24;
    _gprim1 = _gprim1 + 0x24;
  }
  return;
}


/* ===== FUN_0041ccf8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0041ccf8(int param_1)

{
  undefined2 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  int iStack_18;
  undefined4 *puStack_14;
  
  puStack_14 = _gprim2;
  for (iStack_18 = 0; iStack_18 < param_1; iStack_18 = iStack_18 + 1) {
    *(undefined1 *)((int)_gprim1 + 7) = 0x34;
    _gprim1[1] = *(undefined4 *)(_gpoly + 4);
    _gprim1[4] = *(undefined4 *)(_gpoly + 8);
    _gprim1[7] = *(undefined4 *)(_gpoly + 0xc);
    puVar1 = (undefined2 *)(_gtexture + (*(int *)(_gpoly + 0xe) >> 0x10) * 0xc);
    *(undefined2 *)((int)_gprim1 + 0x1a) = *puVar1;
    *(undefined2 *)((int)_gprim1 + 0xe) = *(undefined2 *)(_gpoly + 0x12);
    *(undefined2 *)(_gprim1 + 3) = puVar1[2];
    *(undefined2 *)(_gprim1 + 6) = puVar1[3];
    *(undefined2 *)(_gprim1 + 9) = puVar1[4];
    puVar3 = _gprim1;
    puVar4 = puStack_14;
    for (iVar2 = 10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined1 *)puVar4 = *(undefined1 *)puVar3;
      puVar3 = (undefined4 *)((int)puVar3 + 1);
      puVar4 = (undefined4 *)((int)puVar4 + 1);
    }
    puStack_14 = puStack_14 + 10;
    _gpoly = _gpoly + 0x1c;
    _gprim1 = _gprim1 + 10;
  }
  _gprim2 = puStack_14;
  return;
}


/* ===== FUN_0041cdd0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0041cdd0(int param_1)

{
  undefined2 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  int iStack_18;
  undefined4 *puStack_14;
  
  puStack_14 = _gprim2;
  for (iStack_18 = 0; iStack_18 < param_1; iStack_18 = iStack_18 + 1) {
    *(undefined1 *)((int)_gprim1 + 7) = 0x34;
    puVar1 = (undefined2 *)((*(int *)(_gpoly + 6) >> 0x10) * 0xc + _gtexture);
    *(undefined2 *)((int)_gprim1 + 0x1a) = *puVar1;
    *(undefined2 *)((int)_gprim1 + 0xe) = *(undefined2 *)(_gpoly + 10);
    *(undefined2 *)(_gprim1 + 3) = puVar1[2];
    *(undefined2 *)(_gprim1 + 6) = puVar1[3];
    *(undefined2 *)(_gprim1 + 9) = puVar1[4];
    puVar3 = _gprim1;
    puVar4 = puStack_14;
    for (iVar2 = 10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined1 *)puVar4 = *(undefined1 *)puVar3;
      puVar3 = (undefined4 *)((int)puVar3 + 1);
      puVar4 = (undefined4 *)((int)puVar4 + 1);
    }
    puStack_14 = puStack_14 + 10;
    _gpoly = _gpoly + 0x18;
    _gprim1 = _gprim1 + 10;
  }
  _gprim2 = puStack_14;
  return;
}


/* ===== FUN_0041d834 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0041d834(int param_1)

{
  undefined2 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  int iStack_18;
  undefined4 *puStack_14;
  
  puStack_14 = _gprim2;
  for (iStack_18 = 0; iStack_18 < param_1; iStack_18 = iStack_18 + 1) {
    *(undefined1 *)((int)_gprim1 + 7) = 0x3c;
    _gprim1[1] = *(undefined4 *)(_gpoly + 4);
    _gprim1[4] = *(undefined4 *)(_gpoly + 8);
    _gprim1[7] = *(undefined4 *)(_gpoly + 0xc);
    _gprim1[10] = *(undefined4 *)(_gpoly + 0x10);
    puVar1 = (undefined2 *)((*(int *)(_gpoly + 0x12) >> 0x10) * 0xc + _gtexture);
    *(undefined2 *)((int)_gprim1 + 0x1a) = *puVar1;
    *(undefined2 *)((int)_gprim1 + 0xe) = *(undefined2 *)(_gpoly + 0x16);
    *(undefined2 *)(_gprim1 + 3) = puVar1[2];
    *(undefined2 *)(_gprim1 + 6) = puVar1[3];
    *(undefined2 *)(_gprim1 + 9) = puVar1[4];
    *(undefined2 *)(_gprim1 + 0xc) = puVar1[5];
    puVar3 = _gprim1;
    puVar4 = puStack_14;
    for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined1 *)puVar4 = *(undefined1 *)puVar3;
      puVar3 = (undefined4 *)((int)puVar3 + 1);
      puVar4 = (undefined4 *)((int)puVar4 + 1);
    }
    puStack_14 = puStack_14 + 0xd;
    _gpoly = _gpoly + 0x20;
    _gprim1 = _gprim1 + 0xd;
  }
  _gprim2 = puStack_14;
  return;
}


/* ===== FUN_0041d918 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0041d918(int param_1)

{
  undefined2 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  int iStack_18;
  undefined4 *puStack_14;
  
  puStack_14 = _gprim2;
  for (iStack_18 = 0; iStack_18 < param_1; iStack_18 = iStack_18 + 1) {
    *(undefined1 *)((int)_gprim1 + 7) = 0x3c;
    puVar1 = (undefined2 *)((*(int *)(_gpoly + 6) >> 0x10) * 0xc + _gtexture);
    *(undefined2 *)((int)_gprim1 + 0x1a) = *puVar1;
    *(undefined2 *)((int)_gprim1 + 0xe) = *(undefined2 *)(_gpoly + 10);
    *(undefined2 *)(_gprim1 + 3) = puVar1[2];
    *(undefined2 *)(_gprim1 + 6) = puVar1[3];
    *(undefined2 *)(_gprim1 + 9) = puVar1[4];
    *(undefined2 *)(_gprim1 + 0xc) = puVar1[5];
    puVar3 = _gprim1;
    puVar4 = puStack_14;
    for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined1 *)puVar4 = *(undefined1 *)puVar3;
      puVar3 = (undefined4 *)((int)puVar3 + 1);
      puVar4 = (undefined4 *)((int)puVar4 + 1);
    }
    puStack_14 = puStack_14 + 0xd;
    _gpoly = _gpoly + 0x1c;
    _gprim1 = _gprim1 + 0xd;
  }
  _gprim2 = puStack_14;
  return;
}


/* ===== FUN_0041e418 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void setup_face_sprite(int param_1)

{
  ushort *puVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  int iStack_18;
  undefined4 *puStack_14;
  
  puStack_14 = _gprim2;
  for (iStack_18 = 0; iStack_18 < param_1; iStack_18 = iStack_18 + 1) {
    *(undefined1 *)((int)_gprim1 + 7) = 0x2c;
    _gprim1[1] = *(undefined4 *)(_gpoly + 4);
    *(char *)((int)_gprim1 + 7) = *(char *)((int)_gprim1 + 7) + -0x10;
    puVar1 = (ushort *)((*(int *)(_gpoly + 6) >> 0x10) * 0xc + _gtexture);
    *(ushort *)((int)_gprim1 + 0x16) = *puVar1 & 0x1f | 0x20;
    *(undefined2 *)((int)_gprim1 + 0xe) = *(undefined2 *)(_gpoly + 10);
    *(ushort *)(_gprim1 + 3) = puVar1[2];
    *(ushort *)(_gprim1 + 5) = puVar1[3];
    *(ushort *)(_gprim1 + 7) = puVar1[4];
    *(ushort *)(_gprim1 + 9) = puVar1[5];
    puVar3 = _gprim1;
    puVar4 = puStack_14;
    for (iVar2 = 10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined1 *)puVar4 = *(undefined1 *)puVar3;
      puVar3 = (undefined4 *)((int)puVar3 + 1);
      puVar4 = (undefined4 *)((int)puVar4 + 1);
    }
    puStack_14 = puStack_14 + 10;
    _gpoly = _gpoly + 0x14;
    _gprim1 = _gprim1 + 10;
  }
  _gprim2 = puStack_14;
  return;
}


