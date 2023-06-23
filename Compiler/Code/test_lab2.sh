#!/bin/bash
path="../Test/lab2/"  # 测试文件所在文件夹路径
for filename in $(ls $path)
do
    if [ "${filename##*.}" = "cmm" ]
    then
        echo -e "\n" "\033[1;37;42m" "./parser" $filename "\033[0m"
        ./parser $path$filename
    fi
done