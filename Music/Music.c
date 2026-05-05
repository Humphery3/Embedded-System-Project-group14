#include "Music.h"
#include "Buzzer.h"
#include "main.h"
#include <stdbool.h>

extern Buzzer_cfg_t buzzer_cfg;

typedef struct {
    int freq;   
    int dur;   
} Note;

#define R 0

// death sound
static const Note dead_seq[] = {
    {460, 1},
    {300, 2},
    {220, 5},
    {R,   6},
};
#define NUM_NOTES_DEATH  4

// win sound
static const Note win_seq[] = {
    {NOTE_A4,  8}, {NOTE_C5,  8}, {NOTE_E5,  8}, {NOTE_A5, 16}, {R, 14},
};
#define NUM_NOTES_WIN 5

// sequencer state
static int  g_track    = -1;
static int  g_idx      = 0;
static int  g_tick_rem = 0;
static bool g_playing  = false;
static int  g_cur_freq = 0;
static int  g_last_bvol = -1;

// maps volume 0-100 down to a calm buzzer duty 0-20
static int scale_vol(int v) {
    return v / 5;
}

static void pick_track(int t, const Note **seq, int *len, bool *loop) {
    switch (t) {
        case MTRACK_TITLE: *seq = NULL;     *len = 0;        *loop = false; break;
        case MTRACK_PLAY:  *seq = NULL;     *len = 0;        *loop = false; break;
        case MTRACK_DEAD:  *seq = dead_seq; *len = NUM_NOTES_DEATH; *loop = false; break;
        case MTRACK_WIN:   *seq = win_seq;  *len = NUM_NOTES_WIN;  *loop = false; break;
        default:           *seq = NULL;     *len = 0;        *loop = false; break;
    }
}

static void play_note(const Note *n, int bvol) {
    g_cur_freq  = n->freq;
    g_last_bvol = bvol;
    if (n->freq == 0 || bvol == 0)
        buzzer_off(&buzzer_cfg);
    else
        buzzer_tone(&buzzer_cfg, n->freq, bvol);
}

void Music_Init(void) {
    g_track    = -1;
    g_idx      = 0;
    g_tick_rem = 0;
    g_playing  = false;
    g_cur_freq = 0;
    g_last_bvol = -1;
}

void Music_Update(MusicTrack track, int volume) {
    int bvol = scale_vol(volume);

    const Note *seq;
    int len;
    bool loop;
    pick_track((int)track, &seq, &len, &loop);

    if (!seq) {
        buzzer_off(&buzzer_cfg);
        return;
    }

    // track changed — restart from beginning
    if ((int)track != g_track) {
        g_track    = (int)track;
        g_idx      = 0;
        g_playing  = true;
        g_tick_rem = seq[0].dur;
        play_note(&seq[0], bvol);
        return;
    }

    // one-shot finished — stay silent
    if (!g_playing) {
        if (bvol == 0) buzzer_off(&buzzer_cfg);
        return;
    }

    // volume changed mid-note — update duty without restarting
    if (bvol != g_last_bvol) {
        g_last_bvol = bvol;
        if (bvol == 0)
            buzzer_off(&buzzer_cfg);
        else if (g_cur_freq != 0)
            buzzer_tone(&buzzer_cfg, g_cur_freq, bvol);
    }

    // advance tick
    if (g_tick_rem > 0) g_tick_rem--;

    if (g_tick_rem == 0) {
        g_idx++;
        if (g_idx >= len) {
            if (loop) {
                g_idx = 0;
            } else {
                g_playing = false;
                g_track   = -1;
                buzzer_off(&buzzer_cfg);
                return;
            }
        }
        g_tick_rem = seq[g_idx].dur;
        play_note(&seq[g_idx], bvol);
    }
}
