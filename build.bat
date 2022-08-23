@echo off

mkdir build
pushd build
cl -FC -Zi V:/newgame/newgame.cpp user32.lib Gdi32.lib Ole32.lib
popd
