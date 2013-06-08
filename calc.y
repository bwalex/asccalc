%{
#include <math.h>
#include <ctype.h>
#include <stdio.h>

#include <gmp.h>
#include <mpfr.h>

#include "hashtable.h"

#include "optype.h"
#include "num.h"
#include "var.h"
#include "ast.h"
#include "func.h"
#include "calc.h"
#include "lex.yy.h"
%}

%union {
  ast_t a;
  char *s;
  explist_t el;
  namelist_t nl;
  cmptype_t ct;
}

%token <a> NUM
%token <s> NAME

%token EOL

%token IF THEN ELSE FI WHILE DO DONE FUNCTION ENDFUNCTION

%nonassoc <ct> CMP

%right '='
%left '-' '+' OR XOR
%left '*' '/' '%' AND 
%right SHR SHL
%nonassoc UMINUS UNEG
%right POW
%nonassoc '!'
%nonassoc <s> PSEL
%nonassoc <s> PSELS


%type <a> exp stmt list
%type <el> explist
%type <nl> namelist

%start clist

%%


stmt:     IF exp THEN list FI             { $$ = ast_newflow(FLOW_IF, $2, $4, NULL); }
        | IF exp THEN list ELSE list FI   { $$ = ast_newflow(FLOW_IF, $2, $4, $6); }
        | WHILE exp DO list DONE          { $$ = ast_newflow(FLOW_WHILE, $2, $4, NULL); }
;


list:     /* nothing */   { $$ = NULL; }
        | stmt list       { if ($2 == NULL) $$ = $1; else $$ = ast_new(OP_LISTING, $1,$2); }
        | exp  ';' list   { if ($3 == NULL) $$ = $1; else $$ = ast_new(OP_LISTING, $1,$3); }
;


clist: /* nothing */
        | clist EOL
        | clist stmt EOL { go($2); }
        | clist exp EOL { go($2); }
        | clist exp ';' EOL { go($2); }
        | clist FUNCTION NAME '(' namelist ')' '=' list ENDFUNCTION EOL { user_newfun($3, $5, $8); }
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
   | exp PSEL             { $$ = ast_newpsel(       $1, $2  ); }
   | exp PSELS            { $$ = ast_newpsel(       $1, $2  ); }
   | NUM                  { $$ = $1; }
   | NAME '(' explist ')' { $$ = ast_newcall($1, $3); }
   | NAME                 { $$ = ast_newref($1); }
   | NAME '=' exp         { $$ = ast_newassign($1, $3); }
;


explist: exp        { $$ = ast_newexplist($1, NULL); } /* last element */
 | exp ',' explist  { $$ = ast_newexplist($1, $3);   }
;


namelist: NAME        { $$ = ast_newnamelist($1, NULL); } /* last element */
 | NAME ',' namelist  { $$ = ast_newnamelist($1, $3);   }
;

%%

