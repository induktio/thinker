#!/bin/sh

VER=`grep -E 'MOD_VERSION.*[0-9]' src/main.h | grep -Eio 'v[0-9.]+'`
ZIP="rel/Thinker_$VER.zip"

if [ -e "patch/thinker.dll" ]; then
  if ! [ -e $ZIP ]; then
    mkdir -p build/tmp
    cd build/tmp
    cp -f ../../patch/{thinker.exe,thinker.dll} .
    cp -f ../../{Readme,Details,Changelog}.md .
    cp -fr ../../docs/* .
    7z a -mx9 -sdel ../../$ZIP .
  fi
fi
