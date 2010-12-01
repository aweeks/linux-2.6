#!/bin/sh

echo "Hello World" > /test/test_disk.txt

i=0
while [ $i -lt 100000 ]
do
	echo "Hello World" >> /test/test_disk.txt
	i=$(($i + 1));
done

sha1sum /test/test_disk.txt
