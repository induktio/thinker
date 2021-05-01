#!/bin/sh

ZIP="rel/Thinker-dev-$(date '+%Y%m%d').zip"
if [ -e "patch/thinker.dll" ]; then
  if ! [ -e $ZIP ]; then
    mkdir -p build/tmp
    cd build/tmp
    cp -f ../../patch/thinker.dll .
    cp -fr ../../docs/* .
    7z a -mx9 -sdel ../../$ZIP .
  fi
fi
