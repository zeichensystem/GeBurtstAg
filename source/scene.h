#ifndef SCENE_H
#define SCENE_H
#include <tonc.h>

#define SCENE_NAME_MAX_LEN 64

// The scene IDs are visible to each scene (#include "../scene.h") and can be used as arguments to "sceneSwitchTo".
typedef enum SceneID {CUBESPACESCENE=0, TESTBEDSCENE, BENCHMARKSCENE, TWISTERSCENE, GBASCENE, } SceneID;

typedef struct Scene {
    char name[SCENE_NAME_MAX_LEN];
    bool hasStarted;
    void (*init)(void); // Load the scene in init (set up actions such as initilising Models, arrays, etc.)
    void (*start)(void); // Called before the scene is first entered (there might be am indefinite delay between init and start, therefore it may be of importance)
    void (*pause)(void); // Called before the scene is paused
    void (*resume)(void); // Called before the scene is resumed after it has been paused
    void (*update)(void); // Called each frame the scene is active; should be used for animation/physics/logic etc.
    void (*draw)(void); // Called after each update; should be used for drawing code
} Scene;

void scenesInit(void);
void scenesDispatchUpdate(void);
void scenesDispatchDraw(void);
void sceneSwitchTo(SceneID sceneID); 



#endif