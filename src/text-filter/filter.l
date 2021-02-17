%{
#include <stdlib.h>
void yyerror(char *);
#include "filter.tab.h"
%}

%%
not {
        return NOT;
}

"&" {
        printf("LEX: & parsed\n");
        return AND;
}

"|" {
        printf("LEX: | parsed\n");
        return OR;
}

[a-z] {
        yylval.iValue = *yytext - 'a';
        return VARIABLE;
}

"logvar" {
        printf("LEX: logvar parsed\n");
        return LOGVAR;
}
[0-1]+ {
//        yylval.iValue = atoi(yytext);
//        return BOOLEAN;
}


[a-z ]+ {
        yylval.sValue = strdup(yytext);
        printf("LEX: string parsed: %s\n", yylval.sValue);
        return STRING;
}

[()=\n] { return *yytext; }
[ \t] ;
, { return *yytext; }
. yyerror("invalid character");
%%
int yywrap(void) {
        return 1;
}
