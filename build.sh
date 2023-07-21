#!/bin/bash

cd "$(dirname "$0")" || exit
cd antlr4 || exit
# antlr4 -no-listener -visitor -package MxAntlr -o ../src/MxParser MxLexer.g4 MxParser.g4 -Dlanguage=Java
antlr4 -no-listener -visitor -package MxAntlr -o ../gen MxLexer.g4 MxParser.g4 -Dlanguage=Cpp
cd ..
