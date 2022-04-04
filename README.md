# A (bad) compression algorithm inspired by lz77.

## Description
This compression algorithm replaces multiple occurences of strings with back-references.
These references are written as: '%d,l', where 'd' is a negative offset and 'l' the length of the reference.
The escape sequence for a single '%' is '%%'.

## Usage
This project provides both a library and an executable using it.
The API should be simple enough, as it consists of only 3 functions and 1 struct.
For details on how to use the `blz77` program, issue `blz77 -h`.
BLZ77 supports 10 levels of compression (from 0 to 9) each improving the compression
for larger files but making it worse for smaller files.

## Installation
Currently, only the Meson build system is supported.
Support for the GNU build system will be added later.

### Meson
```
meson setup build --buildtype=release
meson compile -C build
meson install -C build
```

## TODO
- Implement the '-l' option
- Improve compression performance
- Provide man pages and pkg-config files
