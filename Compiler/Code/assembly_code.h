#ifndef ASSEMBLY_CODE_H
#define ASSEMBLY_CODE_H

#include "inter_code.h"
#include "symbol_table.h"
#define SI_DEBUG 0
#define SI_DEBUG2 0

struct Register {
    int state;  // 表示状态: 0为空闲 1为使用
    char *name; // 表示名字
};

struct stack_node {
    int offset;
    int kind;
    int no;
    struct stack_node *next;
};

struct pid_stack {
    struct stack_node *fp;
    struct pid_stack *next;
};

void a_code_generate(FILE *fp);

#endif