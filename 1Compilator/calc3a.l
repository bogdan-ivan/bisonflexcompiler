%{
#include <stdlib.h>

#include <stdio.h>
#include "keyword.h"
void yyerror(char *);

keyword_t result;
int yylineno;
%}

%%
"//"[^\n]*\n     { ++yylineno; return COMMENT; }
(0|[1-9][0-9]*) {
                yylval.iValue = atoi(yytext);
                return INTEGER;
            }

[-()<>=+*/;{}\%] {
                return *yytext;
             }
">="            return GE;
"<="            return LE;
"=="            return EQ;
"!="            return NE;
[a-zA-Z][_a-zA-Z0-9]* {
                        result = find_keyword_or(yytext, IDENTIFIER);
                        if (IDENTIFIER == result) {
                          yylval.name = _strdup(yytext);
                        }
                        return result;
                      }

[ \t]+        ;       /* ignore whitespace */
\n            { ++yylineno; }
.               yyerror("Unknown character");
%%
int yywrap(void) {
    return 1;
}
