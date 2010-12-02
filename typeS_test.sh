#!/bin/sh

#For the typescript, you must execute with 'script -c ./typeS_test.sh'

test_diskf(){
	set +x
	echo "Hello World" > ./test/test_disk.txt

	i=0
	while [ $i -lt $1 ]
	do
		echo "Hello World" >> ./test/test_disk.txt
		i=$(($i + 1));
	done
	set -x
}

set -x

modprobe look-iosched

cat /sys/block/sdb/queue/scheduler

echo test > /sys/block/sdb/queue/scheduler

cat /sys/block/sdb/queue/scheduler

set +x

echo "Testing three levels of reading and writing with different file sizes"
echo "Variable size is number of lines of 'Hello World' in the file."
echo "See test_disk.sh for more information"

set -x

test_diskf 10
cat test_disk.txt > /dev/null

test_diskf 100
cat test_disk.txt > /dev/null

test_diskf 1000
cat test_disk.txt > /dev/null

echo anticipatory > /sys/block/sdb/queue/scheduler
rmmod look-iosched

exit
