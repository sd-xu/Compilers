#include <stdio.h>
#include "syntax_tree.h"

extern FILE *yyin;        // 输入文件的文件指针
extern int lexical_error; // 词法错误
extern int syntax_error;  // 语法错误

extern void yyrestart(FILE *input_file); // 重置yyin指针
extern int yylex();       // 词法分析接口
extern int yyparse(void); // 语法分析接口

struct Node *root; // 语法树根节点

int main(int argc, char **argv) {
    if (argc <= 1)  return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f); // 重置yyin指针: yyin = f
    yyparse();    // 会调用yylex()
    fclose(f);
    if (!lexical_error && !syntax_error)
        preorder_print(root, 0);
    
    return 0;
}