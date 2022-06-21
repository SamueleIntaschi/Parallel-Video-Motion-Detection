#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $(basename $0) video percent number_of_workers number_of_tries"
    exit 1

elif [ "$1" = "-help" ]; then
    echo "Usage: $(basename $0) video percent number_of_workers number_of_tries"
	exit 1

fi
    
video=$1
nw=$2
tries=$3

mkdir results

make ff
make nt
make seq

if [ -d $video ]; then
    for ((i=1; i<=$tries; i++)); do
        for file in $video/*; do

            v=$file
            video_name=(${v//// })
            o_file="results/"${video_name[1]} 
            o_file=(${o_file//".mp4"/ })
            o_file=$o_file".txt"
            if [ $video = "Videos/people_low1.mp4" ]; then
                percent=5
            elif [ $video = "Videos/people_low2.mp4" ]; then
                percent=13
            elif [ $video = "Videos/people_low3.mp4" ]; then
                percent=35
            else
                percent=20
            fi

            echo $o_file
            
            ./seq $file $percent -output_file $file
            ./nt $file $percent $nw  -output_file $file
            ./ff $file $percent $nw -output_file $file

        done
    done
else
    for ((i=1; i<=$tries; i++)); do

        v=$video
        video_name=(${v//// })
        file="results/"${video_name[1]} 
        file=(${file//".mp4"/ })
        file=$file".txt"
        if [ $video = "Videos/people_low1.mp4" ]; then
            percent=5
        elif [ $video = "Videos/people_low2.mp4" ]; then
            percent=13
        elif [ $video = "Videos/people_low3.mp4" ]; then
            percent=35
        else
            percent=20
        fi

        ./seq $video $percent -output_file $file
        ./nt $video $percent $nw -output_file $file
        ./ff $video $percent $nw -output_file $file

    done
fi
exit 0