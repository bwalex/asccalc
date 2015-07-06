%option reentrant
 /* %option prefix="calc" */
%option nodefault yylineno

%option bison-bridge
%option bison-locations

%{
# include <stdio.h>
# include <assert.h>
# include <gmp.h>
# include <mpfr.h>
# include "hashtable.h"
# include "optype.h"
# include "num.h"
# include "var.h"
# include "ast.h"
# include "func.h"
# include "calc.h"
# include "parse_ctx.h"
# include "calc.tab.h"

#define YY_EXTRA_TYPE struct parse_ctx *
#define YY_USER_ACTION yylloc->first_line = yylineno;

int linecont = 0;
int nesting = 0;

char *incpath = NULL;
%}

/* float exponent */
EXP	([Ee][-+]?[0-9]+)
SUFFIX	([afpnumkMGTPE])

%x reqb
%x reqstr

%%

"**" |
"^^"    { return POW; }

">="    { yylval->ct = CMP_GE; return CMP; }
"<="    { yylval->ct = CMP_LE; return CMP; }
"!="    { yylval->ct = CMP_NE; return CMP; }
"=="    { yylval->ct = CMP_EQ; return CMP; }
">"     { yylval->ct = CMP_GT; return CMP; }
"<"     { yylval->ct = CMP_LT; return CMP; }

"-:"    { return DPSEL; }

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
")" |
"[" |
"]" |
":"     { return yytext[0]; }

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

 /* keywords */
"if"          { ++nesting; return IF; }
"then"        {            return THEN; }
"else"        {            return ELSE; }
"elsif"       {            return ELSIF; }
"fi"          { --nesting; return FI; }
"while"       { ++nesting; return WHILE; }
"do"          {            return DO; }
"done"        { --nesting; return DONE; }
"function"    { ++nesting; return FUNCTION; }
"endfunction" { --nesting; return ENDFUNCTION; }

"require"       BEGIN reqb;
<reqb>[ \t]     /* ignore white space */
<reqb>\"        BEGIN reqstr;
<reqb>.         { printf("Mystery character: %c\n", yytext[0]); BEGIN 0; return yytext[0]; }
<reqstr>[^\\"]* { incpath = strdup(yytext); }
<reqstr>\"      {
                  assert(incpath != NULL);
		  require_file(incpath);
                  free(incpath);
                  BEGIN 0;
                }
<reqstr>.       { printf("Mystery character: %c\n", yytext[0]); BEGIN 0; return yytext[0]; }

 /* Commands */
^"mode "[bdhoxs]"\n" { mode_switch(yytext[5]); /* XXX */ printf("mode switch to %c\n", yytext[5]); }
^"m "[bdhoxs]"\n"    { mode_switch(yytext[2]); /* XXX */ printf("mode switch to %c\n", yytext[2]); }
^"ls\n"           { varlist(); }
^"lsfn\n"         { funlist(); }
^"quit\n"         { graceful_exit();   }
^"exit\n"         { graceful_exit();   }
^"help\n"         { help();    }

 /* names; symtype returns either FUNC or VAR (if in doubt, VAR) */
[a-zA-Z_][a-zA-Z0-9_]*  { yylval->s = strdup(yytext); return NAME; }

 /* Hex and binary numbers */
0[xX][0-9a-fA-F]+ { yylval->a = ast_newnum(NUM_INT, yytext); return NUM; }
0[bB][01]+ { yylval->a = ast_newnum(NUM_INT, yytext); return NUM; }

 /* octal numbers */
0[0-7]+ { yylval->a = ast_newnum(NUM_INT, yytext); return NUM; }

 /* decimal numbers */
(0d)?[0-9]+"."[0-9]*{EXP}?{SUFFIX}? |
(0d)?"."?[0-9]+{EXP}?{SUFFIX}?  { yylval->a = ast_newnum(NUM_FP, yytext); return NUM; }

"#".*   /* ignore comment */
"//".*  /* ignore comment */
[ \t]   /* ignore white space */
\\\n    { linecont = 1; } /* ignore line continuation */
"\n"    { linecont = 0; if (nesting == 0) return EOL; }


<<EOF>> {
          printf("LEX: EOF\n");
          yyterminate();
        }

.       { printf("Mystery character: %c\n", yytext[0]); return yytext[0]; }
%%
