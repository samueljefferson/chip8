import sys
import copy

# error if there are no command line arguments
if len(sys.argv) != 2:
    print('fatal error: requires a file to assemble')
    exit();

# get the file name
source_file = str(sys.argv[1])
# print(source_file)

# use the file name to create name of assembled file
asm_file = copy.copy(source_file)
asm_file = asm_file[:(len(source_file)-3)] + 'ch8'

# open the source file
fo_source = open(source_file, 'r')

lines = fo_source.readlines();
fo_source.close()

buffer = []
for line in lines:
    check = line[:4]
    # skip lines that don't start with a letter or number
    if check[0].isdigit() or check[0].isalpha():
        # print(int(check[:2], 16))
        # print(int(check[2:], 16))
        buffer.append(int(check[:2], 16))
        buffer.append(int(check[2:], 16))


fo_asm = open(asm_file, 'wb')

buffer = bytearray(buffer)

fo_asm.write(buffer)
fo_asm.close()
