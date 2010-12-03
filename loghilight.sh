#!/bin/bash
for line in `tail -n 10 /var/log/syslog`
do
    echo ${line} | GREP_COLOR='1;32' grep ADD --color=ALWAYS
    echo ${line} | GREP_COLOR='1;31' grep DSP --color=ALWAYS
done
