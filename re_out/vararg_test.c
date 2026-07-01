
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

