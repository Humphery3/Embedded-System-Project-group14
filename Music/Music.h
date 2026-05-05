#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

typedef enum {
    MTRACK_TITLE = 0,
    MTRACK_PLAY,
    MTRACK_DEAD,
    MTRACK_WIN,
} MusicTrack;

void Music_Init(void);
void Music_Update(MusicTrack track, int volume);

#endif
