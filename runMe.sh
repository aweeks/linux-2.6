echo "-----"
echo "Executing the make commands"
echo "------\n"

make


if test $? -ne 0
then 
	echo "*******\nMake file failed\n**********"
	exit
fi

make modules_install;

if test $? -ne 0
then 
	echo"*******\nMake file modules failed\n**********"
	exit
fi

echo "Renaming"
mv vmlinux vmlinux-2.6.34.7

echo "Moving the file over"

cp ./arch/i386/boot/bzImage /boot/vmlinuz-2.6.34.7

modprobe look-iosched

echo "Restart to finish the installation" 
