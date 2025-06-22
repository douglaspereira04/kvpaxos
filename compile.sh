git pull --recurse-submodules
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. -DENGINE=$1 -DANSWER=OFF
make -j$(nproc)
