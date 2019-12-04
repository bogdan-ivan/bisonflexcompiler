%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "AST.h"

extern int yylineno;
int yylex(void);
void yyerror(char *s);

ASTNode* oper(int type, int nops, ...);
ASTNode* id(char *name);
ASTNode* literal(int value);
void free_AST(ASTNode*);

void compile(ASTNode *p);
ASTNode *aux_nPtr;

bool syntax_error = false;
%}

%union {
    int iValue;
    char* name;
    ASTNode *nPtr;
};

%token <iValue> INTEGER
%token <name> IDENTIFIER
%token <nPtr> COMMENT
%token WHILE IF PRINT BLOCK FOR VAR
%nonassoc IFX
%nonassoc ELSE

%right ASSIGN
%left OR
%left AND
%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/' '%'
%nonassoc UMINUS

%type <nPtr> stmt expr stmt_list stmt_expr initializer opt_expr error decl var_decl

%%

program:
        function               ; //{ exit(0); }

function:
          function decl         { if(!syntax_error) compile($2); free_AST($2); }
        | /* NULL */
        ;
decl:
	  stmt                           { $$ = $1; }
	| var_decl                       { $$ = $1; }
	| COMMENT			 { $$ = NULL; }
	| error ';'
	| error '}'
	;
        /* `%prec IFX` only removes shift-reduce conflict */

var_decl:
          VAR IDENTIFIER ';'             { $$ = oper(VAR, 2, id($2), NULL); }
	| VAR IDENTIFIER '=' expr ';'    { $$ = oper(VAR, 2, id($2), $4); }
        ;

stmt:
          stmt_expr                                   { $$ = $1; }
        | PRINT expr ';'                 	      { $$ = oper(PRINT, 1, $2); }
        | WHILE '(' expr ')' stmt                     { $$ = oper(WHILE, 2, $3, $5); }
        | FOR '(' initializer opt_expr ';' opt_expr ')' stmt
              {
		/*
		 * stmt;
		 * post_expr;
		*/
                aux_nPtr = oper(';', 2, $8, $6); // (Update: WRONG idea) maybe switch on BLOCK and insert stmt inside (';', 2, stmt_list, stmt)

		/*
		 * while (cond)
		 * 		stmt;
		 *		post_expr;
		*/
                if (NULL == $4) {
                  /* If stmt_expr is empty; insert "true" loop condition. */
                  aux_nPtr = oper(WHILE, 2, literal(1), aux_nPtr);
                }
                else {
                  aux_nPtr = oper(WHILE, 2, $4, aux_nPtr);
                }

		/*
		 * initializer;
		 * while (cond)
		 *		stmt;
		 * 		post_expr;
		*/
                aux_nPtr = oper(';', 2, $3, aux_nPtr);

		/*
		 * {
		 *	 initializer;
		 *	 while (cond)
		 * 		stmt;
		 *		post_expr;
		 * }
		 * ----
		 * Note: post_expr is part of WHILE, but is not within the same scope as `stmt`.
		 * ----
		*/
                $$ = oper(BLOCK, 1, aux_nPtr);
              }
        | IF '(' expr ')' stmt %prec IFX              { $$ = oper(IF, 2, $3, $5); }
        | IF '(' expr ')' stmt ELSE stmt              { $$ = oper(IF, 3, $3, $5, $7); }
        | '{' stmt_list '}'                           { $$ = oper(BLOCK, 1, $2); }
        ;

stmt_list:
          stmt_list decl        { $$ = oper(';', 2, $1, $2); }
        |                       { $$ = NULL; }
        ;

  // used by for only
initializer:
          stmt_expr                      { $$ = $1; }
        | PRINT expr ';'                 { $$ = oper(PRINT, 1, $2); }
        | var_decl                       { $$ = $1; }
        ;

stmt_expr:
          ';'                            { $$ = oper(';', 2, NULL, NULL); }
        | expr ';'                       { $$ = $1; }
        ;

  // used by for only
opt_expr:
          expr                           { $$ = $1; }
        | /* nothing */                  { $$ = NULL; }
        ;

expr:
          INTEGER                            { $$ = literal($1); }
        | IDENTIFIER                         { $$ = id($1); }
        | '-' expr %prec UMINUS              { $$ = oper(UMINUS, 1, $2); }
        | IDENTIFIER '=' expr %prec ASSIGN   { $$ = oper('=', 2, id($1), $3); }
        | expr '+' expr                      { $$ = oper('+', 2, $1, $3); }
        | expr '-' expr                      { $$ = oper('-', 2, $1, $3); }
        | expr '*' expr                      { $$ = oper('*', 2, $1, $3); }
        | expr '/' expr                      { $$ = oper('/', 2, $1, $3); }
        | expr '%' expr                      { $$ = oper('%', 2, $1, $3); }
        | expr '<' expr                      { $$ = oper('<', 2, $1, $3); }
        | expr '>' expr                      { $$ = oper('>', 2, $1, $3); }
        | expr GE expr                       { $$ = oper(GE, 2, $1, $3); }
        | expr LE expr                       { $$ = oper(LE, 2, $1, $3); }
        | expr NE expr                       { $$ = oper(NE, 2, $1, $3); }
        | expr EQ expr                       { $$ = oper(EQ, 2, $1, $3); }
        | expr OR expr                       { $$ = oper(OR, 2, $1, $3); }
        | expr AND expr                      { $$ = oper(AND, 2, $1, $3); }
        | '(' expr ')'                       { $$ = $2; }
        ;

%%

void yyerror(char *s) {
    syntax_error = true;
    fprintf(stderr, "line %d: %s\n", yylineno, s);
}
