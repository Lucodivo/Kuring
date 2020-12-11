@REM compile shaders to SPIR-V
@pushd build
C:/VulkanSDK/1.2.141.2/Bin32/glslc.exe -c ../src/shaders/PosColor.vert
C:/VulkanSDK/1.2.141.2/Bin32/glslc.exe -c ../src/shaders/RayMarchSphere.frag
@popd