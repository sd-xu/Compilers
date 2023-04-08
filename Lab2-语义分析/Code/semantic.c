#include "semantic.h"

#define FONT_COLOR "\033[1;31m"
#define RESET "\033[0m"

void semantic_error_handler(int type, int lineno, char *msg) {
    fprintf(stderr, FONT_COLOR "Error type %d at Line %d: " RESET, type, lineno);
    switch(type) {
        case 1:     fprintf(stderr, "Undefined variable \"%s\".\n", msg);                            break;
        case 2:     fprintf(stderr, "Undefined function \"%s\".\n", msg);                            break;
        case 3:     fprintf(stderr, "Redefined variable \"%s\".\n", msg);                            break;
        case 4:     fprintf(stderr, "Redefined function \"%s\".\n", msg);                            break;
        case 5:     fprintf(stderr, "Type mismatched for assignment.\n");                            break;
        case 6:     fprintf(stderr, "The left-hand side of an assignment must be a variable.\n");    break;
        case 7:     fprintf(stderr, "Type mismatched for operands.\n");                              break;
        case 8:     fprintf(stderr, "Type mismatched for return.\n");                                break;
        case 9:     fprintf(stderr, "Function \"%s\" is not applicable for arguments.\n", msg);      break;
        case 10:    fprintf(stderr, "\"%s\" is not an array.\n", msg);                               break;
        case 11:    fprintf(stderr, "\"%s\" is not a function.\n", msg);                             break;
        case 12:    fprintf(stderr, "\"%s\" is not an integer.\n", msg);                             break;
        case 13:    fprintf(stderr, "Illegal use of \".\".\n");                                      break;
        case 14:    fprintf(stderr, "Non-existent field \"%s\".\n", msg);                            break;
        case 15:    fprintf(stderr, "Redefined field \"%s\".\n", msg);                               break;
        case 16:    fprintf(stderr, "Duplicated name \"%s\".\n", msg);                               break;
        case 17:    fprintf(stderr, "Undefined structure \"%s\".\n", msg);                           break;
        case 18:    fprintf(stderr, "Undefined function \"%s\".\n", msg);                            break;
        case 19:    fprintf(stderr, "Inconsistent declaration of function \"%s\".\n", msg);          break;
        default:    fprintf(stderr, "Other Mistakes, content is: %s\n", msg);                        break;
    }
}

int depth_ = 0;
int struct_no_name_cnt = 0;
Symbol_Stack global_scope = NULL;

void semantic_analysis(Node root) {
    depth_ = 0;
    Program_check(root);
    // show_global_table();
    // show_scope_table();
    // show_struct_table();
}

void Program_check(Node curr) {
/*
    Program -> ExfDefList
*/
// printf("Program_check\n");
    global_scope = symbol_init();
    Node ExtDefList_node = get_child(curr, 0);
    ExtDefList_check(ExtDefList_node);
    check_func_def();
}

void ExtDefList_check(Node curr) {
/*
    ExfDefList -> ExfDef ExfDefList
        | (empty)
*/
// printf("ExtDefList_check\n");
    Node ExtDef_node = get_child(curr, 0);
    ExtDef_check(ExtDef_node);

    Node ExtDefList_node = get_child(curr, 1);
    if (ExtDefList_node)
        ExtDefList_check(ExtDefList_node);
}

// ExtDef可以得到某一个全局变量的所有信息, 即Type和name
void ExtDef_check(Node curr) {
/*
    ExtDef -> Specifier ExtDecList SEMI
        | Specifier SEMI
        | Specifier FunDec CompSt
        | Specifier FunDec SEMI // 新增, 用于2.1
*/
// printf("ExtDef_check\n");
    Type node_type = NULL;
    
    Node Specifier_node = get_child(curr, 0);
    Node tmp_node1 = get_child(curr, 1);
    Node tmp_node2 = get_child(curr, 2);

    if (Specifier_node)
        node_type = Specifier_check(Specifier_node);

    // 空: Specifier SEMI
    if (tmp_node2) {
        // Specifier ExtDecList SEMI
        if (tmp_node1 && strcmp(tmp_node1->name, "ExtDecList") == 0)
            ExtDecList_check(tmp_node1, node_type);
        else { 
            Node FunDec_node = tmp_node1;
            // Specifier Fundec SEMI
            if (strcmp(tmp_node2->name, "SEMI") == 0) {
                Symbol_Stack tmp_scope = enter_scope();
                FunDec_check(FunDec_node, 0, node_type, tmp_scope); // 声明
                exit_scope();
            }
            else {
            // Specifier Fundec Compst
                Symbol_Stack tmp_scope = enter_scope();
                FunDec_check(FunDec_node, 1, node_type, tmp_scope);
                
                Node Compst_node = tmp_node2;
                depth_++;
                CompSt_check(Compst_node, tmp_scope, node_type);
                depth_--;
                exit_scope();
            }
        }
    }
}

void ExtDecList_check(Node curr, Type type) {
/*
    ExtDecList -> VarDec
        | VarDec COMMA ExtDecList
*/
// printf("ExtDecList_check\n");
    Node VarDec_node = get_child(curr, 0);
    FieldList vardec_field = VarDec_check(VarDec_node, type);

    if (query_symbol_name(vardec_field->name, depth_)) // 重复定义
        semantic_error_handler(3, curr->lineno, vardec_field->name);
    
    Symbol_Node insert_node = symbol(VARIABLE, vardec_field->type, vardec_field->name, 1, depth_);
    insert_symbol(insert_node, global_scope);

    Node tmp_node = get_child(curr, 1);
    if (tmp_node) {
        Node ExtDecList_node = get_child(curr, 2);
        if (ExtDecList_node)
            ExtDecList_check(ExtDecList_node, type);
    }
}

// 返回该节点的Type
Type Specifier_check(Node curr) {
/*
    Specifier -> TYPE
        | StructSpecifier
    StructSpecifier -> STRUCT OptTag LC DefList RC
        | STRUCT Tag
    OptTag -> ID
        | (empty)
    Tag -> ID
*/
// printf("Specifier_check\n");
    Type type = malloc(sizeof(struct Type_));
    Node tmp_node0 = get_child(curr, 0);

    if (strcmp(tmp_node0->name, "TYPE") == 0) {
        type->kind = BASIC;
        if (strcmp(tmp_node0->value.val_ID, "int") == 0)
            type->u.basic = 0; // INT  
        else if (strcmp(tmp_node0->value.val_ID, "float") == 0)
            type->u.basic = 1; // FLOAT
    }
    else if (strcmp(tmp_node0->name, "StructSpecifier") == 0) {
        type->kind = STRUCTURE;
        Node tmp_node1 = get_child(tmp_node0, 1);

        if (strcmp(tmp_node1->name, "OptTag") == 0) {
            Node tmp_node2 = get_child(tmp_node1, 0);

            if (strcmp(tmp_node2->name, "ID") == 0) {
                char *struct_name = tmp_node2->value.val_ID;
                Type query_type =  malloc(sizeof(struct Type_));
                int query_is_def;
                int result1 = query_symbol_exist(&query_type, struct_name, &query_is_def, depth_);
                
                if (result1) {
                    semantic_error_handler(16, tmp_node2->lineno, struct_name);
                    return NULL;
                }
                // 不存在该名字的结构体, 获取type之后存进struct表

                strcpy(type->u.structure_.name, struct_name);
                type->u.structure_.structure = NULL;

                // 插global
                Symbol_Node insert_node = symbol(STRUCT_NAME, type, struct_name, 1, 0);
                insert_symbol(insert_node, global_scope);

                // 要不要查重
                insert_struct(type, struct_name); // 插struct表

                // StructSpecifier -> STRUCT OptTag LC DefList RC 现在开始处理DefList
                Node DefList_node = get_child(tmp_node0, 3);
                if (strcmp(DefList_node->name, "DefList") == 0)
                    type->u.structure_.structure = DefList_struct_check(DefList_node, global_scope);
            }
        }
        else if (strcmp(tmp_node1->name, "Tag") == 0) {
            // 需要检查这个结构体在不在结构体表里面
            Node ID_node = get_child(tmp_node1, 0);
            char *tmp_name = ID_node->value.val_ID;
            Type tmp_type = NULL;
            int tmp_def;

            int tmp_result = query_symbol_exist(&tmp_type, tmp_name, &tmp_def, depth_);
            if (tmp_result == 0) {
                // 结构体未定义
                semantic_error_handler(17, ID_node->lineno, tmp_name);
                return NULL;
            }
            else if (tmp_type == NULL || tmp_type->kind != STRUCTURE) {
                semantic_error_handler(17, ID_node->lineno, tmp_name);
                return NULL;
            }
            else
                return tmp_type;
        }
        else if (strcmp(tmp_node1->name, "LC") == 0) {
            // 匿名结构
            struct_no_name_cnt++;
            char *struct_name = malloc(sizeof(char)*32);
            strcpy(struct_name, "");
            strcpy(type->u.structure_.name, struct_name);
            type->u.structure_.structure = NULL;

            insert_struct(type, struct_name); // 插struct表

            Node DefList_node = get_child(tmp_node0, 2);
            if (strcmp(DefList_node->name, "DefList") == 0)
                type->u.structure_.structure = DefList_struct_check(DefList_node, global_scope);
        }
    }
    return type;
}

FieldList VarDec_check(Node curr, Type type) {
/*    
    VarDec -> ID
        | VarDec LB INT RB
*/
// printf("VarDec_check\n");
    FieldList field = malloc(sizeof(struct FieldList_));
    field->tail = NULL;
    Node tmp_node = get_child(curr, 0);
    if (strcmp(tmp_node->name, "ID") == 0) {
        field->type = type;
        strcpy(field->name, tmp_node->value.val_ID);
    }
    else {
        // 递归
        // 首先获得名字
        int cnt = 0;
        while (tmp_node->first_child) {
            tmp_node = tmp_node->first_child;
            cnt++;
        }
        Type *type_list = malloc(sizeof(Type *)*(cnt+2));
        strcpy(field->name, tmp_node->value.val_ID);
        tmp_node = get_child(curr, 0);

        // 现在是数组; a[10][3][2] 访问顺序:2->3->10->ID
        Node INT_node1 = NULL;
        Type tmp_type1 = NULL;
        cnt--;
        int max_cnt = cnt;

        while (tmp_node->first_child) {
            Type cur_type =  malloc(sizeof(struct Type_));
            INT_node1 = tmp_node->sibling->sibling;
            cur_type->kind = ARRAY;
            cur_type->u.array.size = INT_node1->value.val_int;
            type_list[cnt] = cur_type;
            cnt--;
            tmp_node = tmp_node->first_child;
        }

        tmp_type1 = type_list[0];
        Type temp_type2 = tmp_type1;
        type_list[max_cnt]->u.array.elem = type;
        for (int i = 1; i <= max_cnt; i++) {
            temp_type2->u.array.elem = type_list[i];
            temp_type2 = temp_type2->u.array.elem;
        }

        field->type = tmp_type1;
    }

    return field;
}

void FunDec_check(Node curr, const int is_def, const Type res_type, Symbol_Stack scope) {
/*    
    FunDec -> ID LP VarList RP
        | ID LP RP
*/
// printf("FunDec_check\n");
    // 如果之前没有定义/声明过, 就插入声明, 否则不插入; 并且声明只插入一次
    Type query_type = NULL;
    int query_is_def;
    Node ID_node = get_child(curr, 0);
    char *func_name = ID_node->value.val_ID;
    int result = query_symbol(&query_type, func_name, &query_is_def, depth_); // 函数都是最外层声明的, 不需用query_symbol_exist

    // 处理VarList
    // 先处理好type, 然后和查到的比较, 然后考虑插不插入
    Node tmp_node = get_child(curr, 2);
    Type func_type = malloc(sizeof(struct Type_));
    FieldList paras = NULL;

    if (strcmp(tmp_node->name, "VarList") != 0) {
        func_type->u.function.num = 0;
        func_type->u.function.paras = NULL;
    }
    else { // 处理VarList
        Node Varlist_node = tmp_node;
        depth_++; // VarList 是局部作用域
        paras = VarList_check(Varlist_node, scope);
        depth_--;
        int cnt = 0;
        FieldList tmp = paras;
        while (tmp) {
            cnt++;
            tmp = tmp->tail;
        }

        func_type->u.function.num = cnt;
        func_type->u.function.paras = paras;        
    }

    func_type->kind = FUNCTION;
    func_type->u.function.ret = res_type;

    // 然后进行检查
    if (result) { 
        // 找到一次了, 当且仅当定义的时候可以填表
        if (is_def) {
            if (query_is_def) {
                // 重复定义
                semantic_error_handler(4, curr->lineno, ID_node->value.val_ID);
            }
            else if (type_check(query_type, func_type) == 0) {
                // 定义和声明type不同报错
                semantic_error_handler(19, curr->lineno, ID_node->value.val_ID);
            }
            else { // 定义, 没有重复定义, 不和前面的冲突, 填表
                Symbol_Node insert_node = symbol(FUNCTION_NAME, func_type, func_name, is_def, depth_);
                insert_symbol(insert_node, global_scope);
            }
        }
        else { // 找到的是声明
            if (type_check(query_type, func_type) == 0) {
                // type不同报错
                semantic_error_handler(19, curr->lineno, ID_node->value.val_ID);
            }
        }
    }
    else { // 没有找到, 填表
        Symbol_Node insert_node = symbol(FUNCTION_NAME, func_type, func_name, is_def, depth_);
        insert_symbol(insert_node, global_scope);

        // 如果是声明, 加到func_dec链表
        if (is_def == 0)
            insert_func_dec(func_name, curr->lineno);
    }
    // 如果是定义的话, 没有重复定义就插入hash表
    // 如果是声明的话, 如果之前没有声明/定义过就插入hash表;因此全局变量中, 和函数同名的找到的第一个要么是定义, 要么是没有定义过的声明
    // 声明的函数插入声明链表用于最后的查找
}

FieldList VarList_check(Node curr, Symbol_Stack scope) {
/*
    VarList -> ParamDec COMMA VarList
        | ParamDec;
*/
// printf("VarList_check\n");
    // 需要注册
    Node ParamDec_node = get_child(curr, 0);
    FieldList result = ParamDec_check(ParamDec_node);
    Type query_type1 = malloc(sizeof(struct Type_));
    int query_is_def1;
    int result1 = query_symbol(&query_type1, result->name, &query_is_def1, 0);

    // 变量与已定义结构体重复
    if (result1 && query_type1 && query_type1->kind == STRUCTURE)
        semantic_error_handler(3, curr->lineno, result->name);

    Symbol_Node insert_node = symbol(VARIABLE, result->type, result->name, 1, depth_);
    insert_symbol(insert_node, scope);

    FieldList tmp = result;
    Node tmp_node = curr;
    // 这是第一个, 然后把它们串起来
    while (1) {
        if (get_child(tmp_node, 1) == NULL)
            break;
        
        tmp_node = get_child(tmp_node, 2);
        Node tmp_param = get_child(tmp_node, 0);
        FieldList VarDec_field = ParamDec_check(tmp_param);

        Type query_type =  (malloc(sizeof(struct Type_)));
        int query_is_def;
        int result1 = query_symbol(&query_type, VarDec_field->name, &query_is_def, 0);
        if (result1 == 0 && query_type && query_type->kind == STRUCTURE)
            semantic_error_handler(3, curr->lineno, VarDec_field->name);

        insert_node = symbol(VARIABLE, VarDec_field->type, VarDec_field->name, 1, depth_);
        insert_symbol(insert_node, scope);

        // 往局部作用域里面插入元素
        tmp->tail = VarDec_field;
        tmp = tmp->tail;
    }
    tmp->tail = NULL;
    return result;
}

FieldList ParamDec_check(Node curr) {
/*
    ParamDec -> Specifier VarDec
*/
// printf("ParamDec_check\n");
    Node Specifier_node = get_child(curr, 0);
    Node VarDec_node = get_child(curr, 1);
    Type node_type = Specifier_check(Specifier_node);
    FieldList result = VarDec_check(VarDec_node, node_type);
    return result;
}

void CompSt_check(Node curr, Symbol_Stack scope, Type res_type) {
/*
    CompSt -> LC DefList StmtList RC
    DefList -> Def DefList
        | (empty)
    StmtList -> Stmt StmtList
        | (empty)
*/
// printf("CompSt_check\n");
    Node tmp_node = get_child(curr, 1);

    if (strcmp(tmp_node->name, "DefList") == 0) {
        DefList_check(tmp_node, scope);

        Node StmtList_node = get_child(curr, 2);
        if (strcmp(StmtList_node->name, "StmtList") == 0) {
            StmtList_check(StmtList_node, scope, res_type);
        }
    }
    else if (strcmp(tmp_node->name, "StmtList") == 0) {
        StmtList_check(tmp_node, scope, res_type);
    }
}

void StmtList_check(Node curr, Symbol_Stack scope, Type res_type) {
/*
    StmtList -> Stmt StmtList
        | (empty)
    Stmt -> Exp SEMI+
        | CompSt
        | RETURN Exp SEMI
        | IF LP Exp RP Stmt
        | IF LP Exp RP Stmt ELSE Stmt
        | WHILE LP Exp RP Stmt
*/
// printf("StmtList_check\n");
    Node Stmt_node = get_child(curr, 0);
    Node tmp_node = get_child(curr, 1);
    Stmt_check(Stmt_node, scope, res_type);
    if (tmp_node)
        StmtList_check(tmp_node, scope, res_type) ;
}

void Stmt_check(Node curr, Symbol_Stack scope, Type res_type) {
/*
    Stmt -> Exp SEMI
        | CompSt
        | RETURN Exp SEMI
        | IF LP Exp RP Stmt
        | IF LP Exp RP Stmt ELSE Stmt
        | WHILE LP Exp RP Stmt
*/
// printf("Stmt_check\n");
    Node tmp_node1 = get_child(curr, 0);
    if (strcmp(tmp_node1->name, "Exp") == 0) {
        Type tmp_exp_type = Exp_check(tmp_node1);
    }
    else if (strcmp(tmp_node1->name, "CompSt") == 0) {
        depth_++;
        Symbol_Stack tmp_scope = enter_scope();
        CompSt_check(tmp_node1, tmp_scope, res_type);
        exit_scope();
        depth_--;
    }
    else if (strcmp(tmp_node1->name, "RETURN") == 0) {
        Node Exp_node = get_child(curr, 1);
        Type return_type = Exp_check(Exp_node);
        if (return_type) {
            int result = type_check(res_type, return_type);
            if (result == 0) // 返回类型不等
                semantic_error_handler(8, curr->lineno, NULL);
        }
    }
    else if (strcmp(tmp_node1->name, "WHILE") == 0) {
        Node Exp_node = get_child(curr, 2);
        Node Stmt_node = get_child(curr, 4);
        Type while_type = Exp_check(Exp_node);
        if (while_type) {
            // while条件非int，操作类型不匹配
            if (while_type->kind != BASIC || while_type->u.basic != 0)
                semantic_error_handler(7, curr->lineno, NULL);
        }
        Stmt_check(Stmt_node, scope, res_type);
    }
    else if (strcmp(tmp_node1->name, "IF") == 0) {
        Node Exp_node = get_child(curr, 2);
        Node ELSE_node = get_child(curr, 5); // ELSE
        Type if_type = Exp_check(Exp_node);
        if (if_type) {
            // if条件非int
            if (if_type->kind != BASIC || if_type->u.basic != 0)
                semantic_error_handler(7, curr->lineno, NULL);
        }

        if (ELSE_node == NULL) {
            Node Stmt_node = get_child(curr, 4);
            Stmt_check(Stmt_node, scope, res_type);
        }
        else {
            Node Stmt_node1 = get_child(curr, 4);
            Node Stmt_node2 = get_child(curr, 6);
            Stmt_check(Stmt_node1, scope, res_type);
            Stmt_check(Stmt_node2, scope, res_type);
        }
    }
}

void DefList_check(Node curr, Symbol_Stack scope) {
/*    
    DefList -> Def DefList
        | (empty) 
            注意为空的时候是 def-- (empty), 而不是 Deflist-->(empty)
    Def -> Specifier DecList SEMI
*/
// printf("DefList_check\n");
    Node tmp_node = get_child(curr, 0);
    if (tmp_node) {
        Node Def_node = tmp_node;
        Def_check(Def_node, scope);

        Node DefList_node = get_child(curr, 1);
        if (DefList_node)
            DefList_check(DefList_node, scope);
    }
}

void Def_check(Node curr, Symbol_Stack scope) {
/*    
    Def -> Specifier DecList SEMI
    DecList -> Dec
        | Dec COMMA DecList
*/
// printf("Def_check\n");
    Node Specifier_node = get_child(curr, 0);
    Type type = Specifier_check(Specifier_node);

    Node DecList_node = get_child(curr, 1);
    DecList_check(DecList_node, scope, type);
}

void DecList_check(Node curr, Symbol_Stack scope, Type type) {
/*
    DecList -> Dec
        | Dec COMMA DecList
    Dec -> VarDec
        | VarDec ASSIGNOP Exp
*/
// printf("DecList_check\n");
    Node Dec_node = get_child(curr, 0);
    Dec_check(Dec_node, scope, type);
    Node tmp_node = get_child(curr, 1);
    if (tmp_node) {
        Node DecList_node = get_child(curr, 2);
        if (DecList_node)
            DecList_check(DecList_node, scope, type); // 递归
    }
}

void Dec_check(Node curr, Symbol_Stack scope, Type type) {
/*
    Dec -> VarDec
        | VarDec ASSIGNOP Exp
*/
// printf("Dec_check\n");
    Node VarDec_node = get_child(curr, 0);
    FieldList VarDec_field = VarDec_check(VarDec_node, type);

    int result = query_symbol_name(VarDec_field->name, depth_); // 定义考虑的是当前层
    Type query_type = malloc(sizeof(struct Type_));
    int useless_is_def;
    int query_kind;
    int result1 = query_symbol_exist2(&query_type, VarDec_field->name, &useless_is_def, depth_, &query_kind);

    Node tmp_node = get_child(curr, 1);
    if (tmp_node == NULL) {
        if (result) {
            // 符号已存在, 重复定义
            semantic_error_handler(3, curr->lineno, VarDec_field->name);
        }
        else if (result1 && query_type->kind == STRUCTURE && query_kind == STRUCT_NAME)
            // 与结构体名字重复, 重复定义
            semantic_error_handler(3, curr->lineno, VarDec_field->name);
        else {
            // 插入
            Symbol_Node insert_node = symbol(VARIABLE, VarDec_field->type, VarDec_field->name, 1, depth_);
            insert_symbol(insert_node, scope);
        }
    }
    else {
        if (result)
            // 符号已存在, 重复定义
            semantic_error_handler(3, curr->lineno, VarDec_field->name);
        else {
            Symbol_Node insert_node = symbol(VARIABLE, VarDec_field->type, VarDec_field->name, 1, depth_);
            insert_symbol(insert_node, scope);

            Node Exp_node = get_child(curr, 2);
            Type exp_type = Exp_check(Exp_node);
            // 检查表达式类型
            if (exp_type) {
                int result = type_check(VarDec_field->type, exp_type);
                if (result == 0)
                    // 赋值号左右类型不匹配
                    semantic_error_handler(5, curr->lineno, NULL);
                else if (result1 && query_type->kind == STRUCTURE && query_kind == STRUCT_NAME)
                    // 与结构体名字重复, 重复定义
                    semantic_error_handler(3, curr->lineno, VarDec_field->name);
            }
        }
    }
}

Type Exp_check(Node curr) {
/*
    Exp -> Exp ASSIGNOP Exp
        | Exp AND Exp
        | Exp OR Exp
        | Exp RELOP Exp
        | Exp PLUS Exp
        | Exp MINUS Exp
        | Exp STAR Exp
        | Exp DIV Exp

        | LP Exp RP
        | MINUS Exp
        | NOT Exp

        | ID LP Args RP
        | ID LP RP
        | Exp LB Exp RB
        | Exp DOT ID;

        | ID
        | INT
        | FLOAT
*/
// printf("Exp_check\n");
    if (curr == NULL)   return NULL;
    Type result = NULL;
    Node tmp_node1 = get_child(curr, 0);
    Node tmp_node2 = get_child(curr, 1);

    // 先判断左值错误; 左值: ID, EXP DOT ID(结构体) Exp LB Exp RB (数组)
    if (strcmp(tmp_node1->name, "Exp") == 0) {
        if (tmp_node2 && strcmp(tmp_node2->name, "ASSIGNOP") == 0) {
            Node tmp_node11 = get_child(tmp_node1, 0);
            Node tmp_node12 = get_child(tmp_node1, 1);
            // 左边这个Exp 是 exp->ID 还是 exp->exp
            if (tmp_node12 == NULL) { // 一元, exp->ID
                // 非标识符(ID), 则赋值号左边只有右值
                if (strcmp(tmp_node11->name, "ID") != 0) {
                    semantic_error_handler(6, curr->lineno, NULL);
                    return NULL;
                }
            }
            else { // exp->exp
                Node tmp_node13 = get_child(tmp_node1, 2);
                if (tmp_node13 == NULL) { // 二元
                    // 无二元左值, 赋值号左边只有右值
                    semantic_error_handler(6, curr->lineno, NULL);
                    return NULL;
                }
                else {
                    Node tmp_node14 = get_child(tmp_node1, 3);
                    if (tmp_node14 == NULL) { // 三元, EXP DOT ID
                        if (strcmp(tmp_node11->name, "Exp") != 0 || strcmp(tmp_node12->name, "DOT") != 0 || strcmp(tmp_node13->name, "ID") != 0) {
                            semantic_error_handler(6, curr->lineno, NULL);
                            return NULL;
                        }
                    }
                    else { // 四元, Exp LB Exp RB
                        if (strcmp(tmp_node11->name, "Exp") != 0 || strcmp(tmp_node12->name, "LB") != 0 || strcmp(tmp_node13->name, "Exp") != 0 || strcmp(tmp_node14->name, "RB") != 0) {
                            semantic_error_handler(6, curr->lineno, NULL);
                            return NULL;
                        }
                    }
                }
            }
        }
    }

    /*
        | ID
        | INT
        | FLOAT
    */
    if (tmp_node2 == NULL) { // 一元
        if (strcmp(tmp_node1->name, "ID") == 0) {
            Type query_type0 = malloc(sizeof(struct Type_));
            int query_is_def0;
            int result_local = query_symbol(&query_type0, tmp_node1->value.val_ID, &query_is_def0, depth_); // 当前层
            
            Type query_type1 = malloc(sizeof(struct Type_));
            int query_is_def1;
            int query_kind;
            int result_global = query_symbol_exist2(&query_type1, tmp_node1->value.val_ID, &query_is_def1, depth_, &query_kind); // 所有层
            
            if (result_local) { // 当前层找到, 局部定义
                result = query_type0;
                return result;
            }
            else if (result_global == 0 || (result_global && query_kind != VARIABLE)) {
                // 全局也无定义, 或全局定义不是变量
                semantic_error_handler(1, curr->lineno, tmp_node1->value.val_ID);
                return NULL;
            }
            else { // 全局有定义
                result = query_type1;
                return result;
            }
        }
        else if (strcmp(tmp_node1->name, "INT") == 0) {
            result = malloc(sizeof(struct Type_));
            result->kind = BASIC;
            result->u.basic = 0;
            return result;
        }
        else if (strcmp(tmp_node1->name, "FLOAT") == 0) {
            result = malloc(sizeof(struct Type_));
            result->kind = BASIC;
            result->u.basic = 1;
            return result;
        }
    }
    else {
        Node tmp_node3 = get_child(curr, 2);

        /*  第一部分
            | Exp ASSIGNOP Exp
            | Exp AND Exp
            | Exp OR Exp
            | Exp RELOP Exp
            | Exp PLUS Exp
            | Exp MINUS Exp
            | Exp STAR Exp
            | Exp DIV Exp
        */
        if (tmp_node3) {
            Node tmp_node4 = get_child(curr, 3);
            // 三元, 且第三项是Exp
            if (tmp_node4 == NULL && strcmp(tmp_node3->name, "Exp") == 0 && strcmp(tmp_node2->name, "LB") != 0) {
                Node Exp_node1 = tmp_node1;
                Node Exp_node2 = tmp_node3;
                Type exp1_type = Exp_check(Exp_node1);
                Type exp2_type = Exp_check(Exp_node2);
                if (exp1_type && exp2_type) {
                    int tmp_result = type_check(exp1_type, exp2_type);
                    if (tmp_result == 0 && strcmp(tmp_node2->name, "ASSIGNOP") == 0) {
                        semantic_error_handler(5, curr->lineno, NULL);
                        return NULL;
                    }

                    if (tmp_result == 0) {
                        semantic_error_handler(7, curr->lineno, NULL);
                        return NULL;
                    }
                    else {
                        result = exp1_type;
                        return result;
                    }
                }
                else // 如果是NULL, exp里会报错, 不重复报错
                    return NULL;
            }
        }
        
        /*  第二部分 
            | LP Exp RP
            | MINUS Exp
            | NOT Exp
        */
        if (strcmp(tmp_node1->name, "LP") == 0 || strcmp(tmp_node1->name, "MINUS") == 0 || strcmp(tmp_node1->name, "NOT") == 0) {
            Node Exp_node = tmp_node2;
            result = Exp_check(Exp_node);
            return result;
        }

        /*  第三部分
            | ID LP Args RP 函数
            | ID LP RP

            | Exp LB Exp RB 数组
            | Exp DOT ID    结构体
        */
        // 函数部分, 因为此时不是一元, 只需判断第一个是不是ID
        // 需要检查这个函数的存在性, 得到函数的params交给下一层检查, 并且查看这个ID是不是函数类型
        if (strcmp(tmp_node1->name, "ID") == 0) {
            char *func_name = tmp_node1->value.val_ID;
            Type query_type = malloc(sizeof(struct Type_));
            int query_is_def = 0;
            int query_result = query_symbol_exist(&query_type, func_name, &query_is_def, depth_); // 全局搜索
            result = query_type->u.function.ret;

            if (query_result) { // 找到了, 判断类型
                if (query_type->kind != FUNCTION) {
                    semantic_error_handler(11, curr->lineno, func_name);
                    return NULL;
                }
            }
            else { // 没找到或者不是定义
                semantic_error_handler(2, curr->lineno, func_name);
                return NULL;
            }

            if (strcmp(tmp_node3->name, "Args") == 0) {
                if (query_type->u.function.paras == NULL) {
                    semantic_error_handler(9, curr->lineno, func_name);
                    return NULL;
                }
                else {
                    /*
                        Args -> Exp COMMA Args
                            | Exp;
                    */
                    // 检查args的数量
                    int cnt = 0;
                    Node cnt_node = tmp_node3;
                    while (1) {
                        cnt++;
                        Node tmp_cnt_node = get_child(cnt_node, 1);
                        if (tmp_cnt_node == NULL)   break;
                        cnt_node = get_child(cnt_node, 2);
                    }
                    if (cnt != query_type->u.function.num) {
                        semantic_error_handler(9, curr->lineno, func_name);
                        return NULL;
                    }

                    int args_result = Args_check(tmp_node3, query_type->u.function.paras, func_name);
                    if (args_result)    
                        return result;
                    else    
                        return NULL;
                }
            }
            else {
                if (query_type->u.function.paras) {
                    semantic_error_handler(9, curr->lineno, func_name);
                    return NULL;
                }
                else
                    return result;
            }
            
        }
        else {
            Node tmp_node4 = get_child(curr, 3);
            if (tmp_node4 == NULL) {
                // 结构体部分, 首先这个结构体要存在, 然后这个域名要存在, 然后返回这个域名的type
                if (strcmp(tmp_node1->name, "Exp") == 0 && strcmp(tmp_node2->name, "DOT") == 0 && strcmp(tmp_node3->name, "ID") == 0) {
                    Type exp_type = Exp_check(tmp_node1);
                    if (exp_type) {
                        if (exp_type->kind != STRUCTURE) {
                            semantic_error_handler(13, curr->lineno, NULL);
                            return NULL;
                        }
                        else {
                            //  查域名: 遍历type的structure_后的所有FieldList(tail)
                            Type query_type = malloc(sizeof(struct Type_));
                            int query_result = query_struct_field(&query_type, exp_type, tmp_node3->value.val_ID);
                            
                            if (query_result) {
                                result = query_type;
                                return result;
                            }
                            else { // 域名不存在
                                semantic_error_handler(14, curr->lineno, tmp_node3->value.val_ID);
                                return NULL;
                            }
                        }
                    }
                    else
                        return NULL;
                }
            }
            else {
                // 数组部分
                if (strcmp(tmp_node1->name, "Exp") == 0 && strcmp(tmp_node2->name, "LB") == 0 && strcmp(tmp_node3->name, "Exp") == 0) {
                    Type type1 = Exp_check(tmp_node1);
                    Type type3 = Exp_check(tmp_node3);
                    
                    if (type1 == NULL || type3 == NULL)
                        return NULL;

                    if (type1->kind != ARRAY) {
                        Node ID_node = get_child(tmp_node1, 0);
                        semantic_error_handler(10, curr->lineno, ID_node->value.val_ID);
                        return NULL; // 直接返回了, 所以如果同时还有错误12, 不会同时报
                    }
                    else if (type3->kind != BASIC || type3->u.basic != 0) {
                        // 大胆猜测所有情况都是float
                        Node INT_node = get_child(tmp_node3, 0);
                        char num_str[100];
                        sprintf(num_str, "%g", INT_node->value.val_float);
                        semantic_error_handler(12, curr->lineno, num_str);
                        return NULL;
                    }

                    result = type1->u.array.elem;
                    return result;
                }
            }
        }
    }
    return NULL;
}

int Args_check(Node curr, FieldList paras, char *func_name) {
/*
    Args -> Exp COMMA Args
        | Exp;
*/
// printf("Args_check\n");
    Node Exp_node = get_child(curr, 0);
    Node tmp_node = get_child(curr, 1);

    if (paras == NULL) {
        semantic_error_handler(9, curr->lineno, func_name);
        return 0;
    }

    Type tmp_type = Exp_check(Exp_node);
    if (tmp_type) {
        if (paras->type == NULL) // 参数类型NULL
            semantic_error_handler(9, curr->lineno, func_name);
        else {
            int result = type_check(tmp_type, paras->type);
            if (result == 0) { // 参数类型不匹配
                semantic_error_handler(9, curr->lineno, func_name);
                return 0;
            }
        }
    }

    if (tmp_node) {
        if (paras->tail == NULL) { // 参数数目不匹配
            semantic_error_handler(9, curr->lineno, func_name);
            return 0;
        }
        else {
            Node args_node = get_child(curr, 2);
            return Args_check(args_node, paras->tail, func_name);
        }
    }

    return 1;
}

FieldList DefList_struct_check(Node curr, Symbol_Stack scope) {
/*    
    DefList -> Def DefList
        | (empty) 
    Def -> Specifier DecList SEMI
*/
// printf("DefList_struct_check\n");
    FieldList ret_field = NULL;
    Node tmp_node = get_child(curr, 0);
    if (tmp_node) {
        Node Def_node = tmp_node;
        ret_field = Def_struct_check(Def_node, scope);

        Node DefList_node = get_child(curr, 1);
        if (DefList_node) {
            if (ret_field)
                ret_field->tail = DefList_struct_check(DefList_node, scope);
            else
                ret_field = DefList_struct_check(DefList_node, scope);
        }
    }

    return ret_field;
}

FieldList Def_struct_check(Node curr, Symbol_Stack scope) {
/*    
    Def -> Specifier DecList SEMI
    DecList -> Dec
        | Dec COMMA DecList
*/
// printf("Def_struct_check\n");
    FieldList ret_field = NULL;

    Node Specifier_node = get_child(curr, 0);
    Type type = Specifier_check(Specifier_node); // 继续判断类型, 返回的一定是封装好的Type, 所以可以处理结构体内嵌套结构体

    if (type) {
        Node DecList_node = get_child(curr, 1);
        ret_field = DecList_struct_check(DecList_node, scope, type);
    }

    return ret_field;
}

FieldList DecList_struct_check(Node curr, Symbol_Stack scope, Type type) {
/*
    DecList -> Dec
        | Dec COMMA DecList
    Dec -> VarDec
        | VarDec ASSIGNOP Exp
*/
// printf("DecList_struct_check\n");
    FieldList ret_field = NULL;

    Node Dec_node = get_child(curr, 0);
    ret_field = Dec_struct_check(Dec_node, scope, type);

    Node tmp_node = get_child(curr, 1);
    if (tmp_node) {
        Node DecList_node = get_child(curr, 2);
        if (DecList_node) { // 递归
            if (ret_field)
                ret_field->tail = DecList_struct_check(DecList_node, scope, type); 
            else
                ret_field = DecList_struct_check(DecList_node, scope, type);
        }
    }

    return ret_field;
}

FieldList Dec_struct_check(Node curr, Symbol_Stack scope, Type type) {
/*
    Dec -> VarDec
        | VarDec ASSIGNOP Exp
*/
// printf("Dec_struct_check\n");
    // FieldList ret_field = malloc(sizeof(struct FieldList_));
    FieldList ret_field = NULL;

    Node VarDec_node = get_child(curr, 0);
    FieldList VarDec_field = VarDec_check(VarDec_node, type);

    Node tmp_node = get_child(curr, 1);
    if (tmp_node) // 定义不能初始化
        semantic_error_handler(15, curr->lineno, VarDec_field->name);
    else {
        int result = query_symbol_name(VarDec_field->name, 0); // depth_: 0
        if (result) // 现在在结构体内
            semantic_error_handler(15, curr->lineno, VarDec_field->name);
        else {
            // 结构体类型在Specifier_check里插入
            Symbol_Node insert_node = symbol(VARIABLE, VarDec_field->type, VarDec_field->name, 1, 0);
            insert_symbol(insert_node, scope);
            ret_field = &insert_node->field;
        }
    }
    return ret_field;
}