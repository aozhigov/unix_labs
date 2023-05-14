#!/bin/bash

make myinit

touch /tmp/in
touch /tmp/out
touch /tmp/myconf

echo /bin/sleep 5 /tmp/in /tmp/out >> /tmp/myconf
echo /bin/sleep 10 /tmp/in /tmp/out >> /tmp/myconf
echo /bin/sleep 15 /tmp/in /tmp/out >> /tmp/myconf

./myinit /tmp/myconf

sleep 1

echo -------- >> result.txt
ps -ef | grep "/bin/sleep 5" >> result.txt
ps -ef | grep "/bin/sleep 10" >> result.txt
ps -ef | grep "/bin/sleep 7" >> result.txt
pkill -f "/bin/sleep 5"

sleep 15

echo /bin/sleep 5 /tmp/in /tmp/out > /tmp/myconf

killall -HUP myinit

echo -------- >> result.txt

ps -ef | grep "/bin/sleep 1" >> result.txt
ps -ef | grep "/bin/sleep 3" >> result.txt

echo LOG >> result.txt

cat /tmp/myinit.log >> result.txt
killall myinit