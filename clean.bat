@echo OFF
setlocal
set "sourcedir=build"
set "keepfile=main.sln"
set "keepdir=.vs"

rem remove files for clean
for /d %%a in ("%sourcedir%\*") do if /i not "%%~nxa"=="%keepdir%" rd /S /Q "%%a"
for %%a in ("%sourcedir%\*") do if /i not "%%~nxa"=="%keepfile%" del "%%a"

endlocal