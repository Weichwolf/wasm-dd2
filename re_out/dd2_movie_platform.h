#ifndef DD2_MOVIE_PLATFORM_H
#define DD2_MOVIE_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
unsigned dd2_movie_now_ms(void);
void dd2_movie_wait(void);
void dd2_movie_present(const uint32_t* argb);
int dd2_movie_audio_start(const int16_t*,size_t,unsigned,unsigned);
int dd2_movie_audio_done(void);
void dd2_movie_audio_stop(void);
/* Win32 RGB32 -> RGB565 destination -> display RGB. Canvas/native texture
 * storage is 640x480 ARGB8888. RECT right/bottom are MCI width/height. */
int dd2_movie_render(const uint8_t*,unsigned,unsigned,const int32_t*,uint32_t*);
#endif
