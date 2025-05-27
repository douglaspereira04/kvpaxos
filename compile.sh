git submodule update --init --recursive
git pull --recurse-submodules
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. -DINFO=ON -DANSWER=OFF -DQ_SIZE=$1
make -j8
