%{
#include <string.h>
#include <stdio.h>
void yyerror(char *s) {
        fprintf(stderr, "%s\n", s);
}
int yylex(void);
int yylex_destroy(void);
void *yy_scan_string(const char*);
int result;
char log_line[1024];
%}
%union {
    char *sValue;
    int iValue;
}

%token <sValue> STRING
%token <sValue> WORD 

%token LOGVAR
%token AND OR NOT
%left AND OR NOT

%type <iValue> expr 
%%

prog :
prog logassigned '\n' statement '\n'
|
;
statement :
expr { result = $1; /* printf("%d\n", $1); */ }
;

logassigned :
LOGVAR '=' STRING { /* printf("LOGVAR GRAM\n"); */ strcpy(log_line, $3); free($3); }
;


expr:
WORD { if (strstr(log_line, $1) != NULL) {
                /* printf("%s is substring of %s\n", $1, log_line); */
                $$ = 1;
         } else {
                /* printf("%s doesn't substring of %s\n", $1, log_line); */
                $$ = 0;
         }

         free($1);
       }
| expr OR expr { if ($1 == 1 || $3 == 1) { $$ = 1; } else { $$ = 0; } }
| expr AND expr { $$ = $1 * $3; }
| NOT expr { if ($2 == 1) { $$ = 0; } else { $$ = 1; } }
| '(' expr ')' { $$ = $2; }
;
%%

int log_string_match(char *log, char *match) {
        int len = snprintf(0, 0, "logvar=\"%s\"\n%s\n", log, match);
        char *final_log = malloc(len + 1);
        snprintf(final_log, len + 1, "logvar=\"%s\"\n%s\n", log, match);
        yy_scan_string(final_log);
        yyparse();
        yylex_destroy();
        free(final_log);
        if (result) return 1;
        return 0;
}

