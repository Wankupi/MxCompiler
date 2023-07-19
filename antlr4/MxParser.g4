parser grammar MxParser;

options {
	tokenVocab = MxLexer;
}

test: expression;

file: (defineFunction | defineClass | defineVariableStmt)* EOF;

defineClass: Class Identifier '{' '}';

defineFunction:
	typename Identifier '(' functionParameterList? ')' '{' stmt* '}';

defineVariable: typename Identifier;

functionParameterList: defineVariable (',' defineVariable)*;

suite: stmt | ('{' stmt* '}');

simpleStmt: (exprList? ';');

stmt: simpleStmt | ifStmt | whileStmt | forStmt | flowStmt | defineVariableStmt;

ifStmt:
	'if' '(' expression ')' suite (
		ElseIf '(' expression ')' suite
	)* ('else' suite)?;

whileStmt: 'while' '(' expression ')' suite;

forStmt:
	'for' '(' expression ';' expression ';' expression ')' suite;

flowStmt: ('continue' | 'break' | ('return' expression)) ';';

defineVariableStmt:
	typename variableAssign (',' variableAssign)* ';';

variableAssign: Identifier ('=' expression)?;

exprList: expression (',' expression)*;

expression:
	'(' exprList ')'
	| expression '[' exprList ']'
	| expression '(' exprList? ')'
	| expression '.' Identifier
	| expression '++'
	| expression '--'
	| <assoc = right> '++' expression
	| <assoc = right> '--' expression
	| <assoc = right> '~' expression
	| <assoc = right> '!' expression
	| <assoc = right> '+' expression
	| <assoc = right> '-' expression
	| expression '*' expression
	| expression '/' expression
	| expression '%' expression
	| expression '+' expression
	| expression '-' expression
	| expression '<<' expression
	| expression '>>' expression
	| expression '<' expression
	| expression '>' expression
	| expression '<=' expression
	| expression '>=' expression
	| expression '==' expression
	| expression '!=' expression
	| expression '&' expression
	| expression '^' expression
	| expression '|' expression
	| expression '&&' expression
	| expression '||' expression
	| <assoc = right> expression '?' expression ':' expression
	| <assoc = right> expression '=' expression
	| literalExpr
	| Identifier;

typename: BasicType;

literalExpr: Number | String | Null | True | False;

