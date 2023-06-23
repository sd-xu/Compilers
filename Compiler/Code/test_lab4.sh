#!/bin/bash
path1="../Test/lab4/"   # 测试文件所在文件夹路径
path2="../result/s/" # 结果输出文件路径
for filename in $(ls $path1)
do
    if [ "${filename##*.}" = "cmm" ]
    then
        t=${filename##*/}
        echo -e "\n" "\033[1;37;42m" "./parser" $path1$filename "-s" $path2${t%.*}".s" "\033[0m"
        ./parser $path1$filename -s $path2${t%.*}.s
    fi
done