#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H
#include <stdarg.h>

enum TOKEN_TYPE {
    TYPE_INT, TYPE_OCT, TYPE_HEX, TYPE_FLOAT, TYPE_ID, TYPE_TYPE, OTHER
};

struct Node {
    int lineno;                 // 非终结符行号
    char type[32];              // 类型
    int is_terminal;            // 是否为终结符
    enum TOKEN_TYPE token_type; // 如果是int float ID TYPE要保存其对应的值
    union {                     // 存储对应类型的值
        unsigned int val_int;
        float val_float;
        char val_ID[32];
    } value;
    struct Node* first_child;   // 第一个子节点
    struct Node* sibling;       // 第一个兄弟节点
};

// 打印语法树, depth:深度
void preorder_print(struct Node* node, int depth);

// 插入终结符（词法单元）节点
struct Node* insert_terminal_node(const char* type, enum TOKEN_TYPE token_type, const char* value);

// 插入非终结符（语法单元）节点
struct Node* insert_nonterminal_node(const char* type, int lineno, int num, ...);

// 设置父节点和兄弟节点
void set_par_sib(struct Node* parent, int num, va_list valist);

#endif