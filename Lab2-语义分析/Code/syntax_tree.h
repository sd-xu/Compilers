#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H
#include <stdarg.h>

enum TOKEN_TYPE {
    TYPE_INT, TYPE_OCT, TYPE_HEX, TYPE_FLOAT, TYPE_ID, TYPE_TYPE, OTHER
};

typedef struct Node_* Node;

struct Node_ {
    int lineno;                 // 非终结符行号
    char name[32];              // 节点名称
    int is_terminal;            // 是否为终结符
    enum TOKEN_TYPE token_type; // 如果是int float ID TYPE要保存其对应的值
    union {                     // 存储对应类型的值
        unsigned int val_int;
        float val_float;
        char val_ID[32];
    } value;
    Node first_child;   // 第一个子节点
    Node sibling;       // 第一个兄弟节点
};

// 打印语法树, depth:深度
void preorder_print(Node node, int depth);

// 插入终结符（词法单元）节点
Node insert_terminal_node(const char* type, enum TOKEN_TYPE token_type, const char* value);

// 插入非终结符（语法单元）节点
Node insert_nonterminal_node(const char* type, int lineno, int num, ...);

// 设置父节点和兄弟节点
void set_par_sib(Node parent, int num, va_list valist);

// 获得第depth+1个“子节点”, 语义分析新增
Node get_child(Node curr, int depth);

#endif