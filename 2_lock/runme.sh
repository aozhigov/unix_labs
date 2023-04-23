#!/bin/bash

COUNT=10
RESULT_FILE_NAME="results.txt"

rm -f lock.lck statistics
make -s locker

pids=()
for (( i=0; i<$COUNT; i++ ))
do
	./locker lock &
	pids+=($!)
done

sleep 5m

completed=0
for pid in ${pids[@]}
do
	kill -SIGINT $pid
	if [[ $? -eq 0 ]]
	then
		((completed++))
	fi
done

echo "Expected - " $COUNT >>"$RESULT_FILE_NAME"
echo "Actual - " $completed >>"$RESULT_FILE_NAME"