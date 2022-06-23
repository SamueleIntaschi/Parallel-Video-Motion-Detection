#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $(basename $0) nw"
    exit 1

elif [ "$1" = "-help" ]; then
    echo "Usage: $(basename $0) nw"
	exit 1
fi

nwp=$1
for file in $(command ls); do
    if [[ $file == *.txt ]]; then
        sum_ff=0
        sum_nt=0
        sum_seq=0
        cnt_ff=0
        cnt_nt=0
        cnt_seq=0
        while IFS="," read -r record type percent nw show usec result; do
            if [ $nw = $nwp ]; then
                if [ $type = "ff" ]; then
                    sum_ff=$(($sum_ff + $usec))
                    cnt_ff=$(($cnt_ff + 1))
                elif [ $type = "nt" ]; then
                    sum_nt=$(($sum_nt + $usec))
                    cnt_nt=$(($cnt_nt + 1))
                elif [ $type = "seq" ]; then
                    sum_seq=$(($sum_seq + $usec))
                    cnt_seq=$(($cnt_seq + 1))
                fi
            fi
        done < "$file"
        echo "In file $file:"
        if [ $cnt_ff -gt 0 ]; then
            avg_ff=$(($sum_ff/$cnt_ff))
            echo "Average time for fastflow implementation: $avg_ff on a total of $cnt_ff executions"
        fi
        if [ $cnt_nt -gt 0 ]; then
            avg_nt=$(($sum_nt/$cnt_nt))
            echo "Average time for native threads implementation: $avg_nt on a total of $cnt_nt executions"
        fi
        if [ $cnt_seq -gt 0 ]; then
            avg_seq=$(($sum_seq/$cnt_seq))
            echo "Average time for sequential implementation: $avg_seq on a total of $cnt_seq executions"
        fi
    fi
done

exit 0