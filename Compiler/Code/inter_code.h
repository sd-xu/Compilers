#ifndef INTER_CODE_H
#define INTER_CODE_H

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include "syntax_tree.h"
#include "symbol_table.h"
#include "semantic.h"

#define IM_DEBUG 0

typedef struct Operand_*Operand;

struct Operand_ {
    enum {
        OP_VARIABLE = 0,
        OP_CONSTANT = 1,
        OP_FUNCTION = 2,
        OP_TEMPVAR = 3,
        OP_LABEL = 4
    }  kind;
    enum {
        OP_ADDRESS,
        OP_VAR
    } ifaddress;

    char *varname;
    int no;
    int value;
    char *funcname;
    int depth;
};

struct InterCode {
    enum {
        //单目
        IN_FUNCTION,
        IN_PARAM,
        IN_RETURN,
        IN_LABEL,
        IN_GOTO,
        IN_WRITE,
        IN_READ,
        IN_ARG,
        //双目
        IN_ASSIGN,
        IN_DEC,
        IN_CALL,
        //三目
        IN_ADD,
        IN_SUB,
        IN_MUL,
        IN_DIV,
        //四目
        IN_IFGOTO
    } kind;

    union {
        struct { Operand op0; } one;
        struct { Operand left, right; } two;
        struct { Operand result, op1, op2; } three;
        struct { // 实际上是op1, relop1, op2, op3
            Operand op1, op2, op3;
            char *relop;
        } four;
    } u;
};

struct Intercodes {
    struct InterCode code;
    struct Intercodes *prev;
    struct Intercodes *next;
};

void printop(Operand op,FILE *fp);
int intermediate_generate(struct Node *cur, FILE *fp, int is_print);
int Program_g(struct Node *cur);
int ExtDefList_g(struct Node *cur);
int ExtDef_g(struct Node *cur);
int FunDec_g(struct Node *cur);
int CompSt_g(struct Node *cur);
int DefList_g(struct Node *cur);
int StmtList_g(struct Node *cur);
int Def_g(struct Node *cur);
int DecList_g(struct Node *cur);
int Dec_g(struct Node *cur);
Operand VarDec_g(struct Node *cur);
Operand Exp_g(struct Node *cur);
int Stmt_g(struct Node *cur);
int Cond_g(struct Node *cur, Operand label_true, Operand label_false);

#endif