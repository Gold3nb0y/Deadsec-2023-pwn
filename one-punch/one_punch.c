#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

char * key = "POWER, GET THE POWER.\0";
void* mapped_region = 0;

int one_punch(){
    __asm__(
    "pop %rdi\n"
    "ret\n"
        );
    return 1;
}

void vuln(){
    char garou[100];
    if(strcmp(key,mapped_region) != 0){
        puts("Two punch man? lameeeeee");
        exit(420);
    } else {
        memset(mapped_region, 0, 21);
        mprotect(mapped_region, 0x1000, PROT_READ);
    }
    fflush(stdout);
    gets(garou);
    return;
}

void init(){
    setvbuf(stdin,(char*)0x0,2,0);
    setvbuf(stdout,(char*)0x0,2,0);
    setvbuf(stderr,(char*)0x0,2,0);
    mapped_region = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_PRIVATE| MAP_ANON, -1, 0);
    memcpy(mapped_region, key, 35);
    return;
}

int main(){
    if(mapped_region == 0){
        init();
    }
    printf("Don't forget your cape! 0x%lx\n",one_punch+8);
    puts("Do you have what it takes to be the strongest hero?");
    vuln();
    puts("Guess not :(");
}


