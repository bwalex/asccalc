%{
#include <math.h>
#include <ctype.h>
#include <stdio.h>

#include <gmp.h>
#include <mpfr.h>

#include "optype.h"
#include "num.h"
#include "var.h"
#include "ast.h"
#include "calc.h"
%}

%union {
  ast_t a;
  char *s;
  explist_t el;
  cmptype_t ct;
}

%token <a> NUM
%token <s> NAME

%token EOL

%token IF THEN ELSE WHILE DO FUNCTION ENDFUNCTION

%nonassoc <ct> CMP

%right '='
%left '-' '+' OR XOR
%left '*' '/' '%' AND 
%right SHR SHL
%nonassoc UMINUS UNEG
%right POW
%nonassoc '!'


%type <a> exp stmt
%type <el> explist

%start clist

%%


stmt:     exp
;

clist: /* nothing */
        | clist stmt EOL { go($2); }
        | error EOL  { yyerrok;  }/* on error, skip until end of line */
;


exp: exp CMP exp          { $$ = ast_newcmp($2, $1, $3); }
   | exp '+' exp          { $$ = ast_new(OP_ADD, $1,$3); }
   | exp '-' exp          { $$ = ast_new(OP_SUB, $1,$3); }
   | exp '*' exp          { $$ = ast_new(OP_MUL, $1,$3); }
   | exp '/' exp          { $$ = ast_new(OP_DIV, $1,$3); }
   | exp '%' exp          { $$ = ast_new(OP_MOD, $1,$3); }
   | exp POW exp          { $$ = ast_new(OP_POW, $1,$3); }
   | exp AND exp          { $$ = ast_new(OP_AND, $1,$3); }
   | exp OR  exp          { $$ = ast_new(OP_OR,  $1,$3); }
   | exp XOR exp          { $$ = ast_new(OP_XOR, $1,$3); }
   | exp SHR exp          { $$ = ast_new(OP_SHR, $1,$3); }
   | exp SHL exp          { $$ = ast_new(OP_SHL, $1,$3); }
   | '(' exp ')'          { $$ = $2; }
   | '-' exp %prec UMINUS { $$ = ast_new(OP_UMINUS, $2, NULL); }
   | '~' exp %prec UNEG   { $$ = ast_new(OP_UINV,   $2, NULL); }
   | exp '!'              { $$ = ast_new(OP_FAC,    $1, NULL); }
   | NUM                  { $$ = $1; }
   | NAME '(' explist ')' { $$ = ast_newcall($1, $3); }
   | NAME                 { $$ = ast_newref($1); }
   | NAME '=' exp         { $$ = ast_newassign($1, $3); }
;


explist: exp        { $$ = ast_newexplist($1, NULL); } /* last element */
 | exp ',' explist  { $$ = ast_newexplist($1, $3);   }
;


 //namelist: name        { $$ = newnamelist($1, NULL); } /* last element */
 // | name ',' namelist  { $$ = newnamelist($1, $3);   }
 //;

%%

