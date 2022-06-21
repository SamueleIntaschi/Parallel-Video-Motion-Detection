#!/bin/bash

#TODO do this operation for each number of threads and for each file

cd results
for file in $(command ls); do
    if [ $file != "results.txt" ]; then
        sum_ff=0
        cnt_ff=0
        while IFS="," read -r record type percent nw show usec result; do
            if [ $type = "ff" ]; then
                sum_ff=$(($sum_ff + $usec))
                cnt_ff=$(($cnt_ff + 1))
            fi
        done < "$file"
        avg_ff=$(($sum_ff/$cnt_ff))
        echo $avg_ff
    fi
done

exit 0