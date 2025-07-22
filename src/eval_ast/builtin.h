#ifndef BUILTIN_H
#define BUILTIN_H

#define _POSIX_C_SOURCE 200809L

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../ast/node.h"
#include "../eval_ast/eval_ast.h"
#include "../io/io.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../utils/dict.h"
#include "../utils/expansion.h"
#include "../utils/stack.h"

void my_exit(struct node *ast);
int my_echo(struct node *ast);
int my_true(struct node *ast);
int my_false(struct node *ast);
int my_unset(struct node *ast, struct stack **d);
int my_dot(struct node *ast, struct stack *s);
int my_cd(struct node *ast, struct stack *s);
int my_export(struct node *ast, struct stack *s);
int my_break_and_continue(struct node *ast, int cmd);

#endif /* ! BUILTIN_H */
