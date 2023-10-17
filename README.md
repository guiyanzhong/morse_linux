# Morse Code Tool for Linux

This is a command-line tool for playing with Morse codes. It convert plain text
to Morse code audio via the Linux ALSA interface.

This program works in one of the three mode: manual key, double paddle key, or
send text directly.

## Compile

```bash
make
```

## Run

```bash
# Print help
./morse -h
# Send text
./morse -t "CQ CQ DE ..."
# Send text with 600 Hz audio frequency
./morse -f 600 -t "CQ CQ DE ..."
# Use double-paddle Morse key connected with a USB-to-RS232 cable.
./morse -d /dev/ttyUSB0 -k 2
```

Program options:

```text
  -t "CQ CQ"     :  text to send
  -k 2           :  Morse key, 1 for manual key, or 2 for double paddle key
  -d /dev/ttyS0  :  serial device connecting the Morse key, default to /dev/ttyUSB0
  -f 600         :  audio frequency, default to 440 Hz
  -h             :  print this help
```

At lease, one of "-t" or "-k" is required.

## License

This project is licensed under the MIT License, please refer to LICENSE.txt for details.
