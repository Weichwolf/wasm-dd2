/* ===== draw_face_4pt_tilt_sprite ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

/* originally FUN_0041edbc */
void draw_face_4pt_tilt_sprite(int param_1)

{
  undefined4 uVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int *piVar12;
  int aiStack_15c [64];
  short sStack_5c;
  short sStack_5a;
  short sStack_58;
  undefined2 uStack_56;
  short sStack_54;
  short sStack_52;
  short sStack_50;
  undefined2 uStack_4e;
  short sStack_4c;
  short sStack_4a;
  short sStack_48;
  undefined2 uStack_46;
  short sStack_44;
  short sStack_42;
  short sStack_40;
  undefined2 uStack_3e;
  int iStack_3c;
  uint uStack_38;
  int *piStack_34;
  int iStack_30;
  int *piStack_2c;
  undefined4 *puStack_28;
  int iStack_24;
  int iStack_20;
  int *piStack_1c;
  int iStack_18;
  int iStack_14;
  
  puStack_28 = _gprim1;
  iStack_14 = _gpoly;
  iStack_20 = _DAT_0071bdc0;
  iStack_3c = _DAT_0071bdc4;
  uStack_38 = DAT_00462d90;
  gte_SetRotMatrix(&world_matrix);
  iStack_30 = 0;
  while (iStack_30 < param_1) {
    piVar12 = (int *)(iStack_20 + (*(int *)(iStack_14 + 10) >> 0x10) * 8);
    piStack_2c = (int *)(iStack_20 + (*(int *)(iStack_14 + 0xc) >> 0x10) * 8);
    piStack_34 = (int *)(iStack_20 + (*(int *)(iStack_14 + 0xe) >> 0x10) * 8);
    piStack_1c = (int *)(iStack_20 + (*(int *)(iStack_14 + 0x10) >> 0x10) * 8);
    iVar10 = (int)(short)*piVar12 + (int)(short)*piStack_2c + (int)(short)*piStack_34 +
             (int)(short)*piStack_1c;
    iVar11 = iVar10 >> 0x1f;
    iVar6 = (*piStack_1c >> 0x10) +
            (*piStack_2c >> 0x10) + (*piVar12 >> 0x10) + (*piStack_34 >> 0x10);
    iVar7 = iVar6 >> 0x1f;
    iVar8 = (*(int *)((int)piStack_1c + 2) >> 0x10) +
            (*(int *)((int)piStack_34 + 2) >> 0x10) +
            (*(int *)((int)piStack_2c + 2) >> 0x10) + (*(int *)((int)piVar12 + 2) >> 0x10);
    iVar9 = iVar8 >> 0x1f;
    sStack_42 = (short)((uint)*piVar12 >> 0x10);
    uStack_3e = (undefined2)((uint)piVar12[1] >> 0x10);
    sVar2 = (short)((int)((iVar10 + iVar11 * -4) - (uint)(iVar11 << 1 < 0)) >> 2);
    sStack_44 = (short)*piVar12 - sVar2;
    sVar3 = (short)((int)((iVar6 + iVar7 * -4) - (uint)(iVar7 << 1 < 0)) >> 2);
    sStack_42 = sStack_42 - sVar3;
    sVar4 = (short)((int)((iVar8 + iVar9 * -4) - (uint)(iVar9 << 1 < 0)) >> 2);
    sStack_40 = (short)piVar12[1] - sVar4;
    sStack_4a = (short)((uint)*piStack_2c >> 0x10);
    uStack_46 = (undefined2)((uint)piStack_2c[1] >> 0x10);
    sStack_4c = (short)*piStack_2c - sVar2;
    sStack_4a = sStack_4a - sVar3;
    sStack_48 = (short)piStack_2c[1] - sVar4;
    sStack_5a = (short)((uint)*piStack_34 >> 0x10);
    uStack_56 = (undefined2)((uint)piStack_34[1] >> 0x10);
    sStack_5c = (short)*piStack_34 - sVar2;
    sStack_5a = sStack_5a - sVar3;
    sStack_58 = (short)piStack_34[1] - sVar4;
    sStack_52 = (short)((uint)*piStack_1c >> 0x10);
    uStack_4e = (undefined2)((uint)piStack_1c[1] >> 0x10);
    sStack_52 = sStack_52 - sVar3;
    sStack_54 = (short)*piStack_1c - sVar2;
    __vr3 = __vr2;
    _DAT_007140f4 = _DAT_007140e4;
    _DAT_007140f8 = _DAT_007140e8;
    _DAT_007140fc = _DAT_007140ec;
    sStack_50 = (short)piStack_1c[1] - sVar4;
    __vr2 = __vr1;
    _DAT_007140e4 = _DAT_00714114;
    _DAT_007140e8 = _DAT_00714118;
    _DAT_007140ec = _DAT_0071411c;
    __vr1 = __vr0;
    _DAT_00714114 = _DAT_00714104;
    _DAT_00714118 = _DAT_00714108;
    _DAT_0071411c = _DAT_0071410c;
    _DAT_00714104 = (int)sVar3;
    _DAT_00714108 = (int)sVar4;
    iVar10 = iStack_14 + 0x14;
    __vr0 = (int)sVar2;
    GTERT();
    aiStack_15c[iStack_30 * 4] = __vr0;
    aiStack_15c[iStack_30 * 4 + 1] = _DAT_00714104;
    aiStack_15c[iStack_30 * 4 + 2] = _DAT_00714108;
    iStack_30 = iStack_30 + 1;
    iStack_14 = iVar10;
  }
  gte_SetRotMatrix((undefined2 *)&tilt_sprite_matrix);
  iStack_24 = _gpoly;
  for (iStack_18 = 0; iStack_18 < param_1; iStack_18 = iStack_18 + 1) {
    _DAT_00714302 = aiStack_15c[iStack_18 * 4];
    _DAT_00714306 = aiStack_15c[iStack_18 * 4 + 1];
    _DAT_0071430a = aiStack_15c[iStack_18 * 4 + 2];
    __vr0 = (int)sStack_44;
    _DAT_00714104 = (int)sStack_42;
    _DAT_00714108 = (int)sStack_40;
    __vr1 = (int)sStack_4c;
    _DAT_00714114 = (int)sStack_4a;
    _DAT_00714118 = (int)sStack_48;
    __vr2 = (int)sStack_5c;
    _DAT_007140e4 = (int)sStack_5a;
    _DAT_007140e8 = (int)sStack_58;
    GTERPT();
    _DAT_00714130 = _DAT_00714140;
    _DAT_00714134 = _DAT_00714144;
    _DAT_00714138 = _DAT_00714148;
    _DAT_0071413c = _DAT_0071414c;
    if ((__flg & uStack_38) == 0) {
      puStack_28[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_28[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_28[6] = _DAT_00714120 & 0xffff | _DAT_00714124 << 0x10;
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
      _DAT_00714104 = (int)sStack_52;
      _DAT_00714108 = (int)sStack_50;
      __vr0 = (int)sStack_54;
      GTERPS();
      iVar10 = _DAT_00714138 + _DAT_00714148 + _DAT_00714158 + _DAT_00714128;
      iVar11 = iVar10 >> 0x1f;
      __otz = (int)((iVar10 + iVar11 * -4) - (uint)(iVar11 << 1 < 0)) >> 2;
      if ((__flg & uStack_38) == 0) {
        puStack_28[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar5 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iStack_3c);
        uVar1 = *puVar5;
        *puVar5 = puStack_28;
        *puStack_28 = uVar1;
      }
    }
    puStack_28 = puStack_28 + 10;
    iStack_24 = iStack_24 + 0x14;
  }
  _gpoly = iStack_24;
  _gprim1 = puStack_28;
  return;
}


/* ===== FUN_0041f220 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void draw_face_tilt_sprite_dpq(int param_1)

{
  undefined4 uVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int *piVar12;
  int aiStack_15c [64];
  short sStack_5c;
  short sStack_5a;
  short sStack_58;
  undefined2 uStack_56;
  short sStack_54;
  short sStack_52;
  short sStack_50;
  undefined2 uStack_4e;
  short sStack_4c;
  short sStack_4a;
  short sStack_48;
  undefined2 uStack_46;
  short sStack_44;
  short sStack_42;
  short sStack_40;
  undefined2 uStack_3e;
  int iStack_3c;
  int iStack_38;
  int iStack_34;
  int *piStack_30;
  undefined4 *puStack_2c;
  int iStack_28;
  uint uStack_24;
  int *piStack_20;
  int *piStack_1c;
  int iStack_18;
  int iStack_14;
  
  puStack_2c = _gprim1;
  iStack_14 = _gpoly;
  iStack_3c = _DAT_0071bdc0;
  iStack_28 = _DAT_0071bdc4;
  uStack_24 = DAT_00462d90;
  gte_SetRotMatrix(&world_matrix);
  iStack_38 = 0;
  while (iStack_38 < param_1) {
    piVar12 = (int *)(iStack_3c + (*(int *)(iStack_14 + 10) >> 0x10) * 8);
    piStack_30 = (int *)(iStack_3c + (*(int *)(iStack_14 + 0xc) >> 0x10) * 8);
    piStack_20 = (int *)(iStack_3c + (*(int *)(iStack_14 + 0xe) >> 0x10) * 8);
    piStack_1c = (int *)(iStack_3c + (*(int *)(iStack_14 + 0x10) >> 0x10) * 8);
    iVar10 = (int)(short)*piStack_1c +
             (int)(short)*piStack_20 + (int)(short)*piVar12 + (int)(short)*piStack_30;
    iVar11 = iVar10 >> 0x1f;
    iVar6 = (*piStack_1c >> 0x10) +
            (*piStack_20 >> 0x10) + (*piStack_30 >> 0x10) + (*piVar12 >> 0x10);
    iVar7 = iVar6 >> 0x1f;
    iVar8 = (*(int *)((int)piStack_1c + 2) >> 0x10) +
            (*(int *)((int)piStack_20 + 2) >> 0x10) +
            (*(int *)((int)piVar12 + 2) >> 0x10) + (*(int *)((int)piStack_30 + 2) >> 0x10);
    iVar9 = iVar8 >> 0x1f;
    sStack_42 = (short)((uint)*piVar12 >> 0x10);
    uStack_3e = (undefined2)((uint)piVar12[1] >> 0x10);
    sVar2 = (short)((int)((iVar10 + iVar11 * -4) - (uint)(iVar11 << 1 < 0)) >> 2);
    sStack_44 = (short)*piVar12 - sVar2;
    sVar3 = (short)((int)((iVar6 + iVar7 * -4) - (uint)(iVar7 << 1 < 0)) >> 2);
    sStack_42 = sStack_42 - sVar3;
    sVar4 = (short)((int)((iVar8 + iVar9 * -4) - (uint)(iVar9 << 1 < 0)) >> 2);
    sStack_40 = (short)piVar12[1] - sVar4;
    sStack_4a = (short)((uint)*piStack_30 >> 0x10);
    uStack_46 = (undefined2)((uint)piStack_30[1] >> 0x10);
    sStack_4c = (short)*piStack_30 - sVar2;
    sStack_4a = sStack_4a - sVar3;
    sStack_48 = (short)piStack_30[1] - sVar4;
    sStack_5a = (short)((uint)*piStack_20 >> 0x10);
    uStack_56 = (undefined2)((uint)piStack_20[1] >> 0x10);
    sStack_5c = (short)*piStack_20 - sVar2;
    sStack_5a = sStack_5a - sVar3;
    sStack_58 = (short)piStack_20[1] - sVar4;
    sStack_52 = (short)((uint)*piStack_1c >> 0x10);
    uStack_4e = (undefined2)((uint)piStack_1c[1] >> 0x10);
    sStack_52 = sStack_52 - sVar3;
    sStack_54 = (short)*piStack_1c - sVar2;
    __vr3 = __vr2;
    _DAT_007140f4 = _DAT_007140e4;
    _DAT_007140f8 = _DAT_007140e8;
    _DAT_007140fc = _DAT_007140ec;
    sStack_50 = (short)piStack_1c[1] - sVar4;
    __vr2 = __vr1;
    _DAT_007140e4 = _DAT_00714114;
    _DAT_007140e8 = _DAT_00714118;
    _DAT_007140ec = _DAT_0071411c;
    __vr1 = __vr0;
    _DAT_00714114 = _DAT_00714104;
    _DAT_00714118 = _DAT_00714108;
    _DAT_0071411c = _DAT_0071410c;
    _DAT_00714104 = (int)sVar3;
    _DAT_00714108 = (int)sVar4;
    iVar10 = iStack_14 + 0x14;
    __vr0 = (int)sVar2;
    GTERT();
    aiStack_15c[iStack_38 * 4] = __vr0;
    aiStack_15c[iStack_38 * 4 + 1] = _DAT_00714104;
    aiStack_15c[iStack_38 * 4 + 2] = _DAT_00714108;
    iStack_38 = iStack_38 + 1;
    iStack_14 = iVar10;
  }
  gte_SetRotMatrix((undefined2 *)&tilt_sprite_matrix);
  iStack_18 = _gpoly;
  for (iStack_34 = 0; iStack_34 < param_1; iStack_34 = iStack_34 + 1) {
    _DAT_00714302 = aiStack_15c[iStack_34 * 4];
    _DAT_00714306 = aiStack_15c[iStack_34 * 4 + 1];
    _DAT_0071430a = aiStack_15c[iStack_34 * 4 + 2];
    __vr0 = (int)sStack_44;
    _DAT_00714104 = (int)sStack_42;
    _DAT_00714108 = (int)sStack_40;
    __vr1 = (int)sStack_4c;
    _DAT_00714114 = (int)sStack_4a;
    _DAT_00714118 = (int)sStack_48;
    __vr2 = (int)sStack_5c;
    _DAT_007140e4 = (int)sStack_5a;
    _DAT_007140e8 = (int)sStack_58;
    GTERPT();
    _DAT_00714130 = _DAT_00714140;
    _DAT_00714134 = _DAT_00714144;
    _DAT_00714138 = _DAT_00714148;
    _DAT_0071413c = _DAT_0071414c;
    if ((__flg & uStack_24) == 0) {
      puStack_2c[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_2c[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_2c[6] = _DAT_00714120 & 0xffff | _DAT_00714124 << 0x10;
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
      _DAT_00714104 = (int)sStack_52;
      _DAT_00714108 = (int)sStack_50;
      __vr0 = (int)sStack_54;
      GTERPS();
      iVar10 = _DAT_00714138 + _DAT_00714148 + _DAT_00714158 + _DAT_00714128;
      iVar11 = iVar10 >> 0x1f;
      __otz = (int)((iVar10 + iVar11 * -4) - (uint)(iVar11 << 1 < 0)) >> 2;
      if ((__flg & uStack_24) == 0) {
        puStack_2c[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar5 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iStack_28);
        uVar1 = *puVar5;
        *puVar5 = puStack_2c;
        *puStack_2c = uVar1;
        __rgb0 = (uint)*(byte *)(iStack_18 + 4);
        gte_dpcs();
        *(undefined1 *)(puStack_2c + 1) = _rgb0;
      }
    }
    puStack_2c = puStack_2c + 10;
    iStack_18 = iStack_18 + 0x14;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_2c;
  return;
}


/* ===== draw_face_3pt_tilt_sprite_dpq ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

/* originally FUN_0041f900 */
void draw_face_3pt_tilt_sprite_dpq(int param_1)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  int *piVar6;
  int iVar7;
  int iVar8;
  int iStack_20;
  int iStack_18;
  undefined4 *puStack_14;
  
  iVar4 = _DAT_0071bdc4;
  iVar3 = _DAT_0071bdc0;
  uVar2 = DAT_00462d90;
  puStack_14 = _gprim1;
  gte_SetRotMatrix((undefined2 *)&tilt_sprite_matrix);
  iStack_18 = _gpoly;
  for (iStack_20 = 0; iStack_20 < param_1; iStack_20 = iStack_20 + 1) {
    piVar6 = (int *)(iVar3 + (*(int *)(iStack_18 + 10) >> 0x10) * 8);
    __vr0 = (int)(short)*piVar6;
    _DAT_00714104 = *piVar6 >> 0x10;
    _DAT_00714108 = *(int *)((int)piVar6 + 2) >> 0x10;
    piVar6 = (int *)(iVar3 + (*(int *)(iStack_18 + 0xc) >> 0x10) * 8);
    __vr1 = (int)(short)*piVar6;
    _DAT_00714114 = *piVar6 >> 0x10;
    _DAT_00714118 = *(int *)((int)piVar6 + 2) >> 0x10;
    piVar6 = (int *)(iVar3 + (*(int *)(iStack_18 + 0xe) >> 0x10) * 8);
    __vr2 = (int)(short)*piVar6;
    _DAT_007140e4 = *piVar6 >> 0x10;
    _DAT_007140e8 = *(int *)((int)piVar6 + 2) >> 0x10;
    GTERPT();
    _DAT_00714130 = _DAT_00714140;
    _DAT_00714134 = _DAT_00714144;
    _DAT_00714138 = _DAT_00714148;
    _DAT_0071413c = _DAT_0071414c;
    if ((__flg & uVar2) == 0) {
      puStack_14[2] = _DAT_00714140 & 0xffff | _DAT_00714144 << 0x10;
      puStack_14[4] = _DAT_00714150 & 0xffff | _DAT_00714154 << 0x10;
      puStack_14[6] = _DAT_00714120 & 0xffff | _DAT_00714124 << 0x10;
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
      piVar6 = (int *)(iVar3 + (*(int *)(iStack_18 + 0x10) >> 0x10) * 8);
      __vr0 = (int)(short)*piVar6;
      _DAT_00714104 = *piVar6 >> 0x10;
      _DAT_00714108 = *(int *)((int)piVar6 + 2) >> 0x10;
      GTERPS();
      iVar7 = _DAT_00714148 + _DAT_00714158 + _DAT_00714128 + _DAT_00714138;
      iVar8 = iVar7 >> 0x1f;
      __otz = (int)((iVar7 + iVar8 * -4) - (uint)(iVar8 << 1 < 0)) >> 2;
      if ((uVar2 & __flg) == 0) {
        puStack_14[8] = (_DAT_00714140 & 0xffff) + _DAT_00714144 * 0x10000;
        puVar5 = (undefined4 *)
                 (((int)((__otz + (__otz >> 0x1f) * -4) - (uint)((__otz >> 0x1f) << 1 < 0)) >> 2) *
                  4 + iVar4);
        uVar1 = *puVar5;
        *puVar5 = puStack_14;
        *puStack_14 = uVar1;
        __rgb0 = (uint)*(byte *)(iStack_18 + 4);
        gte_dpcs();
        *(undefined1 *)(puStack_14 + 1) = _rgb0;
      }
    }
    puStack_14 = puStack_14 + 10;
    iStack_18 = iStack_18 + 0x14;
  }
  _gpoly = iStack_18;
  _gprim1 = puStack_14;
  return;
}


