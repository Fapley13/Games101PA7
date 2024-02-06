cd .\build
@echo off
for /r %%F in (*) do if not "%%F"=="fycmake_build.bat" if not "%%F"=="fybuild_run.bat" del "%%F"
for /d %%D in (*) do rmdir "%%D" /s /q
cmake ..
MSBuild.exe .\RayTraycing.sln