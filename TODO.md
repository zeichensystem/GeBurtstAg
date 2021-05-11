# TODO
- [x] Change model-instance draw options to be properties of the model-instances themselves (so we can have different draw styles for different model-instances and don't have to draw all instances the same)
- [ ] Broadphase with bounding spheres for model-instances
- [ ] Add proper near-plane clipping 
- [x] Fix broken performance measurement (calculate proper averages etc.) 
- [ ] Add basic .obj support
- [ ] Integrate 'apex audio' for .mod support
- [ ] Use division LUTs for triangle-filling (screen space integers) and for the perspective divides (performance optimisation)
- [ ] Give an option to calculate the actual centroid of a face for sorting (instead of our current hack)