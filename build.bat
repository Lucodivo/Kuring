@REM compile shaders to SPIR-V
C:/VulkanSDK/1.2.141.2/Bin32/glslc.exe src/shaders/test_vertex_shader.vert -o build/vert.spv
C:/VulkanSDK/1.2.141.2/Bin32/glslc.exe src/shaders/test_fragment_shader.frag -o build/frag.spv

@echo off
REM MTd - Application will use the multithread, static version of the run-time library. Uses LIBCMTD.lib to resolve symbols. Debug version.
REM MDd - Application will use the multithread-specific and DLL-specific version of the run-time library. Uses MSVCRT.lib to resolve symbols. Debug version.
REM nologo - Surpresses display of copyright bannerwhen compiler stars up
REM fp:fast - Specifies floating point behavior, fast is optimized for speed and space]
REM Gm - Enables minimal rebuild
REM GR - Check object types at runtime
REM EHa - Error Handling all
REM Od - Disable optimizations
REM Oi - Generate intrinsic functions
REM WX - Treats all warnings as errors
REM W4 - Display level 1-4 warnings that aren't off by default
REM wd4201 - disable nameless struct/union warning
REM wd4505 - disable unused function warning
REM wd4100 - disable unreferenced parameter warning
REM wd4189 - disable unreferenced local variable warning
REM FC - Display full path of source code files passed to compiler in diagnostics
REM Z7 - Produce object files that also contain full symbolic debugging information for use with a debugger
set IncludePath=C:/developer/dependencies/include
set LibraryPath=C:/developer/dependencies/libs
set CommonCompilerOptions=-MDd -nologo -fp:fast -GR -EHa -Od -Oi -WX -W4 -wd4201 -wd4505 -wd4100 -wd4189 -wd4530 -wd4324 -FC -Z7 -I%IncludePath% -NOT_DEBUG=0
REM incremental:no - Disables incremental linking
REM opt:Ref - eliminates functions and data that are never referenced
set CommonLinkerOptions= -incremental:no -opt:ref gdi32.lib kernel32.lib user32.lib shell32.lib /LIBPATH:%LibraryPath% glfw3-x64-d.lib vulkan-1.lib

IF NOT EXIST build mkdir build
REM pushd: cd and save previous dir
pushd build

REM Fm - Tells linker to produce a mapfile
@echo on
cl %CommonCompilerOptions% ../src/*.cpp -Fmmain.map /link %CommonLinkerOptions%
@echo off
REM popd: pop previously saved dir
popd
