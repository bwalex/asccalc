%option noyywrap nodefault yylineno
%{
# include <stdio.h>
# include <gmp.h>
# include <mpfr.h>
# include "optype.h"
# include "num.h"
# include "var.h"
# include "ast.h"
# include "calc.h"
# include "calc.tab.h"
%}

/* float exponent */
EXP	([Ee][-+]?[0-9]+)
SUFFIX	([afpnumkMGTPE])

%%

"**" |
"^^"    { return POW; }

 /* single character ops */
"+" |
"-" |
"*" |
"%" |
"/" |
"=" |
"," |
"~" |
";" |
"!" |
"(" |
")"     { return yytext[0]; }

"or" |
"OR" |
"|"     { return OR; }

"and" |
"AND" |
"&"     { return AND; }

"xor" |
"XOR" |
"^"     { return XOR; }

">>"    { return SHR; }
"<<"    { return SHL; }

 /* Commands */
^"mode "[bdhox]$ { mode_switch(yytext[5]); /* XXX */ printf("mode switch to %c\n", yytext[5]); }
^"m "[bdhox]$    { mode_switch(yytext[2]); /* XXX */ printf("mode switch to %c\n", yytext[2]); }
^"ls"$           { varlist(); }
^"lsfn"$         { funlist(); }
^"quit"$         { exit(0);   }
^"help"$         { help();    }

 /* names; symtype returns either FUNC or VAR (if in doubt, VAR) */
[a-zA-Z_][a-zA-Z0-9_]*  { yylval.s = strdup(yytext); return NAME; }

 /* Hex and binary numbers */
0[xX][0-9a-fA-F]+ { yylval.a = ast_newnum(NUM_INT, yytext); return NUM; }
0[bB][01]+ { yylval.a = ast_newnum(NUM_INT, yytext); return NUM; }

 /* octal numbers */
0[0-7]+ { yylval.a = ast_newnum(NUM_INT, yytext); return NUM; }

 /* decimal numbers */
(0d)?[0-9]+"."[0-9]*{EXP}?{SUFFIX}? |
(0d)?"."?[0-9]+{EXP}?{SUFFIX}?  { yylval.a = ast_newnum(NUM_FP, yytext); return NUM; }

"//".*  
[ \t]   /* ignore white space */ 
\\\n    printf("... "); /* ignore line continuation */
"\n"    { return EOL; }

.       { printf("Mystery character: %c\n", yytext[0]); return yytext[0]; }
%%
