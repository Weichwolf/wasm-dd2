/* ===== FUN_00417efc ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_flat(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((uVar3 & *(uint *)(&rot_flags + iVar5 * 4)) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[3] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[4] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
    }
    puStack_14 = puStack_14 + 5;
    iStack_18 = iStack_18 + 0x10;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_004180c8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_flat_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar5 * 4) & uVar3) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[3] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[4] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
    }
    puStack_14 = puStack_14 + 5;
    iStack_18 = iStack_18 + 0x10;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041828c ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl draw_face_3pt_flat_dpq(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int local_1c;
  int local_18;
  undefined4 *local_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  local_14 = _gprim1;
  local_18 = _gpoly;
  for (local_1c = 0; local_1c < param_1; local_1c = local_1c + 1) {
    iVar5 = (*(int *)(local_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(local_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(local_18 + 10) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((uVar3 & *(uint *)(&rot_flags + iVar5 * 4)) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      local_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      local_14[3] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      local_14[4] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = local_14;
      *local_14 = uVar2;
    }
    local_14 = local_14 + 5;
    local_18 = local_18 + 0x10;
  }
  _gpoly = local_18;
  _gprim1 = local_14;
  return;
}


/* ===== FUN_00418458 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_flat_dpq_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar5 * 4) & uVar3) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[3] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[4] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
    }
    puStack_14 = puStack_14 + 5;
    iStack_18 = iStack_18 + 0x10;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_00418678 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_flat(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar6 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar6);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071414c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar7 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714144 - _DAT_00714124) * (_DAT_00714140 - _DAT_00714150) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[3] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[4] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar7 = *(int *)(iStack_18 + 0xc) >> 0x10;
      iVar8 = iVar7 * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar6 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar6 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar6;
      if ((*(uint *)(&rot_flags + iVar7 * 4) & uVar3) == 0) {
        puStack_14[5] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar5 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar5;
        *puVar5 = puStack_14;
        *puStack_14 = uVar2;
      }
    }
    puStack_14 = puStack_14 + 6;
    iStack_18 = iStack_18 + 0x10;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041888c ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_flat_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    iVar5 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar8 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar8);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071412c = *(undefined4 *)(iVar8 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - iVar5) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    _DAT_00714154 = iVar5;
    if (((uVar3 & *(uint *)(&rot_flags + iVar6 * 4)) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[3] = _DAT_00714150 & 0xffff | iVar5 << 0x10;
      puStack_14[4] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      iVar6 = *(int *)(iStack_18 + 0xc) >> 0x10;
      iVar8 = iVar6 * 0x10;
      _DAT_00714138 = _DAT_00714148;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar5 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar5 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar5;
      if ((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) {
        puStack_14[5] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar7 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar7;
        *puVar7 = puStack_14;
        *puStack_14 = uVar2;
      }
    }
    puStack_14 = puStack_14 + 6;
    iStack_18 = iStack_18 + 0x10;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_00418aa4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_flat_dpq(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar6 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar6);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071414c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar7 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714144 - _DAT_00714124) * (_DAT_00714140 - _DAT_00714150) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[3] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[4] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar7 = *(int *)(iStack_18 + 0xc) >> 0x10;
      iVar8 = iVar7 * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar6 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar6 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar6;
      if ((*(uint *)(&rot_flags + iVar7 * 4) & uVar3) == 0) {
        puStack_14[5] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar5 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar5;
        *puVar5 = puStack_14;
        *puStack_14 = uVar2;
      }
    }
    puStack_14 = puStack_14 + 6;
    iStack_18 = iStack_18 + 0x10;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_00418cb8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_flat_dpq_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    iVar5 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar8 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar8);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071412c = *(undefined4 *)(iVar8 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - iVar5) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    _DAT_00714154 = iVar5;
    if (((uVar3 & *(uint *)(&rot_flags + iVar6 * 4)) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[3] = _DAT_00714150 & 0xffff | iVar5 << 0x10;
      puStack_14[4] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      iVar6 = *(int *)(iStack_18 + 0xc) >> 0x10;
      iVar8 = iVar6 * 0x10;
      _DAT_00714138 = _DAT_00714148;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar5 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar5 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar5;
      if ((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) {
        puStack_14[5] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
                    /* END-> C:\PcMpe\libs\libgte.c: ? */
        puVar7 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar7;
        *puVar7 = puStack_14;
        *puStack_14 = uVar2;
      }
    }
    puStack_14 = puStack_14 + 6;
    iStack_18 = iStack_18 + 0x10;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_00418fe0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_text(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(iStack_18 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(iStack_18 + 0xe) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((uVar3 & *(uint *)(&rot_flags + iVar5 * 4)) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
    }
    puStack_14 = puStack_14 + 8;
    iStack_18 = iStack_18 + 0x14;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_004196a8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_text_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int *piVar7;
  int iVar8;
  int iVar9;
  int iStack_1c;
  int *piStack_18;
  undefined4 *puStack_14;
  
  iVar5 = _gnormals;
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  piStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar8 = (*(int *)((int)piStack_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar8);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
    iVar8 = (piStack_18[3] >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar8);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071415c = *(undefined4 *)(iVar8 + 0x716dcc);
    iVar8 = *(int *)((int)piStack_18 + 0xe) >> 0x10;
    iVar9 = iVar8 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar9);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar9);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar9);
    _DAT_0071412c = *(undefined4 *)(iVar9 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar8 * 4) & uVar3) == 0) &&
       (iVar8 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar9 = iVar8 >> 0x1f,
       __otz = (int)((iVar8 + iVar9 * -0x40) - (uint)(iVar9 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
      __vr3 = __vr2;
      _DAT_007140f4 = _DAT_007140e4;
      _DAT_007140f8 = _DAT_007140e8;
      _DAT_007140fc = _DAT_007140ec;
      __vr2 = __vr1;
      _DAT_007140e4 = _DAT_00714114;
      _DAT_007140e8 = _DAT_00714118;
      _DAT_007140ec = _DAT_0071411c;
      __vr1 = __vr0;
      _DAT_00714114 = _DAT_00714104;
      _DAT_00714118 = _DAT_00714108;
      _DAT_0071411c = _DAT_0071410c;
      piVar7 = (int *)((*piStack_18 >> 0x10) * 8 + iVar5);
      __vr0 = (int)(short)*piVar7;
      _DAT_00714108 = *(int *)((int)piVar7 + 2) >> 0x10;
      _DAT_00714104 = *piVar7 >> 0x10;
      iVar8 = (short)*__lmptr * __vr0 + (*__lmptr >> 0x10) * _DAT_00714104 +
              (*(int *)((int)__lmptr + 2) >> 0x10) * _DAT_00714108;
      iVar9 = iVar8 >> 0x1f;
      iVar8 = (uint)*(byte *)(piStack_18 + 1) *
              ((int)((iVar8 + iVar9 * -0x1000) - (uint)(iVar9 << 0xb < 0)) >> 0xc);
      iVar9 = iVar8 >> 0x1f;
      iVar8 = (int)((iVar8 + iVar9 * -0x1000) - (uint)(iVar9 << 0xb < 0)) >> 0xc;
      if (iVar8 < 1) {
        iVar8 = 0;
      }
      if (iVar8 + __bcrgb < 0xff) {
        iVar8 = (*__lmptr >> 0x10) * _DAT_00714104 + (short)*__lmptr * __vr0 +
                (*(int *)((int)__lmptr + 2) >> 0x10) * _DAT_00714108;
        iVar9 = iVar8 >> 0x1f;
        iVar8 = (uint)*(byte *)(piStack_18 + 1) *
                ((int)((iVar8 + iVar9 * -0x1000) - (uint)(iVar9 << 0xb < 0)) >> 0xc);
        iVar9 = iVar8 >> 0x1f;
        __rgb0 = (int)((iVar8 + iVar9 * -0x1000) - (uint)(iVar9 << 0xb < 0)) >> 0xc;
        if (__rgb0 < 1) {
          __rgb0 = 0;
        }
        __rgb0 = __rgb0 + __bcrgb;
      }
      else {
        __rgb0 = 0xff;
      }
      *(undefined1 *)(puStack_14 + 1) = _rgb0;
    }
    puStack_14 = puStack_14 + 8;
    piStack_18 = piStack_18 + 5;
  }
  _gpoly = piStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_004199b0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl draw_face_3pt_text_dpq(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  int local_1c;
  int local_18;
  undefined4 *local_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  local_14 = _gprim1;
  local_18 = _gpoly;
  for (local_1c = 0; local_1c < param_1; local_1c = local_1c + 1) {
    iVar6 = (*(int *)(local_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar6);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071414c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = (*(int *)(local_18 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(local_18 + 0xe) >> 0x10;
    iVar7 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) &&
       (iVar6 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar6 >> 0x1f,
       __otz = (int)((iVar6 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      local_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      local_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      local_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar5 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar5;
      *puVar5 = local_14;
      *local_14 = uVar2;
      __rgb0 = (uint)*(byte *)(local_18 + 4);
      gte_dpcs();
      *(undefined1 *)(local_14 + 1) = _rgb0;
    }
    local_14 = local_14 + 8;
    local_18 = local_18 + 0x14;
  }
  _gpoly = local_18;
  _gprim1 = local_14;
  return;
}


/* ===== FUN_0041a0ac ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_text_dpq_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int *piVar7;
  int iVar8;
  int iVar9;
  int iStack_1c;
  undefined4 *puStack_18;
  int *piStack_14;
  
  iVar5 = _gnormals;
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_18 = _gprim1;
  piStack_14 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar8 = (*(int *)((int)piStack_14 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar8);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
    iVar8 = (piStack_14[3] >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar8);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071415c = *(undefined4 *)(iVar8 + 0x716dcc);
    iVar8 = *(int *)((int)piStack_14 + 0xe) >> 0x10;
    iVar9 = iVar8 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar9);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar9);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar9);
    _DAT_0071412c = *(undefined4 *)(iVar9 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((uVar3 & *(uint *)(&rot_flags + iVar8 * 4)) == 0) &&
       (iVar8 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar9 = iVar8 >> 0x1f,
       __otz = (int)((iVar8 + iVar9 * -0x40) - (uint)(iVar9 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_18[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_18[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_18[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_18;
      *puStack_18 = uVar2;
      __rgb0 = (uint)*(byte *)(piStack_14 + 1);
      __vr3 = __vr2;
      _DAT_007140f4 = _DAT_007140e4;
      _DAT_007140f8 = _DAT_007140e8;
      _DAT_007140fc = _DAT_007140ec;
      __vr2 = __vr1;
      _DAT_007140e4 = _DAT_00714114;
      _DAT_007140e8 = _DAT_00714118;
      _DAT_007140ec = _DAT_0071411c;
      __vr1 = __vr0;
      _DAT_00714114 = _DAT_00714104;
      _DAT_00714118 = _DAT_00714108;
      _DAT_0071411c = _DAT_0071410c;
      piVar7 = (int *)((*piStack_14 >> 0x10) * 8 + iVar5);
      __vr0 = (int)(short)*piVar7;
      _DAT_00714104 = *piVar7 >> 0x10;
      _DAT_00714108 = *(int *)((int)piVar7 + 2) >> 0x10;
      gte_ncds();
      *(undefined1 *)(puStack_18 + 1) = _rgb0;
    }
    puStack_18 = puStack_18 + 8;
    piStack_14 = piStack_14 + 5;
  }
  _gpoly = piStack_14;
  _gprim1 = puStack_18;
  return;
}


/* ===== FUN_0041a40c ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_text(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int iVar9;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 0xc) >> 0x10) * 0x10;
                    /* END-> C:\PcMpe\system\file.C: ? */
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    iVar5 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar8 = *(int *)(iStack_18 + 0xe) >> 0x10;
    iVar6 = iVar8 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar6);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071412c = *(undefined4 *)(iVar6 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - iVar5) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    iVar6 = _DAT_00714148;
    _DAT_00714154 = iVar5;
    iVar9 = __opz;
    if ((uVar3 & *(uint *)(&rot_flags + iVar8 * 4)) == 0) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | iVar5 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar5 = *(int *)(iStack_18 + 0x10) >> 0x10;
      iVar9 = iVar5 * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar9);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar9);
      iVar6 = *(int *)(&DAT_00716dc8 + iVar9);
      _DAT_0071414c = *(undefined4 *)(iVar9 + 0x716dcc);
      iVar9 = (_DAT_00714144 - _DAT_00714124) * (_DAT_00714140 - _DAT_00714150) -
              (_DAT_00714140 - _DAT_00714120) * (_DAT_00714144 - _DAT_00714154);
      if ((uVar3 & *(uint *)(&rot_flags + iVar5 * 4)) == 0) {
        _DAT_00714148 = iVar6 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
        iVar5 = _DAT_00714148 >> 0x1f;
        __otz = (int)((_DAT_00714148 + iVar5 * -4) - (uint)(iVar5 << 1 < 0)) >> 2;
        if ((__opz < 1) || (-1 < iVar9)) {
          _DAT_00714148 = iVar6;
          __opz = iVar9;
          puStack_14[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
          puVar7 = (undefined4 *)
                   (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2)
                    * 4 + iVar4);
          uVar2 = *puVar7;
          *puVar7 = puStack_14;
          *puStack_14 = uVar2;
          iVar6 = _DAT_00714148;
          iVar9 = __opz;
        }
      }
    }
    __opz = iVar9;
    _DAT_00714148 = iVar6;
    puStack_14 = puStack_14 + 10;
    iStack_18 = iStack_18 + 0x14;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041acf4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_text_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  int *piVar9;
  int iVar10;
  int iStack_1c;
  undefined4 *puStack_18;
  int *piStack_14;
  
  iVar5 = _gnormals;
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_18 = _gprim1;
  piStack_14 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar6 = (*(int *)((int)piStack_14 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar6);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071414c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar7 = (piStack_14[3] >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar7);
    iVar6 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071415c = *(undefined4 *)(iVar7 + 0x716dcc);
    iVar7 = *(int *)((int)piStack_14 + 0xe) >> 0x10;
    iVar10 = iVar7 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar10);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar10);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar10);
    _DAT_0071412c = *(undefined4 *)(iVar10 + 0x716dcc);
    __opz = (_DAT_00714144 - _DAT_00714124) * (_DAT_00714140 - _DAT_00714150) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - iVar6);
    _DAT_00714120 = uVar1;
    _DAT_00714154 = iVar6;
    if (((*(uint *)(&rot_flags + iVar7 * 4) & uVar3) == 0) && (__opz < 1)) {
      puStack_18[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_18[4] = _DAT_00714150 & 0xffff | iVar6 << 0x10;
      puStack_18[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar7 = (piStack_14[4] >> 0x10) * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar7);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar7);
      iVar6 = *(int *)(&DAT_00716dc8 + iVar7);
      _DAT_0071414c = *(undefined4 *)(iVar7 + 0x716dcc);
      _DAT_00714148 = iVar6 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar7 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar7 * -4) - (uint)(iVar7 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar6;
      if ((*(uint *)(&rot_flags + (piStack_14[4] >> 0x10) * 4) & uVar3) == 0) {
        puStack_18[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar8 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar8;
        *puVar8 = puStack_18;
        *puStack_18 = uVar2;
        __vr3 = __vr2;
        _DAT_007140f4 = _DAT_007140e4;
        _DAT_007140f8 = _DAT_007140e8;
        _DAT_007140fc = _DAT_007140ec;
        __vr2 = __vr1;
        _DAT_007140e4 = _DAT_00714114;
        _DAT_007140e8 = _DAT_00714118;
        _DAT_007140ec = _DAT_0071411c;
        __vr1 = __vr0;
        _DAT_00714114 = _DAT_00714104;
        _DAT_00714118 = _DAT_00714108;
        _DAT_0071411c = _DAT_0071410c;
        piVar9 = (int *)((*piStack_14 >> 0x10) * 8 + iVar5);
        __vr0 = (int)(short)*piVar9;
        _DAT_00714108 = *(int *)((int)piVar9 + 2) >> 0x10;
        _DAT_00714104 = *piVar9 >> 0x10;
        iVar6 = (short)*__lmptr * __vr0 + _DAT_00714104 * (*__lmptr >> 0x10) +
                (*(int *)((int)__lmptr + 2) >> 0x10) * _DAT_00714108;
        iVar7 = iVar6 >> 0x1f;
        iVar6 = (uint)*(byte *)(piStack_14 + 1) *
                ((int)((iVar6 + iVar7 * -0x1000) - (uint)(iVar7 << 0xb < 0)) >> 0xc);
        iVar7 = iVar6 >> 0x1f;
        iVar6 = (int)((iVar6 + iVar7 * -0x1000) - (uint)(iVar7 << 0xb < 0)) >> 0xc;
        if (iVar6 < 1) {
          iVar6 = 0;
        }
        if (iVar6 + __bcrgb < 0xff) {
          iVar6 = (*(int *)((int)__lmptr + 2) >> 0x10) * _DAT_00714108 +
                  _DAT_00714104 * (*__lmptr >> 0x10) + (short)*__lmptr * __vr0;
          iVar7 = iVar6 >> 0x1f;
          iVar6 = (uint)*(byte *)(piStack_14 + 1) *
                  ((int)((iVar6 + iVar7 * -0x1000) - (uint)(iVar7 << 0xb < 0)) >> 0xc);
          iVar7 = iVar6 >> 0x1f;
          __rgb0 = (int)((iVar6 + iVar7 * -0x1000) - (uint)(iVar7 << 0xb < 0)) >> 0xc;
          if (__rgb0 < 1) {
            __rgb0 = 0;
          }
          __rgb0 = __rgb0 + __bcrgb;
        }
        else {
          __rgb0 = 0xff;
        }
        *(undefined1 *)(puStack_18 + 1) = _rgb0;
      }
    }
    puStack_18 = puStack_18 + 10;
    piStack_14 = piStack_14 + 5;
  }
  _gpoly = piStack_14;
  _gprim1 = puStack_18;
  return;
}


/* ===== FUN_0041b054 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_text_dpq(int param_1)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar3 = _DAT_0071bdc4;
  uVar2 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar4 = (*(int *)(iStack_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar4);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar4);
    _DAT_0071414c = *(undefined4 *)(iVar4 + 0x716dcc);
    iVar4 = (*(int *)(iStack_18 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar4);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar4);
    _DAT_0071415c = *(undefined4 *)(iVar4 + 0x716dcc);
    iVar5 = *(int *)(iStack_18 + 0xe) >> 0x10;
    iVar4 = iVar5 * 0x10;
    _DAT_00714120 = *(uint *)(&rot_points + iVar4);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar4);
    _DAT_0071412c = *(undefined4 *)(iVar4 + 0x716dcc);
    iVar7 = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - _DAT_00714154) * (_DAT_00714140 - _DAT_00714120);
    iVar4 = _DAT_00714148;
    __opz = iVar7;
    if ((*(uint *)(&rot_flags + iVar5 * 4) & uVar2) == 0) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[6] = _DAT_00714120 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar5 = *(int *)(iStack_18 + 0x10) >> 0x10;
      iVar8 = iVar5 * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar4 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      __opz = (_DAT_00714144 - _DAT_00714124) * (_DAT_00714140 - _DAT_00714150) -
              (_DAT_00714144 - _DAT_00714154) * (_DAT_00714140 - _DAT_00714120);
      if (((*(uint *)(&rot_flags + iVar5 * 4) & uVar2) == 0) &&
         ((_DAT_00714148 = iVar4 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148,
          iVar5 = _DAT_00714148 >> 0x1f,
          __otz = (int)((_DAT_00714148 + iVar5 * -4) - (uint)(iVar5 << 1 < 0)) >> 2, iVar7 < 1 ||
          (-1 < __opz)))) {
        _DAT_00714148 = iVar4;
        puStack_14[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar6 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar3);
        uVar1 = *puVar6;
        *puVar6 = puStack_14;
        *puStack_14 = uVar1;
        __rgb0 = (uint)*(byte *)(iStack_18 + 4);
        gte_dpcs();
        *(undefined1 *)(puStack_14 + 1) = _rgb0;
        iVar4 = _DAT_00714148;
      }
    }
    _DAT_00714148 = iVar4;
    puStack_14 = puStack_14 + 10;
    iStack_18 = iStack_18 + 0x14;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041b97c ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_text_dpq_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int *piVar8;
  int iVar9;
  int iStack_1c;
  int *piStack_18;
  undefined4 *puStack_14;
  
  iVar5 = _gnormals;
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  piStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar6 = (*(int *)((int)piStack_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar6);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071414c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = (piStack_18[3] >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)((int)piStack_18 + 0xe) >> 0x10;
    iVar9 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar9);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar9);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar9);
    _DAT_0071412c = *(undefined4 *)(iVar9 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - _DAT_00714154) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar9 = (piStack_18[4] >> 0x10) * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar9);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar9);
      iVar6 = *(int *)(&DAT_00716dc8 + iVar9);
      _DAT_0071414c = *(undefined4 *)(iVar9 + 0x716dcc);
      _DAT_00714148 = iVar6 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar9 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar9 * -4) - (uint)(iVar9 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar6;
      if ((*(uint *)(&rot_flags + (piStack_18[4] >> 0x10) * 4) & uVar3) == 0) {
        puStack_14[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar7 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar7;
        *puVar7 = puStack_14;
        *puStack_14 = uVar2;
        __rgb0 = (uint)*(byte *)(piStack_18 + 1);
        __vr3 = __vr2;
        _DAT_007140f4 = _DAT_007140e4;
        _DAT_007140f8 = _DAT_007140e8;
        _DAT_007140fc = _DAT_007140ec;
        __vr2 = __vr1;
        _DAT_007140e4 = _DAT_00714114;
        _DAT_007140e8 = _DAT_00714118;
        _DAT_007140ec = _DAT_0071411c;
        __vr1 = __vr0;
        _DAT_00714114 = _DAT_00714104;
        _DAT_00714118 = _DAT_00714108;
        _DAT_0071411c = _DAT_0071410c;
        piVar8 = (int *)((*piStack_18 >> 0x10) * 8 + iVar5);
        __vr0 = (int)(short)*piVar8;
        _DAT_00714104 = *piVar8 >> 0x10;
        _DAT_00714108 = *(int *)((int)piVar8 + 2) >> 0x10;
        gte_ncds();
        *(undefined1 *)(puStack_14 + 1) = _rgb0;
      }
    }
    puStack_14 = puStack_14 + 10;
    piStack_18 = piStack_18 + 5;
  }
  _gpoly = piStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041bcc4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_gour(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 0xe) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(iStack_18 + 0x10) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(iStack_18 + 0x12) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((uVar3 & *(uint *)(&rot_flags + iVar5 * 4)) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
    }
    puStack_14 = puStack_14 + 7;
    iStack_18 = iStack_18 + 0x18;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041be90 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_gour_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar5 * 4) & uVar3) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
    }
    puStack_14 = puStack_14 + 7;
    iStack_18 = iStack_18 + 0x14;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041c054 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl draw_face_3pt_gour_dpq(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int local_1c;
  int local_18;
  undefined4 *local_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  local_14 = _gprim1;
  local_18 = _gpoly;
  for (local_1c = 0; local_1c < param_1; local_1c = local_1c + 1) {
    iVar5 = (*(int *)(local_18 + 0xe) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(local_18 + 0x10) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(local_18 + 0x12) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((uVar3 & *(uint *)(&rot_flags + iVar5 * 4)) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      local_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      local_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      local_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = local_14;
      *local_14 = uVar2;
    }
    local_14 = local_14 + 7;
    local_18 = local_18 + 0x18;
  }
  _gpoly = local_18;
  _gprim1 = local_14;
  return;
}


/* ===== FUN_0041c220 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_gour_dpq_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar5 * 4) & uVar3) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
    }
    puStack_14 = puStack_14 + 7;
    iStack_18 = iStack_18 + 0x14;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041c49c ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_gour(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar6 = (*(int *)(iStack_18 + 0x12) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar6);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071414c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 0x14) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(iStack_18 + 0x16) >> 0x10;
    iVar7 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714144 - _DAT_00714124) * (_DAT_00714140 - _DAT_00714150) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar7 = *(int *)(iStack_18 + 0x18) >> 0x10;
      iVar8 = iVar7 * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar6 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar6 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar6;
      if ((*(uint *)(&rot_flags + iVar7 * 4) & uVar3) == 0) {
        puStack_14[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar5 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar5;
        *puVar5 = puStack_14;
        *puStack_14 = uVar2;
      }
    }
    puStack_14 = puStack_14 + 9;
    iStack_18 = iStack_18 + 0x1c;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041c6b0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_gour_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    iVar5 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar8 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar8);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071412c = *(undefined4 *)(iVar8 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - iVar5) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    _DAT_00714154 = iVar5;
    if (((uVar3 & *(uint *)(&rot_flags + iVar6 * 4)) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | iVar5 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      iVar6 = *(int *)(iStack_18 + 0xc) >> 0x10;
      iVar8 = iVar6 * 0x10;
      _DAT_00714138 = _DAT_00714148;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar5 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar5 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar5;
      if ((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) {
        puStack_14[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar7 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar7;
        *puVar7 = puStack_14;
        *puStack_14 = uVar2;
      }
    }
    puStack_14 = puStack_14 + 9;
    iStack_18 = iStack_18 + 0x18;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041c8c8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_gour_dpq(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 0x12) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 0x14) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    iVar5 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(iStack_18 + 0x16) >> 0x10;
    iVar8 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar8);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071412c = *(undefined4 *)(iVar8 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - iVar5) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    _DAT_00714154 = iVar5;
    if (((uVar3 & *(uint *)(&rot_flags + iVar6 * 4)) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | iVar5 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      iVar6 = *(int *)(iStack_18 + 0x18) >> 0x10;
      iVar8 = iVar6 * 0x10;
      _DAT_00714138 = _DAT_00714148;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar5 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar5 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar5;
      if ((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) {
        puStack_14[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar7 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar7;
        *puVar7 = puStack_14;
        *puStack_14 = uVar2;
      }
    }
    puStack_14 = puStack_14 + 9;
    iStack_18 = iStack_18 + 0x1c;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041cae0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_gour_dpq_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 6) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 8) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    iVar5 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(iStack_18 + 10) >> 0x10;
    iVar8 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar8);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071412c = *(undefined4 *)(iVar8 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - iVar5) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    _DAT_00714154 = iVar5;
    if (((uVar3 & *(uint *)(&rot_flags + iVar6 * 4)) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | iVar5 << 0x10;
      puStack_14[6] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      iVar6 = *(int *)(iStack_18 + 0xc) >> 0x10;
      iVar8 = iVar6 * 0x10;
      _DAT_00714138 = _DAT_00714148;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar5 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar5 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar5;
      if ((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) {
        puStack_14[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar7 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar7;
        *puVar7 = puStack_14;
        *puStack_14 = uVar2;
      }
    }
    puStack_14 = puStack_14 + 9;
    iStack_18 = iStack_18 + 0x18;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041ce8c ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_pict(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar5 = (*(int *)(iStack_18 + 0x12) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar5);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071414c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = (*(int *)(iStack_18 + 0x14) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(iStack_18 + 0x16) >> 0x10;
    iVar7 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((uVar3 & *(uint *)(&rot_flags + iVar5 * 4)) == 0) &&
       (iVar5 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar7 = iVar5 >> 0x1f,
       __otz = (int)((iVar5 + iVar7 * -0x40) - (uint)(iVar7 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[5] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[8] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
    }
    puStack_14 = puStack_14 + 10;
    iStack_18 = iStack_18 + 0x1c;
                    /* END-> C:\PCMPE\graphics\sprite.C: ? */
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041d058 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_pict_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int *piVar7;
  int iVar8;
  int iVar9;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar5 = _gnormals;
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar8 = (*(int *)(iStack_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar8);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
    iVar8 = (*(int *)(iStack_18 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar8);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071415c = *(undefined4 *)(iVar8 + 0x716dcc);
    iVar8 = *(int *)(iStack_18 + 0xe) >> 0x10;
    iVar9 = iVar8 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar9);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar9);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar9);
    _DAT_0071412c = *(undefined4 *)(iVar9 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar8 * 4) & uVar3) == 0) &&
       (iVar8 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar9 = iVar8 >> 0x1f,
       __otz = (int)((iVar8 + iVar9 * -0x40) - (uint)(iVar9 << 5 < 0)) >> 6, __opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[5] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[8] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = puStack_14;
      *puStack_14 = uVar2;
      __vr3 = __vr2;
      _DAT_007140f4 = _DAT_007140e4;
      _DAT_007140f8 = _DAT_007140e8;
      _DAT_007140fc = _DAT_007140ec;
      __vr2 = __vr1;
      _DAT_007140e4 = _DAT_00714114;
      _DAT_007140e8 = _DAT_00714118;
      _DAT_007140ec = _DAT_0071411c;
      __vr1 = __vr0;
      _DAT_00714114 = _DAT_00714104;
      _DAT_00714118 = _DAT_00714108;
      _DAT_0071411c = _DAT_0071410c;
      piVar7 = (int *)((*(int *)(iStack_18 + 0x10) >> 0x10) * 8 + iVar5);
      __vr0 = (int)(short)*piVar7;
      _DAT_00714108 = *(int *)((int)piVar7 + 2) >> 0x10;
      _DAT_00714104 = *piVar7 >> 0x10;
      iVar8 = (short)*__lmptr * __vr0 + (*__lmptr >> 0x10) * _DAT_00714104 +
              (*(int *)((int)__lmptr + 2) >> 0x10) * _DAT_00714108;
      iVar9 = iVar8 >> 0x1f;
      iVar8 = (uint)*(byte *)(iStack_18 + 4) *
              ((int)((iVar8 + iVar9 * -0x1000) - (uint)(iVar9 << 0xb < 0)) >> 0xc);
      iVar9 = iVar8 >> 0x1f;
      iVar8 = (int)((iVar8 + iVar9 * -0x1000) - (uint)(iVar9 << 0xb < 0)) >> 0xc;
      if (iVar8 < 1) {
        iVar8 = 0;
      }
      if (iVar8 + __bcrgb < 0xff) {
        iVar8 = (*__lmptr >> 0x10) * _DAT_00714104 + (short)*__lmptr * __vr0 +
                (*(int *)((int)__lmptr + 2) >> 0x10) * _DAT_00714108;
        iVar9 = iVar8 >> 0x1f;
        iVar8 = (uint)*(byte *)(iStack_18 + 4) *
                ((int)((iVar8 + iVar9 * -0x1000) - (uint)(iVar9 << 0xb < 0)) >> 0xc);
        iVar9 = iVar8 >> 0x1f;
        __rgb0 = (int)((iVar8 + iVar9 * -0x1000) - (uint)(iVar9 << 0xb < 0)) >> 0xc;
        if (__rgb0 < 1) {
          __rgb0 = 0;
        }
        __rgb0 = __rgb0 + __bcrgb;
      }
      else {
        __rgb0 = 0xff;
      }
      *(undefined1 *)(puStack_14 + 1) = _rgb0;
    }
    puStack_14 = puStack_14 + 10;
    iStack_18 = iStack_18 + 0x18;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041d360 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl draw_face_3pt_pict_dpq(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  int iVar8;
  undefined4 *puVar9;
  int iVar10;
  int local_20;
  undefined4 *local_18;
  int local_14;
  
  iVar5 = _DAT_0071bdc4;
  iVar4 = _DAT_0071bdc0;
  uVar3 = DAT_00462d90;
  local_18 = _gprim1;
  local_14 = _gpoly;
  for (local_20 = 0; local_20 < param_1; local_20 = local_20 + 1) {
    piVar6 = (int *)((*(int *)(local_14 + 0x12) >> 0x10) * 8 + iVar4);
    __vr0 = (int)(short)*piVar6;
    _DAT_00714108 = *(int *)((int)piVar6 + 2) >> 0x10;
    _DAT_00714104 = *piVar6 >> 0x10;
    piVar6 = (int *)((*(int *)(local_14 + 0x14) >> 0x10) * 8 + iVar4);
    __vr1 = (int)(short)*piVar6;
    _DAT_00714118 = *(int *)((int)piVar6 + 2) >> 0x10;
    _DAT_00714114 = *piVar6 >> 0x10;
    piVar6 = (int *)((*(int *)(local_14 + 0x16) >> 0x10) * 8 + iVar4);
    __vr2 = (int)(short)*piVar6;
    _DAT_007140e4 = *piVar6 >> 0x10;
    _DAT_007140e8 = *(int *)((int)piVar6 + 2) >> 0x10;
    GTERPT();
    _DAT_00714130 = _DAT_00714140;
    _DAT_00714134 = _DAT_00714144;
    _DAT_00714138 = _DAT_00714148;
    _DAT_0071413c = _DAT_0071414c;
    iVar7 = (*(int *)(local_14 + 0x12) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar7);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071414c = *(undefined4 *)(iVar7 + 0x716dcc);
    iVar8 = (*(int *)(local_14 + 0x14) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar8);
    iVar7 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071415c = *(undefined4 *)(iVar8 + 0x716dcc);
    iVar8 = *(int *)(local_14 + 0x16) >> 0x10;
    iVar10 = iVar8 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar10);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar10);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar10);
    _DAT_0071412c = *(undefined4 *)(iVar10 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - iVar7) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    _DAT_00714154 = iVar7;
    if (((*(uint *)(&rot_flags + iVar8 * 4) & uVar3) == 0) &&
       (iVar8 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar10 = iVar8 >> 0x1f,
       __otz = (int)((iVar8 + iVar10 * -0x40) - (uint)(iVar10 << 5 < 0)) >> 6, __opz < 1)) {
      local_18[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      local_18[5] = _DAT_00714150 & 0xffff | iVar7 << 0x10;
      local_18[8] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar9 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar5);
      uVar2 = *puVar9;
      *puVar9 = local_18;
      *local_18 = uVar2;
      __rgb0 = (uint)*(byte *)(local_14 + 4);
      gte_dpcs();
      *(undefined1 *)(local_18 + 1) = _rgb0;
    }
    local_18 = local_18 + 10;
    local_14 = local_14 + 0x1c;
  }
  _gpoly = local_14;
  _gprim1 = local_18;
  return;
}


/* ===== FUN_0041d5e8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl draw_face_3pt_pict_dpq_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int *piVar7;
  int iVar8;
  int iVar9;
  int local_1c;
  undefined4 *local_18;
  int local_14;
  
  iVar5 = _gnormals;
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  local_18 = _gprim1;
  local_14 = _gpoly;
  for (local_1c = 0; local_1c < param_1; local_1c = local_1c + 1) {
    iVar8 = (*(int *)(local_14 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar8);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
    iVar8 = (*(int *)(local_14 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar8);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071415c = *(undefined4 *)(iVar8 + 0x716dcc);
    iVar8 = *(int *)(local_14 + 0xe) >> 0x10;
    iVar9 = iVar8 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar9);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar9);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar9);
    _DAT_0071412c = *(undefined4 *)(iVar9 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((uVar3 & *(uint *)(&rot_flags + iVar8 * 4)) == 0) &&
       (iVar8 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15, iVar9 = iVar8 >> 0x1f,
       __otz = (int)((iVar8 + iVar9 * -0x40) - (uint)(iVar9 << 5 < 0)) >> 6, __opz < 1)) {
      local_18[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      local_18[5] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      local_18[8] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      puVar6 = (undefined4 *)
               (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) * 4
               + iVar4);
      uVar2 = *puVar6;
      *puVar6 = local_18;
      *local_18 = uVar2;
      __rgb0 = (uint)*(byte *)(local_14 + 4);
      __vr3 = __vr2;
      _DAT_007140f4 = _DAT_007140e4;
      _DAT_007140f8 = _DAT_007140e8;
      _DAT_007140fc = _DAT_007140ec;
      __vr2 = __vr1;
      _DAT_007140e4 = _DAT_00714114;
      _DAT_007140e8 = _DAT_00714118;
      _DAT_007140ec = _DAT_0071411c;
      __vr1 = __vr0;
      _DAT_00714114 = _DAT_00714104;
      _DAT_00714118 = _DAT_00714108;
      _DAT_0071411c = _DAT_0071410c;
      piVar7 = (int *)((*(int *)(local_14 + 0x10) >> 0x10) * 8 + iVar5);
      __vr0 = (int)(short)*piVar7;
      _DAT_00714104 = *piVar7 >> 0x10;
      _DAT_00714108 = *(int *)((int)piVar7 + 2) >> 0x10;
      gte_ncds();
      *(undefined1 *)(local_18 + 1) = _rgb0;
    }
    local_18 = local_18 + 10;
    local_14 = local_14 + 0x18;
  }
  _gpoly = local_14;
  _gprim1 = local_18;
  return;
}


/* ===== FUN_0041d9e0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_pict(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar6 = (*(int *)(iStack_18 + 0x16) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar6);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071414c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = (*(int *)(iStack_18 + 0x18) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(iStack_18 + 0x1a) >> 0x10;
    iVar7 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar7);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071412c = *(undefined4 *)(iVar7 + 0x716dcc);
    __opz = (_DAT_00714144 - _DAT_00714124) * (_DAT_00714140 - _DAT_00714150) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - _DAT_00714154);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) && (__opz < 1)) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[5] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[8] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar7 = *(int *)(iStack_18 + 0x1c) >> 0x10;
      iVar8 = iVar7 * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar6 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar6 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar6;
      if ((*(uint *)(&rot_flags + iVar7 * 4) & uVar3) == 0) {
        puStack_14[0xb] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar5 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar5;
        *puVar5 = puStack_14;
        *puStack_14 = uVar2;
      }
    }
    puStack_14 = puStack_14 + 0xd;
    iStack_18 = iStack_18 + 0x20;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_0041dbf4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_pict_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  int *piVar9;
  int iVar10;
  int iStack_1c;
  undefined4 *puStack_18;
  int iStack_14;
  
  iVar5 = _gnormals;
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  puStack_18 = _gprim1;
  iStack_14 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar6 = (*(int *)(iStack_14 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar6);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071414c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar7 = (*(int *)(iStack_14 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar7);
    iVar6 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071415c = *(undefined4 *)(iVar7 + 0x716dcc);
    iVar7 = *(int *)(iStack_14 + 0xe) >> 0x10;
    iVar10 = iVar7 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar10);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar10);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar10);
    _DAT_0071412c = *(undefined4 *)(iVar10 + 0x716dcc);
    __opz = (_DAT_00714144 - _DAT_00714124) * (_DAT_00714140 - _DAT_00714150) -
            (_DAT_00714140 - uVar1) * (_DAT_00714144 - iVar6);
    _DAT_00714120 = uVar1;
    _DAT_00714154 = iVar6;
    if (((*(uint *)(&rot_flags + iVar7 * 4) & uVar3) == 0) && (__opz < 1)) {
      puStack_18[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_18[5] = _DAT_00714150 & 0xffff | iVar6 << 0x10;
      puStack_18[8] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar7 = *(int *)(iStack_14 + 0x10) >> 0x10;
      iVar10 = iVar7 * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar10);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar10);
      iVar6 = *(int *)(&DAT_00716dc8 + iVar10);
      _DAT_0071414c = *(undefined4 *)(iVar10 + 0x716dcc);
      _DAT_00714148 = iVar6 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar10 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar10 * -4) - (uint)(iVar10 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar6;
      if ((*(uint *)(&rot_flags + iVar7 * 4) & uVar3) == 0) {
        puStack_18[0xb] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar8 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar8;
        *puVar8 = puStack_18;
        *puStack_18 = uVar2;
        __vr3 = __vr2;
        _DAT_007140f4 = _DAT_007140e4;
        _DAT_007140f8 = _DAT_007140e8;
        _DAT_007140fc = _DAT_007140ec;
        __vr2 = __vr1;
        _DAT_007140e4 = _DAT_00714114;
        _DAT_007140e8 = _DAT_00714118;
        _DAT_007140ec = _DAT_0071411c;
        __vr1 = __vr0;
        _DAT_00714114 = _DAT_00714104;
        _DAT_00714118 = _DAT_00714108;
        _DAT_0071411c = _DAT_0071410c;
        piVar9 = (int *)((*(int *)(iStack_14 + 0x12) >> 0x10) * 8 + iVar5);
        __vr0 = (int)(short)*piVar9;
        _DAT_00714108 = *(int *)((int)piVar9 + 2) >> 0x10;
        _DAT_00714104 = *piVar9 >> 0x10;
        iVar6 = (short)*__lmptr * __vr0 + _DAT_00714104 * (*__lmptr >> 0x10) +
                (*(int *)((int)__lmptr + 2) >> 0x10) * _DAT_00714108;
        iVar7 = iVar6 >> 0x1f;
        iVar6 = (uint)*(byte *)(iStack_14 + 4) *
                ((int)((iVar6 + iVar7 * -0x1000) - (uint)(iVar7 << 0xb < 0)) >> 0xc);
        iVar7 = iVar6 >> 0x1f;
        iVar6 = (int)((iVar6 + iVar7 * -0x1000) - (uint)(iVar7 << 0xb < 0)) >> 0xc;
        if (iVar6 < 1) {
          iVar6 = 0;
        }
        if (iVar6 + __bcrgb < 0xff) {
          iVar6 = (*(int *)((int)__lmptr + 2) >> 0x10) * _DAT_00714108 +
                  _DAT_00714104 * (*__lmptr >> 0x10) + (short)*__lmptr * __vr0;
          iVar7 = iVar6 >> 0x1f;
          iVar6 = (uint)*(byte *)(iStack_14 + 4) *
                  ((int)((iVar6 + iVar7 * -0x1000) - (uint)(iVar7 << 0xb < 0)) >> 0xc);
          iVar7 = iVar6 >> 0x1f;
          __rgb0 = (int)((iVar6 + iVar7 * -0x1000) - (uint)(iVar7 << 0xb < 0)) >> 0xc;
          if (__rgb0 < 1) {
            __rgb0 = 0;
          }
          __rgb0 = __rgb0 + __bcrgb;
        }
        else {
          __rgb0 = 0xff;
        }
        *(undefined1 *)(puStack_18 + 1) = _rgb0;
      }
    }
    puStack_18 = puStack_18 + 0xd;
    iStack_14 = iStack_14 + 0x1c;
  }
  _gpoly = iStack_14;
  _gprim1 = puStack_18;
  return;
}


/* ===== FUN_0041df54 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl draw_face_4pt_pict_dpq(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iVar8;
  int local_1c;
  int local_18;
  undefined4 *local_14;
  
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  local_14 = _gprim1;
  local_18 = _gpoly;
  for (local_1c = 0; local_1c < param_1; local_1c = local_1c + 1) {
    iVar7 = (*(int *)(local_18 + 0x16) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar7);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar7);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar7);
    _DAT_0071414c = *(undefined4 *)(iVar7 + 0x716dcc);
    iVar5 = (*(int *)(local_18 + 0x18) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar5);
    iVar7 = *(int *)(&DAT_00716dc4 + iVar5);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar5);
    _DAT_0071415c = *(undefined4 *)(iVar5 + 0x716dcc);
    iVar5 = *(int *)(local_18 + 0x1a) >> 0x10;
    iVar8 = iVar5 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar8);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar8);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar8);
    _DAT_0071412c = *(undefined4 *)(iVar8 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - iVar7) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    _DAT_00714154 = iVar7;
    if (((uVar3 & *(uint *)(&rot_flags + iVar5 * 4)) == 0) && (__opz < 1)) {
      local_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      local_14[5] = _DAT_00714150 & 0xffff | iVar7 << 0x10;
      local_14[8] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar5 = *(int *)(local_18 + 0x1c) >> 0x10;
      iVar8 = iVar5 * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar8);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar8);
      iVar7 = *(int *)(&DAT_00716dc8 + iVar8);
      _DAT_0071414c = *(undefined4 *)(iVar8 + 0x716dcc);
      _DAT_00714148 = iVar7 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar8 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar7;
      if ((*(uint *)(&rot_flags + iVar5 * 4) & uVar3) == 0) {
        local_14[0xb] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar6 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar6;
        *puVar6 = local_14;
        *local_14 = uVar2;
        __rgb0 = (uint)*(byte *)(local_18 + 4);
        gte_dpcs();
        *(undefined1 *)(local_14 + 1) = _rgb0;
      }
    }
    local_14 = local_14 + 0xd;
    local_18 = local_18 + 0x20;
  }
  _gpoly = local_18;
  _gprim1 = local_14;
  return;
}


/* ===== FUN_0041e184 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl draw_face_4pt_pict_dpq_lit(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int *piVar8;
  int iVar9;
  int iVar10;
  int local_1c;
  int local_18;
  undefined4 *local_14;
  
  iVar5 = _gnormals;
  iVar4 = _DAT_0071bdc4;
  uVar3 = DAT_00462d90;
  local_14 = _gprim1;
  local_18 = _gpoly;
  for (local_1c = 0; local_1c < param_1; local_1c = local_1c + 1) {
    iVar6 = (*(int *)(local_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar6);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071414c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = (*(int *)(local_18 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar6);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar6);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar6);
    _DAT_0071415c = *(undefined4 *)(iVar6 + 0x716dcc);
    iVar6 = *(int *)(local_18 + 0xe) >> 0x10;
    iVar9 = iVar6 * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar9);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar9);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar9);
    _DAT_0071412c = *(undefined4 *)(iVar9 + 0x716dcc);
    __opz = (_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
            (_DAT_00714144 - _DAT_00714154) * (_DAT_00714140 - uVar1);
    _DAT_00714120 = uVar1;
    if (((*(uint *)(&rot_flags + iVar6 * 4) & uVar3) == 0) && (__opz < 1)) {
      local_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      local_14[5] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      local_14[8] = uVar1 & 0xffff | _DAT_00714124 << 0x10;
      _DAT_00714138 = _DAT_00714148;
      iVar9 = *(int *)(local_18 + 0x10) >> 0x10;
      iVar10 = iVar9 * 0x10;
      _DAT_00714140 = *(uint *)(&rot_points + iVar10);
      _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar10);
      iVar6 = *(int *)(&DAT_00716dc8 + iVar10);
      _DAT_0071414c = *(undefined4 *)(iVar10 + 0x716dcc);
      _DAT_00714148 = iVar6 + _DAT_00714158 + _DAT_00714128 + _DAT_00714148;
      iVar10 = _DAT_00714148 >> 0x1f;
      __otz = (int)((_DAT_00714148 + iVar10 * -4) - (uint)(iVar10 << 1 < 0)) >> 2;
      _DAT_00714148 = iVar6;
      if ((*(uint *)(&rot_flags + iVar9 * 4) & uVar3) == 0) {
        local_14[0xb] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar7 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar2 = *puVar7;
        *puVar7 = local_14;
        *local_14 = uVar2;
        __rgb0 = (uint)*(byte *)(local_18 + 4);
        __vr3 = __vr2;
        _DAT_007140f4 = _DAT_007140e4;
        _DAT_007140f8 = _DAT_007140e8;
        _DAT_007140fc = _DAT_007140ec;
        __vr2 = __vr1;
        _DAT_007140e4 = _DAT_00714114;
        _DAT_007140e8 = _DAT_00714118;
        _DAT_007140ec = _DAT_0071411c;
        __vr1 = __vr0;
        _DAT_00714114 = _DAT_00714104;
        _DAT_00714118 = _DAT_00714108;
        _DAT_0071411c = _DAT_0071410c;
        piVar8 = (int *)((*(int *)(local_18 + 0x12) >> 0x10) * 8 + iVar5);
        __vr0 = (int)(short)*piVar8;
        _DAT_00714104 = *piVar8 >> 0x10;
        _DAT_00714108 = *(int *)((int)piVar8 + 2) >> 0x10;
        gte_ncds();
        *(undefined1 *)(local_14 + 1) = _rgb0;
      }
    }
    local_14 = local_14 + 0xd;
    local_18 = local_18 + 0x1c;
  }
  _gpoly = local_18;
  _gprim1 = local_14;
  return;
}


