# 测试方法

## lab1、2

- **命令格式**：`·/parser test.cmm`
  1. 检测并输出词法错误和语法错误
  2. 没有则检测并输出语义错误
  3. 都没有则打印语法树



## lab3、4

- 这里不再打印语法树

**支持的命令格式：**

- `./parser test.cmm -ir out.ir`
  - 只输出.ir中间代码文件
- `./parser test.cmm -s out.s`
  - 只输出.s汇编代码文件
- `./parser test.cmm -ir out.ir -s out.s`
  - 同时输出.ir中间代码文件和.s汇编代码文件
  - 顺序严格，不支持-s在-ir之前



## 测试脚本

- [test_lab1.sh](Compiler\Code\test_lab1.sh) ：测试`·/parser test.cmm`

- [test_lab2.sh](Compiler\Code\test_lab2.sh) ：测试`·/parser test.cmm`

- [test_lab3.sh](Compiler\Code\test_lab3.sh) ：测试`./parser test.cmm -ir out.ir`

- [test_lab4.sh](Compiler\Code\test_lab4.sh) ：测试：`./parser test.cmm -s out.s`

- [test_ir-s.sh](Compiler\Code\test_ir-s.sh) ：测试：`./parser test.cmm -ir out.ir -s out.s`
- 文件路径可修改