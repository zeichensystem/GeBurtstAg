import re
import pathlib
import argparse
import textwrap

# In respect to the project directory.
SOURCE_DIR = "source/"
SCENE_DIR = "source/scenes/"


def generate_template(name, mode=None):
    outpath_base = pathlib.Path(".").joinpath(SCENE_DIR).joinpath("scene" + name)
    outpath_c = outpath_base.with_suffix(".c")
    outpath_h = outpath_base.with_suffix(".h")
    
    if outpath_c.exists() or outpath_h.exists():
        raise ValueError(f"Implementation or header file of the same name already exists:\n{str(outpath_c)} {str(outpath_h)}")

    headerTemplate = textwrap.dedent(f"""
    #ifndef SCENE_{name.upper()}_H
    #define SCENE_{name.upper()}_H

    void scene{name}Init(void);
    void scene{name}Start(void);
    void scene{name}Pause(void);
    void scene{name}Resume(void);

    void scene{name}Update(void);
    void scene{name}Draw(void);

    #endif  
    """)

    modeInitFun = ""
    if mode: 
        if mode == 5:
            modeInitFun = "videoM5ScaledInit();"
        elif mode == 4:
            modeInitFun = "videoM4Init();"
        else:
            raise ValueError(f"Mode {mode} not yet implemented.")

    implTemplate = textwrap.dedent(f"""
    #include <tonc.h>

    #include "../timer.h"
    #include "../render/draw.h"
    #include "scene{name}.h"

    static Timer timer;

    void scene{name}Init(void) 
    {{ 
        timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
        // Your "constructor" here. This function is called exactly once for every scene (even if you never switch to it) on startup.
    }}

    void scene{name}Update(void) 
    {{
        // Your "game loop" here. 
        timerTick(&timer);
    }}

    void scene{name}Draw(void) 
    {{
        // Your drawing code here (don't call vid_flip, this is done for you after your draw function).
    }}

    void scene{name}Start(void) 
    {{
        // This function is called only once, namely when your scene is first entered. Later, the resume function will be called instead. 
        timerStart(&timer);
        {modeInitFun}
    }}

    void scene{name}Pause(void) 
    {{
        timerStop(&timer);
    }}

    void scene{name}Resume(void) 
    {{
        timerResume(&timer);
        {modeInitFun}
    }}
    """)

    outpath_h.write_text(headerTemplate)
    outpath_c.write_text(implTemplate)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Creates scene templates, and generate appropriate sceneInit.")
    parser.add_argument("--new", type=str, help="Create a new scene of the given name.")
    parser.add_argument("--mode", type=int, help="When creating a new scene, you can optionally specify which display mode the scene should use.")

    generationMode  = parser.add_mutually_exclusive_group()

    args = parser.parse_args()

    if args.new:
        name = args.new.strip().capitalize()
        name =  re.sub(r"\W", "", name) # Remove non-word characters (underscores are okay).
        if len(name) < 1:
            raise ValueError(f"'{args.new}' is not a valid scene name. It must be a valid C-identifier.")
        generate_template(name, args.mode)
        