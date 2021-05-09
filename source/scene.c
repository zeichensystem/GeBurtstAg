#include <memory.h>

#include "scene.h"
#include "logutils.h"
#include "scenes/test.h"
#include "scenes/3d.h"


#define SCENE_NUM 2
static Scene scenes[SCENE_NUM];
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

void scenesInit(void) 
{
    scenes[0] = sceneNew("Test scene", sceneTestInit, sceneTestStart, sceneTestPause, sceneTestResume, sceneTestUpdate, sceneTestDraw);
    scenes[1] = sceneNew("3d scene", scene3dInit, scene3dStart, scene3dPause, scene3dResume, scene3dUpdate, scene3dDraw);
    for (int i = 0; i < SCENE_NUM; ++i) {
        scenes[i].init();
    }
    currentSceneID = 1; // The ID of the initial scene
}

void scenesDispatchUpdate(void) 
{
    assertion(currentSceneID < SCENE_NUM, "scene.c: scenesDispatchUpdate(): currentSceneID < SCENE_NUM");
    if (!scenes[currentSceneID].hasStarted) {
        scenes[currentSceneID].start();
        scenes[currentSceneID].hasStarted = true;
    }

    scenes[currentSceneID].update();
}

void scenesDispatchDraw(void) 
{
    assertion(currentSceneID < SCENE_NUM, "scene.c: scenesDispatchUpdate(): currentSceneID < SCENE_NUM");
    scenes[currentSceneID].draw();
}
