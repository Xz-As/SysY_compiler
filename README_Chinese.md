# SysY_compiler
项目介绍：
SysY编译器，可以编译SysY文件，生成arm指令
编写语言：C++
项目已经由g++编译好了
项目中"unistd.h"代码是为了在windows下运行时，填补缺失的unistd库


运行项目：
"""
./compiler_test file_path -S -o output_path
"""
file_path为样例文件路径，output_path为输出文件路径
指令会被输出到output_path中，其中

算例测试：
在./test路径下
使用qemu-arm虚拟机进行测试
编译器生成汇编执行后得到的输出在.output文件，算例应有输出在.out文件
输出对比：
"""
x=0
y=0
for file in *.sy	
do
 qemu-arm ${file%.sy}"_run">${file%.sy}".output" #2>/dev/null
 diff -q -B -w -b ${file%.sy}".output" ${file%.sy}".out">/dev/null
 if [ $? == 0 ];then
  echo -e "\033[32m PASS  "$file"\033[0m"
  let x++;
 else
  echo -e "\033[31m WRONG "$file"\033[0m"
 fi
 let y++
done
echo "PASS/TOTAL = "$x" / "$y
"""

qemu生成＋输出对比
"""
x=0
y=0
for file in *.sy	
do 
 ../compiler_test $file -S -o ${file%.sy}".s">/dev/null 2>&1
 arm-linux-gnueabihf-g++ ${file%.sy}".s" -o ${file%.sy}"_run" ./libsysy.so >/dev/null 2>&1
 qemu-arm ${file%.sy}"_run">${file%.sy}".output" #2>/dev/null
 diff -q -B -w -b ${file%.sy}".output" ${file%.sy}".out">/dev/null
 if [ $? == 0 ];then
  echo -e "\033[32m PASS  "$file"\033[0m"
  let x++;
 else
  echo -e "\033[31m WRONG "$file"\033[0m"
 fi
 let y++
done
echo "PASS/TOTAL = "$x" / "$y
"""

在windows下运行时，代码中的请将
"""
include <unistd.h>
"""
改为
include "unistd.h"
直接在编译器中编译运行即可


文件位置：
测试样例在文件夹中，.sy文件就是样例文件
输出文件路径中目录应是已存在的，已经建立了out文件夹用于存放输出，也可以自己新建文件夹

作者：马彦祥
