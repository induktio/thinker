#!/usr/bin/python

import re
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--revert", action="store_true")
parser.add_argument("-b", "--base", default="400000")
args = parser.parse_args()
base = args.base
revert = args.revert

i = 0
for s1 in sys.stdin:
    if ': ' in s1:
        m = re.search(r"^(\w+): (\w\w) (\w\w)", s1)
        if m:
            addr = int(m.group(1), 16) + int(base, 16)
            byte = m.group(2) if revert else m.group(3)
            sys.stdout.write('PatchByte(0x%X, 0x%s);\n' % (addr, byte))
            


