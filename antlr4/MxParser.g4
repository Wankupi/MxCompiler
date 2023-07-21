parser grammar MxParser;

options {
	tokenVocab = MxLexer;
}

file: (defineFunction | defineClass | defineVariableStmt)* EOF;

defineClass:
	Class Identifier '{' (
		defineVariableStmt
		| defineFunction
		| constructFunction
	)* '}' ';';

defineFunction:
	typename Identifier '(' functionParameterList? ')' block;

constructFunction:
	Identifier '(' functionParameterList? ')' block;

defineVariable: typename Identifier;

functionParameterList: defineVariable (',' defineVariable)*;

suite: stmt | block;
block: '{' stmt* '}';

simpleStmt: (exprList? ';');

stmt:
	simpleStmt
	| ifStmt
	| whileStmt
	| forStmt
	| flowStmt
	| defineVariableStmt
	| block;

ifStmt:
	'if' '(' expression ')' suite (
		ElseIf '(' expression ')' suite
	)* ('else' suite)?;

whileStmt: 'while' '(' expression ')' suite;

forStmt:
	'for' '(' start = expression? ';' condition = expression? ';' step = expression? ')' suite;

flowStmt: ('continue' | 'break' | ('return' expression?)) ';';

defineVariableStmt:
	typename variableAssign (',' variableAssign)* ';';

variableAssign: Identifier ('=' expression)?;

exprList: expression (',' expression)*;

expression:
	'(' exprList ')'											# WrapExpr
	| expression '[' exprList ']'								# ArrayAccess
	| expression '(' exprList? ')'								# FuncCall
	| expression '.' Identifier									# MemberAccess
	| expression op = ('++' | '--')								# SelfUpdate
	| <assoc = right> op = ('++' | '--') expression				# SelfUpdate
	| <assoc = right> op = ('+' | '-' | '!' | '~') expression	# SingleExpr
	| 'new' typename											# NewExpr
	| expression op = ('*' | '/' | '%') expression				# BinaryExpr
	| expression op = ('+' | '-') expression					# BinaryExpr
	| expression op = ('<<' | '>>') expression					# BinaryExpr
	| expression op = ('<' | '<=' | '>' | '>=') expression		# CmpExpr
	| expression op = ('==' | '!=') expression					# CmpExpr
	| expression op = '&' expression							# BinaryExpr
	| expression op = '^' expression							# BinaryExpr
	| expression op = '|' expression							# BinaryExpr
	| expression op = '&&' expression							# BinaryExpr
	| expression op = '||' expression							# BinaryExpr
	| <assoc = right> expression '?' expression ':' expression	# CondExpr
	| <assoc = right> expression '=' expression					# AssignExpr
	| literalExpr												# LiterExpr
	| Identifier												# AtomExpr;

typename: (BasicType | Identifier) ('[' Number ']')* ('[' ']')*;

literalExpr: Number | String | Null | True | False;

