# Init the 42sh project
AC_INIT([42sh], [1.0], [])

# Setup Automake
AM_INIT_AUTOMAKE([subdir-objects] [foreign])

# Pretty display of Makefile rules
AM_SILENT_RULES([yes])

# Enable ar for Makefile
AM_PROG_AR

# Check if ranlib is available
AC_PROG_RANLIB

# Check if a C compiler is available
AC_PROG_CC

# Check if a compiler has this list of flags
AX_COMPILER_FLAGS([], [], [], [-Wall -Wextra -Werror -Wvla -pedantic -std=c99])

# List Makefiles in subdirectories
AC_CONFIG_FILES([
                    Makefile
                    src/Makefile
                    src/lexer/Makefile
                    src/parser/Makefile
                    src/ast/Makefile
                    src/io/Makefile
                    src/eval_ast/Makefile
                    src/utils/Makefile
                    tests/Makefile
                ])
AC_OUTPUT
