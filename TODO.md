# TODO

## Important Features
- [ ] Basic .obj support
- [ ] Integration of 'apex audio' for .mod support
- [ ] Camera paths (splines?)
- [ ] Mode switching
- [ ] Particle systems
- [ ] Affine texture mapping (cf. fatmap.txt)

## Implementation details and Bugfixes
- [ ] Broadphase with bounding spheres for model-instances
- [ ] Proper near-plane clipping 
- [ ] Use division LUTs for triangle-filling (integers) and for perspective divides (fixed point)
- [ ] Fix ugly rasterisation bugs
- [ ] Faster triangle filling (refer to fatmap.txt)
- [ ] Option to calculate the actual centroid of a face for sorting
- [ ] Better handling of lookAt singularity (looking completely down/up)

## Done
- [x] Change model-instance draw options to be properties of the model-instances themselves (so we can have different draw styles for different model-instances and don't have to draw all instances the same)
- [x] Fix broken performance measurement (calculate proper averages etc.) 
