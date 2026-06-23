/* ===== FUN_004362b0 ===== */

undefined4 SmokeCtrl(int param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)(param_1 + 0x60) + -1;
  *(int *)(param_1 + 0x60) = iVar1;
  iVar2 = buffer_num;
  if (iVar1 == 0) {
    return 0;
  }
  iVar1 = (int)*(short *)(*(int *)(param_1 + 0x80) + *(int *)(param_1 + 0x84) * 2);
  *(undefined *)(param_1 + 0xc + buffer_num * 0x28) = (&DAT_00466376)[iVar1 * 0x18];
  *(undefined *)(param_1 + 0xd + iVar2 * 0x28) = (&DAT_00466377)[iVar1 * 0x18];
  *(char *)(param_1 + 0x14 + iVar2 * 0x28) = (&DAT_00466376)[iVar1 * 0x18] + '?';
  *(undefined *)(param_1 + 0x15 + iVar2 * 0x28) = (&DAT_00466377)[iVar1 * 0x18];
  *(undefined *)(param_1 + 0x1c + iVar2 * 0x28) = (&DAT_00466376)[iVar1 * 0x18];
  *(char *)(param_1 + 0x1d + iVar2 * 0x28) = (&DAT_00466377)[iVar1 * 0x18] + '?';
  *(char *)(param_1 + 0x24 + iVar2 * 0x28) = (&DAT_00466376)[iVar1 * 0x18] + '?';
  *(char *)(param_1 + 0x25 + iVar2 * 0x28) = (&DAT_00466377)[iVar1 * 0x18] + '?';
  *(ushort *)(param_1 + 0x16 + iVar2 * 0x28) =
       (ushort)*(byte *)(param_1 + 0xac) << 5 | *(ushort *)(&DAT_00466370 + iVar1 * 0x18) & 0xff9f;
  *(undefined2 *)(param_1 + 0xe + iVar2 * 0x28) = *(undefined2 *)(&DAT_00466372 + iVar1 * 0x18);
  *(int *)(param_1 + 0x54) = *(int *)(param_1 + 0x54) + 5;
  if (*(int *)(param_1 + 0x78) < *(int *)(param_1 + 0x74)) {
    *(int *)(param_1 + 0x78) = *(int *)(param_1 + 0x78) + *(int *)(param_1 + 0x7c);
  }
  else {
    *(int *)(param_1 + 0x78) = *(int *)(param_1 + 0x74);
  }
  *(int *)(param_1 + 0x84) = *(int *)(param_1 + 0x84) + 1;
  if ((*(uint *)(param_1 + 0x88) >> 8 & 0xfffff0) << 8 !=
      (*(uint *)(param_1 + 0xa0) >> 8 & 0xfffff0) << 8) {
    *(int *)(param_1 + 0x88) = *(int *)(param_1 + 0x88) + *(int *)(param_1 + 0x94);
    *(int *)(param_1 + 0x8c) = *(int *)(param_1 + 0x8c) + *(int *)(param_1 + 0x98);
    *(int *)(param_1 + 0x90) = *(int *)(param_1 + 0x90) + *(int *)(param_1 + 0x9c);
  }
  iVar2 = buffer_num * 0x28 + param_1;
  *(char *)(iVar2 + 4) = (char)(*(int *)(param_1 + 0x88) >> 0xc);
  *(char *)(iVar2 + 5) = (char)(*(int *)(param_1 + 0x8c) >> 0xc);
  *(char *)(iVar2 + 6) = (char)(*(int *)(param_1 + 0x90) >> 0xc);
  return 1;
}


/* ===== FUN_00436448 ===== */

undefined4 FireCtrl(int param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)(param_1 + 0x60) + -1;
  *(int *)(param_1 + 0x60) = iVar1;
  iVar2 = buffer_num;
  if (iVar1 == 0) {
    return 0;
  }
  iVar1 = (int)*(short *)(*(int *)(param_1 + 0x80) + *(int *)(param_1 + 0x84) * 2);
  *(undefined *)(param_1 + 0xc + buffer_num * 0x28) = (&DAT_00466436)[iVar1 * 0x18];
  *(undefined *)(param_1 + 0xd + iVar2 * 0x28) = (&DAT_00466437)[iVar1 * 0x18];
  *(char *)(param_1 + 0x14 + iVar2 * 0x28) = (&DAT_00466436)[iVar1 * 0x18] + '?';
  *(undefined *)(param_1 + 0x15 + iVar2 * 0x28) = (&DAT_00466437)[iVar1 * 0x18];
  *(undefined *)(param_1 + 0x1c + iVar2 * 0x28) = (&DAT_00466436)[iVar1 * 0x18];
  *(char *)(param_1 + 0x1d + iVar2 * 0x28) = (&DAT_00466437)[iVar1 * 0x18] + '?';
  *(char *)(param_1 + 0x24 + iVar2 * 0x28) = (&DAT_00466436)[iVar1 * 0x18] + '?';
  *(char *)(param_1 + 0x25 + iVar2 * 0x28) = (&DAT_00466437)[iVar1 * 0x18] + '?';
  *(undefined2 *)(param_1 + 0x16 + iVar2 * 0x28) = *(undefined2 *)(&DAT_00466430 + iVar1 * 0x18);
  *(undefined2 *)(param_1 + 0xe + iVar2 * 0x28) = *(undefined2 *)(&DAT_00466432 + iVar1 * 0x18);
  *(int *)(param_1 + 0x54) = *(int *)(param_1 + 0x54) + *(int *)(param_1 + 0x6c);
  if (*(int *)(param_1 + 0x78) < *(int *)(param_1 + 0x74)) {
    *(int *)(param_1 + 0x78) = *(int *)(param_1 + 0x78) + *(int *)(param_1 + 0x7c);
  }
  else {
    *(int *)(param_1 + 0x78) = *(int *)(param_1 + 0x74);
  }
  *(int *)(param_1 + 0x84) = *(int *)(param_1 + 0x84) + 1;
  if ((*(uint *)(param_1 + 0x88) >> 8 & 0xfffff0) << 8 !=
      (*(uint *)(param_1 + 0xa0) >> 8 & 0xfffff0) << 8) {
    *(int *)(param_1 + 0x88) = *(int *)(param_1 + 0x88) + *(int *)(param_1 + 0x94);
    *(int *)(param_1 + 0x8c) = *(int *)(param_1 + 0x8c) + *(int *)(param_1 + 0x98);
    *(int *)(param_1 + 0x90) = *(int *)(param_1 + 0x90) + *(int *)(param_1 + 0x9c);
  }
  iVar2 = buffer_num * 0x28 + param_1;
  *(char *)(iVar2 + 4) = (char)(*(int *)(param_1 + 0x88) >> 0xc);
  *(char *)(iVar2 + 5) = (char)(*(int *)(param_1 + 0x8c) >> 0xc);
  *(char *)(iVar2 + 6) = (char)(*(int *)(param_1 + 0x90) >> 0xc);
  return 1;
}


/* ===== FUN_00436804 ===== */

undefined4 SparksCtrl(int param_1)

{
  int iVar1;
  char cVar2;
  char cVar4;
  int iVar3;
  
  iVar3 = *(int *)(param_1 + 0x60) + -1;
  *(int *)(param_1 + 0x60) = iVar3;
  iVar1 = buffer_num;
  if (iVar3 == 0) {
    return 0;
  }
  *(char *)(param_1 + 0xc + buffer_num * 0x28) = DAT_004664f6;
  cVar2 = DAT_004664f7;
  *(char *)(param_1 + 0xd + iVar1 * 0x28) = DAT_004664f7;
  cVar4 = DAT_004664f6 + '\x0f';
  *(char *)(param_1 + 0x14 + iVar1 * 0x28) = cVar4;
  *(char *)(param_1 + 0x15 + iVar1 * 0x28) = cVar2;
  *(char *)(param_1 + 0x1c + iVar1 * 0x28) = DAT_004664f6;
  cVar2 = DAT_004664f7 + '\x0f';
  *(char *)(param_1 + 0x1d + iVar1 * 0x28) = cVar2;
  *(char *)(param_1 + 0x24 + iVar1 * 0x28) = cVar4;
  *(char *)(param_1 + 0x25 + iVar1 * 0x28) = cVar2;
  *(undefined2 *)(param_1 + 0x16 + iVar1 * 0x28) = DAT_004664f0;
  *(undefined2 *)(param_1 + 0xe + iVar1 * 0x28) = DAT_004664f2;
  *(int *)(param_1 + 0x6c) = *(int *)(param_1 + 0x6c) + -2;
  *(int *)(param_1 + 0x54) = *(int *)(param_1 + 0x54) + *(int *)(param_1 + 0x6c) / 2;
  *(int *)(param_1 + 0x50) = *(int *)(param_1 + 0x50) + *(int *)(param_1 + 0x68) / 2;
  *(int *)(param_1 + 0x58) = *(int *)(param_1 + 0x58) + *(int *)(param_1 + 0x70) / 2;
  if (*(int *)(param_1 + 0x78) < *(int *)(param_1 + 0x74)) {
    *(int *)(param_1 + 0x78) = *(int *)(param_1 + 0x78) + *(int *)(param_1 + 0x7c);
  }
  else {
    *(int *)(param_1 + 0x78) = *(int *)(param_1 + 0x74);
  }
  if ((*(uint *)(param_1 + 0x88) >> 8 & 0xfffff0) << 8 !=
      (*(uint *)(param_1 + 0xa0) >> 8 & 0xfffff0) << 8) {
    *(int *)(param_1 + 0x88) = *(int *)(param_1 + 0x88) + *(int *)(param_1 + 0x94);
    *(int *)(param_1 + 0x8c) = *(int *)(param_1 + 0x8c) + *(int *)(param_1 + 0x98);
    *(int *)(param_1 + 0x90) = *(int *)(param_1 + 0x90) + *(int *)(param_1 + 0x9c);
  }
  iVar1 = buffer_num;
  *(char *)(param_1 + 4 + buffer_num * 0x28) = (char)(*(int *)(param_1 + 0x88) >> 0xc);
  *(char *)(param_1 + 5 + iVar1 * 0x28) = (char)(*(int *)(param_1 + 0x8c) >> 0xc);
  *(char *)(param_1 + 6 + iVar1 * 0x28) = (char)(*(int *)(param_1 + 0x90) >> 0xc);
  return 1;
}


/* ===== FUN_00436a6c ===== */

undefined4 SteamCtrl(int param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)(param_1 + 0x60) + -1;
  *(int *)(param_1 + 0x60) = iVar1;
  iVar2 = buffer_num;
  if (iVar1 == 0) {
    return 0;
  }
  iVar1 = (int)*(short *)(*(int *)(param_1 + 0x80) + *(int *)(param_1 + 0x84) * 2);
  *(undefined *)(param_1 + 0xc + buffer_num * 0x28) = (&DAT_00466376)[iVar1 * 0x18];
  *(undefined *)(param_1 + 0xd + iVar2 * 0x28) = (&DAT_00466377)[iVar1 * 0x18];
  *(char *)(param_1 + 0x14 + iVar2 * 0x28) = (&DAT_00466376)[iVar1 * 0x18] + '?';
  *(undefined *)(param_1 + 0x15 + iVar2 * 0x28) = (&DAT_00466377)[iVar1 * 0x18];
  *(undefined *)(param_1 + 0x1c + iVar2 * 0x28) = (&DAT_00466376)[iVar1 * 0x18];
  *(char *)(param_1 + 0x1d + iVar2 * 0x28) = (&DAT_00466377)[iVar1 * 0x18] + '?';
  *(char *)(param_1 + 0x24 + iVar2 * 0x28) = (&DAT_00466376)[iVar1 * 0x18] + '?';
  *(char *)(param_1 + 0x25 + iVar2 * 0x28) = (&DAT_00466377)[iVar1 * 0x18] + '?';
  *(ushort *)(param_1 + 0x16 + iVar2 * 0x28) =
       (ushort)*(byte *)(param_1 + 0xac) << 5 | *(ushort *)(&DAT_00466370 + iVar1 * 0x18) & 0xff9f;
  *(undefined2 *)(param_1 + 0xe + iVar2 * 0x28) = *(undefined2 *)(&DAT_00466372 + iVar1 * 0x18);
  *(int *)(param_1 + 0x54) = *(int *)(param_1 + 0x54) + *(int *)(param_1 + 0x6c);
  if (*(int *)(param_1 + 0x78) < *(int *)(param_1 + 0x74)) {
    *(int *)(param_1 + 0x78) = *(int *)(param_1 + 0x78) + *(int *)(param_1 + 0x7c);
  }
  else {
    *(int *)(param_1 + 0x78) = *(int *)(param_1 + 0x74);
  }
  *(int *)(param_1 + 0x84) = *(int *)(param_1 + 0x84) + 1;
  if ((*(uint *)(param_1 + 0x88) >> 8 & 0xfffff0) << 8 !=
      (*(uint *)(param_1 + 0xa0) >> 8 & 0xfffff0) << 8) {
    *(int *)(param_1 + 0x88) = *(int *)(param_1 + 0x88) + *(int *)(param_1 + 0x94);
    *(int *)(param_1 + 0x8c) = *(int *)(param_1 + 0x8c) + *(int *)(param_1 + 0x98);
    *(int *)(param_1 + 0x90) = *(int *)(param_1 + 0x90) + *(int *)(param_1 + 0x9c);
  }
  iVar2 = buffer_num * 0x28 + param_1;
  *(char *)(iVar2 + 4) = (char)(*(int *)(param_1 + 0x88) >> 0xc);
  *(char *)(iVar2 + 5) = (char)(*(int *)(param_1 + 0x8c) >> 0xc);
  *(char *)(iVar2 + 6) = (char)(*(int *)(param_1 + 0x90) >> 0xc);
  return 1;
}


