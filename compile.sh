git pull --recurse-submodules
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. -DINFO=ON -DANSWER=OFF -DQ_SIZE=$1
make -j$(nproc)
