# Morse Code Tool for Linux

This is a command-line tool for playing with Morse Codes. It convert plain text
to Morse code audio via the Linux ALSA interface.

## Compile

```bash
make
```

## Run

```bash
# Generate Morse code for sample text "CQ CQ DE BH4FYQ K"
./morse
# Generate Morse code for specified text
./morse -t "CQ CQ DE ..."
# Generate Morse code for specified text using specified frequency
./morse -f 600 -t "CQ CQ DE ..."
```

## License

This project is licensed under the MIT License, please refer to LICENSE.txt for details.
