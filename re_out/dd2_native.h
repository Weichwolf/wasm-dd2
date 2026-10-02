#ifndef DD2_NATIVE_H
#define DD2_NATIVE_H
#ifdef DD2_NATIVE_SDL
void dd2_native_init(void);
int dd2_native_enabled(void);
void dd2_native_poll(void);
void dd2_native_present(const unsigned char*,const unsigned char*);
void dd2_native_audio(const float*,unsigned,unsigned);
#else
#define dd2_native_init() ((void)0)
#define dd2_native_enabled() 0
#define dd2_native_poll() ((void)0)
#endif
#endif
