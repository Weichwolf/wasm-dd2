#ifndef DD2_CD_H
#define DD2_CD_H
#include <stdint.h>
int dd2_mci_send(unsigned device, unsigned command, unsigned flags, uint32_t *params);
void dd2_cd_pump(void);
unsigned dd2_platform_ms(void);
unsigned dd2_audio_ms(void);
#endif
