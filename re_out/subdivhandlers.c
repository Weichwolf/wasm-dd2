/* ===== FUN_004191ac ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_3pt_text_squash(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  short sStack_2c;
  short sStack_28;
  short sStack_24;
  int iStack_1c;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar2 = _DAT_0071bdc4;
  puStack_14 = _gprim1;
  iStack_18 = _gpoly;
  for (iStack_1c = 0; iStack_1c < param_1; iStack_1c = iStack_1c + 1) {
    iVar3 = (*(int *)(iStack_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar3);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar3);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar3);
    _DAT_0071414c = *(undefined4 *)(iVar3 + 0x716dcc);
    iVar3 = (*(int *)(iStack_18 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar3);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar3);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar3);
    _DAT_0071415c = *(undefined4 *)(iVar3 + 0x716dcc);
    iVar3 = (*(int *)(iStack_18 + 0xe) >> 0x10) * 0x10;
    _DAT_00714120 = *(uint *)(&rot_points + iVar3);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar3);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar3);
    _DAT_0071412c = *(undefined4 *)(iVar3 + 0x716dcc);
    iVar3 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15;
    iVar6 = iVar3 >> 0x1f;
    iVar3 = (iVar3 + iVar6 * -0x40) - (uint)(iVar6 << 5 < 0);
    __otz = iVar3 >> 6;
    if ((int)((_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
             (_DAT_00714140 - _DAT_00714120) * (_DAT_00714144 - _DAT_00714154)) < 1) {
      iVar6 = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
      sStack_2c = (short)iVar6;
      iVar7 = (_DAT_00714150 & 0xffff) + _DAT_00714154 * 0x10000;
      sStack_28 = (short)iVar7;
      iVar8 = (_DAT_00714120 & 0xffff) + _DAT_00714124 * 0x10000;
      sStack_24 = (short)iVar8;
      iVar3 = iVar3 >> 0x1f;
      iVar3 = (int)((__otz + iVar3 * -4) - (uint)(iVar3 << 1 < 0)) >> 2;
      if (0 < iVar3) {
        if (sStack_2c < 0x294) {
          if (sStack_2c < -0x153) {
            iVar6 = CONCAT22(0x78 - (short)((((iVar6 >> 0x10) + -0x78) * 500) / (sStack_2c + -0xa0))
                             ,0xfeac);
          }
        }
        else {
          iVar6 = CONCAT22((short)((((iVar6 >> 0x10) + -0x78) * 500) / (sStack_2c + -0xa0)) + 0x78,
                           0x294);
        }
        sStack_2c = (short)iVar6;
        iVar5 = iVar6 >> 0x10;
        if (iVar6 < 0x1720000) {
          if (iVar5 < -0x81) {
            iVar6 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_2c + -0xa0) * 0xfa) / (iVar5 + -0x78)));
          }
        }
        else {
          iVar6 = CONCAT22(0x172,(short)(((sStack_2c + -0xa0) * 0xfa) / (iVar5 + -0x78)) + 0xa0);
        }
        if (sStack_28 < 0x294) {
          if (sStack_28 < -0x153) {
            iVar7 = CONCAT22(0x78 - (short)((((iVar7 >> 0x10) + -0x78) * 500) / (sStack_28 + -0xa0))
                             ,0xfeac);
          }
        }
        else {
          iVar7 = CONCAT22((short)((((iVar7 >> 0x10) + -0x78) * 500) / (sStack_28 + -0xa0)) + 0x78,
                           0x294);
        }
        sStack_28 = (short)iVar7;
        iVar5 = iVar7 >> 0x10;
        if (iVar7 < 0x1720000) {
          if (iVar5 < -0x81) {
            iVar7 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_28 + -0xa0) * 0xfa) / (iVar5 + -0x78)));
          }
        }
        else {
          iVar7 = CONCAT22(0x172,(short)(((sStack_28 + -0xa0) * 0xfa) / (iVar5 + -0x78)) + 0xa0);
        }
        if (sStack_24 < 0x294) {
          if (sStack_24 < -0x153) {
            iVar8 = CONCAT22(0x78 - (short)((((iVar8 >> 0x10) + -0x78) * 500) / (sStack_24 + -0xa0))
                             ,0xfeac);
          }
        }
        else {
          iVar8 = CONCAT22((short)((((iVar8 >> 0x10) + -0x78) * 500) / (sStack_24 + -0xa0)) + 0x78,
                           0x294);
        }
        sStack_24 = (short)iVar8;
        iVar5 = iVar8 >> 0x10;
        if (iVar8 < 0x1720000) {
          if (iVar5 < -0x81) {
            iVar8 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_24 + -0xa0) * 0xfa) / (iVar5 + -0x78)));
          }
        }
        else {
          iVar8 = CONCAT22(0x172,(short)(((sStack_24 + -0xa0) * 0xfa) / (iVar5 + -0x78)) + 0xa0);
        }
        puStack_14[2] = iVar6;
        puStack_14[4] = iVar7;
        puStack_14[6] = iVar8;
        puVar4 = (undefined4 *)(iVar3 * 4 + iVar2);
        uVar1 = *puVar4;
        *puVar4 = puStack_14;
        *puStack_14 = uVar1;
      }
    }
    puStack_14 = puStack_14 + 8;
    iStack_18 = iStack_18 + 0x14;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


/* ===== FUN_00419b94 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl draw_face_3pt_text_dpq_squash(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  short local_2c;
  short local_28;
  short local_24;
  int local_1c;
  int local_18;
  undefined4 *local_14;
  
  iVar2 = _DAT_0071bdc4;
  local_14 = _gprim1;
  local_18 = _gpoly;
  for (local_1c = 0; local_1c < param_1; local_1c = local_1c + 1) {
    iVar3 = (*(int *)(local_18 + 10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar3);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar3);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar3);
    _DAT_0071414c = *(undefined4 *)(iVar3 + 0x716dcc);
    iVar3 = (*(int *)(local_18 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar3);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar3);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar3);
    _DAT_0071415c = *(undefined4 *)(iVar3 + 0x716dcc);
    iVar3 = (*(int *)(local_18 + 0xe) >> 0x10) * 0x10;
    _DAT_00714120 = *(uint *)(&rot_points + iVar3);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar3);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar3);
    _DAT_0071412c = *(undefined4 *)(iVar3 + 0x716dcc);
    iVar3 = (_DAT_00714128 + _DAT_00714148 + _DAT_00714158) * 0x15;
    iVar6 = iVar3 >> 0x1f;
    iVar3 = (iVar3 + iVar6 * -0x40) - (uint)(iVar6 << 5 < 0);
    __otz = iVar3 >> 6;
    if ((int)((_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
             (_DAT_00714140 - _DAT_00714120) * (_DAT_00714144 - _DAT_00714154)) < 1) {
      iVar6 = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
      local_2c = (short)iVar6;
      iVar7 = (_DAT_00714150 & 0xffff) + _DAT_00714154 * 0x10000;
      local_28 = (short)iVar7;
      iVar8 = (_DAT_00714120 & 0xffff) + _DAT_00714124 * 0x10000;
      local_24 = (short)iVar8;
      iVar3 = iVar3 >> 0x1f;
      iVar3 = (int)((__otz + iVar3 * -4) - (uint)(iVar3 << 1 < 0)) >> 2;
      if (0 < iVar3) {
        if (local_2c < 0x294) {
          if (local_2c < -0x153) {
                    /* END-> C:\PCMPE\movie\movie.C: ? */
            iVar6 = CONCAT22(0x78 - (short)((((iVar6 >> 0x10) + -0x78) * 500) / (local_2c + -0xa0)),
                             0xfeac);
          }
        }
        else {
          iVar6 = CONCAT22((short)((((iVar6 >> 0x10) + -0x78) * 500) / (local_2c + -0xa0)) + 0x78,
                           0x294);
        }
        local_2c = (short)iVar6;
        iVar5 = iVar6 >> 0x10;
        if (iVar6 < 0x1720000) {
          if (iVar5 < -0x81) {
            iVar6 = CONCAT22(0xff7e,0xa0 - (short)(((local_2c + -0xa0) * 0xfa) / (iVar5 + -0x78)));
          }
        }
        else {
          iVar6 = CONCAT22(0x172,(short)(((local_2c + -0xa0) * 0xfa) / (iVar5 + -0x78)) + 0xa0);
        }
        if (local_28 < 0x294) {
          if (local_28 < -0x153) {
            iVar7 = CONCAT22(0x78 - (short)((((iVar7 >> 0x10) + -0x78) * 500) / (local_28 + -0xa0)),
                             0xfeac);
          }
        }
        else {
          iVar7 = CONCAT22((short)((((iVar7 >> 0x10) + -0x78) * 500) / (local_28 + -0xa0)) + 0x78,
                           0x294);
        }
        local_28 = (short)iVar7;
        iVar5 = iVar7 >> 0x10;
        if (iVar7 < 0x1720000) {
          if (iVar5 < -0x81) {
            iVar7 = CONCAT22(0xff7e,0xa0 - (short)(((local_28 + -0xa0) * 0xfa) / (iVar5 + -0x78)));
          }
        }
        else {
          iVar7 = CONCAT22(0x172,(short)(((local_28 + -0xa0) * 0xfa) / (iVar5 + -0x78)) + 0xa0);
        }
        if (local_24 < 0x294) {
          if (local_24 < -0x153) {
            iVar8 = CONCAT22(0x78 - (short)((((iVar8 >> 0x10) + -0x78) * 500) / (local_24 + -0xa0)),
                             0xfeac);
          }
        }
        else {
          iVar8 = CONCAT22((short)((((iVar8 >> 0x10) + -0x78) * 500) / (local_24 + -0xa0)) + 0x78,
                           0x294);
        }
        local_24 = (short)iVar8;
        iVar5 = iVar8 >> 0x10;
        if (iVar8 < 0x1720000) {
          if (iVar5 < -0x81) {
            iVar8 = CONCAT22(0xff7e,0xa0 - (short)(((local_24 + -0xa0) * 0xfa) / (iVar5 + -0x78)));
          }
        }
        else {
          iVar8 = CONCAT22(0x172,(short)(((local_24 + -0xa0) * 0xfa) / (iVar5 + -0x78)) + 0xa0);
        }
        local_14[2] = iVar6;
        local_14[4] = iVar7;
        local_14[6] = iVar8;
        puVar4 = (undefined4 *)(iVar3 * 4 + iVar2);
        uVar1 = *puVar4;
        *puVar4 = local_14;
        *local_14 = uVar1;
        __rgb0 = (uint)*(byte *)(local_18 + 4);
        gte_dpcs();
        *(undefined1 *)(local_14 + 1) = _rgb0;
      }
    }
    local_14 = local_14 + 8;
    local_18 = local_18 + 0x14;
  }
  _gpoly = local_18;
  _gprim1 = local_14;
  return;
}


/* ===== FUN_0041a66c ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_text_squash(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  short sStack_2c;
  short sStack_28;
  int iStack_20;
  short sStack_18;
  short sStack_14;
  
  iVar3 = _DAT_0071bdc4;
  iStack_20 = _gpoly;
  for (iVar11 = 0; iVar11 < param_1; iVar11 = iVar11 + 1) {
    iVar4 = (*(int *)(iStack_20 + 10) >> 0x10) * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar4);
    iVar10 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714138 = *(int *)(&DAT_00716dc8 + iVar4);
    iVar4 = (*(int *)(iStack_20 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar4);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar4);
    _DAT_0071415c = *(undefined4 *)(iVar4 + 0x716dcc);
    iVar4 = (*(int *)(iStack_20 + 0xe) >> 0x10) * 0x10;
    _DAT_00714120 = *(uint *)(&rot_points + iVar4);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar4);
    _DAT_0071412c = *(undefined4 *)(iVar4 + 0x716dcc);
    iVar6 = (uVar1 & 0xffff) + iVar10 * 0x10000;
    sStack_28 = (short)iVar6;
    iVar7 = (_DAT_00714150 & 0xffff) + _DAT_00714154 * 0x10000;
    sStack_2c = (short)iVar7;
    iVar8 = (_DAT_00714120 & 0xffff) + _DAT_00714124 * 0x10000;
    iVar4 = (*(int *)(iStack_20 + 0x10) >> 0x10) * 0x10;
    sStack_18 = (short)iVar8;
    _DAT_00714140 = *(uint *)(&rot_points + iVar4);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar4);
    _DAT_0071414c = *(undefined4 *)(iVar4 + 0x716dcc);
    iVar4 = _DAT_00714148 + _DAT_00714158 + _DAT_00714128 + _DAT_00714138;
    iVar9 = iVar4 >> 0x1f;
    iVar4 = (iVar4 + iVar9 * -4) - (uint)(iVar9 << 1 < 0);
    __otz = iVar4 >> 2;
    if (((int)((uVar1 - _DAT_00714150) * (iVar10 - _DAT_00714124) -
              (uVar1 - _DAT_00714120) * (iVar10 - _DAT_00714154)) < 1) ||
       (-1 < (int)((_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
                  (_DAT_00714144 - _DAT_00714154) * (_DAT_00714140 - _DAT_00714120)))) {
      iVar10 = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
      sStack_14 = (short)iVar10;
      iVar4 = iVar4 >> 0x1f;
      iVar4 = (int)((__otz + iVar4 * -4) - (uint)(iVar4 << 1 < 0)) >> 2;
      if (0 < iVar4) {
        if (sStack_28 < 0x294) {
          if (sStack_28 < -0x153) {
            iVar6 = CONCAT22(0x78 - (short)((((iVar6 >> 0x10) + -0x78) * 500) / (sStack_28 + -0xa0))
                             ,0xfeac);
          }
        }
        else {
          iVar6 = CONCAT22((short)((((iVar6 >> 0x10) + -0x78) * 500) / (sStack_28 + -0xa0)) + 0x78,
                           0x294);
        }
        sStack_28 = (short)iVar6;
        iVar9 = iVar6 >> 0x10;
        if (iVar6 < 0x1720000) {
          if (iVar9 < -0x81) {
            iVar6 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_28 + -0xa0) * 0xfa) / (iVar9 + -0x78)));
          }
        }
        else {
          iVar6 = CONCAT22(0x172,(short)(((sStack_28 + -0xa0) * 0xfa) / (iVar9 + -0x78)) + 0xa0);
        }
        if (sStack_2c < 0x294) {
          if (sStack_2c < -0x153) {
            iVar7 = CONCAT22(0x78 - (short)((((iVar7 >> 0x10) + -0x78) * 500) / (sStack_2c + -0xa0))
                             ,0xfeac);
          }
        }
        else {
          iVar7 = CONCAT22((short)((((iVar7 >> 0x10) + -0x78) * 500) / (sStack_2c + -0xa0)) + 0x78,
                           0x294);
        }
        sStack_2c = (short)iVar7;
        iVar9 = iVar7 >> 0x10;
        if (iVar7 < 0x1720000) {
          if (iVar9 < -0x81) {
            iVar7 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_2c + -0xa0) * 0xfa) / (iVar9 + -0x78)));
          }
        }
        else {
          iVar7 = CONCAT22(0x172,(short)(((sStack_2c + -0xa0) * 0xfa) / (iVar9 + -0x78)) + 0xa0);
        }
        if (sStack_18 < 0x294) {
          if (sStack_18 < -0x153) {
            iVar8 = CONCAT22(0x78 - (short)((((iVar8 >> 0x10) + -0x78) * 500) / (sStack_18 + -0xa0))
                             ,0xfeac);
          }
        }
        else {
          iVar8 = CONCAT22((short)((((iVar8 >> 0x10) + -0x78) * 500) / (sStack_18 + -0xa0)) + 0x78,
                           0x294);
        }
        sStack_18 = (short)iVar8;
        iVar9 = iVar8 >> 0x10;
        if (iVar8 < 0x1720000) {
          if (iVar9 < -0x81) {
            iVar8 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_18 + -0xa0) * 0xfa) / (iVar9 + -0x78)));
          }
        }
        else {
          iVar8 = CONCAT22(0x172,(short)(((sStack_18 + -0xa0) * 0xfa) / (iVar9 + -0x78)) + 0xa0);
        }
        if (sStack_14 < 0x294) {
          if (sStack_14 < -0x153) {
            iVar10 = CONCAT22(0x78 - (short)((((iVar10 >> 0x10) + -0x78) * 500) /
                                            (sStack_14 + -0xa0)),0xfeac);
          }
        }
        else {
                    /* END-> C:\PcMpe\compress\compress.C: ? */
          iVar10 = CONCAT22((short)((((iVar10 >> 0x10) + -0x78) * 500) / (sStack_14 + -0xa0)) + 0x78
                            ,0x294);
        }
        sStack_14 = (short)iVar10;
        iVar9 = iVar10 >> 0x10;
        if (iVar10 < 0x1720000) {
          if (iVar9 < -0x81) {
            iVar10 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_14 + -0xa0) * 0xfa) / (iVar9 + -0x78)))
            ;
          }
        }
        else {
          iVar10 = CONCAT22(0x172,(short)(((sStack_14 + -0xa0) * 0xfa) / (iVar9 + -0x78)) + 0xa0);
        }
        _gprim1[2] = iVar6;
        _gprim1[4] = iVar7;
        _gprim1[6] = iVar8;
        _gprim1[8] = iVar10;
        puVar5 = (undefined4 *)(iVar4 * 4 + iVar3);
        uVar2 = *puVar5;
        *puVar5 = _gprim1;
        *_gprim1 = uVar2;
      }
    }
    _gprim1 = _gprim1 + 10;
    iStack_20 = iStack_20 + 0x14;
  }
  _gpoly = iStack_20;
  return;
}


/* ===== FUN_0041b2d8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_4pt_text_dpq_squash(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  short sStack_30;
  short sStack_2c;
  int iStack_24;
  int iStack_20;
  short sStack_18;
  short sStack_14;
  
  iVar3 = _DAT_0071bdc4;
  iStack_20 = _gpoly;
  puVar6 = _gprim1;
  for (iStack_24 = 0; iStack_24 < param_1; iStack_24 = iStack_24 + 1) {
    iVar4 = (*(int *)(iStack_20 + 10) >> 0x10) * 0x10;
    uVar1 = *(uint *)(&rot_points + iVar4);
    iVar11 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714138 = *(int *)(&DAT_00716dc8 + iVar4);
    iVar4 = (*(int *)(iStack_20 + 0xc) >> 0x10) * 0x10;
    _DAT_00714150 = *(uint *)(&rot_points + iVar4);
    _DAT_00714154 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714158 = *(int *)(&DAT_00716dc8 + iVar4);
    _DAT_0071415c = *(undefined4 *)(iVar4 + 0x716dcc);
    iVar4 = (*(int *)(iStack_20 + 0xe) >> 0x10) * 0x10;
    _DAT_00714120 = *(uint *)(&rot_points + iVar4);
    _DAT_00714124 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714128 = *(int *)(&DAT_00716dc8 + iVar4);
    _DAT_0071412c = *(undefined4 *)(iVar4 + 0x716dcc);
    iVar7 = (uVar1 & 0xffff) + iVar11 * 0x10000;
    sStack_30 = (short)iVar7;
    iVar8 = (_DAT_00714150 & 0xffff) + _DAT_00714154 * 0x10000;
    sStack_2c = (short)iVar8;
    iVar4 = (*(int *)(iStack_20 + 0x10) >> 0x10) * 0x10;
    _DAT_00714140 = *(uint *)(&rot_points + iVar4);
    _DAT_00714144 = *(int *)(&DAT_00716dc4 + iVar4);
    _DAT_00714148 = *(int *)(&DAT_00716dc8 + iVar4);
    _DAT_0071414c = *(undefined4 *)(iVar4 + 0x716dcc);
    iVar9 = (_DAT_00714120 & 0xffff) + _DAT_00714124 * 0x10000;
    sStack_14 = (short)iVar9;
    iVar4 = _DAT_00714148 + _DAT_00714158 + _DAT_00714128 + _DAT_00714138;
    iVar10 = iVar4 >> 0x1f;
    iVar4 = (iVar4 + iVar10 * -4) - (uint)(iVar10 << 1 < 0);
    __otz = iVar4 >> 2;
    if (((int)((uVar1 - _DAT_00714150) * (iVar11 - _DAT_00714124) -
              (uVar1 - _DAT_00714120) * (iVar11 - _DAT_00714154)) < 1) ||
       (-1 < (int)((_DAT_00714140 - _DAT_00714150) * (_DAT_00714144 - _DAT_00714124) -
                  (_DAT_00714140 - _DAT_00714120) * (_DAT_00714144 - _DAT_00714154)))) {
      iVar11 = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
      sStack_18 = (short)iVar11;
      iVar4 = iVar4 >> 0x1f;
      iVar4 = (int)((__otz + iVar4 * -4) - (uint)(iVar4 << 1 < 0)) >> 2;
      if (0 < iVar4) {
        if (sStack_30 < 0x294) {
          if (sStack_30 < -0x153) {
            iVar7 = CONCAT22(0x78 - (short)((((iVar7 >> 0x10) + -0x78) * 500) / (sStack_30 + -0xa0))
                             ,0xfeac);
          }
        }
        else {
          iVar7 = CONCAT22((short)((((iVar7 >> 0x10) + -0x78) * 500) / (sStack_30 + -0xa0)) + 0x78,
                           0x294);
        }
        sStack_30 = (short)iVar7;
        iVar10 = iVar7 >> 0x10;
        if (iVar7 < 0x1720000) {
          if (iVar10 < -0x81) {
            iVar7 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_30 + -0xa0) * 0xfa) / (iVar10 + -0x78)))
            ;
          }
        }
        else {
          iVar7 = CONCAT22(0x172,(short)(((sStack_30 + -0xa0) * 0xfa) / (iVar10 + -0x78)) + 0xa0);
        }
        if (sStack_2c < 0x294) {
          if (sStack_2c < -0x153) {
            iVar8 = CONCAT22(0x78 - (short)((((iVar8 >> 0x10) + -0x78) * 500) / (sStack_2c + -0xa0))
                             ,0xfeac);
          }
        }
        else {
          iVar8 = CONCAT22((short)((((iVar8 >> 0x10) + -0x78) * 500) / (sStack_2c + -0xa0)) + 0x78,
                           0x294);
        }
        sStack_2c = (short)iVar8;
        iVar10 = iVar8 >> 0x10;
        if (iVar8 < 0x1720000) {
          if (iVar10 < -0x81) {
            iVar8 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_2c + -0xa0) * 0xfa) / (iVar10 + -0x78)))
            ;
          }
        }
        else {
          iVar8 = CONCAT22(0x172,(short)(((sStack_2c + -0xa0) * 0xfa) / (iVar10 + -0x78)) + 0xa0);
        }
        if (sStack_14 < 0x294) {
          if (sStack_14 < -0x153) {
            iVar9 = CONCAT22(0x78 - (short)((((iVar9 >> 0x10) + -0x78) * 500) / (sStack_14 + -0xa0))
                             ,0xfeac);
          }
        }
        else {
          iVar9 = CONCAT22((short)((((iVar9 >> 0x10) + -0x78) * 500) / (sStack_14 + -0xa0)) + 0x78,
                           0x294);
        }
        sStack_14 = (short)iVar9;
        iVar10 = iVar9 >> 0x10;
        if (iVar9 < 0x1720000) {
          if (iVar10 < -0x81) {
            iVar9 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_14 + -0xa0) * 0xfa) / (iVar10 + -0x78)))
            ;
          }
        }
        else {
          iVar9 = CONCAT22(0x172,(short)(((sStack_14 + -0xa0) * 0xfa) / (iVar10 + -0x78)) + 0xa0);
        }
        if (sStack_18 < 0x294) {
          if (sStack_18 < -0x153) {
            iVar11 = CONCAT22(0x78 - (short)((((iVar11 >> 0x10) + -0x78) * 500) /
                                            (sStack_18 + -0xa0)),0xfeac);
          }
        }
        else {
          iVar11 = CONCAT22((short)((((iVar11 >> 0x10) + -0x78) * 500) / (sStack_18 + -0xa0)) + 0x78
                            ,0x294);
        }
        sStack_18 = (short)iVar11;
        iVar10 = iVar11 >> 0x10;
        if (iVar11 < 0x1720000) {
          if (iVar10 < -0x81) {
            iVar11 = CONCAT22(0xff7e,0xa0 - (short)(((sStack_18 + -0xa0) * 0xfa) / (iVar10 + -0x78))
                             );
          }
        }
        else {
          iVar11 = CONCAT22(0x172,(short)(((sStack_18 + -0xa0) * 0xfa) / (iVar10 + -0x78)) + 0xa0);
        }
        puVar6[2] = iVar7;
        puVar6[4] = iVar8;
        puVar6[6] = iVar9;
        puVar6[8] = iVar11;
        puVar5 = (undefined4 *)(iVar4 * 4 + iVar3);
        uVar2 = *puVar5;
        *puVar5 = puVar6;
        *puVar6 = uVar2;
        __rgb0 = (uint)*(byte *)(iStack_20 + 4);
        gte_dpcs();
        *(undefined1 *)(puVar6 + 1) = _rgb0;
      }
    }
    puVar6 = puVar6 + 10;
    iStack_20 = iStack_20 + 0x14;
  }
  _gpoly = iStack_20;
  _gprim1 = puVar6;
  return;
}


