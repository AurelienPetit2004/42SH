#ifndef EVAL_AST_H
#define EVAL_AST_H

#include <stdbool.h>

#include "../ast/node.h"
#include "../utils/expansion.h"
#include "../utils/stack.h"
#include "builtin.h"

int eval_ast(struct node *ast, struct stack *st);

#endif /* ! EVAL_AST_H */
