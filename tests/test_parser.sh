#!/bin/sh

echo hello world
echo "----"
echo -n 'echo hello world' | ./src/42sh
echo $?
echo "============="

echo foo ; echo bar
echo "----"
echo -n 'echo foo ; echo bar' | ./src/42sh
echo $?
echo "============="

echo foo; echo bar
echo "----"
echo -n 'echo foo; echo bar' | ./src/42sh
echo $?
echo "============="

echo foo ; echo bar ;
echo "----"
echo -n 'echo foo ; echo bar ;'| ./src/42sh
echo $?
echo "============="

if true
then
   echo "b est supérieure à c"
fi
echo "----"
echo -n 'if true
then
   echo 'b est supérieure à c'
fi' | ./src/42sh
echo $?
echo "============="

if false; true; then echo "a"; echo "b"; echo "c"; fi
echo "----"
echo -n 'if false; true; then echo a; echo b; echo c; fi'| ./src/42sh
echo $?
echo "============="

if false; then echo "false"; else echo "true"; fi
echo "----"
echo -n 'if false; then echo false; else echo true; fi'| ./src/42sh
echo $?
echo "============="

if
    if echo a;
    then true;
    else false;
    fi;
then echo -E "\nb\t";
else echo c;
fi
echo '----'
echo -n 'if
    if echo a;
    then true;
    else false;
    fi;
then echo -E \nb\t;
else echo c;
fi #print a b'| ./src/42sh
echo $?
echo "============"

if false;
    true;
then
    echo a;
    echo b; echo c;
fi
echo "----"
echo -n 'if false
    true
then
    echo a
    echo b; echo c
fi'| ./src/42sh
echo $?
echo "============="

while true; do if echo hello; then true; else false; fi done | false && true || false; echo hello world;
