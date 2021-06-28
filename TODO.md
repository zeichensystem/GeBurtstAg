# TODO

## Important Features
- [ ] Integration of 'apex audio' for .mod support       
- [ ] Camera paths (splines?)
- [ ] Animations (and "native" wireframe model support (only edges, not faces; maybe even 2d))
- [ ] Particle systems
- [ ] Affine texture mapping (cf. fatmap.txt)

## Implementation details and Bugfixes       
- [ ] Proper near-plane clipping 
- [ ] Use division LUTs for triangle-filling (integers) and for perspective divides (fixed point)
- [ ] Broadphase with bounding spheres for model-instances (and option for models with fewer faces which get activated if their distance to the camera is large).
- [ ] use sin_lut instead of fxSin for better accuracy maybe. 
- [ ] Put models into ROM (const)   
- [ ] Option for pre-sorted geometry (in case the camera moves only backward/forwards etc. it would be more efficient).
- [ ] Option to calculate the actual centroid of a face for sorting
- [ ] Better handling of lookAt singularity (looking completely down/up)

## Done
- [x] Change model-instance draw options to be properties of the model-instances themselves (so we can have different draw styles for different model-instances and don't have to draw all instances the same)
- [x] Fix broken performance measurement (calculate proper averages etc.) 
- [x] Fix ugly rasterisation bugs
- [x] Faster triangle filling (refer to fatmap.txt)
- [x] Basic .obj support
- [x] Simplify perspective calculations                    
- [x] Don't recalculate vertex projections for each face! 
- [x] Better polygon sorting (ordering table)
- [x] Mode switching
- [x] Backface culling winding order/normal problem
- [x] Key chording for scene switching

- [x] Handle .obj colours on import (so far, we just set all colours to a default one).
- [x] Use libtonc qran instead of rand 
