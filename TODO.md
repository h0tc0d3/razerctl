# TODO List

## BSD HIDRAW Support

- [Freebsd hidraw](https://man.freebsd.org/cgi/man.cgi?query=hidraw&sektion=4)

## LOD Support

Razer device support LOD?

```text
Command Class: 0b
Command Id: 80
Args Bytes: 04
Args: 01 04 00 00
```

???

```text
Command Class: 0b
Command Id: 8e
Args Bytes: 04
Args: 04 00 00 00
```

### RAZER MAMBA ELITE

Set Lift-Off Distance:

```text
Command Class: 0x0b
Command Id: 0x03
Args: 00 04 01

Command Class: 0x0b
Command Id: 0x05

Args:
1 - 00 04 0a 0c 02 00 00 00 00 10 00
2 - 00 04 0a 10 83 00 00 00 00 03 00
3 - 00 04 0a 10 83 00 00 00 00 03 00
4 - 00 04 0a 10 83 00 00 00 00 03 00
5 - 00 04 0a 10 83 00 00 00 00 03 00
6 - 00 04 0a 10 83 00 00 00 00 03 00
7 - 00 04 0a 10 83 00 00 00 00 03 00
8 - 00 04 0a 10 83 00 00 00 00 03 00
9 - 00 04 0a 10 83 00 00 00 00 03 00
10 -00 04 0a 10 83 00 00 00 00 03 00
```

How to get lod value? Only two values?
