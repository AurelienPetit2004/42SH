lib_LIBRARIES = libutils.a

libutils_a_SOURCES =  \
  dict.c \
  dict.h \
  expansion.c \
  expansion.h \
  stack.c \
  stack.h

libutils_a_CPPFLAGS = -I$(top_srcdir)/src

libutils_a_CFLAGS = -std=c99 -Werror -Wall -Wextra -Wvla -pedantic

noinst_LIBRARIES = libutils.a
