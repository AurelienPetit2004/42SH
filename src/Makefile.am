# define the subdirectories

SUBDIRS = eval_ast \
		  parser \
		  ast \
		  lexer \
		  io \
		  utils

bin_PROGRAMS = 42sh

42sh_SOURCES = 42sh.c

42sh_CPPFLAGS = -I%D%

42sh_CFLAGS = -std=c99 -Werror -Wall -Wextra -Wvla -pedantic

42sh_LDADD =  \
	parser/libparser.a \
	ast/libast.a \
	lexer/liblexer.a \
	io/libio.a \
	eval_ast/libeval_ast.a \
	utils/libutils.a
