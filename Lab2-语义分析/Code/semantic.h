#ifndef SEMANTIC_H
#define SEMANTIC_H
#include "syntax_tree.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>

void semantic_error_handler(int type, int lineno, char *msg);

void semantic_analysis(Node root);

void Program_check(Node curr);

void ExtDefList_check(Node curr);

void ExtDef_check(Node curr);

void ExtDecList_check(Node curr, Type type);

Type Specifier_check(Node curr);

FieldList VarDec_check(Node curr, Type type);

void FunDec_check(Node curr, const int is_def, const Type res_type, Symbol_Stack scope);

FieldList VarList_check(Node curr, Symbol_Stack scope);

FieldList ParamDec_check(Node curr);

void CompSt_check(Node curr, Symbol_Stack scope, Type res_type);

void StmtList_check(Node curr, Symbol_Stack scope, Type res_type);

void Stmt_check(Node curr, Symbol_Stack scope, Type res_type);

void DefList_check(Node curr, Symbol_Stack scope);

void Def_check(Node curr, Symbol_Stack scope);

void DecList_check(Node curr, Symbol_Stack scope, Type type);

void Dec_check(Node curr, Symbol_Stack scope, Type type);

Type Exp_check(Node curr);

int Args_check(Node curr, FieldList paras, char *func_name);

FieldList DefList_struct_check(Node curr, Symbol_Stack scope);
FieldList Def_struct_check(Node curr, Symbol_Stack scope);
FieldList DecList_struct_check(Node curr, Symbol_Stack scope, Type type);
FieldList Dec_struct_check(Node curr, Symbol_Stack scope, Type type);

#endif