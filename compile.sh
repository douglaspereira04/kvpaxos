{
  git pull --recurse-submodules
  git submodule update --init --recursive
  mkdir build
  cd build
  cmake -DCMAKE_BUILD_TYPE=Release .. -DINFO=ON -DLINEARIZABLE=ON -DENGINE=$1 -DANSWER=OFF -DREPARTITIONING=$2 -DQ_SIZE=$3
  make -j$(nproc)
} 2>&1 | tee build_output.log