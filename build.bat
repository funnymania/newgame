@echo off

mkdir build
pushd build
cl -Zi V:/newgame/newgame.cpp user32.lib Gdi32.lib
popd
