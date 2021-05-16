#ifndef TRACKER_H
#define TRACKER_H


#include <tonc.h>

#define NUM_TRACKS 8 // maximum number of different tracks; only one can be played at a time

#define QUARTER 32 // invariant: minimum is 16 (when QUARTER == 16, the dotted thirty second is 2 + 1 frame)
#define DOTTED_QUARTER (QUARTER + (QUARTER >> 1) )
#define HALF (QUARTER << 1)
#define DOTTED_HALF (HALF + (HALF >> 1))
#define WHOLE (QUARTER << 2)
#define EIGHT (QUARTER >> 1)
#define DOTTED_EIGHT ((EIGHT + (EIGHT >> 1))
#define SIXTEENTH (QUARTER >> 2)
#define DOTTED_SIXTEENTH ((SIXTEENTH + (SIXTEENTH >> 1))
#define THRITY_SECOND (QUARTER >> 3)
#define DOTTED_THIRTY_SECOND (THIRTY_SECOND + (THIRTY_SECOND >> 1))


#define NOTE_REST 12

typedef struct Note {
    u8 tone; // low nibble: halftone (0-11); high nibble: octave (0 -> octave -2, 7 -> octave 5)
    u16 frameDur;
} Note;

Note noteNew(u8 halftone, u8 octave, u16 duration);

typedef struct Track {
    u16 currentNoteFrame[2];
    u32 chanLen[2];
    u32 chanIdx[2];
    Note* chan[2];
} Track;

int trackNew(Note *chanOne, Note *chanTwo, u32 sizeOne, u32 sizeTwo);


void trackerInit(void);
void trackSelect(s32 idx);
void trackTick(void);
void tracksUpdate(void);


#endif