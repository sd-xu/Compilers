#include "assembly_code.h"

extern struct Intercodes *inter_head;
struct Register _reg[32]; // 寄存器数组, 保存寄存器编号, 名字和状态

extern int var_cnt;
extern int temp_cnt;
extern int label_cnt;
extern int dec_cnt;
extern void printop(Operand op, FILE *fp);
extern struct Intercodes *inter_head;

int func_state = 0;
char *fun_name = NULL; // 当前函数的名字, 在main中为null
struct stack_node *stack_sp;
struct stack_node *stack_fp;
struct stack_node *stack_head;

struct pid_stack *pid_head;
struct pid_stack *pid_cur;


// 朴素 从内存取出到t0, t1, t2(8, 9, 10)然后计算然后放回去; 其余寄存器作为中间变量使用
// t4(12) 存放地址
void init_reg() {
    // 初始化
    // 寄存器的名字和状态初始化
    for (int i = 0; i < 32; ++i) {
        _reg[i].state = 0;
    }
    // 别名初始化;
    _reg[0].name = "$zero";
    _reg[1].name = "$at";
    _reg[2].name = "$v0";
    _reg[3].name = "$v1";

    _reg[4].name = "$a0";
    _reg[5].name = "$a1";
    _reg[6].name = "$a2";
    _reg[7].name = "$a3";

    _reg[8].name = "$t0";
    _reg[9].name = "$t1";
    _reg[10].name = "$t2";
    _reg[11].name = "$t3";

    _reg[12].name = "$t4";
    _reg[13].name = "$t5";
    _reg[14].name = "$t6";
    _reg[15].name = "$t7";

    _reg[16].name = "$s0";
    _reg[17].name = "$s1";
    _reg[18].name = "$s2";
    _reg[19].name = "$s3";

    _reg[20].name = "$s4";
    _reg[21].name = "$s5";
    _reg[22].name = "$s6";
    _reg[23].name = "$s7";

    _reg[24].name = "$t8";
    _reg[25].name = "$t9";

    _reg[26].name = "$k0";
    _reg[27].name = "$k1";

    _reg[28].name = "$gp";
    _reg[29].name = "$sp";

    _reg[30].name = "$fp";
    _reg[31].name = "$ra";
}

void init_data(FILE *fp) {
    // 数据段初始化;
    fprintf(fp, ".data\n");
    fprintf(fp, "_prompt: .asciiz \"Enter an integer:\"\n_ret: .asciiz \"\\n\"\n.globl main\n");
}

int check_var(Operand op) {
    return (op->kind==OP_VARIABLE || op->kind==OP_TEMPVAR);
}

int find_op(Operand cur) { // 对于 variable tempvar 在stack中寻找来确定要不要开坑; 找不到是0 找到是1
    struct stack_node *tempnode = stack_fp;
    int kind = cur->kind;
    int no = cur->no;
    int success = 0;
//     printf("kind：%d no:%d\n", kind, no);
    while (tempnode != NULL) {
        if (tempnode->no==no && tempnode->kind==kind) {
            success=1;
            break;
        }

        tempnode = tempnode->next;
    }
    return success;
}

void show_stack() {
    printf("\n------stack-blow------\n");
    struct stack_node *temp = stack_head->next;
    while (temp != NULL) {
        if (temp->kind == OP_VARIABLE) {
            printf("v%d offset:%d\n", temp->no, temp->offset);
        }
        else {
            printf("t%d offset:%d\n", temp->no, temp->offset);
        }
        temp = temp->next;
    }

    printf("-----------------------\n\n");
}

void push_pid(struct stack_node *fp) {
    struct pid_stack *temp = (struct pid_stack *)malloc(sizeof(struct pid_stack));
    temp->fp = fp;
    temp->next = NULL;
    pid_cur->next = temp;
    pid_cur = temp;
}

void pop_pid() {
    struct pid_stack *temp = pid_head;
    while (1) {
        if (temp == NULL) {
            break;
        }
        if (temp->next == pid_cur) {
            temp->next = NULL;
            free(pid_cur);
            pid_cur = temp;
            break;
        }
        temp = temp->next;
    }
}

void push_op(Operand op, int offset) {
    struct stack_node *temp = (struct stack_node *)(malloc(sizeof(struct stack_node)));
    int kind = op->kind;
    if (kind==OP_VARIABLE || kind==OP_TEMPVAR) {
        ;
    }
    else {
        printf("wrong push kind:%d no:%d value:%d\n", kind, op->no, op->value);
        printf("can only push op of tempvar or variable\n");
        assert(0);
    }
    int no = op->no;
    temp->kind = kind;
    temp->no = op->no;
    temp->offset = offset;
    temp->next = NULL;
    stack_sp->next = temp;
    stack_sp = temp;
}

void pop_op() {
    // 删除->next=fp之后的所有内容
    // 从pid_stack中取出新的fp
    struct stack_node *temp = stack_head;
    int success = 0;
    while (1) {
        if (temp == NULL) {
            break;
        }
        if (temp == stack_fp) {
            temp->next = NULL;
            // 可以选择free 等修改
            success = 1;
            break;
        }
        if (temp->next == stack_fp) {
            temp->next = NULL;
            // 可以选择free 等修改
            success = 1;
            break;
        }
        temp = temp->next;
    }
    if (success == 0) {
        printf("bug in pop_op!\n");
        assert(0);
    }
    stack_fp = stack_head;
    // stack_fp = pid_cur->fp;
    stack_sp = temp;
    // pop_pid();
}

void init_code(FILE *fp) {
    // 函数段初始化, read and write
    // read
    fprintf(fp, ".text\n");
    fprintf(fp, "_func_read:\n");
    if (!SI_DEBUG2) {
        fprintf(fp, "  li $v0, 4\n");
        fprintf(fp, "  la $a0, _prompt\n");
        fprintf(fp, "  syscall\n");
    }
    fprintf(fp, "  li $v0, 5\n");
    fprintf(fp, "  syscall\n");
    fprintf(fp, "  jr $ra\n\n");
    // write
    fprintf(fp, "_func_write:\n");
    fprintf(fp, "  li $v0, 1\n");
    fprintf(fp, "  syscall\n");
    fprintf(fp, "  li $v0, 4\n");
    fprintf(fp, "  la $a0, _ret\n");
    fprintf(fp, "  syscall\n");
    fprintf(fp, "  move $v0, $0\n");
    fprintf(fp, "  jr $ra\n");

    stack_head = (struct stack_node *)(malloc(sizeof(struct stack_node)));
    stack_head->next = NULL;
    stack_fp = stack_head;
    stack_sp = stack_head;
    stack_head->no = -1;

    pid_head = (struct pid_stack *)(malloc(sizeof(struct pid_stack)));
    pid_head->next = NULL;
    pid_cur = pid_head;

}

int find_op_offset(Operand cur) {
    struct stack_node *tempnode = stack_fp;
    int kind = cur->kind;
    int no = cur->no;
    int offset = -1;
    while (tempnode != NULL) {
        if (tempnode->no==no && tempnode->kind==kind) {
            offset = tempnode->offset;
            break;
        }
        tempnode = tempnode->next;
    }
    if (offset == -1) {
        printf("kind:%d no:%d\n", kind, no);
        printf("bug in find_op_offset\n");
        show_stack();
        assert(0);
    }
    return offset;
}

void load_reg(Operand op, int reg, FILE *fp) { // reg不使用t5, t6, t7, t8
    switch (op->kind) {
        case OP_VARIABLE: {
            int offset = find_op_offset(op);
            if (op->ifaddress == OP_ADDRESS) {
                // 赋给reg地址
                fprintf(fp, "  la %s, %d($fp)\n", _reg[reg].name, -offset);
            }
            // 赋给reg地址里面的值
            else {
                fprintf(fp, "  lw %s, %d($fp)\n", _reg[reg].name, -offset);
            }
            break;
        }
        case OP_CONSTANT: {
            fprintf(fp, "  li %s, %d\n", _reg[reg].name, op->value);
            break;
        }
        case OP_TEMPVAR: {
            int offset = find_op_offset(op);
            if (op->ifaddress == OP_ADDRESS) {
                // 赋给临时寄存器temp var的值也就是目标的地址
                // 通过临时寄存器得到值
                fprintf(fp, "  lw %s, %d($fp)\n", _reg[14].name, -offset);
                // t6;
                fprintf(fp, "  lw %s, 0(%s)\n", _reg[reg].name, _reg[14].name);
            }
            else {
                // 赋给寄存器temp_var里面的值
                fprintf(fp, "  lw %s, %d($fp)\n", _reg[reg].name, -offset);
            }
            break;
        }
        case OP_FUNCTION: {
            // 赋给reg FUNCTION的地址
            fprintf(fp, "  la %s, %s\n", _reg[reg].name, op->funcname);
            break;
        }
        case OP_LABEL: {
            fprintf(fp, "  la %s, label%d\n", _reg[reg].name, op->no);
            break;
        }
    }
}

void save_reg(Operand op, int reg, FILE *fp) { // 把reg的值存到内存
    switch (op->kind) {
        case OP_VARIABLE: {
            int offset = find_op_offset(op);
            if (op->ifaddress == OP_ADDRESS) {
                printf("error! can't save address\n");
                assert(0);
            }
            else {
                fprintf(fp, "  sw %s, %d($fp)\n", _reg[reg].name, -offset);
            }
            break;
        }
        case OP_TEMPVAR: {
            int offset = find_op_offset(op);
            if (op->ifaddress == OP_ADDRESS) {
                fprintf(fp, "  lw %s, %d($fp)\n", _reg[14].name, -offset);
                // t6存储要存的目标地址
                fprintf(fp, "  sw %s, 0(%s)\n", _reg[reg].name, _reg[14].name);
                // 往目标地址存
            }
            else {
                fprintf(fp, "  sw %s, %d($fp)\n", _reg[reg].name, -offset);
            }
            break;
        }
        default:
            printf("Impossible to get here in save_reg\n");
            assert(0);
    }
}

void print_relop(char *relop, FILE *fp) {
    if (strcmp(relop, "==") == 0) {
        fprintf(fp, "beq ");
    }
    else if (strcmp(relop, "!=") == 0) {
        fprintf(fp, "bne ");
    }
    else if (strcmp(relop, ">") == 0) {
        fprintf(fp, "bgt ");
    }
    else if (strcmp(relop, "<") == 0) {
        fprintf(fp, "blt ");
    }
    else if (strcmp(relop, ">=") == 0) {
        fprintf(fp, "bge ");
    }
    else if (strcmp(relop, "<=") == 0) {
        fprintf(fp, "ble ");
    }
}

void trans_one_code(FILE *fp, struct Intercodes *cur) {
    int kind = cur->code.kind;
    switch (cur->code.kind) {
        case IN_FUNCTION: {
            if (SI_DEBUG2) {
                fprintf(fp, "#FUNCTION ");
                printop(cur->code.u.one.op0, fp);
                fprintf(fp, " : \n");
            }

            pop_op(); // 更新缓存stack
            // 处理返回地址
            fun_name = cur->code.u.one.op0->funcname;
            
            int offset = 0;
            // 对main特判;
            if (strcmp(fun_name, "main") == 0) {
                fprintf(fp, "\n%s:\n", fun_name);
                func_state = 0;
                fun_name = NULL;
                struct Intercodes *temp = cur;
                int cnt = 0;
                offset += 8; // 返回地址(n-4)(sp);fp(n-8)(sp)
                // 往自己的pid栈里面压入当前的fp
            //     push_pid(stack_fp);
            //     stack_fp=stack_sp; // 更新

                while (temp != inter_head) {
                    switch (temp->code.kind) {
                        case IN_DEC: {
                            Operand tempop1 = temp->code.u.two.left;
                            int find_result = find_op(tempop1);
                            if (find_result == 0) {
                                offset += temp->code.u.two.right->value;
                                push_op(tempop1, offset);
                                
                            }
                            break;
                        }
                        case IN_ARG: {
                            Operand tempop1 = temp->code.u.one.op0;
                            if (check_var(tempop1)) {
                                int find_result = find_op(tempop1);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop1, offset);
                                }
                            }
                            break;
                        }
                        case IN_ADD:
                        case IN_SUB:
                        case IN_MUL:
                        case IN_DIV: {
                            Operand tempop1 = temp->code.u.three.op1;
                            if (check_var(tempop1)) {
                            //     printf("1kind:%d\n", tempop1->kind);
                                int find_result = find_op(tempop1);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop1, offset);
                                    
                                }
                            }
                            Operand tempop2 = temp->code.u.three.op2;
                            if (check_var(tempop2)) {
                                // printf("2kind:%d\n", tempop2->kind);
                                int find_result = find_op(tempop2);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop2, offset);
                                    
                                }
                            }
                            Operand tempop3 = temp->code.u.three.result;
                            int find_result = find_op(tempop3);
                            // printf("3kind:%d\n", tempop3->kind);
                            if (find_result == 0) {
                                offset += 4;
                                push_op(tempop3, offset);
                                
                            }
                            break;
                        }
                        case IN_READ: {
                            Operand tempop1 = temp->code.u.one.op0;
                            int find_result = find_op(tempop1);
                            if (find_result == 0) {
                                offset += 4;
                                push_op(tempop1, offset);
                                
                            }
                            break;
                        }
                        case IN_ASSIGN: {
                            Operand tempop1 = temp->code.u.two.left;
                            if (check_var(tempop1)) {
                                int find_result = find_op(tempop1);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop1, offset);    
                                }
                            }
                            Operand tempop2 = temp->code.u.two.right;
                            if (check_var(tempop2)) {
                                int find_result = find_op(tempop2);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop2, offset);    
                                }
                            }
                            break;
                        }
                        case IN_CALL: {
                            Operand tempop1 = temp->code.u.two.left;
                            int find_result = find_op(tempop1);
                            if (find_result == 0) {
                                offset += 4;
                                push_op(tempop1, offset);    
                            }
                            break;
                        }
                    }
                    temp = temp->next;
                }
            //     printf("offset:%d\n", offset);
                // 包括 ra fp存储 fp 更替
                // 开辟sp:
                fprintf(fp, "  addi $sp, $sp, -%d\n", offset);
                // 更新地址位置:
                fprintf(fp, "  sw $ra, %d($sp)\n", offset-4);
                fprintf(fp, "  sw $fp, %d($sp)\n", offset-8);
                fprintf(fp, "  addi $fp, $sp, %d\n", offset);

                if (SI_DEBUG)   show_stack();
            }
            else {
                fprintf(fp, "\n_func_%s:\n", fun_name);
                func_state = 1;
                // 先处理PARAM然后处理局部变量
                // vn4*n(sp)
                // vn-1
                // ....
                // v0 sp
            //     push_pid(stack_fp);
            //     stack_fp=stack_sp; // 更新fp

                int param_cnt = 0;
                struct Intercodes *temp = cur->next;
                while (1) {
                    if (temp->code.kind == IN_PARAM) {
                        param_cnt += 1;
                    }
                    else {
                        break;
                    }
                    temp = temp->next;
                }
                // printf("param_cnt:%d\n", param_cnt);
                temp = cur->next;
                offset = 4;
                while (1) {
                    if (temp->code.kind == IN_PARAM) {
                        // param_cnt += 1;
                        offset -= 4;
                        Operand tempop1 = temp->code.u.one.op0;
                        push_op(tempop1, offset);
                    }
                    else {
                        break;
                    }
                    
                    temp = temp->next;
                }
                offset += (param_cnt*4)-4;
                if (offset != 0) {
                    printf("offset now:%d\n", offset);
                    assert(0);
                }
                // 处理局部变量, 和main相似
            //     temp=cur->next;
                offset += 8;
                while (1) {
                    if (temp==inter_head || temp->code.kind==IN_FUNCTION) {
                        break;
                    }
                    switch (temp->code.kind) {
                        case IN_DEC: {
                            Operand tempop1 = temp->code.u.two.left;
                            int find_result = find_op(tempop1);
                            if (find_result == 0) {
                                offset += temp->code.u.two.right->value;
                                push_op(tempop1, offset);
                            }
                            break;
                        }
                        case IN_ARG: {
                            Operand tempop1 = temp->code.u.one.op0;
                            if (check_var(tempop1)) {
                                int find_result = find_op(tempop1);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop1, offset);
                                }
                            }
                            break;
                        }
                        case IN_ADD:
                        case IN_SUB:
                        case IN_MUL:
                        case IN_DIV: {
                            Operand tempop1 = temp->code.u.three.op1;
                            if (check_var(tempop1)) {
                                // printf("1kind:%d\n", tempop1->kind);
                                int find_result = find_op(tempop1);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop1, offset);
                                    
                                }
                            }
                            Operand tempop2 = temp->code.u.three.op2;
                            if (check_var(tempop2)) {
                                // printf("2kind:%d\n", tempop2->kind);
                                int find_result = find_op(tempop2);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop2, offset);
                                    
                                }
                            }
                            Operand tempop3 = temp->code.u.three.result;
                            int find_result = find_op(tempop3);
                            // printf("3kind:%d\n", tempop3->kind);
                            if (find_result == 0) {
                                offset += 4;
                                push_op(tempop3, offset);
                                
                            }
                            break;
                        }
                        case IN_READ: {
                            Operand tempop1 = temp->code.u.one.op0;
                            int find_result = find_op(tempop1);
                            if (find_result == 0) {
                                offset += 4;
                                push_op(tempop1, offset);
                                
                            }
                            break;
                        }
                        case IN_ASSIGN: {
                            Operand tempop1 = temp->code.u.two.left;
                            if (check_var(tempop1)) {
                                int find_result = find_op(tempop1);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop1, offset);    
                                }
                            }
                            Operand tempop2 = temp->code.u.two.right;
                            if (check_var(tempop2)) {
                                int find_result = find_op(tempop2);
                                if (find_result == 0) {
                                    offset += 4;
                                    push_op(tempop2, offset);    
                                }
                            }
                            break;
                        }
                        case IN_CALL: {
                            Operand tempop1 = temp->code.u.two.left;
                            int find_result =find_op(tempop1);
                            if (find_result == 0) {
                                offset += 4;
                                push_op(tempop1, offset);    
                            }
                            break;
                        }
                        
                    }
                    temp = temp->next;
                }
            //     printf("offset2:%d\n", offset);
                // 包括 ra fp存储 fp 更替
                // 开辟sp:
                fprintf(fp, "  addi $sp, $sp, -%d\n", offset);
                // 更新地址位置:
                fprintf(fp, "  sw $ra, %d($sp)\n", offset-4);
                fprintf(fp, "  sw $fp, %d($sp)\n", offset-8);
                fprintf(fp, "  addi $fp, $sp, %d\n", offset);

                if (SI_DEBUG)    show_stack();
            }
            break;
        }
        case IN_PARAM: {
            // 在函数里面处理好
            if (SI_DEBUG2) {
                fprintf(fp, "#PARAM ");
                printop(cur->code.u.one.op0, fp);
                fprintf(fp, "\n");
            }
            break;
        }
        case IN_RETURN: {
            // stack 需要出栈(现在放在FUNCTION开头)
            // 要返回的值存到v0里面
            if (SI_DEBUG2) {
                fprintf(fp, "#RETURN ");
                printop(cur->code.u.one.op0, fp);
                fprintf(fp, "\n");
            }
            if (cur->code.u.one.op0->kind == OP_CONSTANT) {
                // printf("ererer\n\n");
                int value = cur->code.u.one.op0->value;
                int temp_reg = 8;
                load_reg(cur->code.u.one.op0, temp_reg, fp);
                fprintf(fp, "  move $v0, %s\n", _reg[temp_reg].name);
            }
            else {
                int return_reg = 8; // t0
                load_reg(cur->code.u.one.op0, return_reg, fp);

                fprintf(fp, "  move $v0, %s\n", _reg[return_reg].name);
            }

            // 结果已经保存好了, 现在开始修改返回地址和fp
            fprintf(fp, "  move $sp, $fp\n");
            fprintf(fp, "  lw $ra, -4($fp)\n");
            fprintf(fp, "  lw $fp, -8($fp)\n");
            fprintf(fp, "  jr $ra\n");
            
            break;
        }
        case IN_LABEL: {
            if (SI_DEBUG2) {
                fprintf(fp, "#LABEL label");
                printop(cur->code.u.one.op0, fp);
                fprintf(fp, " : \n");
            }
            fprintf(fp, "label%d:\n", cur->code.u.one.op0->no);
            break;
        }
        case IN_GOTO: {
            if (SI_DEBUG2) {
                fprintf(fp, "#GOTO label");
                printop(cur->code.u.one.op0, fp);
                fprintf(fp, "\n");
            }
            fprintf(fp, "  j label%d\n", cur->code.u.one.op0->no);
            break;
        }
        case IN_WRITE: {
            // a0里面存
            if (SI_DEBUG2) {
                fprintf(fp, "#WRITE ");
                printop(cur->code.u.one.op0, fp);
                fprintf(fp, "\n");
            }
            int temp_reg = 8;
            int write_reg = 4;
            load_reg(cur->code.u.one.op0, temp_reg, fp);
            fprintf(fp, "  move %s, %s\n", _reg[write_reg].name, _reg[temp_reg].name);
            fprintf(fp, "  jal _func_write\n");
            break;
        }
        case IN_READ: {
            if (SI_DEBUG2) {
                fprintf(fp, "#READ ");
                printop(cur->code.u.one.op0, fp);
                fprintf(fp, "\n");
            }
            int temp_reg = 8;
            int read_reg = 2;
            fprintf(fp, "  jal _func_read\n");
            fprintf(fp, "  move %s, %s\n", _reg[temp_reg].name, _reg[read_reg].name);
            save_reg(cur->code.u.one.op0, temp_reg, fp);
            break;
        }
        case IN_ARG: {
            // 往栈里面压
            // SP增长
            // stack_node不需要额外增加(到PARAM的地方再更新)
            if (SI_DEBUG2) {
                fprintf(fp, "#ARG ");
                printop(cur->code.u.one.op0, fp); 
                fprintf(fp, " \n");
            }
            fprintf(fp, "  addi $sp,$sp,-4\n");
            int arg_reg = 16; // s0
            load_reg(cur->code.u.one.op0, arg_reg, fp);
            fprintf(fp, "  sw %s, 0($sp)\n", _reg[arg_reg].name);
        
            break;
        }
        case IN_ASSIGN: {
            if (SI_DEBUG2) {
                fprintf(fp, "#");
                printop(cur->code.u.two.left, fp);
                fprintf(fp, " := ");
                printop(cur->code.u.two.right, fp);
                fprintf(fp, "\n");
            }
            int left_reg = 8;
            int right_reg = 9;
            load_reg(cur->code.u.two.left, left_reg, fp);
            load_reg(cur->code.u.two.right, right_reg, fp);
            fprintf(fp, "  move %s, %s\n", _reg[left_reg].name, _reg[right_reg].name);
            save_reg(cur->code.u.two.left, left_reg, fp);
            break;
        }
        case IN_DEC: {
            // Done in init
            if (SI_DEBUG2) {
                fprintf(fp, "#DEC ");
                printop(cur->code.u.two.left, fp);
                fprintf(fp, " %d", cur->code.u.two.right->value);
                fprintf(fp, "\n");
            }    
            break;
        }
        case IN_CALL: {
            if (SI_DEBUG2) {
                    fprintf(fp, "#");
                    printop(cur->code.u.two.left, fp);
                    fprintf(fp, " := CALL ");
                    printop(cur->code.u.two.right, fp);
                    fprintf(fp, "\n");
            }
            if (strcmp(cur->code.u.two.right->funcname, "main") != 0)
                fprintf(fp, "  jal _func_%s\n", cur->code.u.two.right->funcname);
            else {
                fprintf(fp, "  jal %s\n", cur->code.u.two.right->funcname);
            }
            // 从v0里面取;    存到对应的内存
            int return_reg = 2;
            save_reg(cur->code.u.two.left, return_reg, fp);
            break;
        }
        case IN_ADD: {
            if (SI_DEBUG2) {
                fprintf(fp, "#");
                printop(cur->code.u.three.result, fp);
                fprintf(fp, " := ");
                printop(cur->code.u.three.op1, fp);
                fprintf(fp, " + ");
                printop(cur->code.u.three.op2, fp);
                fprintf(fp, "\n");
            }
            int result_reg = 8; // t0
            int left_reg = 9;   // t1
            int right_reg = 10; // t2
            // load_reg(cur->code.u.three.result, result_reg, fp);
            load_reg(cur->code.u.three.op1, left_reg, fp);
            load_reg(cur->code.u.three.op2, right_reg, fp);
            fprintf(fp, "  add %s, %s, %s\n", _reg[result_reg].name, _reg[left_reg].name, _reg[right_reg].name);
            save_reg(cur->code.u.three.result, result_reg, fp);
            break;
        }
        case IN_SUB: {
            if (SI_DEBUG2) {
                fprintf(fp, "#");
                printop(cur->code.u.three.result, fp);
                fprintf(fp, " := ");
                printop(cur->code.u.three.op1, fp);
                fprintf(fp, " - ");
                printop(cur->code.u.three.op2, fp);
                fprintf(fp, "\n");
            }
            int result_reg = 8; // t0
            int left_reg = 9;   // t1
            int right_reg = 10; // t2
            load_reg(cur->code.u.three.op1, left_reg, fp);
            load_reg(cur->code.u.three.op2, right_reg, fp);
            fprintf(fp, "  sub %s, %s, %s\n", _reg[result_reg].name, _reg[left_reg].name, _reg[right_reg].name);
            save_reg(cur->code.u.three.result, result_reg, fp);
            break;
        }
        case IN_MUL: {
            if (SI_DEBUG2) {
                fprintf(fp, "#");
                printop(cur->code.u.three.result, fp);
                fprintf(fp, " := ");
                printop(cur->code.u.three.op1, fp);
                fprintf(fp, " * ");
                printop(cur->code.u.three.op2, fp);
                fprintf(fp, "\n");
            }
            int result_reg = 8; // t0
            int left_reg = 9;   // t1
            int right_reg = 10; // t2
            load_reg(cur->code.u.three.op1, left_reg, fp);
            load_reg(cur->code.u.three.op2, right_reg, fp);
            fprintf(fp, "  mul %s, %s, %s\n", _reg[result_reg].name, _reg[left_reg].name, _reg[right_reg].name);
            save_reg(cur->code.u.three.result, result_reg, fp);
            break;
        }
        case IN_DIV: {
            if (SI_DEBUG2) {
                fprintf(fp, "#");
                printop(cur->code.u.three.result, fp);
                fprintf(fp, " := ");
                printop(cur->code.u.three.op1, fp);
                fprintf(fp, " / ");
                printop(cur->code.u.three.op2, fp);
                fprintf(fp, "\n");
            }
            int result_reg = 8; // t0
            int left_reg = 9;   // t1
            int right_reg = 10; // t2
            load_reg(cur->code.u.three.op1, left_reg, fp);
            load_reg(cur->code.u.three.op2, right_reg, fp);
            fprintf(fp, "  div %s, %s\n", _reg[left_reg].name, _reg[right_reg].name);
            fprintf(fp, "  mflo %s\n", _reg[result_reg].name);
            save_reg(cur->code.u.three.result, result_reg, fp);
            break;
        }
        case IN_IFGOTO: {
            if (SI_DEBUG2) {
                fprintf(fp, "#IF ");
                printop(cur->code.u.four.op1, fp);
                fprintf(fp, " %s ", cur->code.u.four.relop);
                printop(cur->code.u.four.op2, fp);
                fprintf(fp, " GOTO label");
                printop(cur->code.u.four.op3, fp);
                fprintf(fp, "\n");
            }
            int op1_reg = 8;
            int op2_reg = 9;
            
            load_reg(cur->code.u.four.op1, op1_reg, fp);
            load_reg(cur->code.u.four.op2, op2_reg, fp);
            fprintf(fp, "  ");
            print_relop(cur->code.u.four.relop, fp);
            // beq sdsdsd
            fprintf(fp, "%s, %s, label%d\n", _reg[op1_reg].name, _reg[op2_reg].name, cur->code.u.four.op3->no); 
            break;
        }
    }
}

void trans_code(FILE *fp) {
    struct Intercodes *temp = inter_head->next;
    while (temp != inter_head) {
        trans_one_code(fp, temp);
        temp = temp->next;
    }
}

void a_code_generate(FILE *fp) {
    init_reg();
    init_data(fp);
    init_code(fp);
    trans_code(fp);
}