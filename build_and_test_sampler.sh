#! /bin/sh

echo "Beginning build and test of tlm2freesampler."

#cd tlm2freesampler
cd ~/work/tlm2freesampler
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make
./tlm2freesampler
return $?
