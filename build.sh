#!/bin/bash
cd antlr4
antlr4 -no-listener -visitor -package MxParser -o ../src/MxParser MxLexer.g4 MxParser.g4 
