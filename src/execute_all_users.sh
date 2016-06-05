#!/bin/sh


UTENTI="3 4 17 30 41 62 68 128 153 163"

for USER in $UTENTI
do
	nohup ./gigeo $USER /home/nds/geolife.db > "log_"$USER"_accuracy.txt" 2>&1 &
done
