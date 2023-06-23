#!/bin/bash
path1="../Test/ir-s/"   # 测试文件所在文件夹路径
path2="../result/ir/" # 结果输出文件路径-ir
path3="../result/s/"  # 结果输出文件路径-ir
for filename in $(ls $path1)
do
    if [ "${filename##*.}" = "cmm" ]
    then
        t=${filename##*/}
        echo -e "\n" "\033[1;37;42m" "./parser" $path1$filename "-ir" $path2${t%.*}".ir" "-s" $path3${t%.*}".s" "\033[0m"
        ./parser $path1$filename -ir $path2${t%.*}.ir -s $path3${t%.*}.s
    fi
done