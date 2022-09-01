@echo off

mkdir build
pushd build
cl -DNEWGAME_INTERNAL=1 -DNEWGAME_SLOW=1 -FC -Zi V:/newgame/win32_newgame.cpp user32.lib Gdi32.lib Ole32.lib
popd
