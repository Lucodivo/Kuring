@call build_spirv.bat

@echo off
rem calling vcvarsall is unnecessary if the command prompt has previously called it
if "%1"=="" goto vcvarsall_skip
if "%1"=="vcvarsall" call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
:vcvarsall_skip

rem MTd - Application will use the multithread, static version of the run-time library. Uses LIBCMTD.lib to resolve symbols. Debug version.
rem MDd - Application will use the multithread-specific and DLL-specific version of the run-time library. Uses MSVCRT.lib to resolve symbols. Debug version.
rem nologo - Surpresses display of copyright bannerwhen compiler stars up
rem fp:fast - Specifies floating point behavior, fast is optimized for speed and space]
rem Gm - Enables minimal rebuild
rem GR - Check object types at runtime
rem EHa - Error Handling all
rem Od - Disable optimizations
rem Oi - Generate intrinsic functions
rem WX - Treats all warnings as errors
rem W4 - Display level 1-4 warnings that aren't off by default
rem wd4201 - disable nameless struct/union warning
rem wd4505 - disable unused function warning
rem wd4100 - disable unreferenced parameter warning
rem wd4189 - disable unreferenced local variable warning
rem wd4458 - disable function parameter hides class member
rem FC - Display full path of source code files passed to compiler in diagnostics
rem Z7 - Produce object files that also contain full symbolic debugging information for use with a debugger
set IncludePath=C:/developer/dependencies/include
set LibraryPath=C:/developer/dependencies/libs
set CommonCompilerOptions=-MDd -nologo -fp:fast -GR -EHa -Od -Oi -WX -W4 -wd4201 -wd4505 -wd4100 -wd4189 -wd4530 -wd4324 -wd4458 -FC -Z7 -I%IncludePath% -DNOT_DEBUG=0
rem incremental:no - Disables incremental linking
rem opt:Ref - eliminates functions and data that are never referenced
set CommonLinkerOptions= -incremental:no -opt:ref gdi32.lib kernel32.lib user32.lib shell32.lib /LIBPATH:%LibraryPath% glfw3-x64-d.lib vulkan-1.lib

IF NOT EXIST build mkdir build
rem pushd: cd and save previous dir
pushd build

rem Fm - Tells linker to produce a mapfile
@echo on
cl %CommonCompilerOptions% ../src/*.cpp -Fmmain.map -FeKuring.exe /link %CommonLinkerOptions%
@echo off
rem popd: pop previously saved dir
popd

