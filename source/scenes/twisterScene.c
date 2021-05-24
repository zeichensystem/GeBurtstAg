#include <tonc.h>
#include <stdlib.h>
#include <string.h>


#include "twisterScene.h"
#include "../globals.h"
#include "../commondefs.h"
#include "../logutils.h"
#include "../timer.h"
#include "../render/draw.h"

static Timer timer;

#define MAX_RENDER_TWISTERS 4

typedef struct Twister { 
    int freqStep, phaseOffset, numTwists, id;
    FIXED_12 amp;
    FIXED_12 x, z, zInv;
} ALIGN4 Twister;

static Twister twisters[MAX_RENDER_TWISTERS];
static Twister *twistPtrs[MAX_RENDER_TWISTERS];


#define RAINBOW_SIZE 6
#define TWISTER_PAL_SIZE 14
static COLOR pal[TWISTER_PAL_SIZE];

typedef enum {
    CLRIDX_BLACK=0,

    CLRIDX_RED, 
    CLRIDX_ORANGE, 
    CLRIDX_YELLOW,
    CLRIDX_GREEN, 
    CLRIDX_BLUE,
    CLRIDX_PURPLE,

    CLRIDX_PASTEL_BLUE,
    CLRIDX_PINKSHADE_START,
    CLRIDX_PINKSHADE_END=13
} CLR_IDX;

static int radiusX = 42;
static int radiusZ = 20;
static int letterboxTop = 40;
static int letterboxBottom = 120;

void twisterSceneInit(void) 
{     
    timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);

    for (int i = 0; i < MAX_RENDER_TWISTERS; ++i) {
        twisters[i].amp = int2fx12(80);
        twisters[i].numTwists = 6; // Lower me for better framerates. 
        twisters[i].freqStep = TAU / M4_HEIGHT;
        twisters[i].phaseOffset = TAU/twisters[i].numTwists;
        twisters[i].id = i;
        twisters[i].x = 0;
        twisters[i].z = 0;
        twistPtrs[i] = twisters + i;
    }

}        

IWRAM_CODE_ARM void twisterSceneUpdate(void) 
{
    timerTick(&timer);

    int letterboxTrans =  + fx12ToInt(8 * lu_sin(fx12ToInt(timer.time * TAU)));
    letterboxTop = 40;
    letterboxBottom = 120;
    
    for (int i = 0; i < MAX_RENDER_TWISTERS; ++i) {
        int t = fx12ToInt(timer.time *  PI / 2);
        twisters[i].x = radiusX * lu_cos(i * TAU / MAX_RENDER_TWISTERS + t);
        twisters[i].z = int2fx12(radiusZ * 2) + radiusZ * lu_sin(i * TAU  / MAX_RENDER_TWISTERS + t);
    }  
}


// Libtonc m4_hline, but without x1 x2 normalisation, as we can guarantee x1 < x2 (measurably better performance).
INLINE void m4_hline_nonorm(int x1, int y, int x2, u32 clr)
{
	clr &= 0xFF;
	uint width= x2-x1+1;
	u16 *dstL= (u16*)((u8*)vid_page +y*(uint)M4_WIDTH + (x1&~1));
	// --- Left unaligned pixel ---
	if(x1&1)
	{
		*dstL= (*dstL & 0xFF) + (clr<<8);
		width--;
		dstL++;
	}
	// --- Right unaligned pixel ---
	if(width&1)
		dstL[width/2]= (dstL[width/2]&~0xFF) + clr;
	width /= 2;
	// --- Aligned line ---
	if(width)
	    memset16(dstL, dup8(clr), width);
}


IWRAM_CODE_ARM static void renderTwisters(Twister **tw, int num) 
{
    for (int i = 1; i < num; ++i) {
        for (int j = i; j > 0 && tw[j]->z > tw[j - 1]->z ; --j) { // We sort descending (big z values here mean farther in the background, i.e. those twisters are drawn first).
            Twister *tmp;
            tmp = tw[j];
            tw[j] = tw[j - 1];
            tw[j - 1] = tmp;
        }
    }
    // for (int i = 0; i < num - 1; ++i) {
    //     assertion(tw[i]->z >= tw[i+1]->z, "twister.c: renderTwisters: twisters depth-sorted");
    // }

    for (int idx = 0; idx < num; ++idx) {
        tw[idx]->zInv = fx12div(int2fx12(1), tw[idx]->z >> 2); // Cache the expensive "perspective" divisions. 
    }

    for (int y = letterboxTop; y < letterboxBottom; y+=1) {
        FIXED yfac = int2fx12(y + 100) / M4_HEIGHT;
        for (int idx = 0; idx < num; ++idx) {
            int dir = tw[idx]->id % 2 ? 1 : -1;
            int center_x = M4_WIDTH / 2 + fx12ToInt(fx12mul(yfac, tw[idx]->x)) + fx12ToInt(12* lu_sin(tw[idx]->id * PI/5 + tw[idx]->freqStep * y + dir * fx12ToInt(timer.time * TAU)));
    
            for (int x = 0; x < tw[idx]->numTwists; ++x) {
                int x1, x2;

                int phase = x * tw[idx]->phaseOffset + dir * fx12ToInt(timer.time * TAU);
                FIXED_12 sinval = lu_sin(phase - tw[idx]->freqStep * y / 4);
                x1 = center_x + fx12ToInt(fx12mul(fx12mul(tw[idx]->amp, sinval), tw[idx]->zInv));

                phase = (x+1 < tw[idx]->numTwists ? x+1 : 0) * tw[idx]->phaseOffset + dir * fx12ToInt(timer.time * TAU);
                sinval = lu_sin(phase - tw[idx]->freqStep * y / 4);
                x2 = center_x + fx12ToInt(fx12mul(fx12mul(tw[idx]->amp, sinval), tw[idx]->zInv));
                
                CLR_IDX clr;
                if (x <= 2) {
                    clr = CLRIDX_PINKSHADE_START + x;
                } else {
                    clr = CLRIDX_PINKSHADE_START + 2 + (3 - x);
                }
                if (x1 < x2) {
                    m4_hline_nonorm(x1, y, x2, clr);
                }
            }
          
            // for (int x = 0; x < tw[idx]->numTwists; ++x) {
            //     int x1 = x_vals[x];
            //     int x2 = x_vals[x+1 < tw[idx]->numTwists ? x+1 : 0 ];
            //     CLR_IDX clr;
            //     if (!key_held(KEY_START)) {
            //         if (x <= 2) {
            //             clr = CLRIDX_PINKSHADE_START + x;
            //         } else {
            //             clr = CLRIDX_PINKSHADE_START + 2 + (3 - x);
            //         }
            //     } else {
            //         clr = x + CLRIDX_RED <= CLRIDX_PURPLE ? x + CLRIDX_RED : CLRIDX_RED;
            //     }
            //     if (x1 < x2) {
            //         m4_hline_nonorm(x1, y, x2, clr );
            //     }
            // }
        }
    }
}

IWRAM_CODE_ARM void twisterSceneDraw(void) 
{
    memset32(vid_page, quad8(CLRIDX_BLACK), 15 * M4_WIDTH/4); // TODO: REMOVE ME (only here for debugging so the fps text does not overdraw).
    memset32(vid_page + letterboxTop * M4_WIDTH / 2, quad8(CLRIDX_PASTEL_BLUE), (letterboxBottom - letterboxTop) * M4_WIDTH/4);
    renderTwisters(twistPtrs, MAX_RENDER_TWISTERS);
}

static void videoModeInit(void) {
        videoM4Init();
        
        pal[0] = CLR_BLACK;
        pal[1] = RGB15_SAFE(28, 0, 0); // red
        pal[2] = RGB15_SAFE(31, 29, 0); // yellow
        pal[3] = RGB15_SAFE(31, 17, 0); // orange
        pal[4] = RGB15_SAFE(0, 16, 5); // green
        pal[5] = RGB15_SAFE(0, 9, 31); // blue
        pal[6] = RGB15_SAFE(14, 1, 17); // purple

        pal[7] = RGB15_SAFE(0, 25, 27); // pastel-blue

        pal[8] = RGB15_SAFE(31, 26, 30); // pinkshades
        pal[9] = RGB15_SAFE(31, 16, 27);
        pal[10] = RGB15_SAFE(31, 6, 24);
        pal[11] = RGB15_SAFE(25, 0, 19);
        pal[12] = RGB15_SAFE(15, 0, 11);
        pal[13] = RGB15_SAFE(5, 0, 4);

        setM4Pal(pal, TWISTER_PAL_SIZE);
}


void twisterSceneStart(void) {
    videoModeInit();    
    m4_fill(CLRIDX_BLACK);
    timerStart(&timer);
}

void twisterScenePause(void) {
        timerStop(&timer);
}

void twisterSceneResume(void) 
{
    videoModeInit();
    m4_fill(CLRIDX_BLACK);
    timerResume(&timer);
}
