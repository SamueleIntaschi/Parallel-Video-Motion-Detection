#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $(basename $0) type nw k"
    exit 1

elif [ "$1" = "-help" ]; then
    echo "Usage: $(basename $0) type nw k"
	exit 1

fi

t=$1
p=$3
nwp=$2
for file in $(command ls); do
    if [[ $file == *.txt ]]; then
        echo "In file $file, with k = $p, average values for number of threads are"
        for ((i=-1; i<=$nwp; i++)); do
            #./compute_avg.sh $file $t $i $p 
            sum=0
            cnt=0
            while IFS="," read -r record type percent nw show usec result; do
                if [ $nw = $i ]; then
                    if [ $type = $t ]; then
                        if [ $percent = $p ]; then
                            sum=$(($sum + $usec))
                            cnt=$(($cnt + 1))
                        fi
                    fi
                fi
            done < "$file"
            if [ $cnt -gt 0 ]; then
                avg=$(($sum/$cnt))
                echo "$i $avg"
            fi
        done
    fi
done

exit 0