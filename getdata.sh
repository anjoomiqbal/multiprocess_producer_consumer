#!/bin/bash


echo "gcc multiprocess.c -o server -lpthread -lm"
gcc multiprocess.c -o server -lpthread -lm

file_name="./output/output_B_BUFFER_SIZE.txt"
echo "output file path $file_name"
x=1
i=0
for ((i=0;i<6;i++))
do
    gcc multiprocess.c -o server -lpthread -lm
	echo "i= $i ...............changing B_BUFFER_SIZE = $x......................"
	echo "i= $i ...............changing B_BUFFER_SIZE = $x......................" &>> $file_name
    echo "./server T   B   P   C   Pt   Rs   Ct1   Ct2   Pi"
    echo "./server 200 $x 200 20 50 50 50 50 50"
          ./server 200 $x 200 20 50 50 50 50 50 &>> $file_name
    x=$(($x*4))
done



file_name="./output/output_P_COUNT.txt"
echo "output file path $file_name"
x=1
i=0
for ((i=0;i<6;i++))
do
	echo "i= $i ...............changing P_COUNT = $x......................" &>> $file_name
	echo "i= $i ...............changing P_COUNT = $x......................"
    echo "./server T   B   P   C   Pt   Rs   Ct1   Ct2   Pi"
    echo "./server 120 200 $x 200 50 50 50 50 50"
          ./server 120 200 $x 200 50 50 50 50 50 &>> $file_name
    x=$(($x*4))
done



file_name="./output/output_C_COUNT.txt"
echo "output file path $file_name"
x=1
i=0
for ((i=0;i<8;i++))
do
	echo "i= $i ...............changing C_COUNT = $x......................" &>> $file_name
	echo "i= $i ...............changing C_COUNT = $x......................"
    echo "./server T   B   P   C   Pt   Rs   Ct1   Ct2   Pi"
    echo "./server 200 200 200 $x 50 50 50 50 50"
          ./server 200 200 200 $x 50 50 50 50 50 &>> $file_name
    x=$(($x*4))
done



file_name="./output/output_P_t_PRODUCER_DELAY_DIST_PARAM.txt"
echo "output file path $file_name"
x=1
i=0
for ((i=0;i<6;i++))
do
	echo "i= $i ...............changing P_t_PRODUCER_DELAY_DIST_PARAM = $x......................" &>> $file_name
	echo "i= $i ...............changing P_t_PRODUCER_DELAY_DIST_PARAM = $x......................"
    echo "./server T   B   P   C   Pt   Rs   Ct1   Ct2   Pi"
    echo "./server 200 200 20 20 $x 50 50 50 50"
          ./server 200 200 200 20 $x 50 50 50 50 &>> $file_name
    x=$(($x*4))
done



file_name="./output/output_R_s_REQUEST_SIZE_DIST_PARAM.txt"
echo "output file path $file_name"
x=1
i=0
for ((i=0;i<6;i++))
do
	echo "i= $i ...............changing R_s_REQUEST_SIZE_DIST_PARAM = $x......................" &>> $file_name
	echo "i= $i ...............changing R_s_REQUEST_SIZE_DIST_PARAM = $x......................"
    echo "./server T   B   P   C   Pt   Rs   Ct1   Ct2   Pi"
    echo "./server 200 200 20 20 50 $x 50 50 50"
          ./server 200 200 200 20 50 $x 50 50 50 &>> $file_name
    x=$(($x*4))
done



file_name="./output/output_C_t1_CONSUMED_WITH_IO_ONLY_DIST_PARAM.txt"
echo "output file path $file_name"
x=1
i=0
for ((i=0;i<6;i++))
do
	echo "i= $i ...............changing C_t1_CONSUMED_WITH_IO_ONLY_DIST_PARAM = $x......................" &>> $file_name
	echo "i= $i ...............changing C_t1_CONSUMED_WITH_IO_ONLY_DIST_PARAM = $x......................"
    echo "./server T   B   P   C   Pt   Rs   Ct1   Ct2   Pi"
    echo "./server 120 200 200 20 50 50 $x 50 50"
          ./server 120 200 200 20 50 50 $x 50 50 &>> $file_name
    x=$(($x*4))
done



file_name="./output/output_C_t2_CONSUMED_WITH_IO_DISC_DB_DIST_PARAM.txt"
echo "output file path $file_name"
x=1
i=0
for ((i=0;i<6;i++))
do
	echo "i= $i ...............changing C_t2_CONSUMED_WITH_IO_DISC_DB_DIST_PARAM = $x......................" &>> $file_name
	echo "i= $i ...............changing C_t2_CONSUMED_WITH_IO_DISC_DB_DIST_PARAM = $x......................"
    echo "./server T   B   P   C   Pt   Rs   Ct1   Ct2   Pi"
    echo "./server 120 200 200 20 50 50 50 $x 50"
          ./server 120 200 200 20 50 50 50 $x 50 &>> $file_name
    x=$(($x*4))
done



file_name="./output/output_P_i_PROBABILITY_OF_C_t1.txt"
echo "output file path $file_name"
x=1
i=0
for ((i=0;i<6;i++))
do
	echo "i= $i ...............changing P_i_PROBABILITY_OF_C_t1 = $x......................" &>> $file_name
	echo "i= $i ...............changing P_i_PROBABILITY_OF_C_t1 = $x......................"
    echo "./server T   B   P   C   Pt   Rs   Ct1   Ct2   Pi"
    echo "./server 120 200 200 20 50 50 50 50 $x"
          ./server 120 200 200 20 50 50 50 50 $x &>> $file_name
    x=$(($x*4))
done


file_name="./output/output_TOTAL_RUN_TIME.txt"
echo "output file path $file_name"

x=2
i=0
for ((i=0;i<6;i++))
do
	echo "i= $i ...............changing TOTAL_RUN_TIME = $x ......................" &>> $file_name
	echo "i= $i ...............changing TOTAL_RUN_TIME = $x ......................"
    echo "./server T   B   P   C   Pt   Rs   Ct1   Ct2   Pi"
    echo "./server $x 100 20 20 50 50 50 50 50"
          ./server $x 100 20 20 50 50 50 50 50 &>> $file_name
    x=$(($x*4))
done


