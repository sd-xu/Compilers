#include <stdio.h>
#include "syntax_tree.h"
#include "semantic.h"
#include "intercode.h"

extern FILE *yyin;
extern int lexError;
extern int syntaxError;
extern struct Node *root;
extern int yyrestart(FILE *);
extern int yylex();
extern int yyparse();

int main(int argc, char **argv) {    
    if (argc <= 1) return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f) {
    perror(argv[1]);
    	return 1;
    }
    FILE *fp = fopen(argv[2], "wt+");
    if (!fp) {
        perror(argv[2]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if (lexError==0 && syntaxError==0) {
        // tree_search(root, 0);
        semantic_check(root);
        intermediate_generate(root, fp);
    }
    return 0;
};