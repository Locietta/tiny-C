parser grammar CParser;

options {
	tokenVocab = CLexer;
}

// These are all supported parser sections:

// Parser file header. Appears at the top in all parser related files. Use e.g. for copyrights.
@parser::header {/* parser/listener/visitor header section */}

// Appears before any #include in h + cpp files.
@parser::preinclude {/* parser precinclude section */}

// Follows directly after the standard #includes in h + cpp files.
@parser::postinclude {
/* parser postinclude section */
#ifndef _WIN32
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
}

// Directly preceeds the parser class declaration in the h file (e.g. for additional types etc.).
@parser::context {/* parser context section */}

// Appears in the private part of the parser in the h file. The function bodies could also appear in
// the definitions section, but I want to maximize Java compatibility, so we can also create a Java
// parser from this grammar. Still, some tweaking is necessary after the Java file generation (e.g.
// bool -> boolean).
@parser::members {
/* public parser declarations/members section */
bool myAction() { return true; }
bool doesItBlend() { return true; }
void cleanUp() {}
void doInit() {}
void doAfter() {}
}

// Appears in the public part of the parser in the h file.
@parser::declarations {/* private parser declarations section */}

// Appears in line with the other class member definitions in the cpp file.
@parser::definitions {/* parser definitions section */}

// Additionally there are similar sections for (base)listener and (base)visitor files.
@parser::listenerpreinclude {/* listener preinclude section */}
@parser::listenerpostinclude {/* listener postinclude section */}
@parser::listenerdeclarations {/* listener public declarations/members section */}
@parser::listenermembers {/* listener private declarations/members section */}
@parser::listenerdefinitions {/* listener definitions section */}

@parser::baselistenerpreinclude {/* base listener preinclude section */}
@parser::baselistenerpostinclude {/* base listener postinclude section */}
@parser::baselistenerdeclarations {/* base listener public declarations/members section */}
@parser::baselistenermembers {/* base listener private declarations/members section */}
@parser::baselistenerdefinitions {/* base listener definitions section */}

@parser::visitorpreinclude {/* visitor preinclude section */}
@parser::visitorpostinclude {/* visitor postinclude section */}
@parser::visitordeclarations {/* visitor public declarations/members section */}
@parser::visitormembers {/* visitor private declarations/members section */}
@parser::visitordefinitions {/* visitor definitions section */}

@parser::basevisitorpreinclude {/* base visitor preinclude section */}
@parser::basevisitorpostinclude {/* base visitor postinclude section */}
@parser::basevisitordeclarations {/* base visitor public declarations/members section */}
@parser::basevisitormembers {/* base visitor private declarations/members section */}
@parser::basevisitordefinitions {/* base visitor definitions section */}

// Actual grammar start. prog: decl_list EOF ;

prog: decl_list EOF;

decl_list: decl*;

decl: var_decl | func_decl;

simple_var_decl:
	Identifier (Assign Constant)?	# no_array_decl
//	| Identifier LeftBracket Constant RightBracket (Assign init_list)? # array_decl
	; 

init_list: LeftBrace (Constant (Comma Constant)*)? RightBrace;

var_decl:
	type_spec simple_var_decl (Comma simple_var_decl)* Semi;

type_spec:
	Char
	| Int
	| Long
	| Float
	| Short
	| Void
	| Struct Identifier; // struct type

func_decl:
	type_spec Identifier LeftParen params RightParen comp_stmt;

params: param_list | (Void)?;

param_list: (param Comma)* param;

param:
	type_spec Identifier
	| type_spec Identifier LeftBracket RightBracket;

comp_stmt: LeftBrace (stmt)* RightBrace;

stmt:				// no need to override
	expr_stmt		
	| comp_stmt		
	| selec_stmt	
	| iter_stmt		
	| return_stmt	
	| var_decl		
	| break_stmt
	| continue_stmt
	;

break_stmt: Break Semi;

continue_stmt: Continue Semi;

expr_stmt: expr Semi | Semi;	

selec_stmt: If LeftParen expr RightParen stmt  (Else stmt )?;

iter_stmt: While LeftParen expr RightParen stmt;

return_stmt: Return (expr)? Semi;

expr: var assign expr 	# assign_expr
	| unary_expr	# not_assign_expr // no need to override
	;

unary_expr: unary_operator unary_expr | oror_expr;

unary_operator: Not | Plus | Minus;

var: Identifier;

assign: Assign | PlusAssign | MinusAssign | MulAssign | DivAssign | ModAssign;

oror_expr: oror_expr OrOr andand_expr | andand_expr;

andand_expr: andand_expr AndAnd equal_expr | equal_expr;

equal_expr: equal_expr (Equal | NotEqual) compare_expr | compare_expr;

compare_expr: compare_expr relop add_expr | add_expr;

relop:
	Less
	| LessEqual
	| Greater
	| GreaterEqual
	;

add_expr: add_expr (Plus | Minus) term | term;

term: term (Mul | Div | Mod) factor | factor;

factor: 
	LeftParen expr RightParen 	# paren_factor
	| var 						# var_factor
	| call 						# call_factor	// no need to override
	| Constant					# const_factor
	;

call: Identifier LeftParen args RightParen;

args: ((expr Comma)* expr)?;	// no need to override

//expr: expr (Plus | Minus) INT # AddSub | INT # Num ;