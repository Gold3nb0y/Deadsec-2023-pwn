#include <stddef.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

void* shrmem_head = NULL;
int shrmem_id;
char chat_filename[0x10];
FILE* chatroom;
char username[16];
char* read_line;
int pid;
int username_size;

#define MSG_MAX_SIZE = 0xe8;

typedef struct message{
    uint64_t next;
    char name[0x10];
    char message[0xe8];
} message;


//setup the process to work on remote
void setup(){
    setvbuf(stdin,(char*)0x0,2,0);
    setvbuf(stdout,(char*)0x0,2,0);
    setvbuf(stderr,(char*)0x0,2,0);
    return;
}

void overwrite_stdout(){
    printf("%s", "> ");
    fflush(stdout);
}

//connect to the target server and read the filename
void connect(){
    printf("Please enter the id of the server you'd like to connect to: ");
    char number[0x20];
    read(0, number, 0x1f); 
    shrmem_id = atoi(number);
    shrmem_head = shmat(shrmem_id, (void*)0, 0);
    if((int64_t)shrmem_head == -1){
        puts("Could not connect to server");
        exit(-1);
    }

    printf("Please enter the password generated by the server: ");
    char password[17];
    memset(password, 0, 17);
    read(0, password, 0x10);
    if(memcmp(password, shrmem_head+0xd0, 0x10) != 0){
        puts("Incorrect password, exiting...");
        exit(0);
    } else {
        puts("Authenticated");
    }
    getchar(); //catch the trailing newline

    memset(chat_filename, 0, 0x10);
    memcpy(chat_filename, (char*)(shrmem_head+0x10), 14);
    chatroom = fopen(chat_filename, "r");
    if(chatroom == NULL){
        puts("failed to open chatfile, contact admin");
        exit(-1);
    }
    fclose(chatroom);

    memset(username, 0, 16);
    printf("Please give a username(max size of 14): ");
    username_size = read(0, username, 14)-1;
    if(username_size == 13) username[14] = 0xa; //make sure there is a new line
    read_line = malloc(0x420);
    return;
}

void cleanup(int sig_num){
    shmdt(shrmem_head);
    if(chatroom != NULL) fclose(chatroom);
    chatroom = NULL;
    if(read_line != NULL) free(read_line);
    read_line = NULL;
    kill(pid, SIGKILL);
}

int parse_recvd_msg(){
    char* tosend = malloc(0x420);
    ssize_t bytes_read = 0;
    ssize_t count = 0;
    size_t len = 0;
    char name[16];

    bytes_read = getline(&read_line,&len, chatroom);
    if(bytes_read == -1) return 0;
    memcpy(name, read_line, bytes_read-1);

    bytes_read = getline(&read_line, &len, chatroom);
    assert(bytes_read != -1);
    int i = 1;
    while(strncmp(read_line, "----message break----", 21)!=0 && count < 0x420){
        memcpy(tosend+count, read_line, bytes_read);
        count += bytes_read;
        bytes_read = getline(&read_line, &len, chatroom);
        i++;
    }

    if(strncmp(username, name, username_size) == 0) return 0;

    printf("[%s]: ", name);
    write(1, tosend, count);
    return 1;
}

//a forked process attaches and reads the new messages from the chatroom
void read_loop(){
    int message_count = 0;
    while(1){
        size_t len = 0;
        sleep(2);
        chatroom = fopen(chat_filename, "r");
        if(chatroom == NULL) exit(-1);
        int temp_count = 0;
        int overwrite = 0;
        while(getline(&read_line, &len, chatroom) != -1){
            if(strncmp(read_line,"----message start----", 21) == 0){
                if(temp_count < message_count){
                    temp_count++;
                } else {
                    overwrite = parse_recvd_msg() || overwrite;
                    message_count++;
                    temp_count++;
                }
            }
            if(overwrite) overwrite_stdout();
        }
        if(chatroom != NULL) fclose(chatroom);
        chatroom = NULL;
    }
}

//allows a user to exit and also send messages to the server
void input_loop(){
    char* mes = malloc(0x100);
    struct message* msg_head = (struct message*)(shrmem_head+0x100);
    overwrite_stdout();
    while(1){
        while(memcmp(shrmem_head, "\x00\x00\x00\x00\x00\x00\x00\x00", 8)!=0){sleep(0.01);} //wait until the key is cleared
        int count = 2;
        while(msg_head->next != 0){
            msg_head = (struct message*)(shrmem_head+(0x100*count));
            count++;
        }
        msg_head->next=1; 
        memset(mes, 0, 0x100);
        uint b_count = read(0, mes, 0xe8);
        mes[b_count-1] = '\n'; //garentee newline
        strcpy(msg_head->message, mes);
        memcpy(msg_head->name, username, 16);
        msg_head->next = 2;
        if(memcmp(shrmem_head+0xf0, "\x00", 1) == 0) memset(shrmem_head+0xf0, 1,1);
        msg_head = (struct message*)(shrmem_head+0x100);
        overwrite_stdout();
    }
}

//launch everything
int main(int argv, char* argc[]){
    setup();
    connect();
    signal(SIGINT, cleanup);
    signal(SIGALRM, cleanup);
    alarm(120); 
    pid = fork();
    if(pid == 0){
        read_loop();
    } else {
        input_loop();
        cleanup(0);
    }
}