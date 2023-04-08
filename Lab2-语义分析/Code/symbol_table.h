#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct Symbol_Node_* Symbol_Node;
typedef struct Symbol_Stack_* Symbol_Stack;
typedef struct Func_List_* Func_List;

struct Type_ {
    enum { BASIC=0, ARRAY=1, STRUCTURE=2, FUNCTION=3 } kind;
    union {
        // 基本类型, 0:int, 1:float
        int basic;
        // 数组类型信息包括元素类型与数组大小构成
        struct { Type elem; int size; } array;
        // 结构体类型信息是一个链表
        struct { FieldList structure; char name[32]; } structure_;
        struct { 
            Type ret;        // 返回参数类型
            int num;         // 参数数量
            FieldList paras; // 接收参数
        } function;
    } u;
};

struct FieldList_ {
    char name[32];  // 域的名字, 和树节点名字长度一样
    Type type;      // 域的类型
    FieldList tail; // 下一个域
};

/* 符号表节点 */
struct Symbol_Node_ {
    enum { VARIABLE=0, STRUCT_NAME=1, FUNCTION_NAME=2 } kind;
    int is_def;               // 1:定义, 0:声明
    int depth;                // 深度, 作用于局部变量和全局变量
    struct FieldList_ field;  // 存类型+名字, 一般tail为NULL
    Symbol_Node hash_next;    // 相同hash的下一个, 十字链表
    Symbol_Node ctrl_next;    // 控制域链表
};

struct Symbol_Stack_ {
    Symbol_Node head;  // 指向该hash值的第一个节点
    Symbol_Stack next; // 仅在作用域控制链表中使用
};

struct Func_List_ {
    char name[32];
    int fun_lineno;
    Func_List next;
};

// hash函数
unsigned int hash_pjw(char* name);

Symbol_Stack symbol_init();

// 插入前创建符号表节点
Symbol_Node symbol(int kind, Type type, char *name, int is_def, int depth);
// 插入符号表节点
void insert_symbol(Symbol_Node node, Symbol_Stack scope);

// 进入/添加作用域
Symbol_Stack enter_scope();

// 离开作用域(一定是最深的那个)
void exit_scope();

// 在函数声明链表添加函数
void insert_func_dec(char *name, int func_lineno);

// 检查每个声明过的函数是否定义了
void check_func_def();

// 在结构体符号表插入结构体
int insert_struct(Type type, char *name);

// 类型检查, 1等, 0不等
int type_check(Type A, Type B);

// 判断两个数组是否强等价
int strong_array_check(Type A, Type B);

int query_symbol_name(char *name, int depth);

// 当前层查找, 0:不存在, 1:存在
int query_symbol(Type *type, char *name, int *is_def, int depth);

// 外层查找, 0:不存在, 1:存在
int query_symbol_exist(Type* type, char *name, int *is_def, int depth);

// 外层查找, 0:不存在, 1:存在
int query_symbol_exist2(Type* type, char *name, int *is_def, int depth, int *kind);

// 好像没用, 0:不存在, 1:存在
int query_struct_name(char *name);

// 好像没用, 0:不存在, 1:存在
int query_struct(Type *type, char *name);

// 查域名, 0:不存在, 1:存在
int query_struct_field(Type* query_type, Type struct_type, char *name);

void show_global_table();
void show_scope_table();
void show_struct_table();

#endif