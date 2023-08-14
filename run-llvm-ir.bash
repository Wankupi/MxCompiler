#!/bin/bash

set -e

sed -n '/=== input ===/,/=== end ===/{//!p}' "$2" >test.in
sed -n '/=== output ===/,/=== end ===/{//!p}' "$2" >test.ans
exitCode=$(grep 'ExitCode:' "$2" | sed 's/ExitCode: //g')

echo 'compile to llvm IR'
$1 -emit-llvm $2 >test.ll
echo 'compile to binary'
clang -m32 test.ll builtin.ll -o binary 1>/dev/null 2>/dev/null
echo 'run binary'

set +e
./binary <test.in >test.out
if [[ $? -ne $exitCode ]]; then
	echo 'exit code not match, should be ' "$exitCode" ' but got ' $?
	exit 1
fi
set -e
echo 'compare output'
diff -qZB test.out test.ans
