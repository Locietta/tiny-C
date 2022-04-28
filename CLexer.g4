
lexer grammar CLexer;

options {
	language = Cpp;
}

// These are all supported lexer sections:

// Lexer file header. Appears at the top of h + cpp files. Use e.g. for copyrights.
@lexer::header {/* lexer header section */}

// Appears before any #include in h + cpp files.
@lexer::preinclude {/* lexer precinclude section */}

// Follows directly after the standard #includes in h + cpp files.
@lexer::postinclude {
/* lexer postinclude section */
#ifndef _WIN32
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
}

// Directly preceds the lexer class declaration in the h file (e.g. for additional types etc.).
@lexer::context {/* lexer context section */}

// Appears in the public part of the lexer in the h file.
@lexer::members {/* public lexer declarations section */
bool canTestFoo() { return true; }
bool isItFoo() { return true; }
bool isItBar() { return true; }

void myFooLexerAction() { /* do something*/ };
void myBarLexerAction() { /* do something*/ };
}

// Appears in the private part of the lexer in the h file.
@lexer::declarations {/* private lexer declarations/members section */}

// Appears in line with the other class member definitions in the cpp file.
@lexer::definitions {/* lexer definitions section */}

// ============================ Actual Lexer ===================================
// --------------------------- Data Type -------------------------
Char: 'char';
Double: 'double';
Int: 'int';
Long: 'long';
Float: 'float';
Short: 'short';
Void: 'void';
Struct: 'struct';

// ---------------------------- Control Flow ----------------------------
Break: 'break';
Case: 'case';
Continue: 'continue';
Default: 'default';
Do: 'do';
Else: 'else';
For: 'for';
If: 'if';
Return: 'return';
Switch: 'switch';
While: 'while';

// ------------------------------ operator ----------------------------
Sizeof: 'sizeof';

LeftParen: '(';
RightParen: ')';
LeftBracket: '[';
RightBracket: ']';
LeftBrace: '{';
RightBrace: '}';

Less: '<';
LessEqual: '<=';
Greater: '>';
GreaterEqual: '>=';

Plus: '+';
PlusPlus: '++';
Minus: '-';
MinusMinus: '--';
Mul: '*';
Div: '/';
Mod: '%';

AndAnd: '&&';
OrOr: '||';
Not: '!';

Question: '?';
Colon: ':';
Semi: ';';
Comma: ',';

Assign: '=';
// '*=' | '/=' | '%=' | '+=' | '-=' | '<<=' | '>>=' | '&=' | '^=' | '|='
MulAssign: '*=';
DivAssign: '/=';
ModAssign: '%=';
PlusAssign: '+=';
MinusAssign: '-=';

Equal: '==';
NotEqual: '!=';

Dot: '.';

// ------------------------ ID: name of variables ------------------------
Identifier: IdentifierNondigit ( IdentifierNondigit | Digit)*;

fragment IdentifierNondigit: Nondigit;

fragment Nondigit: [a-zA-Z_];

fragment Digit: [0-9];

Constant:
	IntegerConstant
	| FloatingConstant
	| CharacterConstant;

IntegerConstant: DecimalConstant;

fragment DecimalConstant: NonzeroDigit Digit*;

fragment NonzeroDigit: [1-9];

fragment FloatingConstant: DecimalFloatingConstant;

fragment DecimalFloatingConstant: FractionalConstant;

fragment FractionalConstant:
	DigitSequence? '.' DigitSequence
	| DigitSequence '.';

fragment Sign: [+-];

DigitSequence: Digit+;

fragment CharacterConstant: '\'' CCharSequence '\'';

fragment CCharSequence: CChar+;

fragment CChar: ~['\\\r\n] | SimpleEscapeSequence;

fragment SimpleEscapeSequence: '\\' ['"?abfnrtv\\];

StringLiteral: '"' SCharSequence? '"';

fragment SCharSequence: SChar+;

fragment SChar:
	~["\\\r\n]
	| SimpleEscapeSequence
	| '\\\n' // Added line
	| '\\\r\n' ; // Added line

// ComplexDefine : '#' Whitespace? 'define' ~[#\r\n]* -> skip ;

IncludeDirective:
	'#' Whitespace? 'include' Whitespace? (
		('"' ~[\r\n]* '"')
		| ('<' ~[\r\n]* '>')
	) Whitespace? Newline -> skip;

Whitespace: [ \t]+ -> skip;

Newline: ( '\r' '\n'? | '\n') -> skip;

BlockComment: '/*' .*? '*/' -> skip;

LineComment: '//' ~[\r\n]* -> skip;

