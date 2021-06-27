import re
import pathlib
import argparse
import textwrap

# In respect to the project directory.
SOURCE_DIR = "source/"
SCENE_DIR = "source/scenes/"


def generate_template(name, mode=None):
    name_scene = name + "Scene"

    outpath_base = pathlib.Path(".").joinpath(SCENE_DIR).joinpath(name_scene)
    outpath_c = outpath_base.with_suffix(".c")
    outpath_h = outpath_base.with_suffix(".h")
    
    if outpath_c.exists() or outpath_h.exists():
        raise ValueError(f"Implementation or header file of the same name already exists:\n{str(outpath_c)} {str(outpath_h)}")


    headerTemplate = textwrap.dedent(f"""
    #ifndef {name_scene}_H
    #define {name_scene}_H

    void {name_scene}Init(void);
    void {name_scene}Start(void);
    void {name_scene}Pause(void);
    void {name_scene}Resume(void);
    void {name_scene}Update(void);
    void {name_scene}Draw(void);

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
    
    #include "{name_scene}.h"
    #include "../scene.h"
    #include "../globals.h"
    #include "../timer.h"
    #include "../render/draw.h"
    

    static Timer timer;

    void {name_scene}Init(void) 
    {{ 
        timer = timerNew(TIMER_MAX_DURATION, TIMER_REGULAR);
        // Your "constructor" here. This function is called exactly once for every scene (even if you never switch to it) on startup.
    }}

    void {name_scene}Update(void) 
    {{
        // Your "game loop" here. 
        timerTick(&timer);
    }}

    void {name_scene}Draw(void) 
    {{
        // Your drawing code here (don't call vid_flip, this is done for you after your draw function).
    }}

    void {name_scene}Start(void) 
    {{
        // This function is called only once, namely when your scene is first entered. Later, the resume function will be called instead. 
        timerStart(&timer);
        {modeInitFun}
    }}

    void {name_scene}Pause(void) 
    {{
        timerStop(&timer);
    }}

    void {name_scene}Resume(void) 
    {{
        timerResume(&timer);
        {modeInitFun}
    }}
    """)

    outpath_h.write_text(headerTemplate)
    outpath_c.write_text(implTemplate)


def generate_scene_c():

    cfiles = [scene for scene in pathlib.Path(".").joinpath(SCENE_DIR).glob("*.c")]
    scenes = list()
    for scene in cfiles:
        if scene.stem.lower().find("scene") != -1: # We don't want non-scenes to be counted as scenes (all scenes must contain the string scene)
            scenes.append(scene)

    num_scenes = len(scenes)
    if (num_scenes < 1):
        print("No scenes, nothing to be done.")
        return 

    # Parse config file
    config_file = pathlib.Path(".").joinpath(SCENE_DIR).joinpath("config")
    if not config_file.exists():
        raise ValueError("Config file does not exist, abort.")

    start_scene = ""
    match = re.search(r"startscene(\s|:)\s*(\w+)", config_file.read_text())
    if not match: 
        raise ValueError("No start scene given in scene config.")
    else:
        start_scene = match.group(2)

    start_scene_idx = -1
    for idx, scene in enumerate(scenes):
        if scene.stem == start_scene:
            start_scene_idx = idx
            break

    if start_scene_idx == -1:
        raise ValueError(f"{start_scene} is an unknown scene.")

    scene_init_body = ""
    includes = ""
    enum_replace = "typedef enum SceneID {"
    for n, scene in enumerate(scenes):
        scene_name = scene.stem
        scene_init_body += f'scenes[{n}] = sceneNew("{scene_name}", {scene_name}Init, {scene_name}Start, {scene_name}Pause, {scene_name}Resume, {scene_name}Update, {scene_name}Draw);\n        '
        includes += f'#include "{scene.with_suffix(".h").relative_to(SOURCE_DIR)}"\n    '
        if n == 0:
            enum_replace += f"{scene_name.upper()}=0, "
        else:
            enum_replace += f"{scene_name.upper()}, "
    enum_replace += "} SceneID;"
      
    init_fun = textwrap.dedent(f"""
    // !CODEGEN_START

    {includes}
    #define SCENE_NUM {num_scenes}
    static Scene scenes[SCENE_NUM];

    void scenesInit(void) 
    {{
        {scene_init_body}
        for (int i = 0; i < SCENE_NUM; ++i) {{
            scenes[i].init();
        }}
        currentSceneID = {start_scene_idx}; // The ID of the initial scene
        sceneKeySeqInit();
    }}

    // !CODEGEN_END""")
    init_fun = init_fun[1:] # Remove initial newline.

    # Okay, now we have to "inject" the generated function and array declerations into the scene.c sourcefile
    scene_c = pathlib.Path(".").joinpath(SOURCE_DIR).joinpath("scene.c")
    if not scene_c.exists():
        raise ValueError("scene.c does not exist.")
    scene_c_text = scene_c.read_text()
    match_start = re.search(r"//\s*!CODEGEN_START", scene_c_text)
    match_end = re.search(r"//\s*!CODEGEN_END", scene_c_text)
    if not match_start or not match_end or match_start.start() >= match_end.start():
        raise ValueError("Malformed !CODEGEN_START or !CODEGEN_END in scene.c")

    scene_c_text = scene_c_text[:match_start.start()] + init_fun + scene_c_text[match_end.end():]

    # ... and the header file
    scene_h = pathlib.Path(".").joinpath(SOURCE_DIR).joinpath("scene.h")
    if not scene_h.exists():
        raise ValueError("scene.h does not exist, abort.")
    scene_h_text = scene_h.read_text()
    enum_match = re.search(r"typedef\s+enum\s+SceneID\s*{[^;]*;", scene_h_text)
    if not enum_match:
        raise ValueError("scene.h missing sceneId enum")

    scene_h_text = scene_h_text[:enum_match.start()] + enum_replace + scene_h_text[enum_match.end():]

    scene_c.write_text(scene_c_text)
    scene_h.write_text(scene_h_text)
    


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Creates scene templates, and generate appropriate sceneInit.")
    parser.add_argument("--new", type=str, help="Create a new scene of the given name.")
    parser.add_argument("--mode", type=int, help="When creating a new scene, you can optionally specify which display mode the scene should use.")

    args = parser.parse_args()

    if args.new:
        name = args.new.strip()
        name =  re.sub(r"\W", "", name) # Remove non-word characters (underscores are okay).
        if len(name) < 1:
            raise ValueError(f"'{args.new}' is not a valid scene name. It must be a valid C-identifier.")
        generate_template(name, args.mode)
     
    print("Updating scene.c...")
    generate_scene_c()



# match = re.search(r"void\s+scenesInit\s*(\s*void\s*)\s*\{", scene_c_text)
# if not match:
#     raise ValueError("scene.c does not contain a sceneInit function, abort.")
# opening_brace_idx = match.end()
# closing_brace_idx = -1
# function_body = scene_c_text[opening_brace_idx + 1:]
# open_braces = 1
# while open_braces: # Note: This would give us a "nice" infinite loop if the braces are unbalanced in case we have a syntax error in scene.c.
#     open_idx = function_body.find("{")
#     if open_idx != -1:
#         open_braces += 1
#         function_body = function_body[open_idx + 1:]

#     close_idx = function_body.find("}")
#     if close_idx != -1:
#         open_braces -= 1
#         function_body = function_body[close_idx + 1:]

#     if open_braces == 0:
#         closing_brace_idx = close_idx

# scene_c_text = scene_c_text[:match.start()] + init_fun + scene_c_text[closing_brace_idx + 1:] # Finally, repalce sceneInit in the source file text

# # Now that we have inserted the function, we still have to insert the array declarations
# scene_num = f"#define SCENE_NUM {num_scenes}"
# scene_arr = f"static Scene scenes[SCENE_NUM];"
# replace_map = {
#     r"\s*\#define\s+SCENE_NUM\s+\d+": scene_num, 
#     r"\s*static\s+Scene\s+scenes\s*[\s*SCENE_NUM\s*]\s*;": scene_arr,
# }
# for regex, replacement in replace_map.items():
#     if not re.search(regex):
#         raise ValueError("scenes.c does not contain SCENE_NUM or scenes[SCENE_NUM].")
#     scene_c_text = re.sub(regex, replacement)
