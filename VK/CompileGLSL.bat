@glslangvalidator -V VS.vert -o VS.vert.spv
@glslangvalidator -V FS.frag -o FS.frag.spv

@copy *.spv Debug\\

@pause