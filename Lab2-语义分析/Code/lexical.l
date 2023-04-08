%{
    /* 要声明的部分 */
    #include "syntax.tab.h"
    #include "syntax_tree.h"

    int lexical_error = 0;
    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;

    void lexical_error_handler(char *msg);
    void insert_token_node(const char* type, enum TOKEN_TYPE token_type, const char* value);
%}

/* 读取yylineno的值 */
%option yylineno 

/* regular definitions */
delim           [ \t\n]
ws              {delim}+
digit           [0-9]
letter          [a-zA-Z]
decimal         [1-9]{digit}*|0
octonary        0[0-7]+
ill_oct         0[0-7]*[89]+[0-7]*
hexadecimal     0[xX][0-9a-fA-F]+
ill_hex         0[xX][0-9a-fA-F]*[g-zG-Z]+[0-9a-fA-F]*
float_fp        {digit}+\.{digit}+
base            ({digit}*\.{digit}+|{digit}+\.)
float_exp       {base}[eE][+-]?{digit}+
float           {float_fp}|{float_exp}
ill_float       {base}[eE]
id              [a-zA-Z_]({digit}|{letter}|_)*

%%
"/*"            {
                    char c;
                    int line = yylineno; // 记录当前行号
                    int meet = 0, flag = 0;
                    while ((c=input()) != '\0') {
                        if (c == EOF)   break;
                        else if (c == '*')  meet = 1;
                        if (meet) {
                            if (c == '/') {
                                flag = 1;   break;
                            }
                            else if (c != '*')  meet = 0;
                        }
                    }
                    if (!flag) {
                        lexical_error++;
                        printf("Error type A at Line %d: Unmatched \"/*\".\n", line);
                    } 
                }
"//"            { char c;   while ((c = input()) != '\n'); }
\n              { yycolumn = 1; }
<<EOF>>         { return 0; }
{ws}            {  }
"int"|"float"   { insert_token_node("TYPE", TYPE_TYPE, yytext);   return (TYPE);    }
"struct"        { insert_token_node("STRUCT", OTHER, NULL);       return (STRUCT);  }
"return"        { insert_token_node("RETURN", OTHER, NULL);       return (RETURN);  }
"if"            { insert_token_node("IF", OTHER, NULL);           return (IF);      }
"else"          { insert_token_node("ELSE", OTHER, NULL);         return (ELSE);    }
"while"         { insert_token_node("WHILE", OTHER, NULL);        return (WHILE);   }
{decimal}       { insert_token_node("INT", TYPE_INT, yytext);     return (INT);     }
{octonary}      { insert_token_node("INT", TYPE_OCT, yytext);     return (INT);     }
{hexadecimal}   { insert_token_node("INT", TYPE_HEX, yytext);     return (INT);     }
{ill_oct}       { lexical_error_handler("Illegal octal number");                    }
{ill_hex}       { lexical_error_handler("Illegal hexadecimal number");              }
{float}         { insert_token_node("FLOAT", TYPE_FLOAT, yytext); return (FLOAT);   }
{ill_float}     { lexical_error_handler("Illegal floating point number");           }
{id}            { insert_token_node("ID", TYPE_ID, yytext);       return (ID);      }
";"             { insert_token_node("SEMI", OTHER, NULL);         return(SEMI);     }
","             { insert_token_node("COMMA", OTHER, NULL);        return(COMMA);    }
"="             { insert_token_node("ASSIGNOP", OTHER, NULL);     return(ASSIGNOP); }
">"|"<"|">="|"<="|"=="|"!="   { insert_token_node("RELOP", OTHER, NULL); return (RELOP); }
"+"             { insert_token_node("PLUS", OTHER, NULL);  return (PLUS);  }
"-"             { insert_token_node("MINUS", OTHER, NULL); return (MINUS); }
"*"             { insert_token_node("STAR", OTHER, NULL);  return (STAR);  }
"/"             { insert_token_node("DIV", OTHER, NULL);   return (DIV);   }
"&&"            { insert_token_node("AND", OTHER, NULL);   return (AND);   }
"||"            { insert_token_node("OR", OTHER, NULL);    return (OR);    }
"."             { insert_token_node("DOT", OTHER, NULL);   return (DOT);   }
"!"             { insert_token_node("NOT", OTHER, NULL);   return (NOT);   }
"("             { insert_token_node("LP", OTHER, NULL);    return (LP);    }
")"             { insert_token_node("RP", OTHER, NULL);    return (RP);    }
"["             { insert_token_node("LB", OTHER, NULL);    return (LB);    }
"]"             { insert_token_node("RB", OTHER, NULL);    return (RB);    }
"{"             { insert_token_node("LC", OTHER, NULL);    return (LC);    }
"}"             { insert_token_node("RC", OTHER, NULL);    return (RC);    }
.               { lexical_error_handler("Mysterious characters");          }

%%
void lexical_error_handler(char *msg) {
    lexical_error++;
    printf("Error type A at Line %d: %s \"%s\".\n", yylineno, msg, yytext);
}

void insert_token_node(const char *type, enum TOKEN_TYPE token_type, const char *value) {
    yylval.node = insert_terminal_node(type, token_type, value);
    yylval.node->lineno = yylineno;
}

/*
int main(int argc, char **argv) {
    if (argc > 1) {
        if (!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
    }
    while (yylex() != 0);
    return 0;
}

int yywrap(){
    return 1;
}
*/