"""Create controlled test copies; never modify the supplied original checkpoint."""
from pathlib import Path
import struct
import sys
import zlib

source, destination = Path(sys.argv[1]), Path(sys.argv[2])
original = source.read_bytes()
assert original[:8] == b'IRSNAP06'
assert zlib.crc32(original[:-4]) == struct.unpack('<I', original[-4:])[0]
destination.mkdir(parents=True, exist_ok=True)
for name in ('checksum', 'truncated', 'schema-zero', 'schema-future', 'content-missing'):
    data = bytearray(original)
    if name == 'checksum':
        data[-1] ^= 1
    elif name == 'truncated':
        data = data[:len(data)//2]
    else:
        if name == 'schema-zero':
            struct.pack_into('<I', data, 8, 0)
        elif name == 'schema-future':
            struct.pack_into('<I', data, 8, 2)
        else:
            data[28] = ord('X')
        struct.pack_into('<I', data, len(data)-4, zlib.crc32(data[:-4]))
    (destination / f'{name}.irs').write_bytes(data)
