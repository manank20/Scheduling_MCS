#!/bin/sh

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

rm input_allocation.txt input_cores.txt