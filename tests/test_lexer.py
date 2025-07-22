import os
import subprocess
import sys

def colorize(text, color):
    """Ajoute une couleur ANSI au texte."""
    colors = {
            "red": "\033[91m",
            "green": "\033[92m",
            "reset": "\033[0m",
            }
    return f"{colors.get(color, colors['reset'])}{text}{colors['reset']}"

def run_test(binary_path, test_case):
    """Exécute un test unique."""
    print(f"Running {test_case['name']}...")
    try:
        # Exécuter le binaire avec les arguments spécifiés
        result = subprocess.run(
                [binary_path] + test_case["args"],
                text=True,  # Capture le texte des sorties
                capture_output=True,
                input=test_case.get("stdin", ""),
                )

        # Vérifications des résultats
        success = True
        if result.returncode != test_case["expected_returncode"]:
            success = False
            print(colorize(f"FAIL: Return code {result.returncode} != {test_case['expected_returncode']}", "red"))

        if result.stdout.strip() != test_case["expected_stdout"]:
            success = False
            print(colorize(f"FAIL: STDOUT mismatch\nGot:\n{result.stdout}\nExpected:\n{test_case['expected_stdout']}", "red"))

        if result.stderr.strip() != test_case["expected_stderr"]:
            success = False
            print(colorize(f"FAIL: STDERR mismatch\nGot:\n{result.stderr}\nExpected:\n{test_case['expected_stderr']}", "red"))

        if success:
            print(colorize(f"PASS: {test_case['name']}", "green"))
        return success
    except FileNotFoundError:
        print(colorize(f"Binary not found: {binary_path}", "red"))
        return False
def main():
    binary_path = os.getenv("BIN_PATH", "./42sh")
    coverage_mode = os.getenv("COVERAGE", "no")
    output_file = os.getenv("OUTPUT_FILE", None)

    # Définir les cas de tests
    test_cases = [
            {
                "name": "Echo simple",
                "args": ["-c", "echo hello"],
                "expected_stdout": "hello",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Command invalide",
                "args": ["-c", "invalidcommand"],
                "expected_stdout": "",
                "expected_stderr": "42sh: unknown command 'invalidcommand'",
                "expected_returncode": 127,
            },
            {
                "name": "Multiple commandes",
                "args": ["-c", "echo first; echo second"],
                "expected_stdout": "first\nsecond",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Builtin_true",
                "args": ["-c", "true"],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Builtin_false",
                "args": ["-c", "false"],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 1,
            },
            {
                "name": "Echo_command",
                "args": ["-c", "echo hello"],
                "expected_stdout": "hello",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Single_quotes_and_echo_flag-e",
                "args": ["-c", "echo -e \'hello\tworld\'"],
                "expected_stdout": "hello\tworld",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Unknown_command",
                "args": ["-c", "pomme de terre"],
                "expected_stdout": "",
                "expected_stderr": "42sh: unknown command 'pomme'",
                "expected_returncode": 127,
            },
            {
                "name": "List_simple_command",
                "args": ["-c", "echo foo ; echo bar"],
                "expected_stdout": "foo\nbar",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "If_true",
                "args": ["-c", "if true; then echo congrats; fi"],
                "expected_stdout": "congrats",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "If_and_else",
                "args": ["-c", "if false; then echo failed; else echo succeed; fi"],
                "expected_stdout": "succeed",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "If_and_elif",
                "args": ["-c", 
                         "if false; then echo fail; elif true; then echo good; else echo failed; fi"],
                "expected_stdout": "good",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Multiple_elif",
                "args": ["-c",
                         "if false; then echo fail;elif false; then echo false; elif true; then echo good; else echo failed; fi"],
                "expected_stdout": "good",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Conpound_list_and_if",
                "args": ["-c", "if false; true; then echo a; echo b; echo c; fi"],
                "expected_stdout": "a\nb\nc",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "If_as_cond",
                "args": ["-c", "if if echo valid; then true; else false; fi; then echo valid; fi"],
                "expected_stdout": "valid\nvalid",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "If_in_body",
                "args": ["-c", "if true; then if echo valid; then echo valid; else echo fail; fi; fi"],
                "expected_stdout": "valid\nvalid",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Colliding_keyword",
                "args": ["-c", "if echo 1 2 3 then good; then true; else false; fi"],
                "expected_stdout": "1 2 3 then good",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Comment",
                "args": ["-c", "echo 1 2 3 #Comment for testing purposes"],
                "expected_stdout": "1 2 3",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Comment_inside_if_followed_by_newlline",
                "args": ["-c", "if echo 1 2 3 #Comment for testing purposes\n then echo 4 5 6; fi"],
                "expected_stdout": "1 2 3\n4 5 6",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Test_failed_command",
                "args": ["-c", "mv"],
                "expected_stdout": "",
                "expected_stderr": "mv: missing file operand\nTry 'mv --help' for more information.",
                "expected_returncode": 1,
            },
            {
                "name": "Test Stdin",
                "args": [],
                "stdin": "echo aaa",
                "expected_stdout": "aaa",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
    #Nouveau test
            {
                "name": "Create_directory",
                "args": ["-c", "mkdir -p trash || echo created trash"],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Simple_if_test",
                "args": ["-c", "if ! false ; then echo hello | cat ; fi" ],
                "expected_stdout": "hello",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Simple_or_test",
                "args": ["-c", "! true || echo hello" ],
                "expected_stdout": "hello",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Simple_variable_test",
                "args": ["-c", "i=1 && echo $i" ],
                "expected_stdout": "1",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Simple_variable_test",
                "args": ["-c", "i=1; echo $i" ],
                "expected_stdout": "1",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Empty_command",
                "args": ["-c", ""],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Invalid_input",
                "args": ["-c", "12345abcd"],
                "expected_stdout": "",
                "expected_stderr": "42sh: unknown command '12345abcd'",
                "expected_returncode": 127,
            },
            {
                "name": "Long_input",
                "args": ["-c", "echo " + "A" * 10000],
                "expected_stdout": "A" * 10000,
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Piped_commands",
                "args": ["-c", "echo foo | grep foo"],
                "expected_stdout": "foo",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Piped_tests_ls",
                "args": ["-c", "ls ../ | grep -i \"Make*\" | wc -l"],
                "expected_stdout": "3",
                "expected_stderr": "",
                "expected_returncode": 0,
            },

            {
                "name": "Nested_commands_with_stack",
                "args": ["-c", "if echo 1 && echo 2 || echo 3 && echo 4; then echo success; fi"],
                "expected_stdout": "1\n2\n4\nsuccess",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Complex_and_or_conditions",
                "args": ["-c", "false && echo fail || echo pass && echo success"],
                "expected_stdout": "pass\nsuccess",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Multiple_nested_if_with_logical_operators",
                "args":[
                    "-c",
                    "if true; then if false || true; then echo inner; fi; echo outer; fi"
                    ],
                "expected_stdout": "inner\nouter",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Nested_loops_and_conditions",
                "args": [
                    "-c",
                    "for i in 1 2 3; do if true; then echo $i; else echo skip; fi; done"
                    ],
                "expected_stdout": "1\n2\n3",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Hidden_comment_and_condition",
                "args": [
                    "-c",
                    "if true; then echo visible #hidden comment\n fi"
                    ],
                "expected_stdout": "visible",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
        #test de for et while loop
            {
                "name": "While_loop_read_with_cat",
                "args": [],
                "stdin": "echo line1line2line3 > trash/temp.txt; while read line; do echo $line; done < trash/temp.txt",
                "expected_stdout": "",
                "expected_stderr": "42sh: unknown command 'read'",
                "expected_returncode": 127,
            },

            {
                "name": "While_loop_with_cat_pipeline",
                "args": [
                    "-c",
                    "echo '1\n2\n3' | while read num; do echo Number: $num; done"
                    ],
                "expected_stdout": "Number: 1\nNumber: 2\nNumber: 3",
                "expected_stderr": "",
                "expected_returncode": 0,
            },

            {
                "name": "While_loop_interactive_cat",
                "args": [
                    "-c",
                    "echo 'input1\ninput2' > temp.txt; cat temp.txt | while read input; do echo Received: $input; done"

                    ],
                "expected_stdout": "Received: input1\nReceived: input2",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Redir_and_Read_test",
                "args": ["-c", "read -r var1 var2 <<< 'Hello, World!' && echo $var1 && echo $var2"],
                "expected_stdout": "Hello,\nWorld!",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "While_loop_cat_with_line_conditions",
                "args": [
                    "-c",
                    "echo -e 'apple\nbanana\ncarrot' > fruits.txt; while read fruit; do if \"$fruit\" = \"banana\"; then echo Found \"$fruit\"; fi; done < fruits.txt"
                    ],
                "expected_stdout": "Found banana\n",
                "expected_stderr": "",
                "expected_returncode": 0,
            },

            #test complexe
            {
                "name": "For_loop_with_redirection_and_pipes",
                "args": [
                    "-c",
                    "for i in 1 2 3; do echo $i | grep 2 > trash/output.txt; done; cat trash/output.txt"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },

            {
                "name": "While_loop_with_redirection_and_if",
                "args": [
                    "-c",
                    "i=0; while $i -lt 5 ; do if [ $i -eq 3 ] ; then echo $i > output.txt; fi; i=$((i+1)); done; cat output.txt"
                    ],
                "expected_stdout": "3",
                "expected_stderr": "",
                "expected_returncode": 0,
            },

            {
                "name": "If_with_redirection_in_for_loop",
                "args": [
                    "-c",
                    "for i in A B C; do if [ $i = B ] ; then echo $i > output.txt; fi; done; cat output.txt"
                    ],
                "expected_stdout": "B",
                "expected_stderr": "",
                "expected_returncode": 0,
            },

            {
                "name": "Nested_loops_with_pipes_and_if",
                "args": [
                    "-c",
                    "for i in 1 2; do for j in 3 4; do if [ $j -eq 4 ] ; then echo $i$j | grep 14; fi; done; done"
                    ],
                "expected_stdout": "14",
                "expected_stderr": "",
                "expected_returncode": 1,
            },
            {
                "name": "While_with_variable_change",
                "args": [
                    "-c",
                    "tmp=\"0\"; while ! [ \"$tmp\" == \"1\" ] ; do tmp=1; done; echo end"
                    ],
                "expected_stdout": "end",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Until_with_variable_change",
                "args": [
                    "-c",
                    "tmp=\"0\"; until [ \"$tmp\" == \"1\" ] ; do tmp=1; done; echo end"
                    ],
                "expected_stdout": "end",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Simple_function_with_env",
                "args": [
                    "-c",
                    "foo() {echo $1; echo $2; echo $@;}; foo hello world!"
                    ],
                "expected_stdout": "hello\nworld!\nhello world!",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Simple_for_with_env",
                "args": [
                    "-c",
                    "for i in 1 2 3 4; do echo hello$i; done"
                    ],
                "expected_stdout": "hello1\nhello2\nhello3\nhello4",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Failed_for_no_body",
                "args": [
                    "-c",
                    "for i in 1 2 3 4; do done"
                    ],
                "expected_stdout": "",
                "expected_stderr": "42sh: Error general rule_for: unexpected token done, 17",
                "expected_returncode": 2,
            },
            #nouveau test
            {
                "name": "Chained_logical_operations",
                "args": [
                    "-c",
                    "echo start && (echo middle && false || echo fallback) && echo end"
                    ],
                "expected_stdout": "start\nmiddle\nfallback\nend",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Special_characters_in_stack",
                "args": [
                    "-c",
                    "(echo 'A&B' && echo 'C|D') || (echo 'fallback')"
                    ],
                "expected_stdout": "A&B\nC|D",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Pipes_and_redirections",
                "args": [
                    "-c",
                    "echo 'first' | (cat && echo 'second') | grep 'second'"
                    ],
                "expected_stdout": "second",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "While_loop_read_with_cat",
                "args": [],
                "stdin": "echo line1line2line3 > trash/temp.txt; while read line; do echo $line; done < trash/temp.txt",
                "expected_stdout": "",
                "expected_stderr": "42sh: unknown command 'read'",
                "expected_returncode": 127,
            },
            {
                "name": "Builtin_cd_test",
                "args": [
                    "-c",
                    "ls; cd; ls;"
                    ],
                "expected_stdout":"big_output\nMakefile\nMakefile.am\nMakefile.in\nout\noutput.txt\nscript.sh\nscript_tower.sh\ntest_lexer.py\ntest_parser.sh\ntrash\nafs",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Builtin_cd_test_2",
                "args": [
                    "-c",
                    "cd trash; ls"
                    ],
                "expected_stdout": "big_output\ncombined_output\nerror_log\ninput_file\noutput_file\noutput.txt\nsuccess_log\ntemp.txt\ntest_file",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Builtin_cd_test_3",
                "args": [
                    "-c",
                    "cd ../../../../; ls; cd -;"
                    ],
                "expected_stdout":"ING1\nspe\nsup\n/afs/cri.epita.fr/user/n/na/nabil.chartouni/u/ING1/42sh/epita-ing-assistants-acu-42sh-2027-paris-52/tests",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Builtin_cd_test_error",
                "args": [
                    "-c",
                    "cd ew"
                    ],
                "expected_stdout":"",
                "expected_stderr": "cd: ew No such file or directory",
                "expected_returncode": 0,
            },
            {
                "name": "Builtin_test_ultime",
                "args": [
                    "-c",
                    "cd ../../../../spe/tp-spe/epita-prepa-asm-PROG-205-P-01-2027-nabil.chartouni/; ls"
                    ],
                "expected_stdout":"AUTHORS\nfractal_canopy\nimage",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Simple_break",
                "args": [
                    "-c",
                    "while true; do echo a; break; done"
                    ],
                "expected_stdout": "a",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Simple_continue",
                "args": [
                    "-c",
                    "for i in 1 2 3 4 5; do echo $i; continue; echo $i; done"
                    ],
                "expected_stdout": "1\n2\n3\n4\n5",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "For_Break_2",
                "args": [
                    "-c",
                    "for i in 1 2 3 4 5; do while true; do echo a; break 2; done done"
                    ],
                "expected_stdout": "a",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Break_multiple_args",
                "args": [
                    "-c",
                    "while true; do break 1 2 3; done"
                    ],
                "expected_stdout": "",
                "expected_stderr": "42sh: Too many argument in break and continue",
                "expected_returncode": 2,
            },
            {
                "name": "Until_continue_as_cond",
                "args": [
                    "-c",
                    "until continue; do echo a; done"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Until_break_as_cond",
                "args": [
                    "-c",
                    "until break; do echo a; done"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Break_Until",
                "args": [
                    "-c",
                    "until true; do echo a; break; echo b; done"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "While_break_as_cond",
                "args": [
                    "-c",
                    "while break; do echo a; done"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Output_to_file_redirection",
                "args": [
                "-c",
                "echo 'File content' > trash/test_file && cat trash/test_file"
                ],
                "expected_stdout": "File content",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Append_output_redirection",
                "args": [
                "-c",
                "echo 'First line' > trash/output_file && echo 'Second line' >> trash/output_file && cat trash/output_file"
                ],
                "expected_stdout": "First line\nSecond line",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Input_redirection",
                "args": [
                    "-c",
                    "echo 'Input text' > trash/input_file && cat < trash/input_file"
                    ],
                "expected_stdout": "Input text",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Pipe_with_multiple_commands",
                "args": [
                "-c",
                "echo 'line1\nline2' | grep 'line2' | xargs ls"
                ],
                "expected_stdout": "",
                "expected_stderr": "42sh: Error single quotes not closed",
                "expected_returncode": 2,
            },
            {
               "name": "Redirect_stderr_to_file",
                "args": [
                "-c",
                "ls non_existent_file 2> trash/error_log && cat trash/error_log"
                ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 2,
            },
            {
                "name": "Combine_stdout_and_stderr",
                "args": [
                  "-c",
                  "(echo 'This is stdout'; ls non_existent_file) > trash/combined_output 2>&1 && cat trash/combined_output"
                   ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 2,
            },
            {
                "name": "Combine_stdout_and_stdin",
                "args": [
                  "-c",
                  "echo 'This is stdin'; ls trash > trash/big_output 0>&1 && cat trash/big_output"
                   ],
                "expected_stdout": "This is stdin\nbig_output\ncombined_output\nerror_log\ninput_file\noutput_file\noutput.txt\ntemp.txt\ntest_file",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Conditional_with_pipe_and_redirection",
                "args": [
                "-c",
                "(echo 'data' | grep 'data') && echo 'Success' > trash/success_log && cat trash/success_log"
                ],
                "expected_stdout": "data\nSuccess",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Test_rm_nettroyafe",
                "args": [
                    "-c",
                    "rm -rf tests/trash/ ./tests/output.txt"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "Redir_4",
                "args": [
                    "-c",
                    "echo a <> tests/trash/hello.txt"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "redir_5",
                "args": [
                    "-c",
                    "echo a 2>& 1"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "redir_6",
                "args": [
                    "-c",
                    "echo a 0<& 2"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "my_export",
                "args": [
                    "-c",
                    "a=1; (a=2; export a)"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "my_unset",
                "args": [
                    "-c",
                    "a=2; unset a; unset v"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "echo_backslashed",
                "args": [
                    "-c",
                    "echo \\\\ \\n \\t"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "exit",
                "args": [
                    "-c",
                    "(exit 1); (exit -1); exit"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "dot",
                "args": [
                    "-c",
                    ". script_tower.sh"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "echo_backslashed",
                "args": [
                    "script_tower.sh"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "@_and_*_RANDOM_$",
                "args": [
                    "-c",
                    "echo $@; echo \"$@\"; echo $*; echo \"$*\";echo $$; echo \"$$\"; echo $RANDOM; echo \"$RANDOM\"; echo $1; echo \"$1\"; v=echo; $v test"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "expand_backslash",
                "args": [
                    "-c",
                    "echo 'te\st'; echo \\\"\\$\\`\\ \"\\\\\""
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "dict_pop",
                "args": [
                    "-c",
                    "v=32 echo $v"
                    ],
                "expected_stdout": "",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "aux_1",
                "args": [
                    "-c",
                    "foo() { \"$@\"; }; foo echo a"
                    ],
                "expected_stdout": "a",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "aux_4",
                "args": [
                    "-c",
                    "foo() { $@; }; foo echo a"
                    ],
                "expected_stdout": "a",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
            {
                "name": "aux_6",
                "args": [
                    "-c",
                    "foo() { $1 $2; }; foo echo a"
                    ],
                "expected_stdout": "a",
                "expected_stderr": "",
                "expected_returncode": 0,
            },
    ]

    # Exécuter les tests
    passed = 0
    for test_case in test_cases:
        if run_test(binary_path, test_case):
            passed += 1

    # Résumé
    total = len(test_cases)
    percentage = (passed * 100) // total
    print(f"\nTest Summary: {passed}/{total} tests passed.")

    # Écrire dans le fichier si COVERAGE est "yes"
    if True and output_file:
        with open(output_file, "w") as f:
            f.write(str(percentage))

if __name__ == "__main__":
    main()
