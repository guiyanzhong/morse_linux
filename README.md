# Morse Code Tool for Linux

This is a command-line tool for playing with Morse Codes.

## Compile

```bash
mkdir build
cd build
cmake ..
make
```

```bash
# Generate Morse code for sample text "CQ CQ DE BH4FYQ K"
./morse
# Generate Morse code for specified text
./morse -t "CQ CQ DE ..."
# Generate Morse code for specified text using specified frequency
./morse -f 600 -t "CQ CQ DE ..."
```
