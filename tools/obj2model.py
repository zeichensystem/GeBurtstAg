import math
import pathlib
import re
import textwrap
from typing import Dict

MAX_FX8 = 2**23 - 1 # Largest number representable as 24.8 fixed point. We will never run into it with non-ridiculous data. 

def float2fx8(n): 
    return int(n * 256)

class Model:
    class ModelParseError(Exception):
        pass

    class Face:
        def __init__(self):
            self.vert_idx = []
            self.normal_idx: int
            self.color = (31, 31, 31)
        
    def __init__(self, filename: pathlib.Path, max_model_verts=None, max_model_faces=None):
        self.name = re.sub(r"\W", "", filename.stem) # Remove non-word characters.
        if len(self.name) < 1:
            raise Model.ModelParseError(f"'{self.name}' is not a valid model name. It also should be a valid name for a C identifier (I don't validate that properly, but it *should*).")
        self.verts = []
        self.faces = []
        self.normals = []
        self.materials = {}
        self.max_model_faces = max_model_faces
        self.max_model_verts = max_model_verts
        self.input_filename = filename
        self.obj_parse(filename)

    def material_parse(self): 
        mtl_file = pathlib.Path(self.input_filename).with_suffix(".mtl")
        if mtl_file.exists():
            current_mtl = ""
            for line in open(mtl_file):
                line = line.strip().lower()
                line_toks = line.split()

                if len(line_toks) == 2 and line_toks[0] == "newmtl":
                    current_mtl = "".join(line_toks[1:])
                    self.materials[current_mtl] = (0,0,0)
                
                elif len(line_toks) >= 4 and line_toks[0] == "kd":
                    self.materials[current_mtl] = (int(float(line_toks[1]) * 31), int(float(line_toks[2]) * 31), int(float(line_toks[3]) * 31))

    def obj_parse(self, filename: pathlib.Path):
        float2fx8 = lambda n: int(n * 256) 

        self.material_parse()

        current_mtl = ""
        for line_num, line in enumerate(open(filename)):
            line = line.strip().lower()
            line_toks = line.split()
            if len(line_toks) == 0:
                continue
            
            if line.startswith("#"):
                continue
            elif line.startswith("s"): # For now, We ignore shading information.
                continue
            elif line.startswith("o"): # For now, we don't treat named objects seperately.
                continue

            elif line_toks[0] == "usemtl": # Face material
                current_mtl = "".join(line_toks[1:])
            elif line.startswith("vn"): # Normals:
                if (len(line_toks) != 4):
                    raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: Vertex-normal has {len(line_toks) - 1} values, but must have exactly 3.")
                try:
                    norm = []
                    for num in line_toks[1:]:
                        num = float(num)
                        if abs(math.ceil(num)) > MAX_FX8:
                            raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: Vertex value {num} overflows the 24.8 signed fixed point format.")
                        norm.append(float2fx8(num))
                    self.normals.append(norm)
                except ValueError:
                    raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: Vertex-normal contains non-number value.")

            elif line_toks[0] == "v": # Vertices:
                if len(line_toks) != 4:
                    raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: Vertex has {len(line_toks) - 1} values, but must have exactly 3.")
                try:
                    vert = []
                    for num in line_toks[1:]:
                        num = float(num)
                        if  abs(math.ceil(num)) > MAX_FX8:
                            raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: Vertex value {num} overflows the 24.8 signed fixed point format.")
                        vert.append(float2fx8(num))
                    self.verts.append(vert)

                except ValueError:
                    raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: Vertex contains non-number value.")

            elif line.startswith("f"): # Faces:
                if len(line_toks) != 4:
                    raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: Face has {len(line_toks) - 1} vertices, but must have exactly 3 (only tris are supported for now, did you triangulate your self ?).")
                face = Model.Face()
                if current_mtl != "":
                    face.color = self.materials[current_mtl]

                faceHasNormal = False
                for i, vertData in enumerate(line_toks[1:]):
                    try:
                        indices = vertData.split("/") # n/n/n
                        vertIdx = int(indices[0]) - 1 # Subtract 1 to compensate for the .obj files being 1-indexed.
                        if vertIdx < 0 or vertIdx > len(self.verts):
                            raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: Vertex index out of range.")
                        
                        face.vert_idx.append(vertIdx)
                        if len(indices) == 3:
                            face.normal_idx = int(indices[2]) - 1 # Subtract 1, see above. 
                            faceHasNormal = True
                    except ValueError as err:
                        raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: '{line}' contains an invalid, non-integer vertex/normal index.")

                if not faceHasNormal:
                    raise Model.ModelParseError(f"Problem in {filename} on line {line_num+1}: Face has no normal.")

                self.faces.append(face)
        

        if self.max_model_verts != None and len(self.verts) > self.max_model_verts:
            raise Model.ModelParseError(f"Model has {len(self.verts)} vertices while MAX_MODEL_VERTS is {self.max_model_verts}.")

        if self.max_model_faces != None and len(self.faces) > self.max_model_faces:
            raise Model.ModelParseError(f"Model has {len(self.faces)} faces while MAX_MODEL_FACES is {self.max_model_faces}.")


    def generate_code(self) ->Dict:
        # Header file: 
        header_file = textwrap.dedent(f"""
        #ifndef {self.name}Model_H
        #define {self.name}Model_H
        #include "../source/model.h"

        extern Model {self.name}Model;
        void {self.name}ModelInit(void);

        #endif
        """)
        # Implementation/data file:
        verts_string = f"EWRAM_DATA Vec3 {self.name}Verts[{len(self.verts)}] = {{"
        faces_string = f"EWRAM_DATA Face {self.name}Faces[{len(self.faces)}] = {{"
        model_string = f"Model {self.name}Model;" 
        model_initfun= f"void {self.name}ModelInit(void) {{ {self.name}Model = modelNew({self.name}Verts, {self.name}Faces, {len(self.verts)}, {len(self.faces)}); }} "

        for i, vert in enumerate(self.verts):
            verts_string += f"{{.x={vert[0]},.y={vert[1]},.z={vert[2]}}}, "
        verts_string += "};"

        for i, face in enumerate(self.faces):
            normal = self.normals[face.normal_idx]
            face_clr = f"{face.color[0] + (face.color[1]<<5) + (face.color[2]<<10)}"
            faces_string += f"{{.vertexIndex = {{{face.vert_idx[0]}, {face.vert_idx[1]}, {face.vert_idx[2]}}}, .color = {face_clr}, .normal={{{normal[0]}, {normal[1]}, {normal[2]}}}, .type=TriangleFace}}, "
        faces_string += "};"

        data_file = textwrap.dedent(f"""
        #include "{self.name}Model.h"

        {model_string}

        {verts_string}

        {faces_string}

        {model_initfun}
        """)
        return {self.name + "Model.h": header_file, self.name + "Model.c": data_file}
       

def read_model_limits():
    """ Reads MAX_MODEL_VERTS and MAX_MODEL_FACES from source/model.h """
    model_h_path = pathlib.Path(".").joinpath(SOURCE_DIR).joinpath("model.h")
    if not model_h_path.exists:
        raise ValueError(f"'{model_h_path.name}' not found (needed to get the MAX_MODEL_VERTS/FACES).")
    MAX_MODEL_VERTS: int
    MAX_MODEL_FACES: int

    with open(model_h_path) as file:
        code = file.read()
        match = re.search(r"\#define\s+MAX_MODEL_VERTS\s+(\d+)", code)
        if match:
            MAX_MODEL_VERTS = int(match.group(1))
        else:
            raise ValueError(f"'{model_h_path.name}' does not define MAX_MODEL_VERTS.")
        match = re.search(r"\#define\s+MAX_MODEL_FACES\s+(\d+)", code)
        if match:
            MAX_MODEL_FACES = int(match.group(1))
        else: 
            raise ValueError(f"'{model_h_path.name}' does not define MAX_MODEL_FACES.")

    return (MAX_MODEL_VERTS, MAX_MODEL_FACES)


# With respect to the project directory.
SOURCE_DIR = "source/"
MODEL_DIR = "assets/models/"
OUT_DIR_DATA = "data-models/"

if __name__ == "__main__":
    MAX_MODEL_VERTS, MAX_MODEL_FACES = read_model_limits()
    models = [Model(filepath, max_model_verts=MAX_MODEL_VERTS, max_model_faces=MAX_MODEL_FACES) for filepath in pathlib.Path(".").joinpath(MODEL_DIR).glob("*.obj")]

    modelsWritten = 0
    infile_paths = [str(model.input_filename.relative_to(pathlib.Path("."))) for model in models]
    outfile_paths = [] 

    for model in models:
        files = model.generate_code() # Create a .h and .c file from the .obj 
        modelsWritten += 1
        for file_basename, file_content in files.items(): # Write the .h and .c files for each model to disk. The .c files contain the actual data (arrays in EWRAM_DATA), and the .h files are to be included to use said data.
            out = pathlib.Path(".").joinpath(OUT_DIR_DATA).joinpath(file_basename) 
            with open(out, "w") as f:
                f.write(file_content)
                outfile_paths.append(str(out.relative_to(pathlib.Path("."))))

    FAIL = '\033[91m'
    OKGREEN = '\033[92m'
    END = '\033[0m'
    if len(models) == 0 and modelsWritten == 0:
        print("Nothing to be done.")
    elif modelsWritten >= len(models):
        print(f"In:\t{' '.join(infile_paths)}\nOut:\t{' '.join(outfile_paths)}")
        print(f"Converted all models {OKGREEN}(Success){END}")
    elif modelsWritten > 0 and modelsWritten < len(models):
        print(f"In:\t{' '.join(infile_paths)}\nOut:\t{' '.join(outfile_paths)}")
        print(f"Some models could not be converted, but no specific error was caught. This is a bug. {FAIL}(Failure){END}")
