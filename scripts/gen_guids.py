#!/usr/bin/env python3
import sys
import os
import re


if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <output file>")
    exit(-1)

with open(sys.argv[1], 'w') as out:

    out.write('// AUTO-GENERATED FILE\n')
    out.write('#include <Uefi.h>\n')

    #
    # This cursed regex will match against the guids in the source files and will allow
    # us to automatically generate a file containing all of them
    #
    regex = re.compile(r'(0x[a-f0-9A-F]{1,8}(\s*,\s*0x[a-f0-9A-F]{1,4}){2}\s*,\s*{\s*0x[a-f0-9A-F]{1,2}(\s*,\s*0x[a-f0-9A-F]{1,2}){7}\s*})')

    for root, subdirs, files in os.walk("edk2/Include"):
        for file in files:
            # Read the text file
            text = open(os.path.join(root, file)).read()

            # Find all the guids we need
            guids = regex.findall(text)
            if len(guids) == 0:
                continue

            out.write('\n')
            out.write("//\n")
            out.write(f"// {file}\n")
            out.write("//\n")

            # Trim out the guids nicely
            for i in range(len(guids)):
                guids[i] = guids[i][0]

            # now search for the guid variable names
            for line in text.splitlines():
                if line.startswith('extern EFI_GUID'):
                    name = line.split()[2]
                    guid = guids.pop(0)
                    if name.endswith(';'):
                        name = name[:-1]
                    out.write(f'EFI_GUID {name} = {{ {guid} }};\n')

