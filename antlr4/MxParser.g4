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

suite: block | stmt; // block first to avoid suite->stmt->block
block: '{' stmt* '}';

exprStmt: (exprList? ';');

stmt:
	exprStmt
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
	'for' '(' (exprStmt | defineVariableStmt)? condition = expression? ';' step = expression? ')'
		suite;

flowStmt: ('continue' | 'break' | ('return' expression?)) ';';

defineVariableStmt:
	typename variableAssign (',' variableAssign)* ';';

variableAssign: Identifier ('=' expression)?;

exprList: expression (',' expression)*;

expression:
	'(' expression ')'													# WrapExpr
	| expression '[' expression ']'										# ArrayAccess
	| expression '(' exprList? ')'										# FuncCall
	| expression '.' Identifier											# MemberAccess
	| expression op = ('++' | '--')										# RightSingleExpr
	| <assoc = right> op = ('++' | '--') expression						# LeftSingleExpr
	| <assoc = right> op = ('+' | '-' | '!' | '~') expression			# LeftSingleExpr
	| 'new' typename ('(' ')')?											# NewExpr
	| lhs = expression op = ('*' | '/' | '%') rhs = expression			# BinaryExpr
	| lhs = expression op = ('+' | '-') rhs = expression				# BinaryExpr
	| lhs = expression op = ('<<' | '>>') rhs = expression				# BinaryExpr
	| lhs = expression op = ('<' | '<=' | '>' | '>=') rhs = expression	# BinaryExpr
	| lhs = expression op = ('==' | '!=') rhs = expression				# BinaryExpr
	| lhs = expression op = '&' rhs = expression						# BinaryExpr
	| lhs = expression op = '^' rhs = expression						# BinaryExpr
	| lhs = expression op = '|' rhs = expression						# BinaryExpr
	| lhs = expression op = '&&' rhs = expression						# BinaryExpr
	| lhs = expression op = '||' rhs = expression						# BinaryExpr
	| <assoc = right> expression '?' expression ':' expression			# TernaryExpr
	| <assoc = right> expression '=' expression							# AssignExpr
	| literalExpr														# LiterExpr
	| (Identifier | BuiltinId)											# AtomExpr;

typename: (BasicType | Identifier) ('[' good += expression ']')* (
		'[' ']'
	)* ('[' bad += expression ']')*;

literalExpr: Number | String | Null | True | False;

