#ifndef DD2_GAME_CONTENT_VIEWER_H
#define DD2_GAME_CONTENT_VIEWER_H

#include <stdbool.h>

typedef struct {
    int detail;
    bool cockpit;
    bool pose;
} dd2_content_selection;

typedef struct {
    const char *root;
    const char *path;
    dd2_content_selection selection;
} dd2_content_capture_options;

/* Original-free authored-content diagnostic, not the complete game owner. */
int dd2_content_run(const char *root);
bool dd2_content_capture(dd2_content_capture_options options);
void dd2_content_close(void);
int dd2_content_present(void);
int dd2_content_select(int detail, int cockpit);
int dd2_content_set_pose(int enabled);
int dd2_content_detail(void);
int dd2_content_cockpit(void);
int dd2_content_pose(void);

#endif
