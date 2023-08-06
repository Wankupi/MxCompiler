#!/bin/bash

set -e

sed -n '/=== input ===/,/=== end ===/{//!p}' "$2" >input.txt
sed -n '/=== output ===/,/=== end ===/{//!p}' "$2" >ans.txt

echo 'compile to llvm IR'
$1 $2
echo 'compile to binary'
clang -m32 test.ll builtin.ll -o binary 1>/dev/null 2>/dev/null
echo 'run binary'
./binary <input.txt >output.txt
echo 'compare output'
diff -Z ans.txt output.txt
