#!/usr/bin/python

import sys

f0 = open(len(sys.argv) > 1 and sys.argv[1] or 'patch/terranx_def.exe', 'rb') 
f1 = open(len(sys.argv) > 2 and sys.argv[2] or 'patch/terranx_mod.exe', 'rb')
i = 0

while True:
    a = f0.read(1)
    b = f1.read(1)
    if (not a) != (not b):
        sys.stderr.write('File size mismatch\n')
        exit(1)
    if not a:
        exit(0)
    if (a != b):
        print('%08X: %02X %02X' % (i, ord(a), ord(b)))
    i += 1
    
