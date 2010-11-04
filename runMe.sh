#!/bin/sh

rm vmlinux-2.6.34.7

echo "-----"
echo "Executing the make commands"
echo "------\n"

make

if ("$?" != 0) 
	then 
		echo "Make file failed"
		exit
fi

make modules_install;

if ("$?" != 0) 
	then 
		echo "Make file Modules failed"
		exit
fi

echo "Renaming"
mv vmlinux vmlinux-2.6.34.7

echo "Moving the file over"

cp ./arch/i386/boot/bzImage /boot/vmlinuz-2.6.34.7

echo "Restart to finish the installation" 
