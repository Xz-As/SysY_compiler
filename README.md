# SysY_compiler
Project Description.
SysY compiler that compiles SysY files and generates arm instructions
Writing language: C++
Project has been compiled by g++
The "unistd.h" code in the project is to fill in the missing unistd libraries when running under windows.


To run the project.
"""
. /compiler_test file_path -S -o output_path
"""
file_path is the sample file path, output_path is the output file path
The command will be output to output_path, where

Algorithm test.
In the . /test path
Testing with the qemu-arm virtual machine
The compiler generates the output of the assembly execution in the .output file and the examples should be output in the .out file
Output comparison.
"""
x=0
y=0
for file in *.sy	
do
 qemu-arm ${file%.sy}"_run">${file%.sy}".output" #2>/dev/null
 diff -q -B -w -b ${file%.sy}".output" ${file%.sy}".out">/dev/null
 if [ $? == 0 ];then
  echo -e "\033[32m PASS "$file"\033[0m"
  let x++;
 else
  echo -e "\033[31m WRONG "$file"\033[0m"
 fi
 let y++
done
echo "PASS/TOTAL = "$x" / "$y
"""

qemu generate + output comparison
"""
x=0
y=0
for file in *.sy	
do 
 ... /compiler_test $file -S -o ${file%.sy}".s">/dev/null 2>&1
 arm-linux-gnueabihf-g++ ${file%.sy}".s" -o ${file%.sy}"_run" . /libsysy.so >/dev/null 2>&1
 qemu-arm ${file%.sy}"_run">${file%.sy}".output" #2>/dev/null
 diff -q -B -w -b ${file%.sy}".output" ${file%.sy}".out">/dev/null
 if [ $? == 0 ];then
  echo -e "\033[32m PASS "$file"\033[0m"
  let x++;
 else
  echo -e "\033[31m WRONG "$file"\033[0m"
 fi
 let y++
done
echo "PASS/TOTAL = "$x" / "$y
"""

When running under windows, the code in the please set
"""
include <unistd.h>
"""
to
include "unistd.h"
Just compile and run directly in the compiler


File location.
The test samples are in the folder, the .sy file is the sample file
The output file path should be in a directory that already exists, an out folder has been created to store the output, or you can create your own folder

Author: Ma Yanxiang

Translated with www.DeepL.com/Translator (free version)
