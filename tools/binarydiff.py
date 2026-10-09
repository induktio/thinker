#!/usr/bin/python3

import re
import sys

ChunkSize = 4096
NonZero = re.compile(rb'[^\x00]')

def compare():
    if len(sys.argv) != 3:
        sys.stderr.write('Usage: %s <file1> <file2>\n' % sys.argv[0])
        return 2
    path1 = sys.argv[1]
    path2 = sys.argv[2]
    offset = 0
    with open(path1, 'rb') as f1, open(path2, 'rb') as f2:
        while True:
            a = f1.read(ChunkSize)
            b = f2.read(ChunkSize)
            n = min(len(a), len(b))
            mismatch = len(a) != len(b)
            if mismatch:
                a = a[:n]
                b = b[:n]
            if a != b:
                x = int.from_bytes(a, 'little') ^ int.from_bytes(b, 'little')
                mask = x.to_bytes(n, 'little')
                lines = []
                for m in NonZero.finditer(mask):
                    i = m.start()
                    lines.append('%08X: %02X %02X\n' % (offset + i, a[i], b[i]))
                sys.stdout.write(''.join(lines))
            if mismatch:
                sys.stderr.write('File size mismatch\n')
                return 1
            if not n:
                return 0
            offset += n

if __name__ == '__main__':
    sys.exit(compare())
