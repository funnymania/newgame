@echo off

IF NOT EXIST build mkdir build
pushd build
cl -WX -W4 -wd4189 -wd4701 -wd4505 -wd4201 -wd4100 -wd4996 -wd4530 -wd4018 -DNEWGAME_INTERNAL=1 -DNEWGAME_SLOW=1 -FC -Zi V:/newgame/win32_newgame.cpp user32.lib Gdi32.lib Ole32.lib Winmm.lib
popd
