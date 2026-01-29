#!/bin/bash
./build.sh release windows
./build.sh release
rm lyte.zip 2>/dev/null
cp winlib/SDL3-3.2.16/x86_64-w64-mingw32/bin/SDL3.dll SDL3.dll
strip lyte
strip lyte.exe
strip SDL3.dll
zip lyte.zip lyte lyte.exe SDL3.dll data -r
