glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
glslc shader_ui.vert -o vert_ui.spv
glslc shader_ui.frag -o frag_ui.spv
cp *.spv ../../../exec/
rm *.spv
