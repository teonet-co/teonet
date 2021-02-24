%{

#include "grammar-filter.h"
void yyerror(char *);
%}

%option noinput nounput noyywrap

%%
"!" {
        return NOT;
}

"&" {
        /* printf("LEX: & parsed\n"); */
        return AND;
}

"|" {
        /* printf("LEX: | parsed\n"); */
        return OR;
}


"logvar" {
        /* printf("LEX: logvar parsed\n"); */
        return LOGVAR;
}
[a-z_]+ {
        yylval.sValue = strdup(yytext);
        /* printf("LEX: word parsed: .%s.\n", yylval.sValue); */
        return WORD;
}

\"([^\"]*)\" {
        yylval.sValue = malloc(yyleng - 1);
        memcpy(yylval.sValue, yytext + 1, yyleng - 2);
        yylval.sValue[yyleng - 2] = 0;
        /* printf("LEX: string parsed: .%s.\n", yylval.sValue); */
        return STRING;
}


[()=\n] { return *yytext; }
[ \t] { /* printf("LEX: SPACE parsed and skipped\n"); */ } ;
, { return *yytext; }
. yyerror("invalid character");
%%

