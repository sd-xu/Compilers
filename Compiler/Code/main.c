#include <stdio.h>
#include "syntax_tree.h"
#include "semantic.h"
#include "inter_code.h"
#include "assembly_code.h"

extern FILE *yyin;
extern int lexError;
extern int syntaxError;
extern int semanticError;
extern struct Node *root;
extern int yyrestart(FILE *);
extern int yylex();
extern int yyparse();

/*
 *  ./parser test.cmm
 *  ./parser test.cmm -ir out.ir            输出ir文件
 *  ./parser test.cmm -s out.s              输出.s汇编代码文件
 *  ./parser test.cmm -ir out.ir -s out.s   输出ir和s
 */
int main(int argc, char **argv) {
    if (argc <= 1)  return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }

    FILE *f_ir = NULL, *f_s = NULL;
    if (argc == 4) {
        if (strcmp(argv[2], "-ir") == 0) {
            f_ir = fopen(argv[3], "wt+");
            if (!f_ir) {
                perror(argv[3]);
                return 1;
            }
        }
        else if (strcmp(argv[2], "-s") == 0) {
            f_s = fopen(argv[3], "wt+");
            if (!f_s) {
                perror(argv[3]);
                return 1;
            }
        }
        else {
            perror(argv[2]);
            return 1;
        }
    }
    else if (argc == 6) {
        if (strcmp(argv[2], "-ir") == 0 && strcmp(argv[4], "-s") == 0) {
            f_ir = fopen(argv[3], "wt+");
            if (!f_ir) {
                perror(argv[3]);
                return 1;
            }
            f_s = fopen(argv[5], "wt+");
            if (!f_s) {
                perror(argv[5]);
                return 1;
            }
        }
        else {
            perror("顺序：-ir .. -s ..");
            return 1;
        }
    }
    else if (argc != 2) {
        perror("参数个数不正确！");
        return 1;
    }

    yyrestart(f);
    yyparse();

    if (lexError==0 && syntaxError==0) {
        semantic_check(root);

        if (!f_ir && !f_s) {    // ir==0, s==0
            if (semanticError == 0)
                tree_search(root, 0);
        }
        else if (!f_s) {    // ir==1, s==0
            intermediate_generate(root, f_ir, 1);
        }
        else if (!f_ir) {   // ir==0, s==1
            intermediate_generate(root, f_s, 0);
            a_code_generate(f_s);
        }
        else {  // ir==1, s==1
            intermediate_generate(root, f_ir, 1);
            a_code_generate(f_s);
        }
    }

    return 0;
};