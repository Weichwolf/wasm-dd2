#ifndef DD2_SOUND_H
#define DD2_SOUND_H
#include <stdint.h>
/* Platform CD input uses the same ordered DirectSound device as effects.
 * Readers supply normalized source samples; consume reports source frames,
 * including when headless playback advances without producing float output. */
typedef float (*DD2SoundRead)(void*,uint64_t,int);
typedef void (*DD2SoundConsume)(void*,unsigned,unsigned);
void* dd2_snd_music_create(unsigned frames,DD2SoundRead read,DD2SoundConsume consume,void* context);
void dd2_snd_music_destroy(void* buffer);
void dd2_snd_music_stop(void* buffer);
void dd2_snd_music_play(void* buffer);
void dd2_snd_mix_flip(void);
#endif
