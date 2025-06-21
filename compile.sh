git pull --recurse-submodules
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. -DINFO=ON -DLINEARIZABLE=ON -DEDGES=OFF -DENGINE=TKRZW -DANSWER=OFF -DREPARTITIONING=$1 -DQ_SIZE=$2
make -j$(nproc)
