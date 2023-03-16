#!/bin/bash
path="../Test/"  # 测试文件所在文件夹路径
for filename in $(ls $path)
do
    echo -e "\n./parser" $filename
    ./parser $path$filename
done