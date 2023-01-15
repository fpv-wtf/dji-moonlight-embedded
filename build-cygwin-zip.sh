#!/bin/bash
set -xe

rm -rf build-cyg
mkdir build-cyg

cd build-cyg
cmake ../
make -j$(nproc)

mkdir bundle
mv ./moonlight.exe ./bundle

cp ./libgamestream/*.dll ./bundle
ldd ./bundle/moonlight.exe \
  | grep "cy.* => /" \
  | awk '{print $3}' \
  | xargs -I{} sh -c "cp {} ./bundle || true"

cd bundle
cp ../libgamestream/*.dll .
zip -r ../dji-moonlight-embedded-cygwin.zip .
