lib_LIBRARIES = libeval_ast.a

libeval_ast_a_SOURCES =  \
  eval_ast.c \
  eval_ast.h \
  builtin.c \
  builtin.h \
  redir.c \
  redir.h

libeval_ast_a_CPPFLAGS = -I$(top_srcdir)/src

libeval_ast_a_CFLAGS = -std=c99 -Werror -Wall -Wextra -Wvla -pedantic

noinst_LIBRARIES = libeval_ast.a
