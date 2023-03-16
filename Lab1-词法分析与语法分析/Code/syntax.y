%{
    /* 要声明的部分 */
    #include <stdio.h>
    #include "lex.yy.c"
    #include "syntax_tree.h"

    extern struct Node *root;
    int syntax_error = 0;
    void myerror(const char *msg);
    void yyerror(const char *msg);
%}

%locations

/* declared types */
// 变量yylval是YYSTYPE, 以下定义会在.tab.h中生成 YYSTYPE
%union {
    struct Node* node;
}

/* declared tokens */
%token <node> INT FLOAT ID SEMI COMMA ASSIGNOP RELOP
%token <node> PLUS MINUS STAR DIV
%token <node> AND OR NOT
%token <node> DOT
%token <node> TYPE
%token <node> LP RP LB RB LC RC
%token <node> STRUCT RETURN IF ELSE WHILE

/* declared non-terminals */
%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> Specifier StructSpecifier OptTag Tag
%type <node> VarDec FunDec VarList ParamDec
%type <node> CompSt StmtList Stmt
%type <node> DefList Def DecList Dec
%type <node> Exp Args

/* 优先级与结合性 */
%start Program
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left UMINUS // 处理负号
%left DOT
%left LB RB
%left LP RP

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%
/* High-level Definitions */
Program : ExtDefList                            { $$=insert_nonterminal_node("Program", @$.first_line, 1, $1); root=$$; }
        ;

// 0或多个ExtDef
ExtDefList : ExtDef ExtDefList                  { $$=insert_nonterminal_node("ExtDefList", @$.first_line, 2, $1, $2); }
           | /* empty */                        { $$=NULL; }
           ;

// 定义语句: 变量 结构体 函数
ExtDef : Specifier ExtDecList SEMI              { $$=insert_nonterminal_node("ExtDef", @$.first_line, 3, $1, $2, $3); }
       | Specifier SEMI                         { $$=insert_nonterminal_node("ExtDef", @$.first_line, 2, $1, $2); }
       | Specifier FunDec CompSt                { $$=insert_nonterminal_node("ExtDef", @$.first_line, 3, $1, $2, $3); }
       | Specifier ExtDecList error SEMI        { myerror("Missing \";\""); }
       | Specifier error SEMI                   { myerror("Missing \";\""); }
       | Specifier error CompSt                 {  }
       ;

ExtDecList : VarDec                             { $$=insert_nonterminal_node("ExtDecList", @$.first_line, 1, $1); }
           | VarDec COMMA ExtDecList            { $$=insert_nonterminal_node("ExtDecList", @$.first_line, 3, $1, $2, $3); }
           ;

/* Specifiers */
// 类型描述符
Specifier : TYPE                                { $$=insert_nonterminal_node("Specifier", @$.first_line, 1, $1); }
          | StructSpecifier                     { $$=insert_nonterminal_node("Specifier", @$.first_line, 1, $1); }
          ;

// 结构体类型
StructSpecifier : STRUCT OptTag LC DefList RC   { $$=insert_nonterminal_node("StructSpecifier", @$.first_line, 5, $1, $2, $3, $4, $5); }
            | STRUCT Tag                        { $$=insert_nonterminal_node("StructSpecifier", @$.first_line, 2, $1, $2); }
            | STRUCT OptTag LC DefList error RC { myerror("Missing \"}\""); }
            ;

OptTag : ID                                     { $$=insert_nonterminal_node("OptTag", @$.first_line, 1, $1); }
       | /* empty */                            { $$=NULL; }
       ;

Tag : ID                                        { $$=insert_nonterminal_node("Tag", @$.first_line, 1, $1); }
    ;

/* Declarators */
// 变量定义
VarDec : ID                                     { $$=insert_nonterminal_node("VarDec", @$.first_line, 1, $1); }
       | VarDec LB INT RB                       { $$=insert_nonterminal_node("VarDec", @$.first_line, 4, $1, $2, $3, $4); }
       | VarDec LB INT error RB                 { myerror("Missing \"]\""); }   // 把[...]部分全部忽略
       ;

// 函数头定义
FunDec : ID LP VarList RP                       { $$=insert_nonterminal_node("FunDec", @$.first_line, 4, $1, $2, $3, $4); }
       | ID LP RP                               { $$=insert_nonterminal_node("FunDec", @$.first_line, 3, $1, $2, $3); }
       | ID LP VarList error RP                 { myerror("Missing \")\""); }
       | ID LP error RP                         { myerror("Missing \")\""); }
       ;

// 形参
VarList : ParamDec COMMA VarList                { $$=insert_nonterminal_node("VarList", @$.first_line, 3, $1, $2, $3); }
        | ParamDec                              { $$=insert_nonterminal_node("VarList", @$.first_line, 1, $1); }
        ;

ParamDec : Specifier VarDec                     { $$=insert_nonterminal_node("ParamDec", @$.first_line, 2, $1, $2); }
         ;

/* Statements */
// {}内的语句块
CompSt : LC DefList StmtList RC                 { $$=insert_nonterminal_node("CompSt", @$.first_line, 4, $1, $2, $3, $4); }
       /* | LC DefList StmtList error RC           { myerror("Missing \"}\""); } */
       ;

StmtList : Stmt StmtList                        { $$=insert_nonterminal_node("StmtList", @$.first_line, 2, $1, $2); }
         | /* empty */                          { $$=NULL; }
         ;

// 语句
Stmt : Exp SEMI                                 { $$=insert_nonterminal_node("Stmt", @$.first_line, 2, $1, $2); }
     | CompSt                                   { $$=insert_nonterminal_node("Stmt", @$.first_line, 1, $1); }
     | RETURN Exp SEMI                          { $$=insert_nonterminal_node("Stmt", @$.first_line, 3, $1, $2, $3); }
     | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE  { $$=insert_nonterminal_node("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); }
     | IF LP Exp RP Stmt ELSE Stmt              { $$=insert_nonterminal_node("Stmt", @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
     | WHILE LP Exp RP Stmt                     { $$=insert_nonterminal_node("Stmt", @$.first_line, 5, $1, $2, $3, $4, $5); }
     | error Stmt                               {  }
     | Exp error SEMI                           { myerror("Missing \";\""); }
     | RETURN Exp error SEMI                    { myerror("Missing \";\""); }
| IF LP Exp error RP Stmt %prec LOWER_THAN_ELSE { myerror("Missing \")\""); }
     | IF LP Exp error RP Stmt ELSE Stmt        { myerror("Missing \")\""); }
     | WHILE LP Exp error RP Stmt               { myerror("Missing \")\""); }
     ;

/* Local Definitions */
DefList : Def DefList                           { $$=insert_nonterminal_node("DefList", @$.first_line, 2, $1, $2); }
        | /* empty */                           { $$=NULL; }
        ;

Def : Specifier DecList SEMI                    { $$=insert_nonterminal_node("Def", @$.first_line, 3, $1, $2, $3); }
    | Specifier DecList error SEMI              { myerror("missing12 \";\""); }
    ;

DecList : Dec                                   { $$=insert_nonterminal_node("DecList", @$.first_line, 1, $1); }
        | Dec COMMA DecList                     { $$=insert_nonterminal_node("DecList", @$.first_line, 3, $1, $2, $3); }
        ;

Dec : VarDec                                    { $$=insert_nonterminal_node("Dec", @$.first_line, 1, $1); }
    | VarDec ASSIGNOP Exp                       { $$=insert_nonterminal_node("Dec", @$.first_line, 3, $1, $2, $3); }
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp                          { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp AND Exp                               { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp OR Exp                                { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp RELOP Exp                             { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp PLUS Exp                              { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp MINUS Exp                             { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp STAR Exp                              { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp DIV Exp                               { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | LP Exp RP                                 { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | MINUS Exp %prec UMINUS                    { $$=insert_nonterminal_node("Exp", @$.first_line, 2, $1, $2); }
    | NOT Exp                                   { $$=insert_nonterminal_node("Exp", @$.first_line, 2, $1, $2); }
    | ID LP Args RP                             { $$=insert_nonterminal_node("Exp", @$.first_line, 4, $1, $2, $3, $4); }
    | ID LP RP                                  { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | Exp LB Exp RB                             { $$=insert_nonterminal_node("Exp", @$.first_line, 4, $1, $2, $3, $4); }
    | Exp DOT ID                                { $$=insert_nonterminal_node("Exp", @$.first_line, 3, $1, $2, $3); }
    | ID                                        { $$=insert_nonterminal_node("Exp", @$.first_line, 1, $1); }
    | INT                                       { $$=insert_nonterminal_node("Exp", @$.first_line, 1, $1); }
    | FLOAT                                     { $$=insert_nonterminal_node("Exp", @$.first_line, 1, $1); }
    | Exp ASSIGNOP error                        {  }  
    | Exp AND error                             {  }    
    | Exp OR error                              {  }    
    | Exp RELOP error                           {  }    
    | Exp PLUS error                            {  }    
    | Exp MINUS error                           {  }  
    | Exp STAR error                            {  }    
    | Exp DIV error                             {  }
    | MINUS error %prec UMINUS                  {  }           
    | NOT error                                 {  } 
    | LP Exp error RP                           { myerror("Missing \")\""); }
    | ID LP Args error RP                       { myerror("Missing \")\""); }
    | ID LP error RP                            { myerror("Missing \")\""); }
    | Exp LB Exp error RB                       { myerror("Missing \"]\""); }
    ;

Args : Exp COMMA Args                           { $$=insert_nonterminal_node("Args", @$.first_line, 3, $1, $2, $3); }
     | Exp                                      { $$=insert_nonterminal_node("Args", @$.first_line, 1, $1); }
     ;

%%
void yyerror(const char *msg) { // 自动调用, 改输出为 Syntax error
    syntax_error++;
    printf("Error type B at Line %d: %s.\n", yylineno, "Syntax error");
}

void myerror(const char *msg) {
    printf("Error type B at Line %d: %s.\n", yylineno, msg);
}