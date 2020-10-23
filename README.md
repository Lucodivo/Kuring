## Kuring Renderer

Experimental renderer using Vulkan. Early stages WIP.

#### Build instructions:
There is a [*build.bat*](build.bat) file in the top most directory. It requires the following:
- MSVC *cl.exe* compiler for the C++ code
	- To access this compiler you must first run *vcvarsall.bat*
		- on my computer this is found at: *C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat*
- [Vulkan SDK](https://vulkan.lunarg.com/) installed
	- This includes the [glslc](https://github.com/google/shaderc/tree/main/glslc) program that compiles GLSL shader files to SPIR-V
		- *build.bat* will need to be modified to properly access glslc
- [GLFW](https://www.glfw.org) lib and include files
	- Environment variables *IncludePath* and *LibraryPath* in *build.bat* will need to be updated to point to the location of your include/lib folders

##### Thanks to:
- [Sascha Willems - Vulkan repository](https://github.com/SaschaWillems/Vulkan/)
- [Alexander Overvoorde - Vulkan Tutorial](https://vulkan-tutorial.com/)