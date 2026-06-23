/* ===== FUN_0042a6da ===== */

/* WARNING (jumptable): Unable to track spacebase fully for stack */

void FUN_0042a6da(void)

{
  int iVar1;
  int unaff_EBX;
  int iVar2;
  int unaff_EBP;
  uint unaff_ESI;
  byte *pbVar3;
  undefined4 uVar4;
  
  FUN_004166c4(0x744bd0,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_topl);
  FUN_004166c4(0x744bf8,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_topl);
  do {
    do {
      iVar2 = unaff_EBX;
      unaff_ESI = unaff_ESI + 1;
      if (5 < (int)unaff_ESI) {
        return;
      }
      unaff_EBX = iVar2 + 4;
    } while (*(int *)(iVar2 + -0x2c + unaff_EBP) == 0);
    iVar1 = *(int *)(&DAT_00744de4 + iVar2);
    if (iVar1 < 700) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a4;
    }
    else if (iVar1 < 0x5aa) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a8;
    }
    else if (iVar1 < 0x898) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9ac;
    }
    else if (iVar1 < 0xb86) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b0;
    }
    else if (iVar1 < 0xe74) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b4;
    }
    else if (iVar1 < 0x1000) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b8;
    }
    else {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9bc;
    }
    FUN_0045672e(unaff_EBP + -0x18,pbVar3,uVar4);
  } while (5 < unaff_ESI);
                    /* WARNING: Could not recover jumptable at 0x0042a6d3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)((int)&PTR_LAB_0042a4c4 + iVar2))();
  return;
}


/* ===== FUN_0042a703 ===== */

/* WARNING (jumptable): Unable to track spacebase fully for stack */

void FUN_0042a703(void)

{
  int iVar1;
  int unaff_EBX;
  int iVar2;
  int unaff_EBP;
  uint unaff_ESI;
  byte *pbVar3;
  undefined4 uVar4;
  
  FUN_004166c4(0x744c20,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_topr);
  FUN_004166c4(0x744c48,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_topr);
  do {
    do {
      iVar2 = unaff_EBX;
      unaff_ESI = unaff_ESI + 1;
      if (5 < (int)unaff_ESI) {
        return;
      }
      unaff_EBX = iVar2 + 4;
    } while (*(int *)(iVar2 + -0x2c + unaff_EBP) == 0);
    iVar1 = *(int *)(&DAT_00744de4 + iVar2);
    if (iVar1 < 700) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a4;
    }
    else if (iVar1 < 0x5aa) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a8;
    }
    else if (iVar1 < 0x898) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9ac;
    }
    else if (iVar1 < 0xb86) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b0;
    }
    else if (iVar1 < 0xe74) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b4;
    }
    else if (iVar1 < 0x1000) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b8;
    }
    else {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9bc;
    }
    FUN_0045672e(unaff_EBP + -0x18,pbVar3,uVar4);
  } while (5 < unaff_ESI);
                    /* WARNING: Could not recover jumptable at 0x0042a6d3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)((int)&PTR_FUN_0042a4c4 + iVar2))();
  return;
}


/* ===== FUN_0042a72c ===== */

/* WARNING (jumptable): Unable to track spacebase fully for stack */

void FUN_0042a72c(void)

{
  int iVar1;
  int unaff_EBX;
  int iVar2;
  int unaff_EBP;
  uint unaff_ESI;
  byte *pbVar3;
  undefined4 uVar4;
  
  FUN_004166c4(0x744cc0,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_midl);
  FUN_004166c4(0x744ce8,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_midl);
  do {
    do {
      iVar2 = unaff_EBX;
      unaff_ESI = unaff_ESI + 1;
      if (5 < (int)unaff_ESI) {
        return;
      }
      unaff_EBX = iVar2 + 4;
    } while (*(int *)(iVar2 + -0x2c + unaff_EBP) == 0);
    iVar1 = *(int *)(&DAT_00744de4 + iVar2);
    if (iVar1 < 700) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a4;
    }
    else if (iVar1 < 0x5aa) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a8;
    }
    else if (iVar1 < 0x898) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9ac;
    }
    else if (iVar1 < 0xb86) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b0;
    }
    else if (iVar1 < 0xe74) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b4;
    }
    else if (iVar1 < 0x1000) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b8;
    }
    else {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9bc;
    }
    FUN_0045672e(unaff_EBP + -0x18,pbVar3,uVar4);
  } while (5 < unaff_ESI);
                    /* WARNING: Could not recover jumptable at 0x0042a6d3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)((int)&PTR_FUN_0042a4c4 + iVar2))();
  return;
}


/* ===== FUN_0042a752 ===== */

/* WARNING (jumptable): Unable to track spacebase fully for stack */

void FUN_0042a752(void)

{
  int iVar1;
  int unaff_EBX;
  int iVar2;
  int unaff_EBP;
  uint unaff_ESI;
  byte *pbVar3;
  undefined4 uVar4;
  
  FUN_004166c4(0x744d60,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_midr);
  FUN_004166c4(0x744d88,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_midr);
  do {
    do {
      iVar2 = unaff_EBX;
      unaff_ESI = unaff_ESI + 1;
      if (5 < (int)unaff_ESI) {
        return;
      }
      unaff_EBX = iVar2 + 4;
    } while (*(int *)(iVar2 + -0x2c + unaff_EBP) == 0);
    iVar1 = *(int *)(&DAT_00744de4 + iVar2);
    if (iVar1 < 700) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a4;
    }
    else if (iVar1 < 0x5aa) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a8;
    }
    else if (iVar1 < 0x898) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9ac;
    }
    else if (iVar1 < 0xb86) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b0;
    }
    else if (iVar1 < 0xe74) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b4;
    }
    else if (iVar1 < 0x1000) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b8;
    }
    else {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9bc;
    }
    FUN_0045672e(unaff_EBP + -0x18,pbVar3,uVar4);
  } while (5 < unaff_ESI);
                    /* WARNING: Could not recover jumptable at 0x0042a6d3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)((int)&PTR_FUN_0042a4c4 + iVar2))();
  return;
}


/* ===== FUN_0042a778 ===== */

/* WARNING (jumptable): Unable to track spacebase fully for stack */

void FUN_0042a778(void)

{
  int iVar1;
  int unaff_EBX;
  int iVar2;
  int unaff_EBP;
  uint unaff_ESI;
  byte *pbVar3;
  undefined4 uVar4;
  
  FUN_004166c4(0x744c70,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_botl);
  FUN_004166c4(0x744c98,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_botl);
  do {
    do {
      iVar2 = unaff_EBX;
      unaff_ESI = unaff_ESI + 1;
      if (5 < (int)unaff_ESI) {
        return;
      }
      unaff_EBX = iVar2 + 4;
    } while (*(int *)(iVar2 + -0x2c + unaff_EBP) == 0);
    iVar1 = *(int *)(&DAT_00744de4 + iVar2);
    if (iVar1 < 700) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a4;
    }
    else if (iVar1 < 0x5aa) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a8;
    }
    else if (iVar1 < 0x898) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9ac;
    }
    else if (iVar1 < 0xb86) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b0;
    }
    else if (iVar1 < 0xe74) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b4;
    }
    else if (iVar1 < 0x1000) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b8;
    }
    else {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9bc;
    }
    FUN_0045672e(unaff_EBP + -0x18,pbVar3,uVar4);
  } while (5 < unaff_ESI);
                    /* WARNING: Could not recover jumptable at 0x0042a6d3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)((int)&PTR_FUN_0042a4c4 + iVar2))();
  return;
}


/* ===== FUN_0042a79e ===== */

/* WARNING (jumptable): Unable to track spacebase fully for stack */

void FUN_0042a79e(void)

{
  int iVar1;
  int unaff_EBX;
  int iVar2;
  int unaff_EBP;
  uint unaff_ESI;
  byte *pbVar3;
  undefined4 uVar4;
  
  FUN_004166c4(0x744d10,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_botr);
  FUN_004166c4(0x744d38,(char *)(unaff_EBP + -0x18),(ushort *)&sd_damage_botr);
  do {
    do {
      iVar2 = unaff_EBX;
      unaff_ESI = unaff_ESI + 1;
      if (5 < (int)unaff_ESI) {
        return;
      }
      unaff_EBX = iVar2 + 4;
    } while (*(int *)(iVar2 + -0x2c + unaff_EBP) == 0);
    iVar1 = *(int *)(&DAT_00744de4 + iVar2);
    if (iVar1 < 700) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a4;
    }
    else if (iVar1 < 0x5aa) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9a8;
    }
    else if (iVar1 < 0x898) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9ac;
    }
    else if (iVar1 < 0xb86) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b0;
    }
    else if (iVar1 < 0xe74) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b4;
    }
    else if (iVar1 < 0x1000) {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9b8;
    }
    else {
      uVar4 = *(undefined4 *)((int)&PTR_DAT_00464bf8 + iVar2);
      pbVar3 = &DAT_0046c9bc;
    }
    FUN_0045672e(unaff_EBP + -0x18,pbVar3,uVar4);
  } while (5 < unaff_ESI);
                    /* WARNING: Could not recover jumptable at 0x0042a6d3. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)((int)&PTR_FUN_0042a4c4 + iVar2))();
  return;
}


