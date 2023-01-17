#!/bin/sh
if [ $1 != ""  ]
then
cd "$1"
fi
echo pwd
rm -rf stunning
rm -rf build
cmake ./CMakeLists.txt -B build/ -DCMAKE_BUILD_TYPE=Debug
cd build
make
cp stunning ../
cd ../
