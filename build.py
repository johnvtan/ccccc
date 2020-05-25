#!/usr/bin/env python3
import sys
import subprocess
import pathlib

compiler_path = pathlib.Path(__file__).parent.absolute()/'COMPILERBABY'

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage: build.py <filename>')
        sys.exit(-1)

    input_file = sys.argv[1]
    if not input_file.endswith('.c'):
        print('error: filename must end with \'.c\' extension')

    asm_file = input_file.replace('.c', '.s')
    compile_args = [compiler_path, input_file]
    result = subprocess.Popen(compile_args, stdout=subprocess.PIPE)
    output, _ = result.communicate()
    err = result.returncode
    if err != 0:
        print("Compilation failed")
        sys.exit(-1)

    with open(asm_file, 'w+') as out:
        out.write(output.decode('utf-8'))

    executable = asm_file.replace('.s', '')
    assemble_args = ['gcc', '-no-pie', asm_file, '-o', executable]
    subprocess.run(assemble_args)
    
