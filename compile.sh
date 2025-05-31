git pull --recurse-submodules
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. -DINFO=ON -DANSWER=ON -DQ_SIZE=$1
make -j8
