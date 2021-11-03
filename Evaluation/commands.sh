#!/bin/sh

gcc -o create_taskset create_taskset.c
./create_taskset

gcc -o create_times create_times.c
./create_times

echo "------DPS SCHEDULING-----"
cd DPS/
make
make clean

echo "------EDF-VD SCHEDULING-----"
cd ../EDF-VD/
make
make clean

echo "------EDF-VD-DJ SCHEDULING-----"
cd ../EDF-VD-DJ/
make
make clean

echo "------EDF SCHEDULING-----"
cd ../EDF/
make
make clean

cd ../
rm -f statistics_DPS.txt statistics_EDF-VD.txt statistics_EDF-VD-DJ.txt statistics_EDF.txt

cp DPS/statistics.txt statistics_DPS.txt
cp EDF-VD/statistics.txt statistics_EDF-VD.txt
cp EDF-VD-DJ/statistics.txt statistics_EDF-VD-DJ.txt
cp EDF/statistics.txt statistics_EDF.txt

rm input_allocation.txt input_cores.txt

dir="./discarded/util_4/sample_taskset_5"

mkdir -p ${dir}
mkdir -p ${dir}/DPS ${dir}/EDF-VD ${dir}/EDF-VD-DJ ${dir}/EDF
cp DPS/output* ${dir}/DPS
cp EDF/output* ${dir}/EDF
cp EDF-VD/output* ${dir}/EDF-VD
cp EDF-VD-DJ/output* ${dir}/EDF-VD-DJ

cp input_mcs.txt input_rts.txt input_times.txt statistics_DPS.txt statistics_EDF-VD.txt statistics_EDF-VD-DJ.txt statistics_EDF.txt ${dir}/

rm -rf input* statistics* output* core* create_taskset create_times