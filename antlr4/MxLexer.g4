lexer grammar MxLexer;

channels {
	WS,
	COMMENT
}

Comment: (BlockComment | LineComment) -> channel(COMMENT);

BlockComment: '/*' .*? '*/';
LineComment: '//' .*? NewLine;

String: '"' ('\\"' | .)*? '"';
NewLine: ('\r' | '\n' | '\u2028' | '\u2029') -> channel(WS);
Whitespace: (' ' | '\t' | '\u000B' | '\u000C' | '\u00A0') -> channel(WS);

// keywords: Basic Types
BasicType: 'int' | 'bool' | 'string' | 'void';

// keywords: Flow Control
Return: 'return';
Continue: 'continue';
Break: 'break';
ElseIf: 'else' Whitespace 'if';
If: 'if';
Else: 'else';
While: 'while';
For: 'for';

// keywords: Constexpr Value
True: 'true';
False: 'false';
Null: 'null';

// keywords: New
New: 'new';
Class: 'class';

// 1 operators
Increase: '++';
Decrease: '--';
Not: '!';
BitInv: '~';

// 1 or 2 operators
Add: '+';
Minus: '-';

// 2 operators
Multi: '*';
Div: '/';
Mod: '%';
Dot: '.';
BitAnd: '&';
BitOr: '|';
BitXor: '^';
And: '&&';
Or: '||';
Equal: '==';
NotEq: '!=';
Less: '<';
Greater: '>';
LessEq: '<=';
GreaterEq: '>=';
Assign: '=';
ShiftLeft: '<<';
ShiftRight: '>>';

// 3 operators
Ques: '?';
Colon: ':';

WrapLeft: '(';
WrapRight: ')';
BracketLeft: '[';
BracketRight: ']';
BraceLeft: '{';
BraceRight: '}';
Comma: ',';
Semicolon: ';';

Number: Digit+;
fragment Digit: [0-9];

Identifier: [A-Za-z_][A-Za-z_0-9]*;

