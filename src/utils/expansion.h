#ifndef EXPANSION_H
#define EXPANSION_H

#include <stddef.h>
#include <stdio.h>

#include "../ast/node.h"
#include "../lexer/token.h"
#include "dict.h"
#include "stack.h"

int expand(struct node *node, struct stack *s);

#endif /* !EXPANSION_H */
