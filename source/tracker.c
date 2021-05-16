#include <tonc.h>

#include "tracker.h"
#include "logutils.h"

static Track tracks[NUM_TRACKS];
static int lastTrackIdx = -1, currentTrackIdx = -1;

void trackerInit(void) 
{
    // turn sound on
    REG_SNDSTAT= SSTAT_ENABLE;
    // snd1/2 on left/right ;  full volume = 7
    REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR1 | SDMG_SQR2, 7);  //SDMG_BUILD_LR(SDMG_NOISE, 7);
    // DMG ratio to 100%
    REG_SNDDSCNT = SDS_DMG100;
    // chan 1
    REG_SND1SWEEP = SSW_OFF;//  SSW_BUILD(7, 1, 1);             
    REG_SND1FREQ = 0;
    // chan 2
    REG_SND2FREQ = 0;    
    // noise
    //REG_SND4CNT =  SSQR_ENV_BUILD(10, 0, 7);
}

#define TRACK_NOTE(halftone, octave) (((octave + 2) << 4) | ((halftone)&0xF)) 

Note noteNew(u8 halftone, u8 octave, u16 duration) 
{
    Note n;
    n.tone = TRACK_NOTE(halftone, octave);
    n.frameDur = duration;
    return n;
}


int trackNew(Note *chanOne, Note *chanTwo, u32 sizeOne, u32 sizeTwo) 
{
    Track track;
    track.chanIdx[0] = track.chanIdx[1] = 0;
    track.chanLen[0] = sizeOne;
    track.chanLen[1] = sizeTwo;
    track.currentNoteFrame[0] = track.currentNoteFrame[1] = 0;

    if (chanOne) {
        track.chan[0] = chanOne;
    }
    if (chanTwo) {
        track.chan[1] = chanTwo;
    }
    
    assertion(lastTrackIdx < NUM_TRACKS - 1, "tracker.c: trackNew: lastTrackIdx < NUM_TRACKS - 1");
    tracks[++lastTrackIdx] = track;
    return lastTrackIdx;
}


void trackSelect(s32 idx) 
{
    assertion(idx <= lastTrackIdx, "tracker.c: trackSelect: idx <= lastTrackIdx");
    currentTrackIdx = idx;
}


static bool noteIsRest(Note n) 
{
    return (n.tone & 0xF) == NOTE_REST;
}

static void notePlay(Note note, int chan) 
{
    vu16 *chanCntReg = chan == 0 ? &REG_SND1CNT : &REG_SND2CNT;
    vu16 *chanFreqReg = chan == 0 ? &REG_SND1FREQ : &REG_SND2FREQ;

    if (noteIsRest(note)) { // if it's a rest, mute
        *chanCntReg = SSQR_ENV_BUILD(0, 0, 0) | SSQR_DUTY1_2;
        *chanFreqReg = SFREQ_RESET;
        return;
    }

    Track *track = tracks + currentTrackIdx;
    u16 currentNoteFrame = track->currentNoteFrame[chan]; // how many frames into the note we are
    uint rate = SND_RATE(note.tone & 0xF, (note.tone >> 4) - 2); 

    if (currentNoteFrame == 0) { // note attack
        *chanCntReg = SSQR_ENV_BUILD(10, 0, 3) | SSQR_DUTY1_2;
        *chanFreqReg = rate | SFREQ_RESET;
        //*chanCntReg &= ~0x1F; // "...when in continuous mode, alway set the sound lenght to zero after changing the frequency. Otherwise, the sound may stop."
    } 
    // else if (currentNoteFrame == 15) { // note release; hardcoded time for testing
    //     *chanCntReg = SSQR_ENV_BUILD(15, 0, 1) | SSQR_DUTY1_2; 
    //     *chanFreqReg = rate | SFREQ_RESET;
    //     //*chanCntReg &= ~0x1F;
    // }
}


void trackTick(void) 
{ // Our interrupt service routine (called on VBlank)
    assertion(currentTrackIdx <= lastTrackIdx, "tracker.c: trackTick: currentTrackIdx <= lastTrackIdx");
    if (currentTrackIdx < 0) { // no track selected; no work to be done
        return;
    }
    Track *track = tracks + currentTrackIdx;

    for (int chan = 0; chan < 2; ++chan) { // for each channel of the track
        if (track->chanLen[chan] <= 0 || track->chan[chan] == NULL) { // channel empty; skip
            continue;
        }

        Note note = track->chan[chan][track->chanIdx[chan]];
        notePlay(note, chan);           

        if (track->currentNoteFrame[chan] == note.frameDur - 1 ) { // current note is over (careful: if not minus one, it's an off by one error)
            track->chanIdx[chan] = (track->chanIdx[chan] + 1) % track->chanLen[chan]; // advance the tracker "head" (chanIdx[chan]) to the next note of the track
            track->currentNoteFrame[chan] = 0;
        } else { // still within note
            track->currentNoteFrame[chan] += 1;
        }
    }
}
