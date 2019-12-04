#include "yacc_decls.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

extern FILE *yyin;
extern int yylineno;

int yyparse();

/* keywords map */
void init_keywords();

void destruct_keywords();

void destruct_identifiers();

void setup_codegen(const char *output_file_path);
void close_codegen();


bool compiler = true;

int main(int argc, char *argv[]) {
    init_keywords();

    if (compiler) {
        if (argc != 3) {
            puts("Usage: compiler <input_file> <output_file>");
            exit(65);
        }

        setup_codegen(argv[2]);
        if ((yyin = fopen(argv[1], "r")) == NULL) {
            printf("File ` %s ` not found", argv[1]);
            exit(1);
        }
        yyparse();
        close_codegen();
    }
    else if (argc == 2 && (yyin = fopen(argv[1], "r"))) {
        yyparse();
        fclose(yyin);
    } else {
        yyin = stdin;
        yyparse();
    }

    destruct_keywords();
    destruct_identifiers(); // destruct_global_env
    return 0;
}