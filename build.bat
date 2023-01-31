@echo off

IF NOT EXIST build mkdir build
pushd build
del *.pdb
REM echo Copy NettleDark
REM copy V:\odin-queue\build\odin-queue.obj
REM copy V:\odin-queue\build\odin-queue.pdb
copy V:\odin-queue\build\odin-queue.lib
REM echo.
echo Game Layer
cl -nologo -WX -W4 -wd4207 -wd4005 -wd4838 -wd4189 -wd4701 -wd4505 -wd4201 -wd4100 -wd4996 -wd4530 -wd4018 -DNEWGAME_WIN32=1 -DNEWGAME_INTERNAL=1 -DNEWGAME_SLOW=1 -FC -Zi V:/inner_output/newgame.cpp /LD /link -incremental:no /PDB:newgame_%random%.pdb /EXPORT:GameUpdateAndRender
echo.
echo Platform Layer
cl -nologo -WX -W4 -wd4324 -wd4005 -wd4189 -wd4701 -wd4505 -wd4201 -wd4100 -wd4996 -wd4530 -wd4018 -DNEWGAME_INTERNAL=1 -DNEWGAME_SLOW=1 -FC /Fei.o.exe -Zi V:/inner_output/win32_newgame.cpp user32.lib Gdi32.lib Ole32.lib Winmm.lib runtimeobject.lib Advapi32.lib Propsys.lib V:/odin-queue/build/odin-queue.obj Ws2_32.lib
popd
