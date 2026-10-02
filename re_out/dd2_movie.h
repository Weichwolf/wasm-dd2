#ifndef DD2_MOVIE_H
#define DD2_MOVIE_H
#include <stdint.h>
/* -1 means this command belongs to another MCI device. */
int dd2_movie_mci_send(unsigned,unsigned,unsigned,uint32_t*);
void dd2_movie_pump(void);
int dd2_movie_active(void);
/* Test/play a movie through the original engine Play_Movie call. */
int dd2_movie_run(const char* filename);
#endif
