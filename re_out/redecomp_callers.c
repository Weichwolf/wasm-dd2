/* ===== Print @ 004212d4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __cdecl Print(undefined2 param_1,int param_2)

{
  byte bVar1;
  ushort uVar2;
  int iVar3;
  int iVar4;
  byte *pbVar5;
  int iVar6;
  undefined2 uVar7;
  undefined2 uVar8;
  uint uVar9;
  char cVar10;
  int iVar11;
  int iVar12;
  char *pcVar13;
  int in_stack_ffffff78;
  undefined4 in_stack_ffffff7c;
  undefined4 local_80;
  char local_7c [12];
  undefined1 local_70;
  undefined1 local_6f;
  undefined1 local_6e;
  int local_6c;
  int local_68;
  int local_64;
  uint local_60;
  int local_5c;
  int local_58;
  int local_54;
  int local_50;
  int local_4c;
  int local_48;
  int local_44;
  int local_40;
  uint local_3c;
  int local_38;
  int local_34;
  int local_30;
  short local_2c;
  short sStack_2a;
  ushort local_28;
  undefined2 local_26;
  undefined2 uStack_24;
  uint local_22;
  undefined2 local_1e;
  undefined2 uStack_1c;
  int local_18;
  int local_14;
  
  FUN_004221ec();
  local_44 = DAT_00463018;
  local_3c = DAT_00463020;
  *(undefined2 *)(DAT_00463018 * 0x60 + _DAT_0071c008 + 0x5a) = param_1;
  iVar12 = 0;
LAB_00421315:
  do {
    while( true ) {
      while( true ) {
        iVar11 = iVar12;
        if (*(char *)(param_2 + iVar11) == '\0') {
          *(undefined2 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x52) = (undefined2)DAT_0046301c;
          if (_DAT_0071bff8 <= (int)DAT_00463020) {
            System_Error(s_Print__0046c8c0,s_TOO_MUCH_TEXT__0046c8b0);
          }
          for (; iVar12 = _DAT_0071c008, local_44 <= DAT_00463018; local_44 = local_44 + 1) {
            iVar3 = _DAT_0071c008 + local_44 * 0x60;
            local_2c = (short)(char)((uint)*(undefined4 *)(iVar3 + 0x4d) >> 0x18);
            sStack_2a = (short)((int)*(undefined4 *)(iVar3 + 0x4d) >> 0x1f);
            local_70 = *(undefined1 *)(iVar3 + 0x5c);
            iVar11 = *(int *)(iVar3 + 4);
            local_6f = *(undefined1 *)(iVar3 + 0x5d);
            local_6e = *(undefined1 *)(iVar3 + 0x5e);
            pbVar5 = (byte *)(_DAT_0071c00c + CONCAT22(sStack_2a,local_2c) * 8);
            _local_26 = CONCAT22(*(undefined2 *)(pbVar5 + 2),local_26);
            _local_1e = CONCAT22(*(undefined2 *)(pbVar5 + 4),local_1e);
            local_28 = (ushort)*pbVar5;
            local_54 = *(int *)(pbVar5 + 4) >> 0x10;
            local_22 = (uint)CONCAT12(pbVar5[1],(undefined2)local_22);
            uVar2 = *(ushort *)(iVar3 + 0x58);
            iVar3 = 0;
            if (uVar2 != 0) {
              iVar4 = iVar11;
              if (uVar2 < 2) {
                for (; iVar4 < (*(int *)(_DAT_0071c008 + 0x50 + local_44 * 0x60) >> 0x10) + iVar11;
                    iVar4 = iVar4 + 1) {
                  iVar3 = iVar3 + (*(byte *)(*(int *)(&DAT_0071bfd0 +
                                                     CONCAT22(sStack_2a,local_2c) * 4) + 2 +
                                            (uint)(byte)(*(char *)(iVar4 + _DAT_0071c000) - 0x20) *
                                            4) - 1) + local_54;
                }
              }
              else if (uVar2 == 2) {
                for (; iVar4 < (*(int *)(_DAT_0071c008 + 0x50 + local_44 * 0x60) >> 0x10) + iVar11;
                    iVar4 = iVar4 + 1) {
                  local_58 = (uint)(byte)(*(char *)(iVar4 + _DAT_0071c000) - 0x20) * 4;
                  iVar3 = iVar3 + (*(byte *)(*(int *)(&DAT_0071bfd0 +
                                                     CONCAT22(sStack_2a,local_2c) * 4) + local_58 +
                                            2) - 1) + local_54;
                }
                iVar3 = iVar3 / 2;
              }
            }
            iVar6 = local_44 * 0x60;
            iVar4 = _DAT_0071c008 + 0x14;
            *(int *)(_DAT_0071c008 + 8 + iVar6) = *(int *)(_DAT_0071c008 + 8 + iVar6) - iVar3;
            *(int *)(iVar12 + 0x14 + iVar6) = *(int *)(iVar4 + iVar6) - iVar3;
            local_30 = 0;
            do {
              iVar3 = _DAT_0071c008 + local_44 * 0x60;
              iVar12 = *(int *)(iVar3 + 0x10);
              if (iVar12 == 0) {
                iVar12 = 1;
              }
              local_48 = (screen_width / 2) * 0x100 + (*(int *)(iVar3 + 8) << 0x10) / iVar12;
              local_50 = (screen_height / 2) * 0x100 + (*(int *)(iVar3 + 0xc) << 0x10) / iVar12;
              for (local_4c = iVar11; local_38 = _DAT_0071c008 + local_44 * 0x60,
                  local_4c < (*(int *)(local_38 + 0x50) >> 0x10) + iVar11; local_4c = local_4c + 1)
              {
                pbVar5 = (byte *)(*(int *)(&DAT_0071bfd0 + CONCAT22(sStack_2a,local_2c) * 4) +
                                 (uint)(byte)(*(char *)(_DAT_0071c000 + local_4c) - 0x20) * 4);
                local_6c = (uint)*pbVar5 + (int)(short)local_28;
                local_68 = (uint)pbVar5[1] + ((int)local_22 >> 0x10);
                local_64 = pbVar5[2] - 1;
                bVar1 = pbVar5[3];
                local_60 = (uint)bVar1;
                local_34 = (local_64 * 0x10000) / iVar12;
                local_40 = (int)(local_60 << 0x10) / iVar12;
                iVar3 = buffer_num * 4;
                iVar4 = local_4c * 0x28;
                *(char *)(*(int *)(&DAT_0071bfa0 + iVar3) + 0xc + iVar4) = (char)local_6c;
                *(char *)(*(int *)(&DAT_0071bfa0 + iVar3) + 0xd + iVar4) = (char)local_68;
                *(char *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 0x14) =
                     (char)local_6c + (char)local_64;
                *(char *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 0x15) = (char)local_68;
                *(char *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 0x1c) = (char)local_6c;
                cVar10 = (char)local_68 + bVar1;
                *(char *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 0x1d) = cVar10;
                *(char *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 0x24) =
                     (char)local_6c + (char)local_64;
                *(char *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 0x25) = cVar10;
                local_18 = local_48 >> 8;
                uVar7 = (undefined2)((uint)local_48 >> 8);
                *(undefined2 *)(*(int *)(&DAT_0071bfa0 + iVar3) + 8 + iVar4) = uVar7;
                local_14 = local_48 + local_34 >> 8;
                uVar8 = (undefined2)((uint)(local_48 + local_34) >> 8);
                *(undefined2 *)(*(int *)(&DAT_0071bfa0 + iVar3) + 0x10 + iVar4) = uVar8;
                *(undefined2 *)(*(int *)(&DAT_0071bfa0 + iVar3) + 0x18 + iVar4) = uVar7;
                *(undefined2 *)(*(int *)(&DAT_0071bfa0 + iVar3) + 0x20 + iVar4) = uVar8;
                uVar7 = (undefined2)((uint)local_50 >> 8);
                *(undefined2 *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 10) = uVar7;
                *(undefined2 *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 0x12) = uVar7;
                uVar7 = (undefined2)((uint)(local_50 + local_40) >> 8);
                *(undefined2 *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 0x1a) = uVar7;
                *(undefined2 *)(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4 + 0x22) = uVar7;
                FUN_00420cf0(*(int *)(&DAT_0071bfa0 + iVar3) + iVar4,
                             (ushort)((uint)_local_26 >> 0x10),(short)((uint)_local_1e >> 0x10),
                             &local_70,*(int *)(local_38 + 0x4e) >> 0x18);
                local_48 = local_48 + (local_54 << 0x10) / iVar12 + local_34;
              }
              local_30 = local_30 + 1;
              buffer_num = buffer_num ^ 1;
            } while (local_30 < 2);
          }
          return local_3c << 0x10 | DAT_00463020;
        }
        if (*(char *)(param_2 + iVar11) == '%') break;
        *(undefined1 *)(_DAT_0071c000 + DAT_00463020) = *(undefined1 *)(param_2 + iVar11);
        DAT_00463020 = DAT_00463020 + 1;
        DAT_0046301c = DAT_0046301c + 1;
        iVar12 = iVar11 + 1;
      }
      bVar1 = *(byte *)(param_2 + 1 + iVar11);
      iVar12 = iVar11 + 2;
      if (bVar1 < 0x4a) break;
      if (bVar1 < 0x4b) {
        FUN_004221ec();
        bVar1 = *(byte *)(param_2 + iVar12);
        iVar12 = iVar11 + 3;
        if (bVar1 < 0x4c) {
          if (bVar1 == 0x43) {
            *(undefined2 *)(_DAT_0071c008 + 0x58 + DAT_00463018 * 0x60) = 2;
          }
        }
        else if (bVar1 < 0x4d) {
          *(undefined2 *)(_DAT_0071c008 + 0x58 + DAT_00463018 * 0x60) = 0;
        }
        else if (bVar1 == 0x52) {
          *(undefined2 *)(_DAT_0071c008 + 0x58 + DAT_00463018 * 0x60) = 1;
        }
      }
      else if (bVar1 < 0x53) {
        if (0x4f < bVar1) {
          if (bVar1 < 0x51) {
            FUN_004221ec();
            iVar12 = FUN_00422128((int *)&stack0xffffff78,param_2,iVar12);
            pbVar5 = (byte *)(_DAT_0071c008 + DAT_00463018 * 0x60);
            *pbVar5 = *pbVar5 | 0x10;
            *(int *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 8) = in_stack_ffffff78;
            *(undefined4 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0xc) = in_stack_ffffff7c;
            *(undefined4 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x10) = local_80;
          }
          else if (bVar1 == 0x52) {
            FUN_004221ec();
            pbVar5 = (byte *)(_DAT_0071c008 + 1 + DAT_00463018 * 0x60);
            *pbVar5 = *pbVar5 | 2;
          }
        }
      }
      else if (bVar1 < 0x54) {
        FUN_004221ec();
        bVar1 = *(byte *)(param_2 + iVar12);
        iVar12 = iVar11 + 3;
        if (0x4f < bVar1) {
          if (bVar1 < 0x51) {
            iVar12 = FUN_00422184(&local_5c,param_2,iVar12);
            pbVar5 = (byte *)(_DAT_0071c008 + DAT_00463018 * 0x60);
            *pbVar5 = *pbVar5 | 0x40;
            *(short *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x56) = (short)local_5c;
          }
          else if (bVar1 == 0x53) {
            iVar12 = FUN_00422184(&local_5c,param_2,iVar12);
            pbVar5 = (byte *)(_DAT_0071c008 + DAT_00463018 * 0x60);
            *pbVar5 = *pbVar5 | 4;
            *(short *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x54) = (short)local_5c;
          }
        }
      }
      else if (bVar1 < 0x56) {
        if (bVar1 == 0x54) {
          FUN_004221ec();
          bVar1 = *(byte *)(param_2 + iVar12);
          iVar12 = iVar11 + 3;
          if (bVar1 < 0x2d) {
            if (bVar1 == 0x2b) {
              *(undefined1 *)(_DAT_0071c008 + 0x51 + DAT_00463018 * 0x60) = 1;
              pbVar5 = (byte *)(_DAT_0071c008 + DAT_00463018 * 0x60);
              *pbVar5 = *pbVar5 | 2;
            }
          }
          else if (bVar1 < 0x2e) {
            *(undefined1 *)(_DAT_0071c008 + 0x51 + DAT_00463018 * 0x60) = 0xff;
            pbVar5 = (byte *)(_DAT_0071c008 + DAT_00463018 * 0x60);
            *pbVar5 = *pbVar5 | 2;
          }
          else if (0x2e < bVar1) {
            if (bVar1 < 0x30) {
              cVar10 = *(char *)(DAT_00463018 * 0x60 + _DAT_0071c008 + 0x50);
              if ((cVar10 == '\x03') || (cVar10 == '\x04')) {
                *(undefined1 *)(_DAT_0071c008 + 0x51 + DAT_00463018 * 0x60) = 0xff;
              }
              else {
                *(undefined1 *)(DAT_00463018 * 0x60 + 0x51 + _DAT_0071c008) = 2;
              }
            }
            else if (bVar1 == 0x3d) {
              *(undefined1 *)(_DAT_0071c008 + 0x51 + DAT_00463018 * 0x60) = 0;
            }
          }
        }
      }
      else if (bVar1 < 0x57) {
        local_5c = *(int *)((uint)*(byte *)(param_2 + iVar12) * 4 + 0x71bee8);
        FUN_0045672e((int)local_7c,&DAT_0046c89c,local_5c);
        uVar9 = 0xffffffff;
        pcVar13 = local_7c;
        do {
          if (uVar9 == 0) break;
          uVar9 = uVar9 - 1;
          cVar10 = *pcVar13;
          pcVar13 = pcVar13 + 1;
        } while (cVar10 != '\0');
        local_5c = ~uVar9 - 1;
        iVar3 = 0;
        iVar12 = iVar11 + 3;
        if (0 < local_5c) {
          do {
            pcVar13 = local_7c + iVar3;
            iVar3 = iVar3 + 1;
            *(char *)(_DAT_0071c000 + DAT_00463020) = *pcVar13;
            DAT_00463020 = DAT_00463020 + 1;
            DAT_0046301c = DAT_0046301c + 1;
          } while (iVar3 < local_5c);
        }
      }
      else if (bVar1 == 0x5a) {
        FUN_004221ec();
        iVar3 = _DAT_0071c008;
        bVar1 = *(byte *)(param_2 + iVar12);
        iVar12 = iVar11 + 3;
        if (0x4f < bVar1) {
          iVar4 = DAT_00463018 * 0x60;
          if (bVar1 < 0x51) {
            pbVar5 = (byte *)(_DAT_0071c008 + iVar4);
            *(undefined2 *)(_DAT_0071c008 + 0x56 + iVar4) = 0;
            *(byte *)(iVar3 + iVar4) = *pbVar5 & 0xbf;
            iVar12 = iVar11 + 4;
          }
          else if (bVar1 == 0x53) {
            pbVar5 = (byte *)(_DAT_0071c008 + iVar4);
            *(undefined2 *)(_DAT_0071c008 + 0x54 + iVar4) = 0;
            *(byte *)(iVar3 + iVar4) = *pbVar5 & 0xfb;
            iVar12 = iVar11 + 4;
          }
        }
      }
    }
    if (0x42 < bVar1) {
      if (bVar1 < 0x44) {
        FUN_004221ec();
        iVar12 = FUN_004220cc(&local_70,param_2,iVar12);
        pbVar5 = (byte *)(_DAT_0071c008 + DAT_00463018 * 0x60);
        *pbVar5 = *pbVar5 | 1;
        *(undefined1 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x5c) = local_70;
        *(undefined1 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x5d) = local_6f;
        *(undefined1 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x5e) = local_6e;
      }
      else if (bVar1 < 0x46) {
        if (bVar1 == 0x44) {
          FUN_004221ec();
          iVar12 = FUN_00422128((int *)&stack0xffffff78,param_2,iVar12);
          pbVar5 = (byte *)(_DAT_0071c008 + DAT_00463018 * 0x60);
          *pbVar5 = *pbVar5 | 0x20;
          *(int *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x14) = in_stack_ffffff78;
          *(undefined4 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x18) = in_stack_ffffff7c;
          *(undefined4 *)(DAT_00463018 * 0x60 + _DAT_0071c008 + 0x1c) = local_80;
        }
      }
      else if (bVar1 < 0x47) {
        FUN_004221ec();
        *(char *)(_DAT_0071c008 + 0x50 + DAT_00463018 * 0x60) = *(char *)(param_2 + iVar12) + -0x30;
        iVar12 = iVar11 + 3;
      }
      else if (bVar1 == 0x48) {
        local_5c = *(int *)((uint)*(byte *)(param_2 + iVar12) * 4 + 0x71bee8);
        FUN_0045672e((int)local_7c,&DAT_0046c898,local_5c);
        uVar9 = 0xffffffff;
        pcVar13 = local_7c;
        do {
          if (uVar9 == 0) break;
          uVar9 = uVar9 - 1;
          cVar10 = *pcVar13;
          pcVar13 = pcVar13 + 1;
        } while (cVar10 != '\0');
        local_5c = ~uVar9 - 1;
        iVar3 = 0;
        iVar12 = iVar11 + 3;
        if (0 < local_5c) {
          do {
            pcVar13 = local_7c + iVar3;
            iVar3 = iVar3 + 1;
            *(char *)(_DAT_0071c000 + DAT_00463020) = *pcVar13;
            DAT_00463020 = DAT_00463020 + 1;
            DAT_0046301c = DAT_0046301c + 1;
          } while (iVar3 < local_5c);
        }
      }
      goto LAB_00421315;
    }
    if (0x24 < bVar1) {
      if (bVar1 < 0x26) {
        *(undefined1 *)(_DAT_0071c000 + DAT_00463020) = 0x25;
        DAT_00463020 = DAT_00463020 + 1;
        DAT_0046301c = DAT_0046301c + 1;
      }
      else if (bVar1 == 0x41) {
        FUN_004221ec();
        iVar12 = FUN_00422128((int *)&stack0xffffff78,param_2,iVar12);
        pbVar5 = (byte *)(DAT_00463018 * 0x60 + _DAT_0071c008);
        *pbVar5 = *pbVar5 | 0x30;
        *(int *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 8) = in_stack_ffffff78;
        *(undefined4 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0xc) = in_stack_ffffff7c;
        *(undefined4 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x10) = local_80;
        *(int *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x14) = in_stack_ffffff78;
        *(undefined4 *)(DAT_00463018 * 0x60 + _DAT_0071c008 + 0x18) = in_stack_ffffff7c;
        *(undefined4 *)(_DAT_0071c008 + DAT_00463018 * 0x60 + 0x1c) = local_80;
      }
    }
  } while( true );
}

/* ===== Init_Car_Cluts @ 0043a684 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void Init_Car_Cluts(void)

{
  undefined1 *puVar1;
  undefined1 uVar2;
  undefined2 uVar3;
  int iVar4;
  int iVar5;
  ushort *puVar6;
  char local_3c [16];
  undefined1 local_2c [4];
  undefined1 local_28 [4];
  undefined1 local_24 [4];
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  ushort *local_18;
  ushort *local_14;
  
  local_14 = (ushort *)0x7598b0;
  iVar4 = 0;
  puVar6 = (ushort *)0x7598c8;
  local_18 = (ushort *)0x7598e0;
  do {
    if (iVar4 != 0x12) {
      if ((iVar4 == 0) && (race_car != 0)) {
        FUN_0045672e((int)local_1c,&DAT_0046cdd4,4 - race_car);
        FUN_0045672e((int)local_20,&DAT_0046cdd8,4 - race_car);
        FUN_0045672e((int)local_24,&DAT_0046cddc,4 - race_car);
        FUN_0045672e((int)local_28,&DAT_0046cde0,4 - race_car);
        FUN_0045672e((int)local_2c,&DAT_0046cde4,4 - race_car);
        FUN_0045672e((int)local_3c,(byte *)s_CLT_02d_s_0046cde8,(uint)car_lookup,local_20);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        uRam007598a0 = _DAT_00759826;
        FUN_0045672e((int)local_3c,(byte *)s_CLT_02d_s_0046cde8,(uint)car_lookup,local_1c);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        uRam007598a2 = _DAT_00759826;
        FUN_0045672e((int)local_3c,(byte *)s_CLT_02d_s_0046cde8,(uint)car_lookup,local_28);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        uRam007598a4 = _DAT_00759826;
        FUN_0045672e((int)local_3c,(byte *)s_CLT_02d_s_0046cde8,(uint)car_lookup,local_24);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        uRam007598a6 = _DAT_00759826;
        FUN_0045672e((int)local_3c,(byte *)s_CLT_02d_s_0046cde8,(uint)car_lookup,local_2c);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        uRam007598a8 = _DAT_00759826;
        FUN_0045672e((int)local_3c,(byte *)s_P1D1T_d_0046cdf4,4 - race_car);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        uRam007598ae = _DAT_00759826;
      }
      else {
        iVar5 = iVar4 * 0x58;
        FUN_0045672e((int)local_3c,(byte *)s_CLUT_02dB_0046cdfc,(uint)(&car_lookup)[iVar4]);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        *(undefined2 *)(iVar5 + 0x7598a0) = _DAT_00759826;
        FUN_0045672e((int)local_3c,(byte *)s_CLUT_02dA_0046ce08,(uint)(&car_lookup)[iVar4]);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        *(undefined2 *)(iVar5 + 0x7598a2) = _DAT_00759826;
        FUN_0045672e((int)local_3c,(byte *)s_CLUT_02dD_0046ce14,(uint)(&car_lookup)[iVar4]);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        *(undefined2 *)(iVar5 + 0x7598a4) = _DAT_00759826;
        FUN_0045672e((int)local_3c,(byte *)s_CLUT_02dC_0046ce20,(uint)(&car_lookup)[iVar4]);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        *(undefined2 *)(iVar5 + 0x7598a6) = _DAT_00759826;
        FUN_0045672e((int)local_3c,(byte *)s_CLUT_02dE_0046ce2c,(uint)(&car_lookup)[iVar4]);
        Setup_Sprite(0,local_3c,(ushort *)&data);
        *(undefined2 *)(iVar5 + 0x7598a8) = _DAT_00759826;
      }
      FUN_0045672e((int)local_3c,(byte *)s_SMCL_02dB_0046ce38,(uint)(&car_lookup)[iVar4]);
      Setup_Sprite(0,local_3c,(ushort *)&data);
      *(undefined2 *)(iVar4 * 0x58 + 0x7598aa) = _DAT_00759826;
      FUN_0045672e((int)local_3c,(byte *)s_SMCL_02dA_0046ce44,(uint)(&car_lookup)[iVar4]);
      Setup_Sprite(0,local_3c,(ushort *)&data);
      *(undefined2 *)(iVar4 * 0x58 + 0x7598ac) = _DAT_00759826;
    }
    FUN_0045672e((int)local_3c,(byte *)s_DR_02dA_0046ce50,(uint)(&car_lookup)[iVar4]);
    Setup_Sprite(0,local_3c,local_14);
    if ((iVar4 != 0) || (race_car == 0)) {
      *(undefined2 *)(iVar4 * 0x58 + 0x7598ae) = *(undefined2 *)(iVar4 * 0x58 + 0x7598c6);
    }
    FUN_0045672e((int)local_3c,(byte *)s_DR_02dB_0046ce58,(uint)(&car_lookup)[iVar4]);
    Setup_Sprite(0,local_3c,puVar6);
    FUN_0045672e((int)local_3c,(byte *)s_DR_02dC_0046ce60,(uint)(&car_lookup)[iVar4]);
    iVar4 = iVar4 + 1;
    puVar6 = puVar6 + 0x2c;
    Setup_Sprite(0,local_3c,local_18);
    local_14 = local_14 + 0x2c;
    local_18 = local_18 + 0x2c;
  } while (iVar4 < 0x14);
  Setup_Sprite(0,s_BKWN88A_0046ce68,(ushort *)&hold);
  Setup_Sprite(0,s_BKWN88B_0046ce70,(ushort *)0x759840);
  Setup_Sprite(0,s_BKWN88C_0046ce78,(ushort *)0x759858);
  Setup_Sprite(0,s_BKWN88D_0046ce80,(ushort *)0x759870);
  iVar5 = 0;
  Setup_Sprite(0,s_BKWN88E_0046ce88,(ushort *)0x759888);
  iVar4 = 0;
  do {
    *(undefined2 *)(iVar4 + 0x759fbc) = *(undefined2 *)(iVar5 + 0x759834);
    *(undefined1 *)(iVar4 + 0x759fbe) = *(undefined1 *)(iVar5 + 0x75983a);
    puVar1 = (undefined1 *)(iVar5 + 0x75983b);
    iVar5 = iVar5 + 0x18;
    *(undefined1 *)(iVar4 + 0x759fbf) = *puVar1;
    iVar4 = iVar4 + 6;
  } while (iVar5 != 0x78);
  Setup_Sprite(0,s_BON88A_0046ce90,(ushort *)&hold);
  Setup_Sprite(0,s_BON88B_0046ce98,(ushort *)0x759840);
  Setup_Sprite(0,s_BON88C_0046cea0,(ushort *)0x759858);
  iVar5 = 0;
  Setup_Sprite(0,s_ENGINE_0046cea8,(ushort *)0x759870);
  iVar4 = 0;
  do {
    uVar2 = *(undefined1 *)(iVar5 + 0x75983a);
    *(undefined2 *)(iVar4 + 0x759ff8) = *(undefined2 *)(iVar5 + 0x759834);
    *(undefined1 *)(iVar4 + 0x759ffa) = uVar2;
    uVar3 = *(undefined2 *)(iVar5 + 0x759836);
    *(undefined1 *)(iVar4 + 0x759ffb) = *(undefined1 *)(iVar5 + 0x75983b);
    iVar5 = iVar5 + 0x18;
    *(undefined2 *)(iVar4 + 0x759ffc) = uVar3;
    iVar4 = iVar4 + 6;
  } while (iVar5 != 0x60);
  Setup_Sprite(0,s_BUMP88A_0046ceb0,(ushort *)&hold);
  Setup_Sprite(0,s_BUMP88B_0046ceb8,(ushort *)0x759840);
  Setup_Sprite(0,s_BUMP88C_0046cec0,(ushort *)0x759858);
  Setup_Sprite(0,s_BUMP88D_0046cec8,(ushort *)0x759870);
  iVar5 = 0;
  Setup_Sprite(0,s_BUMP88E_0046ced0,(ushort *)0x759888);
  iVar4 = 0;
  do {
    uVar2 = *(undefined1 *)(iVar5 + 0x75983a);
    *(undefined2 *)(iVar4 + 0x759fda) = *(undefined2 *)(iVar5 + 0x759834);
    *(undefined1 *)(iVar4 + 0x759fdc) = uVar2;
    puVar1 = (undefined1 *)(iVar5 + 0x75983b);
    iVar5 = iVar5 + 0x18;
    *(undefined1 *)(iVar4 + 0x759fdd) = *puVar1;
    iVar4 = iVar4 + 6;
  } while (iVar5 != 0x78);
  Setup_Sprite(0,s_BOOT88A_0046ced8,(ushort *)&hold);
  Setup_Sprite(0,s_BOOT88B_0046cee0,(ushort *)0x759840);
  Setup_Sprite(0,s_BOOT88C_0046cee8,(ushort *)0x759858);
  iVar5 = 0;
  Setup_Sprite(0,&DAT_0046cef0,(ushort *)0x759870);
  iVar4 = 0;
  do {
    uVar2 = *(undefined1 *)(iVar5 + 0x75983a);
    *(undefined2 *)(iVar4 + 0x75a022) = *(undefined2 *)(iVar5 + 0x759834);
    *(undefined1 *)(iVar4 + 0x75a024) = uVar2;
    uVar3 = *(undefined2 *)(iVar5 + 0x759836);
    *(undefined1 *)(iVar4 + 0x75a025) = *(undefined1 *)(iVar5 + 0x75983b);
    iVar5 = iVar5 + 0x18;
    *(undefined2 *)(iVar4 + 0x75a026) = uVar3;
    iVar4 = iVar4 + 6;
  } while (iVar5 != 0x60);
  Setup_Sprite(0,s_FRNT88A_0046cef8,(ushort *)&hold);
  Setup_Sprite(0,s_FRNT88B_0046cf00,(ushort *)0x759840);
  Setup_Sprite(0,s_FRNT88C_0046cf08,(ushort *)0x759858);
  Setup_Sprite(0,s_FRNT88D_0046cf10,(ushort *)0x759870);
  iVar5 = 0;
  Setup_Sprite(0,s_FRNT88E_0046cf18,(ushort *)0x759888);
  iVar4 = 0;
  do {
    uVar2 = *(undefined1 *)(iVar5 + 0x75983a);
    *(undefined2 *)(iVar4 + 0x759f80) = *(undefined2 *)(iVar5 + 0x759834);
    *(undefined1 *)(iVar4 + 0x759f82) = uVar2;
    puVar1 = (undefined1 *)(iVar5 + 0x75983b);
    iVar5 = iVar5 + 0x18;
    *(undefined1 *)(iVar4 + 0x759f83) = *puVar1;
    iVar4 = iVar4 + 6;
  } while (iVar5 != 0x78);
  Setup_Sprite(0,s_FRWN88A_0046cf20,(ushort *)&hold);
  Setup_Sprite(0,s_FRWN88B_0046cf28,(ushort *)0x759840);
  Setup_Sprite(0,s_FRWN88C_0046cf30,(ushort *)0x759858);
  Setup_Sprite(0,s_FRWN88D_0046cf38,(ushort *)0x759870);
  iVar5 = 0;
  Setup_Sprite(0,s_FRWN88E_0046cf40,(ushort *)0x759888);
  iVar4 = 0;
  do {
    uVar2 = *(undefined1 *)(iVar5 + 0x75983a);
    *(undefined2 *)(iVar4 + 0x759f9e) = *(undefined2 *)(iVar5 + 0x759834);
    *(undefined1 *)(iVar4 + 0x759fa0) = uVar2;
    puVar1 = (undefined1 *)(iVar5 + 0x75983b);
    iVar5 = iVar5 + 0x18;
    *(undefined1 *)(iVar4 + 0x759fa1) = *puVar1;
    iVar4 = iVar4 + 6;
  } while (iVar5 != 0x78);
  Setup_Sprite(0,s_ROOF88A_0046cf48,(ushort *)&hold);
  Setup_Sprite(0,s_ROOF88B_0046cf50,(ushort *)0x759840);
  iVar5 = 0;
  Setup_Sprite(0,s_ROOF88C_0046cf58,(ushort *)0x759858);
  iVar4 = 0;
  do {
    uVar2 = *(undefined1 *)(iVar5 + 0x75983a);
    *(undefined2 *)(iVar4 + 0x75a010) = *(undefined2 *)(iVar5 + 0x759834);
    *(undefined1 *)(iVar4 + 0x75a012) = uVar2;
    puVar1 = (undefined1 *)(iVar5 + 0x75983b);
    iVar5 = iVar5 + 0x18;
    *(undefined1 *)(iVar4 + 0x75a013) = *puVar1;
    iVar4 = iVar4 + 6;
  } while (iVar5 != 0x48);
  return;
}

/* ===== Set_Load_Textures @ 00445764 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void Set_Load_Textures(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  char local_20 [20];
  
  FUN_0045672e((int)local_20,(byte *)s_LEV_X_LEVEL_TX0_0046cf70,_current_level);
  iVar2 = 1;
  uVar3 = 0x75eb78;
  Add_Buffer_Load_(local_20,0x75eb70,3);
  while( true ) {
    FUN_0045672e((int)local_20,(byte *)s_LEV_X_LEVEL_TX_d_0046cf80,_current_level,iVar2);
    iVar1 = FUN_00415404(local_20);
    if (iVar1 == 0) break;
    iVar2 = iVar2 + 1;
    Add_Buffer_Load_(local_20,uVar3,4);
    uVar3 = uVar3 + 8;
  }
  return;
}

/* ===== FUN_0044a2b0 @ 0044a2b0 ===== */

/* WARNING: Removing unreachable block (ram,0x0044a2de) */

void __cdecl FUN_0044a2b0(undefined4 param_1,int param_2)

{
  ushort uVar1;
  char *pcVar2;
  
  uVar1 = *(ushort *)(&DAT_0073c084 + param_2 * 0x24);
  if (uVar1 == 0) {
    pcVar2 = s___R__JL__T_File__EMPTY_0046d408;
  }
  else {
    if (uVar1 < 2) {
      FUN_0045672e(0x901f1c,(byte *)s___R__JL__T_File__DD2____s_0046d3d4,
                   &DAT_0073c07a + param_2 * 0x24);
      PTR_DAT_00467284 = (undefined *)0x901f1c;
      PTR_DAT_00467298 = (undefined *)0x901f1c;
      return;
    }
    if (uVar1 != 2) {
      PTR_DAT_00467284 = (undefined *)0x901f1c;
      PTR_DAT_00467298 = (undefined *)0x901f1c;
      return;
    }
    pcVar2 = s___R__JL__T_File__USED_0046d3f0;
  }
  FUN_0045672e(0x901f1c,(byte *)pcVar2);
  PTR_DAT_00467298 = (undefined *)0x901f1c;
  PTR_DAT_00467284 = (undefined *)0x901f1c;
  return;
}

/* ===== Init_Graphics @ 00445a70 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void Init_Graphics(void)

{
  int iVar1;
  int iVar2;
  byte local_820 [2048];
  char local_20 [20];
  
  FUN_0041ff50(*(int *)(_level_data + 0x10) + 4);
  Modify_TDF();
  FUN_0045672e((int)local_20,(byte *)s_LEV_X_LEVEL_PAL_0046cf94,_current_level);
  File_Load(local_20,local_820);
  SetPalette(local_820);
  FUN_0045672e((int)local_20,(byte *)s_LEV_X_LEVEL_CLT_0046cfa4,_current_level);
  iVar1 = File_Load(local_20,__clutspace);
  FUN_0045672e((int)local_20,(byte *)s_LEV_X_LEVEL_ECL_0046cfb4,_current_level);
  iVar2 = FUN_00415404(local_20);
  if (iVar2 != 0) {
    File_Load(local_20,(void *)(iVar1 + (int)__clutspace));
  }
  return;
}

/* ===== FUN_00445b40 @ 00445b40 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00445b40(void)

{
  char local_18 [20];
  
  FUN_0045672e((int)local_18,(byte *)s_LEV_X_LEVEL_DAT_0046cfc4,_current_level);
  Add_Buffer_Load_(local_18,0x75eb60,0);
  return;
}

/* ===== FUN_0044b5c0 @ 0044b5c0 ===== */

void FUN_0044b5c0(void)

{
  FUN_0045672e(0x905a20,(byte *)s_CAMCORDR_0046d540);
  return;
}

/* ===== FUN_0042a4d8 @ 0042a4d8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_0042a4d8(int param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  uint uVar4;
  int *piVar5;
  byte *pbVar6;
  undefined4 uVar7;
  int local_34 [6];
  undefined1 local_1c [12];
  
  piVar3 = &DAT_0042a380;
  piVar5 = local_34;
  for (iVar2 = 6; iVar2 != 0; iVar2 = iVar2 + -1) {
    *piVar5 = *piVar3;
    piVar3 = piVar3 + 1;
    piVar5 = piVar5 + 1;
  }
  iVar2 = *(int *)(param_1 * 0x1b2 + 0x75a6ee);
  if (iVar2 != _DAT_00744de0) {
    local_34[0] = 1;
    _DAT_00744de0 = iVar2;
  }
  iVar2 = *(int *)(param_1 * 0x1b2 + 0x75a6f6);
  if (iVar2 != _DAT_00744de4) {
    local_34[1] = 1;
    _DAT_00744de4 = iVar2;
  }
  iVar2 = *(int *)(param_1 * 0x1b2 + 0x75a712);
  if (iVar2 != _DAT_00744de8) {
    local_34[2] = 1;
    _DAT_00744de8 = iVar2;
  }
  iVar2 = *(int *)(param_1 * 0x1b2 + 0x75a6fe);
  if (iVar2 != _DAT_00744dec) {
    local_34[3] = 1;
    _DAT_00744dec = iVar2;
  }
  iVar2 = *(int *)(param_1 * 0x1b2 + 0x75a70a);
  if (iVar2 != _DAT_00744df0) {
    local_34[4] = 1;
    _DAT_00744df0 = iVar2;
  }
  iVar2 = *(int *)(param_1 * 0x1b2 + 0x75a702);
  if (iVar2 != _DAT_00744df4) {
    local_34[5] = 1;
    _DAT_00744df4 = iVar2;
  }
  iVar2 = 0;
  uVar4 = 0;
  do {
    if (*(int *)((int)local_34 + iVar2) != 0) {
      iVar1 = *(int *)(&DAT_00744de0 + iVar2);
      if (iVar1 < 700) {
        uVar7 = *(undefined4 *)((int)&PTR_DAT_00464bf4 + iVar2);
        pbVar6 = &DAT_0046c9a4;
      }
      else if (iVar1 < 0x5aa) {
        uVar7 = *(undefined4 *)((int)&PTR_DAT_00464bf4 + iVar2);
        pbVar6 = &DAT_0046c9a8;
      }
      else if (iVar1 < 0x898) {
        uVar7 = *(undefined4 *)((int)&PTR_DAT_00464bf4 + iVar2);
        pbVar6 = &DAT_0046c9ac;
      }
      else if (iVar1 < 0xb86) {
        uVar7 = *(undefined4 *)((int)&PTR_DAT_00464bf4 + iVar2);
        pbVar6 = &DAT_0046c9b0;
      }
      else if (iVar1 < 0xe74) {
        uVar7 = *(undefined4 *)((int)&PTR_DAT_00464bf4 + iVar2);
        pbVar6 = &DAT_0046c9b4;
      }
      else if (iVar1 < 0x1000) {
        uVar7 = *(undefined4 *)((int)&PTR_DAT_00464bf4 + iVar2);
        pbVar6 = &DAT_0046c9b8;
      }
      else {
        uVar7 = *(undefined4 *)((int)&PTR_DAT_00464bf4 + iVar2);
        pbVar6 = &DAT_0046c9bc;
      }
      FUN_0045672e((int)local_1c,pbVar6,uVar7);
      if (uVar4 < 6) {
                    /* WARNING: Could not recover jumptable at 0x0042a6d3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
        (**(code **)((int)&PTR_LAB_0042a4c0 + iVar2))();
        return;
      }
    }
    uVar4 = uVar4 + 1;
    iVar2 = iVar2 + 4;
    if (5 < (int)uVar4) {
      return;
    }
  } while( true );
}

/* ===== Init_Debris" @ 00423f60 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void Init_Debris_(void)

{
  char cVar1;
  char cVar2;
  undefined2 uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  byte bVar7;
  char cVar8;
  undefined1 uVar9;
  char cVar10;
  byte bVar11;
  int iVar12;
  ushort local_70 [6];
  undefined2 auStack_64 [2];
  char acStack_60 [8];
  ushort local_58 [12];
  ushort local_40 [12];
  char local_28 [16];
  undefined4 local_18;
  byte local_14;
  
  old_index = 0;
  Setup_Sprite(0,s_BON88A_0046c934,local_70);
  Setup_Sprite(0,s_BON88B_0046c93c,local_58);
  for (bVar7 = 0; (int)(uint)bVar7 < num_cars; bVar7 = bVar7 + 1) {
    if (bVar7 == 0x12) {
      Setup_Sprite(0,s_CLUT01B_0046c968,local_40);
      uRam0073c2f4 = (undefined2)local_40._14_4_;
    }
    else {
      if ((bVar7 == 0) && (race_car == 0)) {
        FUN_0045672e((int)local_28,(byte *)s_CLUT_02dB_0046c944,(uint)car_lookup);
      }
      if ((bVar7 == 0) && (race_car == 1)) {
        FUN_0045672e((int)local_28,(byte *)s_CLT_02dB3_0046c950,(uint)car_lookup);
      }
      if ((bVar7 == 0) && (race_car == 2)) {
        FUN_0045672e((int)local_28,(byte *)s_CLT_02dB2_0046c95c,(uint)car_lookup);
      }
      if (bVar7 != 0) {
        FUN_0045672e((int)local_28,(byte *)s_CLUT_02dB_0046c944,(uint)(&car_lookup)[bVar7]);
      }
      Setup_Sprite(0,local_28,local_40);
      *(short *)(&debris_cluts + (uint)bVar7 * 2) = (short)local_40._14_4_;
    }
  }
  uVar5 = 0x1e00;
  do {
    uVar6 = uVar5 & 0xff;
    iVar4 = uVar6 * 0x7c;
    *(undefined1 *)(iVar4 + 0x73c30f) = 0x24;
    uVar9 = (undefined1)(uVar5 >> 8);
    *(undefined1 *)(iVar4 + 0x73c30c) = uVar9;
    *(undefined1 *)(iVar4 + 0x73c30d) = uVar9;
    *(undefined1 *)(iVar4 + 0x73c30e) = uVar9;
    *(undefined2 *)(&polygon_angles + uVar6 * 8) = 0;
    *(undefined1 *)(iVar4 + 0x73c32c) = uVar9;
    *(undefined2 *)(&DAT_0073c2fa + uVar6 * 8) = 0;
    *(undefined1 *)(iVar4 + 0x73c32d) = uVar9;
    *(undefined2 *)(&DAT_0073c2fc + uVar6 * 8) = 0;
    *(undefined1 *)(iVar4 + 0x73c32e) = uVar9;
    bVar7 = (char)uVar5 + 1;
    uVar5 = CONCAT31((int3)(uVar5 >> 8),bVar7);
    *(undefined1 *)(iVar4 + 0x73c32f) = 0x24;
  } while (bVar7 < 2);
  _DAT_0073c34a = 0xffe2;
  _DAT_0073c352 = 0x3c;
  _DAT_0073c354 = 0xffb0;
  _DAT_0073c35c = 0x3c;
  _DAT_0073c348 = 0;
  _DAT_0073c34c = 0xffce;
  _DAT_0073c350 = 0x32;
  _DAT_0073c358 = 0xffce;
  _DAT_0073c35a = 0x32;
  _DAT_0073c3c8 = 0xffce;
  _DAT_0073c3c6 = 0x1e;
  _DAT_0073c3cc = 0x32;
  _DAT_0073c3ce = 0xffc4;
  _DAT_0073c3d8 = 0xffc4;
  _DAT_0073c3c4 = 0xffec;
  _DAT_0073c3d0 = 0x50;
  bVar7 = 0;
  _DAT_0073c3d6 = 10;
  _DAT_0073c3d4 = 0xffce;
  do {
    iVar12 = (uint)bVar7 * 0x7c;
    *(undefined1 *)(iVar12 + 0x73c4e7) = 0x24;
    *(undefined1 *)(iVar12 + 0x73c4e4) = 0x80;
    *(undefined1 *)(iVar12 + 0x73c4e5) = 0x80;
    uVar5 = (uint)(bVar7 & 1);
    *(undefined1 *)(iVar12 + 0x73c4e6) = 0x80;
    iVar4 = uVar5 * 0x18;
    *(char *)(iVar12 + 0x73c4ec) = acStack_60[iVar4 + 2];
    cVar1 = acStack_60[iVar4 + 3];
    cVar2 = acStack_60[iVar4];
    *(char *)(iVar12 + 0x73c4ed) = cVar1;
    cVar10 = acStack_60[iVar4 + 2] + cVar2 + -1;
    *(char *)(iVar12 + 0x73c4f4) = cVar10;
    uVar3 = auStack_64[uVar5 * 0xc];
    *(char *)(iVar12 + 0x73c4f5) = cVar1;
    *(undefined2 *)(iVar12 + 0x73c4f6) = uVar3;
    uVar3 = auStack_64[uVar5 * 0xc + 1];
    *(char *)(iVar12 + 0x73c4fc) = acStack_60[iVar4 + 2];
    cVar1 = acStack_60[iVar4 + 1];
    *(undefined2 *)(iVar12 + 0x73c4ee) = uVar3;
    cVar2 = acStack_60[iVar4 + 3];
    *(undefined1 *)(iVar12 + 0x73c507) = 0x24;
    cVar8 = cVar2 + cVar1 + -1;
    *(char *)(iVar12 + 0x73c4fd) = cVar8;
    *(undefined1 *)(iVar12 + 0x73c504) = 0x80;
    *(undefined1 *)(iVar12 + 0x73c505) = 0x80;
    cVar1 = acStack_60[iVar4 + 2];
    *(undefined1 *)(iVar12 + 0x73c506) = 0x80;
    *(char *)(iVar12 + 0x73c50c) = cVar1;
    *(char *)(iVar12 + 0x73c514) = cVar10;
    cVar1 = acStack_60[iVar4 + 3];
    *(char *)(iVar12 + 0x73c50d) = cVar1;
    cVar2 = acStack_60[iVar4 + 2];
    *(char *)(iVar12 + 0x73c515) = cVar1;
    *(char *)(iVar12 + 0x73c51c) = cVar2;
    *(char *)(iVar12 + 0x73c51d) = cVar8;
    *(undefined1 *)(iVar12 + 0x73c558) = 0;
    uVar3 = auStack_64[uVar5 * 0xc + 1];
    *(undefined2 *)(iVar12 + 0x73c516) = auStack_64[uVar5 * 0xc];
    *(undefined2 *)(iVar12 + 0x73c50e) = uVar3;
    iVar4 = rand();
    *(byte *)(iVar12 + 0x73c559) = (byte)iVar4 & 1;
    iVar4 = rand();
    *(byte *)(iVar12 + 0x73c55a) = (byte)iVar4 & 0x7f;
    iVar4 = rand();
    bVar7 = bVar7 + 1;
    *(byte *)(iVar12 + 0x73c55b) = ((byte)iVar4 & 3) + 1;
  } while (bVar7 < 0x80);
  local_14 = 0;
  do {
    uVar5 = 0;
    _polygon_angles = _polygon_angles + 0x20;
    _DAT_0073c2fa = _DAT_0073c2fa + 0x20;
    _DAT_0073c2fc = _DAT_0073c2fc + -0x20;
    _DAT_0073c300 = _DAT_0073c300 + -0x20;
    _DAT_0073c302 = _DAT_0073c302 + 0x20;
    _DAT_0073c304 = _DAT_0073c304 + -0x20;
    do {
      FUN_004203a0((short *)&DAT_00463878,(int *)(&polygon_angles + (uVar5 >> 8) * 8));
      gte_SetRotMatrix((undefined2 *)&DAT_00463878);
      _DAT_00714302 = DAT_0046388a;
      _DAT_00714306 = DAT_0046388e;
      _DAT_0071430a = DAT_00463892;
      uVar5 = uVar5 & 0xffffff00;
      do {
        uVar6 = uVar5;
        bVar7 = (char)uVar6 + 1;
        RotTrans((int *)(&DAT_0073c348 + (uVar6 & 0xff) * 8 + (uVar6 >> 8) * 0x7c),
                 (int *)(&Anim + (uVar6 & 0xff) * 0x10 +
                                 (uint)local_14 * 0x30 + (uVar6 >> 8) * 0x1800),&local_18);
        uVar5 = CONCAT31((int3)(uVar6 >> 8),bVar7);
      } while (bVar7 < 3);
      bVar11 = (char)(uVar6 >> 8) + 1;
      uVar5 = (uint)CONCAT11(bVar11,bVar7);
    } while (bVar11 < 2);
    local_14 = local_14 + 1;
  } while (local_14 < 0x80);
  return;
}

/* ===== Loading_Screen_From_Slab @ 0044b5d4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl Loading_Screen_From_Slab(int param_1)

{
  char *pcVar1;
  
  if (param_1 == 0) {
    PTR_DAT_00467520 = &DAT_0046d4cc;
  }
  else {
    FUN_0045672e(0x905a30,(byte *)s___R__JC__T__s_to_race_next___0046d54c,param_1);
    PTR_DAT_00467520 = (undefined *)0x905a30;
  }
  if ((_current_level < 1) || (0xc < _current_level)) {
    Setup_Pad(0);
    pcVar1 = s_CAMCORDR_0046d540;
  }
  else {
    Setup_Pad(1);
    pcVar1 = *(char **)(&DAT_00467534 + _current_level * 4);
  }
  FUN_0045672e(0x905a20,(byte *)pcVar1);
  FUN_00450e20(0x4674bc);
  Setup_Screen_Text((int *)&DAT_004674e8);
  Setup_Screen_Lines(&DAT_00467498);
  Rotate_Slab_On((short *)0x0);
  return;
}

/* ===== FUN_0044d538 @ 0044d538 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0044d538(void)

{
  short *psVar1;
  char *pcVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  byte *pbVar6;
  
  iVar4 = 0;
  iVar5 = 0;
  pbVar6 = &champ_info;
  do {
    if (((*(int *)(&DAT_0075a6c6 + iVar5) == 1) || (*(int *)(&DAT_0075a692 + iVar5) == 5)) &&
       (*(short *)(&DAT_0075d852 + iVar4) == 0)) {
      psVar1 = (short *)(iVar4 + 0x9064ae + _DAT_004682f0 * 0x3ac);
      *psVar1 = *psVar1 + 1;
    }
    if (*(short *)(iVar4 + 0x75d842) == 1) {
      pcVar2 = (char *)(iVar4 + 0x9064ac + _DAT_004682f0 * 0x3ac);
      *pcVar2 = *pcVar2 + '\x01';
    }
    iVar3 = _DAT_004682f0 * 0x3ac;
    pcVar2 = (char *)(iVar4 + 0x9064ad + iVar3);
    *pcVar2 = *pcVar2 + *(char *)(iVar4 + 0x75d850);
    iVar5 = iVar5 + 0x1b2;
    FUN_0045672e(iVar3 + 0x90649c + iVar4,pbVar6);
    iVar4 = iVar4 + 0x14;
    pbVar6 = pbVar6 + 0x36;
  } while (iVar4 != 400);
  return;
}

/* ===== FUN_0044fce8 @ 0044fce8 ===== */

void __cdecl FUN_0044fce8(undefined1 *param_1,int param_2)

{
  *param_1 = 0;
  if ((0x40 < param_2) && (param_2 < 0x5b)) {
    FUN_0045672e((int)param_1,(byte *)s___JL__T__c_0046e248,param_2);
  }
  if ((0x2f < param_2) && (param_2 < 0x3a)) {
    FUN_0045672e((int)param_1,(byte *)s___JL__T__c_0046e248,param_2);
  }
  if (param_2 == 0x20) {
    FUN_0045672e((int)param_1,(byte *)s___JL__T_Space_0046e254);
  }
  if (param_2 == 0x25) {
    FUN_0045672e((int)param_1,(byte *)s___JL__T_Left_0046e264);
  }
  if (param_2 == 0x27) {
    FUN_0045672e((int)param_1,(byte *)s___JL__T_Right_0046e274);
  }
  return;
}

/* ===== FUN_0044ff48 @ 0044ff48 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0044ff48(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar3 = 0x907b50;
  iVar4 = 0x907ab0;
  PTR_DAT_004693e0 = (&PTR_s__R_JL_T_Pine_Hills_Raceway_00469314)[DAT_00469560];
  iVar2 = 0;
  do {
    iVar1 = DAT_00469560 * 0x50;
    _value = (uint)*(ushort *)((int)&DAT_004680ca + iVar2 + iVar1);
    _DAT_0071bfac = (uint)*(ushort *)((int)&DAT_004680cc + iVar2 + iVar1);
    _DAT_0071bfb0 = (int)((uint)*(ushort *)((int)&DAT_004680ce + iVar2 + iVar1) * 100) >> 0x10;
    FUN_0045672e(iVar4,(byte *)s___R__JL__T__s_0046e3dc,(int)&fastest_laps + iVar2 + iVar1);
    iVar2 = iVar2 + 0x10;
    iVar4 = iVar4 + 0x20;
    FUN_0045672e(iVar3,(byte *)s___R__JL__T__d__02d__02d_0046e3ec,_value,_DAT_0071bfac,_DAT_0071bfb0
                );
    iVar3 = iVar3 + 0x20;
  } while (iVar2 != 0x50);
  return;
}

/* ===== FUN_0044a3a0 @ 0044a3a0 ===== */

undefined4 __cdecl FUN_0044a3a0(undefined4 param_1,int param_2,undefined4 *param_3)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  char local_20 [12];
  undefined4 local_14;
  
  local_20[0] = '\0';
  iVar3 = param_2 * 0x24;
  local_14 = 0;
  if (*(short *)(&DAT_0073c084 + iVar3) == 0) {
    while( true ) {
      iVar3 = FUN_0044a4e4(local_20);
      uVar2 = 0;
      if (iVar3 == 0) break;
      iVar3 = DupFileCheck(param_1,local_20,param_2);
      if (iVar3 == 0) goto LAB_0044a472;
      FUN_00449c54(6);
    }
  }
  else {
    iVar1 = FUN_00449ee0(1);
    uVar2 = 0;
    if (iVar1 != 0) {
      while( true ) {
        if (*(short *)(&DAT_0073c084 + iVar3) == 1) {
          FUN_0045672e((int)local_20,&DAT_0073c07a + iVar3);
        }
        iVar1 = FUN_0044a4e4(local_20);
        if (iVar1 == 0) {
          return 0;
        }
        iVar1 = DupFileCheck(param_1,local_20,param_2);
        if (iVar1 == 0) break;
        FUN_00449c54(6);
      }
      iVar3 = DeleteFileMC(&card_data + param_2 * 0x24);
      if (iVar3 == 0) {
        FUN_00449c54(3);
        uVar2 = 0;
      }
      else {
        local_14 = 1;
LAB_0044a472:
        FUN_00449dd8(1);
        iVar3 = SaveCardFile(local_20,param_1,param_3);
        if (iVar3 == 0) {
          FUN_00449c54(3);
          return local_14;
        }
        uVar2 = 1;
      }
    }
  }
  return uVar2;
}

/* ===== FUN_0044a4e4 @ 0044a4e4 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __cdecl FUN_0044a4e4(char *param_1)

{
  char cVar1;
  undefined4 *puVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  undefined4 uVar6;
  char *pcVar7;
  undefined1 local_30 [20];
  int local_1c;
  int local_18;
  int local_14;
  
  local_14 = 0;
  local_18 = 0;
  _DAT_004673de = (short)pal_flag * 8;
  local_1c = 0;
  _DAT_00467396 = _DAT_004673de + 0x67;
  _DAT_004673ae = _DAT_004673de + 0x7a;
  _DAT_004673de = _DAT_004673de + 0x66;
  _DAT_004673c6 = _DAT_004673ae;
  Setup_Sprite(0x901f9c,s_CARD_ERR_0046d174,(ushort *)&DAT_00467168);
  FUN_004166c4(0x901fc4,s_CARDTEXT_0046d420,(ushort *)&DAT_00467394);
  Setup_Sprite(0x901fec,s_BACKSPCE_0046d42c,(ushort *)&DAT_004673ac);
  Setup_Sprite(0x902014,s_TICK2_0046d438,(ushort *)&DAT_004673c4);
  Setup_Sprite(0x90203c,s_SMALRING_0046d2fc,(ushort *)&DAT_00467198);
  FUN_004166c4(0x902064,s_CARDCURS_0046d440,(ushort *)&DAT_004673dc);
  DAT_00901fc8 = 0;
  DAT_00901fc9 = 0;
  DAT_00901fca = 0;
  uVar4 = 0xffffffff;
  pcVar7 = param_1;
  do {
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar7 + 1;
  } while (cVar1 != '\0');
  iVar5 = ~uVar4 - 1;
  FUN_0045672e((int)local_30,(byte *)s___R__JC__T__s_0046d44c,param_1);
  PTR_DAT_004672ac = local_30;
  _DAT_004672a6 = 0xffe6;
  PTR_DAT_004672c0 = &DAT_0046d0a4;
  Setup_Screen_Text((int *)&DAT_00467224);
  do {
    Debug_Stub();
    FUN_00420b1c();
    FUN_00422c74();
    if ((_DAT_0071c04a & 0x80) == 0) {
      if ((_DAT_0071c04a & 0x20) != 0) {
        FUN_00451a80();
        Play_Click_FX();
        if (local_18 == 2) {
          local_1c = 1;
        }
        else if (local_14 < 0xc) {
          local_14 = local_14 + 1;
        }
      }
    }
    else {
      FUN_00451a80();
      Play_Click_FX();
      if (local_18 == 2) {
        local_1c = 0;
      }
      else if (0 < local_14) {
        local_14 = local_14 + -1;
      }
    }
    if ((_DAT_0071c04a & 0x10) == 0) {
      if ((_DAT_0071c04a & 0x40) == 0) {
        if ((_DAT_0071c04a & 0x4000) == 0) {
          if ((_DAT_0071c04a & 0x1000) != 0) {
            uVar6 = 0;
LAB_0044a8d2:
            _DAT_004672a6 = 0xfff3;
            PTR_DAT_004672ac = &DAT_0046d0a4;
            PTR_DAT_004672c0 = &DAT_0046d0a4;
            Setup_Screen_Text((int *)&DAT_00467224);
            return uVar6;
          }
        }
        else {
          FUN_00451ae0();
          if (local_18 < 2) {
            if (iVar5 < 8) {
              param_1[iVar5 + 1] = '\0';
              param_1[iVar5] = (char)local_14 + (char)local_18 * '\r' + 'A';
              iVar5 = iVar5 + 1;
            }
          }
          else {
            if (local_1c != 0) {
              uVar6 = 1;
              goto LAB_0044a8d2;
            }
            if (0 < iVar5) {
              iVar5 = iVar5 + -1;
              param_1[iVar5] = '\0';
            }
          }
          FUN_0045672e((int)local_30,(byte *)s___R__JC__T__s_0046d44c,param_1);
          Setup_Screen_Text((int *)&DAT_00467224);
        }
      }
      else {
        FUN_00451a80();
        Play_Click_FX();
        if (local_18 < 2) {
          local_18 = local_18 + 1;
        }
        if (local_18 == 2) {
          if (local_14 < 7) {
            local_1c = 0;
          }
          else {
            local_1c = 1;
          }
        }
      }
    }
    else {
      FUN_00451a80();
      iVar3 = local_18;
      Play_Click_FX();
      if (0 < iVar3) {
        local_18 = iVar3 + -1;
      }
    }
    if (local_18 < 2) {
      _DAT_004673dc = (undefined2)(local_14 * 8 + 0x6c);
      _DAT_004673de = (short)local_18 * 9 + 0x66 + (short)pal_flag * 8;
      FUN_004166c4(0x902064,s_CARDCURS_0046d440,(ushort *)&DAT_004673dc);
    }
    else {
      if (local_1c == 0) {
        _DAT_00467198 = 0x86;
      }
      else {
        _DAT_00467198 = 0xa3;
      }
      _DAT_0046719a = (short)pal_flag * 8 + 0x7a;
      Setup_Sprite(0x90203c,s_SMALRING_0046d2fc,(ushort *)&DAT_00467198);
    }
    puVar2 = (undefined4 *)(*(int *)(_cdb_ + 0x8a) + 8);
    _DAT_00901f9c = *puVar2;
    *puVar2 = &DAT_00901f9c;
    if (local_18 == 2) {
      iVar3 = *(int *)(_cdb_ + 0x8a);
      puVar2 = (undefined4 *)&DAT_0090203c;
    }
    else {
      iVar3 = *(int *)(_cdb_ + 0x8a);
      puVar2 = (undefined4 *)&DAT_00902064;
    }
    uVar6 = *(undefined4 *)(iVar3 + 4);
    *(undefined4 *)(iVar3 + 4) = puVar2;
    *puVar2 = uVar6;
    puVar2 = (undefined4 *)(*(int *)(_cdb_ + 0x8a) + 4);
    _DAT_00901fc4 = *puVar2;
    *puVar2 = &DAT_00901fc4;
    puVar2 = (undefined4 *)(*(int *)(_cdb_ + 0x8a) + 4);
    _DAT_00901fec = *puVar2;
    *puVar2 = &DAT_00901fec;
    puVar2 = (undefined4 *)(*(int *)(_cdb_ + 0x8a) + 4);
    _DAT_00902014 = *puVar2;
    *puVar2 = &DAT_00902014;
    FUN_00449958();
    Draw_Slab();
    Print_Draw();
    Draw_Screen_Polys();
    Draw_Screen_Lines();
    Draw_All(1);
  } while( true );
}

/* ===== FUN_0044b970 @ 0044b970 ===== */

void FUN_0044b970(void)

{
  Setup_Pad(0);
  FUN_0045672e(0x905a20,(byte *)s_CAMCORDR_0046d540);
  FUN_00450e20(0x4674bc);
  Setup_Screen_Text((int *)&DAT_004674e8);
  Setup_Screen_Lines(&DAT_00467498);
  FUN_00450c7c((short *)0x0);
  return;
}

/* ===== Update_Track_Stats @ 0044d478 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl Update_Track_Stats(int param_1)

{
  short *psVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  byte *local_14;
  
  iVar3 = 0;
  iVar2 = 0;
  local_14 = &champ_info;
  iVar4 = *(int *)(&DAT_00468094 + param_1 * 4) * 0x14;
  do {
    if (*(short *)(iVar2 + 0x75d842) == 1) {
      FUN_0045672e((int)(&season_statistics + iVar4 + _DAT_004682f0 * 0x3ac),local_14);
    }
    if (((*(int *)(&DAT_0075a6c6 + iVar3) == 1) || (*(int *)(&DAT_0075a692 + iVar3) == 5)) &&
       (*(short *)(&DAT_0075d852 + iVar2) == 0)) {
      psVar1 = (short *)(iVar4 + 0x9063d2 + _DAT_004682f0 * 0x3ac);
      *psVar1 = *psVar1 + 1;
    }
    psVar1 = (short *)(iVar4 + 0x9063d0 + _DAT_004682f0 * 0x3ac);
    *psVar1 = *psVar1 + *(short *)(iVar2 + 0x75d850);
    iVar3 = iVar3 + 0x1b2;
    local_14 = local_14 + 0x36;
    iVar2 = iVar2 + 0x14;
  } while (iVar2 != 400);
  return;
}

/* ===== Update_Championship_Stats @ 0044d5d8 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void Update_Championship_Stats(void)

{
  int iVar1;
  int iVar2;
  byte *pbVar3;
  
  iVar2 = 0;
  pbVar3 = &champ_info;
  do {
    iVar1 = iVar2 * 0x36;
    iVar2 = iVar2 + 1;
    FUN_0045672e(_DAT_004682f0 * 0x3ac + 0x90662c +
                 ((*(int *)(&DAT_00905af2 + iVar1) >> 0x10) +
                 (*(int *)(iVar1 + 0x905af0) >> 0x10) * 5) * 0x10,pbVar3);
    pbVar3 = pbVar3 + 0x36;
  } while (iVar2 < 0x14);
  return;
}

/* ===== FUN_00455260 @ 00455260 ===== */

void FUN_00455260(void)

{
  int iVar1;
  int iVar2;
  undefined1 *puVar3;
  int iVar4;
  
  puVar3 = &champ_info;
  iVar2 = 0;
  do {
    iVar1 = iVar2 * 0x36;
    iVar4 = (*(int *)(iVar1 + 0x905af8) >> 0x10) + -1;
    FUN_0045672e(iVar4 * 0x1a + 0x908360,&DAT_0046f88c,s__R_JL_T__0046be14,puVar3);
    puVar3 = puVar3 + 0x36;
    iVar2 = iVar2 + 1;
    FUN_0045672e(iVar4 * 0xc + 0x908270,&DAT_0046f894,s__R_JL_T__0046be14,
                 *(int *)(iVar1 + 0x905afa) >> 0x10);
  } while (iVar2 < 0x14);
  return;
}

/* ===== Setup_Driver_Names @ 0044c390 ===== */

void Setup_Driver_Names(void)

{
  int iVar1;
  undefined2 *puVar2;
  undefined1 *puVar3;
  undefined1 *puVar4;
  
  iVar1 = 0;
  if (0 < DAT_00467658) {
    puVar3 = &champ_info;
    puVar4 = &player_names;
    do {
      iVar1 = iVar1 + 1;
      FUN_0045672e((int)puVar3,&DAT_0046d594,puVar4);
      puVar3 = puVar3 + 0x36;
      puVar4 = puVar4 + 0xc;
    } while (iVar1 < DAT_00467658);
  }
  if (DAT_00467658 < 0x14) {
    puVar4 = &champ_info + DAT_00467658 * 0x36;
    iVar1 = DAT_00467658;
    puVar2 = &num_races + DAT_00467658 * 8;
    do {
      iVar1 = iVar1 + 1;
      FUN_0045672e((int)puVar4,&DAT_0046d594,puVar2);
      puVar4 = puVar4 + 0x36;
      puVar2 = puVar2 + 8;
    } while (iVar1 < 0x14);
  }
  return;
}

/* ===== FUN_0044ccb8 @ 0044ccb8 ===== */

void FUN_0044ccb8(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar3 = 0x905fb0;
  iVar2 = 0;
  do {
    iVar1 = DAT_00467bf4 * 0x3ac + 0x90662c + iVar2;
    iVar2 = iVar2 + 0x10;
    FUN_0045672e(iVar3,(byte *)s___R__JL__T__s_0046d714,iVar1);
    iVar3 = iVar3 + 0x20;
  } while (iVar2 != 0x140);
  iVar2 = FUN_0044d630();
  FUN_0045672e(0x905f90,(byte *)s___R__JL__T_Season__d_0046d724,iVar2 + DAT_00467bf4);
  return;
}

/* ===== FUN_0044ced4 @ 0044ced4 ===== */

void FUN_0044ced4(void)

{
  FUN_0045672e(0x9063a0,(byte *)s_DRIVER_d_0046d79c,DAT_00467eec);
  return;
}

/* ===== FUN_0044cef0 @ 0044cef0 ===== */

void FUN_0044cef0(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int local_28;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  
  local_28 = FUN_0044d630();
  iVar2 = Get_Current_Recording_Season();
  iVar3 = 0;
  if (0 < iVar2) {
    local_1c = 0x906350;
    local_14 = 0x9062a0;
    local_18 = 0x906230;
    iVar5 = 0x906300;
    iVar4 = 0;
    do {
      FUN_0045672e(iVar5,(byte *)s___R__JL__T__d_0046d7a8,local_28);
      FUN_0045672e(local_14,(byte *)s___R__JL__T__d_0046d7a8,
                   *(int *)(iVar4 + 0x9064a9 + DAT_00467eec * 0x14) >> 0x18);
      FUN_0045672e(local_1c,(byte *)s___R__JL__T__d_0046d7a8,
                   *(int *)(iVar4 + 0x9064aa + DAT_00467eec * 0x14) >> 0x18);
      iVar1 = iVar4 + 0x9064ac;
      iVar4 = iVar4 + 0x3ac;
      iVar3 = iVar3 + 1;
      iVar5 = iVar5 + 0x10;
      FUN_0045672e(local_18,(byte *)s___R__JL__T__d_0046d7a8,
                   *(int *)(iVar1 + DAT_00467eec * 0x14) >> 0x10);
      local_28 = local_28 + 1;
      local_18 = local_18 + 0x10;
      local_1c = local_1c + 0x10;
      local_14 = local_14 + 0x10;
    } while (iVar3 < iVar2);
  }
  iVar2 = iVar3 * 0x10;
  iVar5 = iVar2 + 0x906230;
  iVar4 = iVar2 + 0x906350;
  local_20 = iVar2 + 0x9062a0;
  iVar2 = iVar2 + 0x906300;
  for (; iVar3 < 5; iVar3 = iVar3 + 1) {
    FUN_0045672e(iVar2,&DAT_0046d7b8);
    FUN_0045672e(local_20,&DAT_0046d7b8);
    FUN_0045672e(iVar4,&DAT_0046d7b8);
    iVar4 = iVar4 + 0x10;
    FUN_0045672e(iVar5,&DAT_0046d7b8);
    iVar5 = iVar5 + 0x10;
    local_20 = local_20 + 0x10;
    iVar2 = iVar2 + 0x10;
  }
  FUN_0044d0e4();
  return;
}

/* ===== FUN_0044d0e4 @ 0044d0e4 ===== */

void FUN_0044d0e4(void)

{
  FUN_0045672e(0x906280,(byte *)s___R__JL__T__s_0046d7bc,DAT_00467eec * 0x14 + 0x90649c);
  FUN_0045672e(0x9062f0,(byte *)s___R__JC__T_Car__02d_0046d7cc,
               (uint)(byte)(&car_lookup)[DAT_00467eec]);
  return;
}

/* ===== Enter_Driver_Names @ 004520f0 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl Enter_Driver_Names(int param_1,int param_2)

{
  char cVar1;
  undefined1 uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  char *pcVar6;
  int iVar7;
  char *pcVar8;
  undefined1 local_4c [40];
  int local_24;
  int local_20;
  char *local_1c;
  int local_18;
  int local_14;
  
  iVar4 = 0;
  local_18 = 0;
  local_20 = 0;
  if (param_2 < 1) {
LAB_00452502:
    iVar4 = local_20;
    FUN_00450c7c((short *)0x0);
    if (0 < iVar4) {
      return iVar4;
    }
    return 1;
  }
  local_24 = param_1 + -1;
  local_1c = &player_names;
LAB_0045211b:
  if (param_1 == 0) {
    PTR_DAT_0046a024 = s__R_JC_T_Fastest_Lap_0046a054;
  }
  else {
    PTR_DAT_0046a024 = &DAT_0046ecdc;
  }
  if (param_1 < 2) {
    PTR_DAT_0046a010 = s__R_JC_T_Enter_your_name_0046a03c;
  }
  else {
    iVar5 = local_20 + 1;
    FUN_0045672e((int)local_4c,(byte *)s__s_player__d_0046ece0,s__R_JC_T_Enter_your_name_0046a03c,
                 iVar5);
    FUN_0045672e((int)local_4c,(byte *)s__s_player__d_0046ece0,s__R_JC_T_Enter_your_name_0046a03c,
                 iVar5);
    PTR_DAT_0046a010 = local_4c;
  }
  PTR_DAT_00469ffc = PTR_DAT_0046a010;
  PTR_DAT_00469fd4[8] = 0;
  _DAT_00469f34 = 0x30;
  _DAT_00469f36 = 0x7b;
  PTR_DAT_00469fe8[8] = 0;
  FUN_00450e20(0x469f70);
  Setup_Screen_Text((int *)&DAT_00469f9c);
  iVar7 = 0;
  iVar5 = 0;
  local_14 = 0;
  Setup_Screen_Lines(&DAT_00469f4c);
  if (local_20 == 0) {
    Rotate_Slab_On((short *)0x0);
  }
  do {
    Debug_Stub();
    FUN_00420b1c();
    FUN_00422c74();
    if (0 < iVar4) {
      iVar4 = iVar4 + -1;
    }
    if ((_DAT_0071c048 & 0x80) == 0) {
      if ((_DAT_0071c048 & 0x20) == 0) {
        if ((_DAT_0071c048 & 0x10) == 0) {
          if ((_DAT_0071c048 & 0x40) == 0) {
            if ((_DAT_0071c04a & 0x1000) != 0) {
              FUN_00450c7c((short *)0x0);
              return 0;
            }
            if ((_DAT_0071c04a & 0x4000) == 0) {
              iVar4 = 0;
            }
            else {
              FUN_00451ae0();
              iVar3 = *(int *)(s__R_JC_T_Fastest_Lap_0046a054 + iVar5 * 0xe + iVar7 + 0x11) >> 0x18;
              if (iVar3 == -2) break;
              if (iVar3 == -1) {
                if (0 < local_14) {
                  PTR_DAT_00469fd4[local_14 + 7] = 0;
                  local_18 = 1;
                  PTR_DAT_00469fe8[local_14 + 7] = 0;
                  local_14 = local_14 + -1;
                }
              }
              else if (local_14 < 8) {
                uVar2 = (undefined1)
                        ((uint)*(int *)(s__R_JC_T_Fastest_Lap_0046a054 + iVar5 * 0xe + iVar7 + 0x11)
                        >> 0x18);
                PTR_DAT_00469fd4[local_14 + 8] = uVar2;
                PTR_DAT_00469fe8[local_14 + 8] = uVar2;
                PTR_DAT_00469fd4[local_14 + 9] = 0;
                local_18 = 1;
                PTR_DAT_00469fe8[local_14 + 9] = 0;
                local_14 = local_14 + 1;
              }
            }
          }
          else if (iVar4 < 2) {
            if (iVar4 == 1) {
              iVar4 = 5;
            }
            else {
              iVar4 = 0x14;
            }
            if (iVar5 < 4) {
              iVar5 = iVar5 + 1;
            }
            else {
              iVar5 = 0;
            }
            local_18 = 1;
          }
        }
        else if (iVar4 < 2) {
          if (iVar4 == 1) {
            iVar4 = 5;
          }
          else {
            iVar4 = 0x14;
          }
          if (iVar5 < 1) {
            iVar5 = 4;
          }
          else {
            iVar5 = iVar5 + -1;
          }
          local_18 = 1;
        }
      }
      else if (iVar4 < 2) {
        if (iVar4 == 1) {
          iVar4 = 5;
        }
        else {
          iVar4 = 0x14;
        }
        if (iVar7 < 0xd) {
          iVar7 = iVar7 + 1;
        }
        else {
          iVar7 = 0;
        }
        local_18 = 1;
      }
    }
    else if (iVar4 < 2) {
      if (iVar4 == 1) {
        iVar4 = 5;
      }
      else {
        iVar4 = 0x14;
      }
      if (iVar7 < 1) {
        iVar7 = 0xd;
      }
      else {
        iVar7 = iVar7 + -1;
      }
      local_18 = 1;
    }
    if (local_18 != 0) {
      local_18 = 0;
      FUN_00451a80();
      Play_Click_FX();
      _DAT_00469f34 = (short)(iVar7 << 4) + 0x30;
      _DAT_00469f36 = (short)iVar5 * 0x11;
      if (iVar5 < 0x33) {
        _DAT_00469f36 = _DAT_00469f36 + 0x7b;
      }
      else {
        _DAT_00469f36 = _DAT_00469f36 + 0x7c;
      }
      Setup_Screen_Text((int *)&DAT_00469f9c);
      FUN_00450e20(0x469f70);
    }
    Draw_Slab();
    Print_Draw();
    Draw_Screen_Polys();
    Draw_Screen_Lines();
    Draw_All(1);
  } while( true );
  if (param_1 == 0) {
    if (local_14 == 0) {
      lap_name_entry = s_Anonymous_0046ecf0[0];
      DAT_009063b1 = s_Anonymous_0046ecf0[1];
      DAT_009063b2 = s_Anonymous_0046ecf0[2];
      DAT_009063b3 = s_Anonymous_0046ecf0[3];
      DAT_009063b4 = s_Anonymous_0046ecf0[4];
      DAT_009063b4_1._0_1_ = s_Anonymous_0046ecf0[5];
      DAT_009063b4_1._1_1_ = s_Anonymous_0046ecf0[6];
      DAT_009063b4_1._2_1_ = s_Anonymous_0046ecf0[7];
      DAT_009063b8 = s_Anonymous_0046ecf0[8];
      DAT_009063b8_1 = s_Anonymous_0046ecf0[9];
      goto LAB_00452495;
    }
    pcVar8 = &lap_name_entry;
  }
  else {
    pcVar8 = local_1c;
    if (local_14 == 0) {
      FUN_0045672e((int)local_1c,(byte *)s_Player__d_0046ecfc,local_20 + 1);
      goto LAB_00452495;
    }
  }
  pcVar6 = PTR_DAT_00469fd4 + 8;
  do {
    cVar1 = *pcVar6;
    *pcVar8 = cVar1;
    if (cVar1 == '\0') break;
    cVar1 = pcVar6[1];
    pcVar6 = pcVar6 + 2;
    pcVar8[1] = cVar1;
    pcVar8 = pcVar8 + 2;
  } while (cVar1 != '\0');
LAB_00452495:
  iVar5 = strcmp(local_1c,s_CREDITZ__0046ed08);
  if (iVar5 == 0) {
    Play_Xtro();
  }
  iVar5 = strcmp(local_1c,s_MACSrPOO_0046ed14);
  if (iVar5 == 0) {
    _playable_bowls = 4;
    _playable_tracks_ = 7;
  }
  if ((local_14 == 0) && (local_24 <= local_20)) goto LAB_00452502;
  local_1c = local_1c + 0xc;
  local_20 = local_20 + 1;
  if (param_2 <= local_20) goto LAB_00452502;
  goto LAB_0045211b;
}

/* ===== FUN_0044d974 @ 0044d974 ===== */

void FUN_0044d974(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined1 *puVar4;
  int iVar5;
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  
  local_28 = FUN_0044d630();
  iVar1 = Get_Current_Recording_Season();
  iVar3 = 0;
  if (0 < iVar1) {
    iVar5 = 0x907720;
    local_20 = 0x907798;
    local_1c = 0x907680;
    local_18 = 0x90775c;
    puVar4 = &season_statistics;
    do {
      FUN_0045672e(local_18,(byte *)s___R__JC__T__d_0046da44,local_28);
      iVar2 = iVar3 * 0x3ac;
      FUN_0045672e(local_1c,(byte *)s___R__JL__T__s_0046da54,puVar4 + DAT_004685e0 * 0x14);
      FUN_0045672e(local_20,(byte *)s___R__JL__T__d_0046da64,
                   *(int *)(DAT_004685e0 * 0x14 + iVar2 + 0x9063ce) >> 0x10);
      iVar3 = iVar3 + 1;
      puVar4 = puVar4 + 0x3ac;
      FUN_0045672e(iVar5,(byte *)s___R__JL__T__d_0046da64,
                   *(int *)(DAT_004685e0 * 0x14 + iVar2 + 0x9063d0) >> 0x10);
      iVar5 = iVar5 + 0xc;
      local_28 = local_28 + 1;
      local_20 = local_20 + 0xc;
      local_1c = local_1c + 0x20;
      local_18 = local_18 + 0xc;
    } while (iVar3 < iVar1);
  }
  iVar1 = iVar3 * 0xc;
  local_14 = iVar1 + 0x907720;
  iVar5 = iVar1 + 0x907798;
  local_24 = iVar3 * 0x20 + 0x907680;
  iVar1 = iVar1 + 0x90775c;
  for (; iVar3 < 5; iVar3 = iVar3 + 1) {
    FUN_0045672e(iVar1,&DAT_0046da04);
    FUN_0045672e(local_24,&DAT_0046da04);
    FUN_0045672e(iVar5,&DAT_0046da04);
    iVar1 = iVar1 + 0xc;
    iVar5 = iVar5 + 0xc;
    FUN_0045672e(local_14,&DAT_0046da04);
    local_14 = local_14 + 0xc;
    local_24 = local_24 + 0x20;
  }
  PTR_DAT_004683e8 = (&PTR_s__R_JL_T_Pine_Hills_Raceway_004682f8)[DAT_004685e0];
  return;
}

/* ===== FUN_00452090 @ 00452090 ===== */

void FUN_00452090(void)

{
  FUN_0045672e(0x907e00,(byte *)s___R__JL__T_Track__d___0046ec74,DAT_00469efc + 1);
  PTR_s__R_JL_T_Liberty_City_00469dc8 = (&PTR_s__R_JL_T_Pine_Hills_Raceway_00469bc4)[DAT_00469efc];
  PTR_s__R_JL_T_Pork_Sword_00469ddc = (&PTR_s__R_JL_T_Slapshot_00469c0c)[DAT_00469efc];
  if (*(int *)(DAT_00469efc * 4 + 0x469c54) == 0) {
    PTR_s__R_JL_T_Jug_00469df0 = s__R_JL_T_Jug_0046ebc0;
    return;
  }
  PTR_s__R_JL_T_Jug_00469df0 = s__R_JL_T_Tuscan_0046ec8c;
  return;
}

/* ===== Select_ChampQS @ 004530bc ===== */

undefined4 Select_ChampQS(void)

{
  FUN_0045672e(0x905f18,(byte *)s_Player_0046eff8);
  DAT_00467658 = 1;
  race_track = 0;
  num_cars = 0x14;
  race_type = 4;
  return 1;
}

/* ===== FUN_00453580 @ 00453580 ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00453580(void)

{
  _DAT_00907e20 = (&track_lookup)[race_track];
  FUN_0045672e(0x907e28,(byte *)s_TRACK_02d_0046f024,_DAT_00907e20);
  FUN_0045672e(0x907e32,(byte *)s_TRACK_02dL_0046f030,_DAT_00907e20);
  return;
}

/* ===== FUN_00454a70 @ 00454a70 ===== */

void FUN_00454a70(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined1 *puVar4;
  
  puVar4 = &champ_info;
  iVar3 = 0;
  do {
    iVar2 = iVar3 * 0x36;
    iVar1 = *(int *)(&DAT_00905af2 + iVar2);
    FUN_0045672e((iVar1 >> 0x10) * 0x1a + 0x908060,&DAT_0046f658,s__R_JL_T__0046b620,puVar4);
    puVar4 = puVar4 + 0x36;
    iVar3 = iVar3 + 1;
    FUN_0045672e((iVar1 >> 0x10) * 0x10 + 0x907f20,&DAT_0046f660,s__R_JL_T__0046b620,
                 *(int *)(iVar2 + 0x905aee) >> 0x10);
  } while (iVar3 < 0x14);
  return;
}

/* ===== FUN_00453b98 @ 00453b98 ===== */

void FUN_00453b98(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  switch(DAT_0046ad00) {
  case 0:
    PTR_DAT_0046ab80 = s__R_JL_T_Division_1_0046f25c;
    break;
  case 1:
    PTR_DAT_0046ab80 = s__R_JL_T_Division_2_0046f270;
    break;
  case 2:
    PTR_DAT_0046ab80 = s__R_JL_T_Division_3_0046f284;
    break;
  case 3:
    PTR_DAT_0046ab80 = s__R_JL_T_Division_4_0046f298;
  }
  iVar3 = 0;
  if (0 < num_cars) {
    iVar2 = 0;
    do {
      iVar1 = *(int *)(&DAT_00905af2 + iVar2);
      if (*(int *)(iVar2 + 0x905af0) >> 0x10 == DAT_0046ad00) {
        FUN_0045672e((iVar1 >> 0x10) * 0x1a + 0x907e90,&DAT_0046f2ac,s__R_JL_T__0046ad0c,
                     &champ_info + iVar2);
        FUN_0045672e((iVar1 >> 0x10) * 0x10 + 0x907e40,&DAT_0046f2b4,s__R_JL_T__0046ad0c,
                     *(int *)(iVar2 + 0x905aee) >> 0x10);
      }
      iVar3 = iVar3 + 1;
      iVar2 = iVar2 + 0x36;
    } while (iVar3 < num_cars);
  }
  return;
}

