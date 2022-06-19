#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $(basename $0) video percent number_of_workers number_of_tries"
    exit 1

elif [ "$1" = "-help" ]; then
    echo "Usage: $(basename $0) video percent number_of_workers number_of_tries"
	exit 1

fi
    
video=$1
percent=$2
nw=$3
tries=$4

make ff
make nt
make seq

if [ -d $video ]; then
    for ((i=1; i<=$tries; i++)); do
        for file in $video/*; do
            ./seq $file $percent  
            ./nt $file $percent $nw
            ./ff $file $percent $nw
        done
    done
else
    for ((i=1; i<=$tries; i++)); do
        ./seq $video $percent  
        ./nt $video $percent $nw
        ./ff $video $percent $nw
    done
fi
exit 0