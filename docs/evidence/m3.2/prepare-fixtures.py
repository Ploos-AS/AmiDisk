"""Create qualification fixtures in a new directory; never modify the source."""
from pathlib import Path
import hashlib
import sys

source = Path(sys.argv[1])
destination = Path(sys.argv[2])
destination.mkdir(exist_ok=False)
reference = source.read_bytes()
assert len(reference) == 901120
changed = bytearray(reference)
offset = (((10 * 2) + 1) * 11 + 5) * 512 + 7
old = changed[offset]
changed[offset] ^= 1
assert sum(a != b for a, b in zip(reference, changed)) == 1
for name, data in [('reference.adf', reference), ('mismatch.adf', changed),
                   ('short.adf', reference[:-1]), ('long.adf', reference + b'\0')]:
    with (destination / name).open('xb') as output:
        output.write(data)
    print(name, len(data), hashlib.sha256(data).hexdigest())
print(f'C10/H1/S5 byte=7 offset={offset} old={old:02x} new={changed[offset]:02x}')
