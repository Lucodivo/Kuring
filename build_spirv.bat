@REM compile shaders to SPIR-V
@pushd build
C:/VulkanSDK/1.2.141.2/Bin32/glslc.exe -c ../src/shaders/test_vertex_shader.vert
C:/VulkanSDK/1.2.141.2/Bin32/glslc.exe -c ../src/shaders/test_fragment_shader.frag
@popd