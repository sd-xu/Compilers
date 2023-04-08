#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syntax_tree.h"

void preorder_print(Node node, int depth) {
    if (node == NULL)   return;
    for (int i = 0; i < depth; i++)
        printf("  "); // 两个空格缩进

    printf("%s", node->name);
    if (node->is_terminal) {
        switch (node->token_type) {
            case TYPE_ID:
            case TYPE_TYPE:
                printf(": %s", node->value.val_ID); break;
            case TYPE_FLOAT:
                printf(": %.6f", node->value.val_float); break;
            case TYPE_INT:
                printf(": %u", node->value.val_int); break;
            default:    
                break;
        }
    }
    else {
        printf(" (%d)", node->lineno);
    }
    printf("\n");

    preorder_print(node->first_child, depth+1);
    preorder_print(node->sibling, depth);
}

Node insert_terminal_node(const char* type, enum TOKEN_TYPE token_type, const char* value) {
    Node node = malloc(sizeof(struct Node_));
    node->is_terminal = 1;
    sscanf(type, "%s", node->name);
    node->first_child = node->sibling = NULL;
    node->token_type = token_type;
    
    switch (node->token_type) {
        case TYPE_ID:
        case TYPE_TYPE:
            sscanf(value, "%s", node->value.val_ID);        break;
        case TYPE_FLOAT:
            sscanf(value, "%f", &node->value.val_float);    break;
        case TYPE_INT:
            sscanf(value, "%u", &node->value.val_int);      break;
        case TYPE_OCT:
            sscanf(value, "%o", &node->value.val_int);
            node->token_type = TYPE_INT;
            break;
        case TYPE_HEX:
            sscanf(value, "%x", &node->value.val_int);
            node->token_type = TYPE_INT;
            break;
        default:    
            break;
    }

    return node;
}

Node insert_nonterminal_node(const char* type, int lineno, int num, ...) {
    Node node = malloc(sizeof(struct Node_));
    node->is_terminal = 0;
    sscanf(type, "%s", node->name);
    node->lineno = lineno;
    node->first_child = node->sibling = NULL;

    va_list valist;
    va_start(valist, num);
    set_par_sib(node, num, valist);
    va_end(valist);

    return node;
}

void set_par_sib(Node parent, int num, va_list valist) {
    int i = 0;
    Node node;
    for(; i < num; i++) { // 找第一个非空节点作为first_child
        node = va_arg(valist, Node);
        if (node != NULL)   
            break;
    }
    i++;
    parent->first_child = node;
    for (; i < num; i++) { // 其余作为子节点的兄弟节点
        node->sibling = va_arg(valist, Node);
        if (node->sibling != NULL)
            node = node->sibling;
    }
}

Node get_child(Node curr, int depth) {
    Node tmp = curr->first_child;
    if (depth == 0)
        return tmp;
    for (int i = 1; i <= depth; i++)
        tmp = tmp->sibling;
    return tmp;
}