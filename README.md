# myfs

It's homework of my OS course. :D

Implementing filesystem in memory.

Directory is supported!

> **Notes:** I didn't spend too much time on improving commands, so just thinking it as a stupid shell script. But the basic functionalities are good.
>
> Ex. `put file ..` won't works, you need to `cd` to it's parent before put this file in.
> Which means you need to `cd ..` then `put file` or `put file new-name` .
>
> Please think all the command like this way. :)
> `ls ..` -> will not work, `cd ..` + `ls` -> will work

only tested on **Linux**

## Run

```bash
make myfs && ./myfs
```

## How to use?

After loaded the partition, type `help` to get a help.

## Colors

Colors are disabled in Windows. Since it's not supported in `cmd.exe`.

```c
#ifdef _WIN32
    #define C_RED
    #define C_GRN
    #define C_YEL
    #define C_BLU
    #define C_MAG
    #define C_CYN
    #define C_WHT
    #define C_RST
#else
    #define C_RED "\x1B[31m"
    #define C_GRN "\x1B[32m"
    #define C_YEL "\x1B[33m"
    #define C_BLU "\x1B[34m"
    #define C_MAG "\x1B[35m"
    #define C_CYN "\x1B[36m"
    #define C_WHT "\x1B[37m"
    #define C_RST "\x1B[0m"
#endif
```

## Endianness

```c
static int32_t first_zero(uint64_t x)
{
    int32_t i;

    // since the representation of int is different
    // in 2 different endianness, we need to use 2
    // different approach to get the first ZERO.
    #ifndef MY_FS_BIG_ENDIAN
        uint8_t* p = ((uint8_t*) &x) - 1;
    #endif

    for (i = 63; i >= 0; --i)
    {
        #ifndef MY_FS_BIG_ENDIAN
            if ((i & 7) == 7) ++p;
            if (((*p >> (i & 7)) & 1) == 0) break;
        #else
            if ((x >> i & 1) == 0) break;
        #endif
    }

    return 63 - i;
}
```
