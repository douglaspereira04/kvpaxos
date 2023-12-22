#!/bin/bash

track_length=(0 1000 10000 100000)
q_size=(0 1000 10000 100000)


for track_length_ in "${track_length[@]}"; do
    for q_size_ in "${q_size[@]}"; do

        #./compile.sh TBB FREE OLD_GRAPH ${track_length_} ${q_size_}
        #mv ./build/bin/replica ./build/bin/free_${track_length_}_${q_size_}

        ./compile.sh TBB NON_STOP OLD_GRAPH ${track_length_} ${q_size_}
        mv ./build/bin/replica ./build/bin/non_stop_${track_length_}_${q_size_}


        ./compile.sh TBB OLD OLD_GRAPH ${track_length_} ${q_size_}
        mv ./build/bin/replica ./build/bin/old_${track_length_}_${q_size_}
    done;
done;

cp -r experiments/* build/bin/