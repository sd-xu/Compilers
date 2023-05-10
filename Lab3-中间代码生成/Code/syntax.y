%{
    #include <stdio.h>
    #include <stdarg.h>
    #include "lex.yy.c"
    extern int yylineno;
    int emptyflag = 0;
    int emptystart = 0;
    int syntaxError = 0;
    /* Declarations */
    void yyerror(char *msg);
    struct Node *add_bisonnode(char* Name, int column);
    void add_parentnode(struct Node *parent, int num_args, ...);
    void tree_search(struct Node *cur, int depth);
    extern struct Node *root;
%}

/*declared types*/
%union {
    struct Node *node;
}

%locations

%token <node> INT
%token <node> FLOAT
%token <node> ID
%token <node> SEMI
%token <node> COMMA
%token <node> ASSIGNOP
%token <node> RELOP
%token <node> PLUS
%token <node> MINUS
%token <node> STAR
%token <node> DIV
%token <node> AND
%token <node> OR
%token <node> DOT
%token <node> NOT
%token <node> TYPE
%token <node> LP
%token <node> RP
%token <node> LB
%token <node> RB
%token <node> LC
%token <node> RC
%token <node> STRUCT
%token <node> RETURN 
%token <node> IF
%token <node> ELSE
%token <node> WHILE
/*declared    nonterminal symbol    from other 7*/

// 7.1.2
%type <node> Program

%type <node> ExtDecList
%type <node> ExtDef
%type <node> ExtDefList
// 7.1.3
%type <node> Specifier
%type <node> StructSpecifier
%type <node> OptTag
%type <node> Tag
// 7.1.4
%type <node> VarDec
%type <node> FunDec
%type <node> VarList
%type <node> ParamDec
// 7.1.5
%type <node> CompSt
%type <node> StmtList
%type <node> Stmt
// 7.1.6
%type <node> DefList
%type <node> Def
%type <node> DecList
%type <node> Dec
// 7.1.7
%type <node> Exp
%type <node> Args

%start Program
%right ASSIGNOP
%left OR
%left AND 
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT 
%left UMINUS // 负号
%left DOT 
%left LB RB 
%left LP RP
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%
// 7.1.2 
/* 初始语法单元Program */
Program : ExtDefList                            { $$ = add_bisonnode("Program", @$.first_line); add_parentnode($$, 1, $1); root = $$; }
        ;

ExtDefList : ExtDef ExtDefList                  { $$ = add_bisonnode("ExtDefList", @$.first_line); add_parentnode($$, 2, $1, $2); }
           | /* Empty */                        { $$ = NULL; emptystart = yylineno; }
           ;

ExtDef : Specifier ExtDecList SEMI              { $$ = add_bisonnode("ExtDef", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
       | Specifier SEMI                         { $$ = add_bisonnode("ExtDef", @$.first_line); add_parentnode($$, 2, $1, $2); }
       | Specifier FunDec CompSt                { $$ = add_bisonnode("ExtDef", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
       | Specifier FunDec SEMI                  { $$ = add_bisonnode("ExtDef", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
       | error SEMI                             { syntaxError += 1; }
       | Specifier error SEMI                   { syntaxError += 1; }
       | error Specifier SEMI                   { syntaxError += 1; }
       ;

ExtDecList : VarDec                             { $$ = add_bisonnode("ExtDecList", @$.first_line); add_parentnode($$, 1, $1); }
           | VarDec COMMA ExtDecList            { $$ = add_bisonnode("ExtDecList", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
           | VarDec error ExtDefList            { syntaxError += 1; }

// 7.1.3
Specifier : TYPE                                { $$ = add_bisonnode("Specifier", @$.first_line); add_parentnode($$, 1, $1); }
          | StructSpecifier                     { $$ = add_bisonnode("Specifier", @$.first_line); add_parentnode($$, 1, $1); }
          ;

StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = add_bisonnode("StructSpecifier", @$.first_line); add_parentnode($$, 5, $1, $2, $3, $4, $5); }
                | STRUCT Tag                    { $$ = add_bisonnode("StructSpecifier", @$.first_line); add_parentnode($$, 2, $1, $2); }
                ;

OptTag : ID                                     { $$ = add_bisonnode("OptTag", @$.first_line); add_parentnode($$, 1, $1); }
       | /* Empty */                            { $$ = NULL; }
       ;

Tag : ID                                        { $$ = add_bisonnode("Tag", @$.first_line); add_parentnode($$, 1, $1); }
    ;

// 7.1.4
VarDec : ID                                     { $$ = add_bisonnode("VarDec", @$.first_line); add_parentnode($$, 1, $1); }
       | VarDec LB INT RB                       { $$ = add_bisonnode("VarDec", @$.first_line); add_parentnode($$, 4, $1, $2, $3, $4); }
       | VarDec LB error RB                     { syntaxError += 1; }
       ;

FunDec : ID LP VarList RP                       { $$ = add_bisonnode("FunDec", @$.first_line); add_parentnode($$, 4, $1, $2, $3, $4); }
       | ID LP RP                               { $$ = add_bisonnode("FunDec", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
       | ID LP error RP                         { syntaxError += 1; }
       | error LP VarList RP                    { syntaxError += 1; }
       ;

VarList : ParamDec COMMA VarList                { $$ = add_bisonnode("VarList", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
        | ParamDec                              { $$ = add_bisonnode("VarList", @$.first_line); add_parentnode($$, 1, $1); }
        ;

ParamDec : Specifier VarDec                     { $$ = add_bisonnode("ParamDec", @$.first_line); add_parentnode($$, 2, $1, $2); }
         ;

// 7.1.5
CompSt : LC DefList StmtList RC                 { $$ = add_bisonnode("CompSt", @$.first_line); add_parentnode($$, 4, $1, $2, $3, $4); }
       ;

StmtList : Stmt StmtList                        { $$ = add_bisonnode("StmtList", @$.first_line); add_parentnode($$, 2, $1, $2); }
         | /* Empty */                          { $$ = NULL; }
         ;

Stmt : Exp SEMI                                 { $$ = add_bisonnode("Stmt", @$.first_line); add_parentnode($$, 2, $1, $2); }           
     | CompSt                                   { $$ = add_bisonnode("Stmt", @$.first_line); add_parentnode($$, 1, $1); } 
     | RETURN Exp SEMI                          { $$ = add_bisonnode("Stmt", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
     | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE  { $$ = add_bisonnode("Stmt", @$.first_line); add_parentnode($$, 5, $1, $2, $3, $4, $5); }
     | IF LP Exp RP Stmt ELSE Stmt              { $$ = add_bisonnode("Stmt", @$.first_line); add_parentnode($$, 7, $1, $2, $3, $4, $5, $6, $7); }
     | WHILE LP Exp RP Stmt                     { $$ = add_bisonnode("Stmt", @$.first_line); add_parentnode($$, 5, $1, $2, $3, $4, $5); }
     | error SEMI                               { syntaxError += 1; }
     | Exp error SEMI                           { syntaxError += 1; }
     | RETURN Exp error                         { syntaxError += 1; }
     | RETURN error SEMI                        { syntaxError += 1; }
     ;

// 7.1.6
DefList : Def DefList                           { $$ = add_bisonnode("DefList", @$.first_line); add_parentnode($$, 2, $1, $2); }
        | /* Empty */                           { $$ = NULL; }
        ;

Def : Specifier DecList SEMI                    { $$ = add_bisonnode("Def", @$.first_line); add_parentnode($$, 3, $1, $2, $3);}
    | Specifier error SEMI                      { syntaxError += 1; }
    | Specifier DecList error                   { syntaxError += 1; }
    ;

DecList : Dec                                   { $$ = add_bisonnode("DecList", @$.first_line); add_parentnode($$, 1, $1); }
        | Dec COMMA DecList                     { $$ = add_bisonnode("DecList", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
        ;

Dec : VarDec                                    { $$ = add_bisonnode("Dec", @$.first_line); add_parentnode($$, 1, $1); }
    | VarDec ASSIGNOP Exp                       { $$ = add_bisonnode("Dec", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    ;

// 7.1.7
Exp : Exp ASSIGNOP Exp                          { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | Exp AND Exp                               { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | Exp OR Exp                                { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | Exp RELOP Exp                             { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | Exp PLUS Exp                              { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | Exp MINUS Exp                             { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | Exp STAR Exp                              { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | Exp DIV Exp                               { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | LP Exp RP                                 { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | MINUS Exp %prec UMINUS                    { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 2, $1, $2); }
    | NOT Exp                                   { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 2, $1, $2); }
    | ID LP Args RP                             { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 4, $1, $2, $3, $4); }
    | ID LP RP                                  { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); };
    | Exp LB Exp RB                             { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 4, $1, $2, $3, $4); }
    | Exp DOT ID                                { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
    | ID                                        { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 1, $1); }
    | INT                                       { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 1, $1); }
    | FLOAT                                     { $$ = add_bisonnode("Exp", @$.first_line); add_parentnode($$, 1, $1); }
    | Exp ASSIGNOP error                        { syntaxError += 1; }
    | Exp AND error                             { syntaxError += 1; }
    | Exp OR error                              { syntaxError += 1; }
    | Exp RELOP error                           { syntaxError += 1; }
    | Exp PLUS error                            { syntaxError += 1; }
    | Exp MINUS error                           { syntaxError += 1; }
    | Exp STAR error                            { syntaxError += 1; }
    | Exp DIV error                             { syntaxError += 1; }
    | LP error RP                               { syntaxError += 1; }
    | MINUS error                               { syntaxError += 1; }
    | NOT error                                 { syntaxError += 1; }
    | ID LP error RP                            { syntaxError += 1; }
    | Exp LB error RB                           { syntaxError += 1; }
    ;
                
Args : Exp COMMA Args                           { $$ = add_bisonnode("Args", @$.first_line); add_parentnode($$, 3, $1, $2, $3); }
     | Exp                                      { $$ = add_bisonnode("Args", @$.first_line); add_parentnode($$, 1, $1); }
     ;

%%
void yyerror(char *msg)  {
    syntaxError += 1;
    printf("Error type B at Line %d  :  %s.\n", yylineno, msg);
}

struct Node *add_bisonnode(char* Name, int column) {
    struct Node *Root = (struct Node *)malloc(sizeof(struct Node));
    Root->child = NULL;
    Root->next_sib = NULL;
    strcpy(Root->name, Name);
    Root->place = 1;
    Root->type = OTHERS;
    Root->column = column;

    if (strcmp(Name, "Program") == 0 && emptyflag == 0) {
        Root->column = emptystart;
    }
    return Root;
}

void add_parentnode(struct Node *parent, int num_args, ...) {
    va_list able;
    va_start(able, num_args);

    struct Node *temp = NULL;
    temp = va_arg(able, struct Node*);
    parent->child = temp;
    for(int i = 1; i < num_args; i++) {
        temp->next_sib = va_arg(able, struct Node*);
        if (temp->next_sib !=  NULL) {
            temp = temp->next_sib;
        }
    }
}