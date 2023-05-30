#!/bin/bash

CONFIG="config.txt"
SOCK_FILE="/tmp/server.sock"
LOG="/tmp/server.log"
NUMBERS="numbers.txt"
ZERO_CHECK="zero"
GENERATOR_SCRIPT="generate.py"
CLIENT="client"
SERVER="server"

make $CLIENT
make $SERVER

rm -f $CONFIG
rm -f $SOCK_FILE
rm -f $LOG
rm -f $NUMBERS

echo $SOCK_FILE >> $CONFIG
python3 $GENERATOR_SCRIPT $NUMBERS
./$SERVER $CONFIG &
sleep 1

function test1 () {
    for i in {1..100}; do
        (./$CLIENT $CONFIG $i 1000 < $NUMBERS) >> /dev/null &
        pids[${i}]=$!
    done

    wait "${pids[@]}"

    echo "fifnished 100 clients" >> result.txt
    echo "Check server state" >> result.txt

    touch $ZERO_CHECK
    echo 0 > $ZERO_CHECK

    ./$CLIENT $CONFIG $i 1000 < $NUMBERS >> /dev/null
    tail -n 1 $LOG >> result.txt
    rm -f $ZERO_CHECK
}

function test2 () {
    test1
    test1

    echo "Check heap" >> result.txt

    head -n 2 $LOG | tail -n 1 >> result.txt
    echo "---" >> result.txt
    tail -n 2 $LOG | head -n 1 >> result.txt
}

function test3 () {
    clients_count=(10 20 30 40 50 60 70 80 90 100)
    delays=(0 0.2 0.4 0.6 0.8 1.0)

    for count in ${clients_count[@]}; do
        for delay in ${delays[@]}; do
            echo "Test: Counts clients = $count; Delay = $delay" >> result.txt
            SECONDS=0

            for ((i=1; i<=$count; i++)); do
                (./$CLIENT $CONFIG $i 30 $delay < $NUMBERS) >> /tmp/client.log &
                pids[${i}]=$!
            done

            wait "${pids[@]}"

            client_time=$(cat /tmp/client.log | grep "client time:" | grep -Eo '[0-9]+' | sort -rn | head -n 1)
            echo "Client time = $client_time" >> result.txt

            duration=$SECONDS
            effective_time=$((duration - client_time))
            echo "Total time = $duration" >> result.txt
            echo "Effective time = $effective_time" >> result.txt
            rm /tmp/client.log
        done
    done

}

echo "Test 1" >> result.txt
test1
echo "" >> result.txt
echo "Test 2" >> result.txt
test2
echo "" >> result.txt
echo "Test 3" >> result.txt
test3
echo "" >> result.txt

pkill -f "./$SERVER $CONFIG"