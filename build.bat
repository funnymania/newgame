@echo off

IF NOT EXIST build mkdir build
pushd build
del *.pdb
echo.
echo Game Layer
cl -nologo -WX -W4 -wd4189 -wd4701 -wd4505 -wd4201 -wd4100 -wd4996 -wd4530 -wd4018 -DNEWGAME_WIN32=1 -DNEWGAME_INTERNAL=1 -DNEWGAME_SLOW=1 -FC -Zi V:/newgame/newgame.cpp /LD /link -incremental:no /PDB:newgame_%random%.pdb /EXPORT:GameUpdateAndRender
echo.
echo Platform Layer
cl -nologo -WX -W4 -wd4189 -wd4701 -wd4505 -wd4201 -wd4100 -wd4996 -wd4530 -wd4018 -DNEWGAME_INTERNAL=1 -DNEWGAME_SLOW=1 -FC -Zi V:/newgame/win32_newgame.cpp user32.lib Gdi32.lib Ole32.lib Winmm.lib 
popd
