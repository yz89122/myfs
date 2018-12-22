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
make a.out && ./a.out
```

## How to use?

After loaded the partition, type `help` to get a help.
