#include "symbol_table.h"
#define SYMBOL_LEN 0x3fff

struct Symbol_Stack_ global_head[SYMBOL_LEN]; // Hash Table: 全局符号表

Symbol_Stack scope_head = NULL; // 作用域控制链表, 表头
Symbol_Stack scope_tail = NULL;

Func_List func_dec_head = NULL; // 函数声明链表
Func_List func_dec_tail = NULL;

struct Symbol_Stack_ struct_head[SYMBOL_LEN]; // 结构体符号表

// hash函数
unsigned int hash_pjw(char* name) {
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if (i = val & ~SYMBOL_LEN) 
            val = (val ^ (i >> 12)) & SYMBOL_LEN;
    }
    return val;
}

Symbol_Stack symbol_init() {
    scope_head = malloc(sizeof(struct Symbol_Stack_));
    scope_head->head = NULL;
    scope_head->next = NULL;
    scope_tail = scope_head;
    return scope_head;
}

// 插入前创建符号表节点
Symbol_Node symbol(int kind, Type type, char *name, int is_def, int depth) {
    Symbol_Node node = malloc(sizeof(struct Symbol_Node_));
    node->kind = kind;
    node->field.type = type;
    strcpy(node->field.name, name);
    node->is_def = is_def;
    node->depth = depth;
    node->hash_next = NULL;
    node->ctrl_next = NULL;
    return node;
}

// 插入符号表节点
void insert_symbol(Symbol_Node node, Symbol_Stack scope) {
    // printf("---------insert: %s\n", node->field.name);
    int idx = hash_pjw(node->field.name);

    if (scope == NULL || node->hash_next || node->ctrl_next)
        printf("Error!");
    else {
        /* 插入当前作用域链表 */
        Symbol_Node curr = scope->head;
        if (curr == NULL)
            scope->head = node;
        else {
            while (curr->ctrl_next) {

                curr = curr->ctrl_next;
            }
            
            curr->ctrl_next = node; // 插入尾
        }
        /* 插入全局符号表 */
        curr = global_head[idx].head;
        if (curr) // 冲突
            node->hash_next = curr; // 头插
        global_head[idx].head = node;
    }

    // show_scope_table();
    // show_global_table();
    // printf("%s\n", node->field.type->u.structure_.structure->name);
}

// 进入/添加作用域
Symbol_Stack enter_scope() {
    Symbol_Stack ret = malloc(sizeof(struct Symbol_Stack_));
    ret->head = NULL;
    ret->next = NULL;
    scope_tail->next = ret;
    scope_tail = ret;
    return ret;
}

// 离开作用域(一定是最深的那个)
void exit_scope() {
    Symbol_Node curr = scope_tail->head;

    /* 横向删, 批量删除 */
    while (curr) {
        int idx = hash_pjw(curr->field.name);
        // 虚拟节点+迭代
        Symbol_Node dmy = malloc(sizeof(struct Symbol_Node_));
        dmy->hash_next = global_head[idx].head;
        Symbol_Node tmp = dmy;
        
        while (tmp && tmp->hash_next) {
            if (tmp->hash_next == curr) {
                tmp->hash_next = curr->hash_next;
                break;
            }
            tmp = tmp->hash_next;
        }
        global_head[idx].head = dmy->hash_next;

        curr = curr->ctrl_next;
    }

    /* 纵向删, 删除作用域(断尾) */
    Symbol_Stack prev = scope_head;
    while (prev->next != scope_tail)
        prev = prev->next;
    scope_tail = prev;
    scope_tail->next = NULL;

    // show_scope_table();
}

// 在函数声明链表添加函数
void insert_func_dec(char *name, int func_lineno) {
    Func_List curr = malloc(sizeof(struct Func_List_));
    strcpy(curr->name, name);
    curr->fun_lineno = func_lineno;
    curr->next = NULL;

    if (func_dec_head == NULL) {
        func_dec_head = curr;
        func_dec_tail = curr;
    }
    else {
        func_dec_tail->next = curr;
        func_dec_tail = curr;
    }
}

// 检查每个声明过的函数是否定义了
void check_func_def() {
    Func_List curr = func_dec_head;
    while (curr) {
        Type useless_type = malloc(sizeof(struct Type_));
        int query_is_def = 0;
        int result = query_symbol(&useless_type, curr->name, &query_is_def, 0);
        if (query_is_def != 1) {
            fprintf(stderr, "\033[1;31m" "Error type 18 at Line %d: " "\033[0m", curr->fun_lineno);
            fprintf(stderr, "Undefined function \"%s\".\n", curr->name);
        }

        curr = curr->next;
    }
}

// 在结构体符号表插入结构体
int insert_struct(Type type, char *name) {
    // printf("insert: %s, type: %d\n", name, type->kind);
    int idx = hash_pjw(name);
    Symbol_Node curr = malloc(sizeof(struct Symbol_Node_));
    curr->field.type = type;
    strcpy(curr->field.name, name);

    curr->hash_next = NULL;
    curr->ctrl_next = NULL;

    if (struct_head[idx].head == NULL)
        struct_head[idx].head = curr;
    else { // 提前查重了
        curr->hash_next = struct_head[idx].head; // 头插
        struct_head[idx].head = curr;
    }

    // show_struct_table();
    return 1;
}

// 类型检查, 1等, 0不等
int type_check(Type A, Type B) {
    if (A == B) return 1;
    else if (A==NULL || B==NULL)    return 0;
    else if (A->kind != B->kind)    return 0;
    else {
        switch (A->kind) {
            case BASIC: {
                return A->u.basic == B->u.basic;
            }
            case ARRAY: {
                // 数组, 这里检测弱相等, 即当两者的维度相等时就相等
                int dim_A = 0, dim_B = 0;
                Type curr_A = A, curr_B = B;
                while (curr_A) {
                    dim_A += 1;
                    curr_A = curr_A->u.array.elem;
                    if (curr_A->kind != ARRAY)
                        break; 
                }
                while (curr_B) {
                    dim_B += 1;
                    curr_B = curr_B->u.array.elem;
                    if (curr_B->kind != ARRAY)
                        break; 
                }
                return dim_A == dim_B;
            }
            case STRUCTURE: {
                FieldList field_A = A->u.structure_.structure;
                FieldList field_B = B->u.structure_.structure;

                while (field_A && field_B) {
                    if (field_A->type->kind != field_B->type->kind)
                        return 0;
                    // 若为数组则检查数组强相等，若不为数组则检查类型相等
                    else if (field_A->type->kind==ARRAY &&
                        !strong_array_check(field_A->type, field_B->type))
                        return 0;
                    else if (!type_check(field_A->type, field_B->type))
                        return 0;
                    field_A = field_A->tail;
                    field_B = field_B->tail;
                }
                // 两者不等长: 0
                return !(field_A || field_B);
            }
            case FUNCTION: {
                FieldList paras_A = A->u.function.paras;
                FieldList paras_B = B->u.function.paras;

                if (A->u.function.num!=B->u.function.num ||
                    !type_check(A->u.function.ret, B->u.function.ret))
                    return 0;

                while (paras_A && paras_B) {
                    if (!type_check(paras_A->type, paras_B->type))
                        return 0;
                    paras_A = paras_A->tail;
                    paras_B = paras_B->tail;
                }
                return !(paras_A || paras_B);

            default:
                printf("No such type!\n");
                break;
            }
        }
    }
}

// 判断两个数组是否强等价
int strong_array_check(Type A, Type B) {
    if (A->u.array.size!=B->u.array.size || 
        A->u.array.elem->kind!=B->u.array.elem->kind)
        return 0;

    if (A->u.array.elem->kind == ARRAY)
        return strong_array_check(A->u.array.elem, B->u.array.elem);
    else
        return type_check(A->u.array.elem, B->u.array.elem);
}

int query_symbol_name(char *name, int depth) {
    Type null_type = malloc(sizeof(struct Type_));
    int null_def;
    return query_symbol(&null_type, name, &null_def, depth);
}

// 当前层查找, 0:不存在, 1:存在
int query_symbol(Type *type, char *name, int *is_def, int depth) {
    int idx = hash_pjw(name);
    Symbol_Node curr = global_head[idx].head;
    if (curr == NULL)
        return 0; // 不存在
    
    while (curr) {
        // 在同一层才算被找到, 否则不算
        if (strcmp(name, curr->field.name) == 0 && depth == curr->depth) {
            *type = curr->field.type;
            *is_def = curr->is_def;
            return 1;
        }
        curr = curr->hash_next;
    }
    return 0;
}

// 外层查找, 0:不存在, 1:存在
int query_symbol_exist(Type* type, char *name, int *is_def, int depth) {
    int idx = hash_pjw(name);
    Symbol_Node curr = global_head[idx].head;
    if (curr == NULL)
        return 0;
    
    while (curr) {
        // 进入一个局部作用域之后depth++, 因此当要找的depth小于depth的时候说明该层的前一层有
        if (strcmp(name, curr->field.name) == 0 && depth >= curr->depth) {
            *type = curr->field.type;
            *is_def = curr->is_def;
            return 1;
        }
        curr = curr->hash_next;
    }
    return 0;
}

// 外层查找, 0:不存在, 1:存在
int query_symbol_exist2(Type* type, char *name, int *is_def, int depth, int *kind) {
    int idx = hash_pjw(name);
    Symbol_Node curr = global_head[idx].head;
    if (curr == NULL)
        return 0;
    
    while (curr) {
        // 进入一个局部作用域之后depth++, 因此当要找的depth小于depth的时候说明该层的前一层有
        if (strcmp(name, curr->field.name) == 0 && depth >= curr->depth){
            *type = curr->field.type;
            *is_def = curr->is_def;
            *kind = curr->kind;
            return 1;
        }
        curr = curr->hash_next;
    }
    return 0;
}

// 好像没用, 0:不存在, 1:存在
int query_struct_name(char *name) {
    Type null_type = malloc(sizeof(struct Type_));
    return query_struct(&null_type, name);
}

// 好像没用, 0:不存在, 1:存在
int query_struct(Type *type, char *name) {
    int idx = hash_pjw(name);
    Symbol_Node curr = struct_head[idx].head;
    if (curr == NULL)
        return 0; // 不存在
    
    while (curr) {
        if (strcmp(name, curr->field.name) == 0) {
            *type = curr->field.type;
            return 1;
        }
        curr = curr->hash_next;
    }
    return 0;
}

// 查域名, 0:不存在, 1:存在
int query_struct_field(Type* query_type, Type struct_type, char *name) {
    // 提前判断kind是STRUCTURE
    FieldList curr = struct_type->u.structure_.structure;
    while (curr) {
        if (strcmp(name, curr->name) == 0) {
            *query_type = curr->type;
            return 1;
        }
        curr = curr->tail;
    }
    return 0;
}

void show_global_table() {
    printf("\033[1;37;42m" "\n------------------------- global_table starts\033[0m\n\n");
    for (int i = 0; i < SYMBOL_LEN; i++) {
        if (global_head[i].head) {
            printf("i: %d",i);
            Symbol_Node tmp = global_head[i].head;
            while (tmp) {
                printf(" -> name: %s, kind: %d, type: %d",tmp->field.name, tmp->kind, tmp->field.type->kind);
                tmp=tmp->hash_next;
            }
            printf("\n");
        }
    }
    printf("\033[1;37;43m" "\n------------------------- global_table ends\033[0m\n\n");
}

void show_scope_table() {
    printf("\033[1;37;42m" "\n------------------------- scope_table starts\033[0m\n\n");
    Symbol_Stack tmp = scope_head;
    int depth = 0;
    while (tmp) {
        printf("depth: %d",depth); 
        Symbol_Node tmp1 = tmp->head;
        while (tmp1) {
            printf(" -> %s",tmp1->field.name);
            tmp1 = tmp1->ctrl_next;
        }
        printf("\n");
        tmp = tmp->next;
        depth++;
    }
    printf("\033[1;37;43m" "\n------------------------- scope_table ends\033[0m\n\n");
}

// 可以改进, 比如打印结构体的每个域（注意结构体嵌套结构体）
void show_struct_table() {
    printf("\033[1;37;42m" "\n------------------------- struct_table starts\033[0m\n\n");
    for (int i = 0; i < SYMBOL_LEN; i++) {
        if (struct_head[i].head) {
            printf("i: %d ", i);
            Symbol_Node tmp = struct_head[i].head;
            while (tmp) {
                printf(" -> name: %s, type: %d", tmp->field.name, tmp->field.type->kind);
                tmp=tmp->hash_next;
            }
            printf("\n");
        }
    }
    printf("\033[1;37;43m" "\n------------------------- struct_table ends\033[0m\n\n");
}