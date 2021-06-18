#!/bin/sh

EXEC_TIMES="input_times.txt"

cp input_mcs.txt DPS/input.txt
cp input_mcs.txt EDF-VD/input.txt
cp input_rts.txt EDF/input.txt

for DIR in "DPS" "EDF-VD" "EDF"; do cp $EXEC_TIMES $DIR/$EXEC_TIMES; done

cd DPS/
./commands.sh

cd ../EDF-VD/
./commands.sh

cd ../EDF/
./commands.sh

cd ../
rm -f statistics_DPS.txt statistics_EDF-VD.txt statistics_EDF.txt

cp DPS/statistics.txt statistics_DPS.txt
cp EDF-VD/statistics.txt statistics_EDF-VD.txt
cp EDF/statistics.txt statistics_EDF.txt