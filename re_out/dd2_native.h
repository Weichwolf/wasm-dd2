#ifndef DD2_NATIVE_H
#define DD2_NATIVE_H
#include <stdint.h>
#include <stddef.h>
#ifdef DD2_NATIVE_SDL
void dd2_native_init(void);
int dd2_native_enabled(void);
void dd2_native_poll(void);
void dd2_native_present(const unsigned char*,const unsigned char*);
void dd2_native_audio(const float*,unsigned,unsigned);
void dd2_native_movie_present(const uint32_t*);
int dd2_native_movie_audio_start(const int16_t*,size_t,unsigned,unsigned);
int dd2_native_movie_audio_done(void);
void dd2_native_movie_audio_stop(void);
#else
#define dd2_native_init() ((void)0)
#define dd2_native_enabled() 0
#define dd2_native_poll() ((void)0)
#endif
#endif
