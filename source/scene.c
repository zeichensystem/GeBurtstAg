#include <string.h>

#include "scene.h"
#include "logutils.h"
#include "globals.h"

#define SHOW_DEBUG 
#define USER_SCENE_SWITCH

static int currentSceneID;

static Scene sceneNew(const char* name, void (*init)(void), void (*start)(void), void (*pause)(void), void (*resume)(void), void (*update)(void), void (*draw)(void)) 
{
    Scene s;
    s.hasStarted = false;
    strlcpy(s.name, name, SCENE_NAME_MAX_LEN);
    s.init = init;
    s.pause = pause;
    s.start = start;
    s.resume = resume;
    s.update = update;
    s.draw = draw;
    return s;
}


// !CODEGEN_START

#include "scenes/cubespaceScene.h"
#include "scenes/testbedScene.h"
#include "scenes/twisterScene.h"

#define SCENE_NUM 3
static Scene scenes[SCENE_NUM];

void scenesInit(void) 
{
    scenes[0] = sceneNew("cubespaceScene", cubespaceSceneInit, cubespaceSceneStart, cubespaceScenePause, cubespaceSceneResume, cubespaceSceneUpdate, cubespaceSceneDraw);
    scenes[1] = sceneNew("testbedScene", testbedSceneInit, testbedSceneStart, testbedScenePause, testbedSceneResume, testbedSceneUpdate, testbedSceneDraw);
    scenes[2] = sceneNew("twisterScene", twisterSceneInit, twisterSceneStart, twisterScenePause, twisterSceneResume, twisterSceneUpdate, twisterSceneDraw);

    for (int i = 0; i < SCENE_NUM; ++i) {
        scenes[i].init();
    }
    currentSceneID = 1; // The ID of the initial scene
}

// !CODEGEN_END   


void sceneSwitchTo(int sceneID) 
{
    assertion(sceneID >= 0 && sceneID < SCENE_NUM, "scene.c: sceneSwitchTo: sceneID in range");

    scenes[currentSceneID].pause();
    currentSceneID = sceneID;

    switch (g_mode) { // Clear the screen according to the mode we are switching from. 
        case DCNT_MODE5:
            m5_fill(CLR_BLACK);
            VBlankIntrWait();
            vid_flip();
            m5_fill(CLR_BLACK);
            break;
        case DCNT_MODE4:
            pal_bg_mem[0] = CLR_BLACK;
            m4_fill(0);
            VBlankIntrWait();
            vid_flip();
            m4_fill(0);
            break;
        default:
            panic("scene.c: sceneSwitchTo: unknown g_mode");
            break;
    }

    if (scenes[sceneID].hasStarted) {
        scenes[sceneID].resume();
    } else {
        scenes[sceneID].start();
        scenes[sceneID].hasStarted = true;
    }
}

static bool sceneUserSceneSwitch(void) 
{ 
    if (key_hit(KEY_SELECT)) {
        sceneSwitchTo((currentSceneID + 1) % SCENE_NUM);
        return true;
    }
    return false;
}

void scenesDispatchUpdate(void) 
{
    assertion(currentSceneID < SCENE_NUM, "scene.c: scenesDispatchUpdate(): currentSceneID < SCENE_NUM");

    key_poll();
    #ifdef USER_SCENE_SWITCH
    sceneUserSceneSwitch();
    #endif

    if (!scenes[currentSceneID].hasStarted) {
        scenes[currentSceneID].start();
        scenes[currentSceneID].hasStarted = true;
    }

    scenes[currentSceneID].update();
}

void scenesDispatchDraw(void) 
{
    assertion(currentSceneID < SCENE_NUM, "scene.c: scenesDispatchUpdate(): currentSceneID < SCENE_NUM");

    if (g_mode != DCNT_MODE5 && g_mode != DCNT_MODE4) {
        VBlankIntrWait();
    }

    scenes[currentSceneID].draw();

    #ifdef SHOW_DEBUG
    int fps = getFps();
    char dbg[64];
    snprintf(dbg, sizeof(dbg),  "FPS: %d", fps);
    switch (g_mode) {
        case DCNT_MODE5:
            m5_puts(8, 8, dbg, CLR_LIME);
            break;
        case DCNT_MODE4:
            m4_puts(8, 8, dbg, 1);
            break;
        case DCNT_MODE3:
            m3_puts(8, 8, dbg, CLR_LIME);
            break;
        default:
            break;
    }
    #endif

    if (g_mode == DCNT_MODE5 || g_mode == DCNT_MODE4) {
        vid_flip();
    }
}
