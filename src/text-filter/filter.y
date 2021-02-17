%{
#include <string.h>
#include <stdio.h>
void yyerror(char *);
int yylex(void);
int sym[26];
char* log_line;
%}
%union {
    char *sValue;
    int iValue;
}

%token <iValue> BOOLEAN
%token <sValue> STRING
%token VARIABLE LOGVAR
%token AND OR NOT
%left AND OR NOT

%type<iValue> expr 
%type<iValue> VARIABLE
%%

prog :
prog logassigned '\n' statement '\n'
|
;
statement :
expr { printf("%d\n", $1); }
| VARIABLE '=' expr { sym[$1] = $3; }
| statement ',' statement
;

logassigned :
LOGVAR '=' STRING { log_line = $3; /*printf("KEK %s\n", log_line);*/ }
;


expr:
//BOOLEAN
STRING { if (strstr(log_line, $1) != NULL) {
                printf("%s is substring of %s\n", $1, log_line);
                $$ = 1;
         } else {
                 printf("%s doesn't substring of %s\n", $1, log_line);
                 $$ = 0;
         }
       }
       
| expr OR expr { if ($1 == 1 || $3 == 1) { $$ = 1; } else { $$ = 0; } }
| expr AND expr { $$ = $1 * $3; }
| NOT expr { if ($2 == 1) { $$ = 0; } else { $$ = 1; } }
| '(' expr ')' { $$ = $2; }
;
%%
#include "lex.yy.c"
void yyerror(char *s) {
        fprintf(stderr, "%s\n", s);
}

int main(void) {
//        yy_scan_string("((1 or 0) and 0)\n");
        yyparse();
//        yylex_destroy();

return 0;
}
