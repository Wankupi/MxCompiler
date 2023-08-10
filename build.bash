#!/bin/bash

cd "$(dirname "$0")" || exit
root=$(pwd)
cd resources/antlr4 || exit
antlr4 -no-listener -visitor -o "${root}/MxAntlr" MxLexer.g4 MxParser.g4 -Dlanguage=Cpp
cd ${root}
