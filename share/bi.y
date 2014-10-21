%{
#include <cstdio>
#include <iostream>

#include "bi.tab.h"

extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

extern int colno;
extern int linno;

void yyerror(const char *s);
%}

%union {
    bool valBool;
    int valInt;
    real valReal;
    char* valString;
    
    bi::OperatorReference* valOperatorReference;
    bi::Statement* valStatement;
    bi::Model* valModel;
    bi::Method* valMethod;
    bi::Function* valFunction;
}

%token COMMENT_EOL COMMENT_START COMMENT_END
%token MODEL FUNCTION METHOD BUILTIN CONST DIM HYPER PARAM INPUT STATE OBS IF WHILE
%token <valString> IDENTIFIER
%token <valBool> BOOLEAN_LITERAL
%token <valInt> INTEGER_LITERAL
%token <valReal> REAL_LITERAL
%token <valString> STRING_LITERAL
%token <valString> RIGHT_ARROW LEFT_ARROW RIGHT_DOUBLE_ARROW DOUBLE_DOT
%token <valString> RIGHT_OP LEFT_OP AND_OP OR_OP LE_OP GE_OP EQ_OP NE_OP
%token <valString> POW_OP ELEM_MUL_OP ELEM_DIV_OP ELEM_POW_OP
%token ENDL
%token OTHER

%type <valOperatorReference> traversal_operator type_operator unary_operator pow_operator multiplicative_operator additive_operator shift_operator relational_operator equality_operator and_operator exclusive_or_operator inclusive_or_operator logical_and_operator logical_or_operator assignment_operator tuple_operator statement_operator
%type <valStatement> type boolean_literal integer_literal real_literal string_literal symbol reference traversal_expression type_expression postfix_expression defaulted_expression unary_expression pow_expression multiplicative_expression additive_expression shift_expression relational_expression equality_expression and_expression exclusive_or_expression inclusive_or_expression logical_and_expression logical_or_expression assignment_expression tuple_expression expression const_declaration dim_declaration var_declaration declaration if_statement while_statement statement
%type <valModel> model
%type <valMethod> method
%type <valFunction> function

%start file
%%

/***************************************************************************
 * Expressions                                                             *
 ***************************************************************************/

type
    : IDENTIFIER  { $$ = new bi::Type($1); }
    ;

boolean_literal
    : BOOLEAN_LITERAL  { $$ = new bi::BooleanLiteral($1); }
    ;

integer_literal
    : INTEGER_LITERAL  { $$ = new bi::IntegerLiteral($1); }
    ;

real_literal
    : REAL_LITERAL  { $$ = new bi::RealLiteral($1); }
    ;

string_literal
    : STRING_LITERAL  { $$ = new bi::StringLiteral($1); }
    ;

symbol
    : IDENTIFIER  { $$ = new bi::Symbol($1); }
    ;
    
reference
    : symbol '[' statement ']'                     { $$ = new bi::Reference($1, $3); }
    | symbol '(' statement ')'                     { $$ = new bi::Reference($1, NULL, $3); }
    | symbol '(' statement ')' '{' statement '}'   { $$ = new bi::Reference($1, NULL, $3, $6); }
    | symbol '{' statement '}'                     { $$ = new bi::Reference($1, NULL, NULL, $3); }
    | symbol                                       { $$ = new bi::Reference($1); }
    ;

traversal_operator
    : '.'  { $$ = new bi::OperatorReference('.'); }
    ;
    
traversal_expression
    : reference
    | traversal_expression traversal_operator reference  { $$ = new BinaryOperator($1, $2, $3); }
    ;
    
type_operator
    : ':'  { $$ = new bi::OperatorReference(':'); }
    ;
    
type_expression
    : traversal_expression
    | traversal_expression type_operator type  { $$ = new BinaryOperator($1, $2, $3); }
    ;
    
postfix_expression
    : type_expression
    | boolean_literal
    | integer_literal
    | real_literal
    | string_literal
    ;

defaulted_expression
    : postfix_expression
    | postfix_expression RIGHT_DOUBLE_ARROW defaulted_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

unary_operator
    : '+'  { $$ = new bi::OperatorReference('+'); }
    | '-'  { $$ = new bi::OperatorReference('-'); }
    | '!'  { $$ = new bi::OperatorReference('!'); }
    ;
    
unary_expression
    : defaulted_expression
    | unary_operator unary_expression  { $$ = new bi::UnaryOperator($1, $2); }
    ;

pow_operator
    : POW_OP       { $$ = new bi::OperatorReference($1); }
    | ELEM_POW_OP  { $$ = new bi::OperatorReference($1); }
    ;

pow_expression
    : unary_expression
    | pow_expression pow_operator unary_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

multiplicative_operator
    : '*'          { $$ = new bi::OperatorReference('*'); }
    | ELEM_MUL_OP  { $$ = new bi::OperatorReference($1); }
    | '/'          { $$ = new bi::OperatorReference('/'); }
    | ELEM_DIV_OP  { $$ = new bi::OperatorReference($1); }
    | '%'          { $$ = new bi::OperatorReference('%'); }
    ;

multiplicative_expression
    : pow_expression
    | multiplicative_expression multiplicative_operator pow_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

additive_operator
    : '+'          { $$ = new bi::OperatorReference('+'); }
    | '-'          { $$ = new bi::OperatorReference('-'); }
    ;

additive_expression
    : multiplicative_expression
    | additive_expression additive_operator multiplicative_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

shift_operator
    : LEFT_OP   { $$ = new bi::OperatorReference($1); }
    | RIGHT_OP  { $$ = new bi::OperatorReference($1); }
    ;

shift_expression
    : additive_expression
    | shift_expression shift_operator additive_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

relational_operator
    : '<'    { $$ = new bi::OperatorReference('<'); }
    | '>'    { $$ = new bi::OperatorReference('>'); }
    | LE_OP  { $$ = new bi::OperatorReference($1); }
    | GE_OP  { $$ = new bi::OperatorReference($1); }
    ;
    
relational_expression
    : shift_expression
    | relational_expression relational_operator shift_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

equality_operator
    : EQ_OP  { $$ = new bi::OperatorReference($1); }
    | NE_OP  { $$ = new bi::OperatorReference($1); }
    ;

equality_expression
    : relational_expression
    | equality_expression equality_operator relational_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

and_operator
    : '&'  { $$ = new bi::OperatorReference('&'); }
    ;
    
and_expression
	: equality_expression
	| and_expression and_operator equality_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
	;

exclusive_or_operator
    : '^'  { $$ = new bi::OperatorReference('^'); }
    ;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression exclusive_or_operator and_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
	;

inclusive_or_operator
    : '|'  { $$ = new bi::OperatorReference('|'); }
    ;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression inclusive_or_operator exclusive_or_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }

logical_and_operator
    : AND_OP  { $$ = new bi::OperatorReference($1); }
    ;

logical_and_expression
    : inclusive_or_expression
    | logical_and_expression logical_and_operator equality_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

logical_or_operator
    : OR_OP  { $$ = new bi::OperatorReference($1); }
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression logical_or_operator logical_and_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

assignment_operator
    : LEFT_ARROW  { $$ = new bi::OperatorReference($1); }
    | '~'         { $$ = new bi::OperatorReference('~'); }
    ;

assignment_expression
    : logical_or_expression
    | logical_or_expression assignment_operator assignment_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

tuple_operator
    : ','  { $$ = new bi::OperatorReference(','); }
    ;
    
tuple_expression
    : assignment_expression
    | tuple_expression tuple_operator assignment_expression  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

expression
    : tuple_expression
    ;    


/***************************************************************************
 * Declarations                                                            *
 ***************************************************************************/
 
const_declaration
    : CONST reference  { $$ = new bi::Const($2); }
    ;

dim_declaration
    : DIM reference  { $$ = new bi::Dim($2); }
    ;

var_declaration
    : INPUT reference  { $$ = new bi::Input($2); }
    | HYPER reference  { $$ = new bi::Hyper($2); }
    | PARAM reference  { $$ = new bi::Param($2); }
    | STATE reference  { $$ = new bi::State($2); }
    | OBS reference  { $$ = new bi::Obs($2); }
    ;

declaration
    : const_declaration
    | dim_declaration
    | var_declaration
    ;


/***************************************************************************
 * Statements                                                              *
 ***************************************************************************/

if_statement
    : IF '(' statement ')' '{' statement '}'  { $$ = new bi::Conditional($3, $6); }
    ;

while_statement
    : WHILE '(' statement ')' '{' statement '}'  { $$ = new bi::Loop($3, $6); }
    ;

statement_operator
    : ';'  { $$ = new bi::OperatorReference(';'); }
    ;

statement
    : expression statement_operator
    | declaration statement_operator
    | if_statement statement_operator
    | while_statement statement_operator
    | expression statement_operator statement       { $$ = new bi::BinaryOperator($1, $2, $3); }
    | declaration statement_operator statement      { $$ = new bi::BinaryOperator($1, $2, $3); }
    | if_statement statement_operator statement     { $$ = new bi::BinaryOperator($1, $2, $3); }
    | while_statement statement_operator statement  { $$ = new bi::BinaryOperator($1, $2, $3); }
    ;

    
/***************************************************************************
 * Files                                                                   *
 ***************************************************************************/

model
    : MODEL reference  { $$ = new bi::Model($2); }
    ;
    
method
    : METHOD reference  { $$ = new bi::Method($2); }
    ;

function
    : FUNCTION symbol '(' statement ')' RIGHT_ARROW '(' statement ')' '{' statement '}'  { $$ = new bi::Function($2, $4, $8, $11); }
    | FUNCTION symbol '(' statement ')' RIGHT_ARROW '(' statement ')' '{' '}'            { $$ = new bi::Function($2, $4, $8); }
    ;

top
    : model
    | method
    | function
    ;

file
    : file top
    | top
    ;

%%

int main() {
  do {
    yyparse();
  } while (!feof(yyin));

  return 0;
}

void yyerror(const char *msg) {
  std::cerr << "Error (line " << linno << " col " << colno << "): " << msg << std::endl;
  exit(-1);
}
