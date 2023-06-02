#include <stddef.h>
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

int shrmem_id;
void* shrmem_head;
FILE* chat_file;
char key[8];
char filename[16];

#define MSG_SIZE = 0xe8;

typedef struct message{
    uint64_t next;
    char name[0x10];
    char message[0xe8];
} message;

void gen_random(char* temp, uint size){
    int fid = open("/dev/urandom",O_RDONLY);
    char c;
    uint count = 0;
    while(count < size){
        read(fid, &c, 1);
        //allow upper/lowercase letters and numbers
        if((c > 0x40 && c < 0x5b) || (c > 0x60 && c < 0x7b) || (c > 0x2f && c < 0x3a)){
            temp[count] = c;
            count++;
        }
    }
    close(fid);
}

void setup(){
    shrmem_id = shmget(IPC_PRIVATE, 0x1000, 0666);
    if(shrmem_id==-1){
        puts("Failed to allocated server memory, contact admin");
        exit(-1);
    } else { 
        printf("Server id: %d\n", shrmem_id);
    }
   
    shrmem_head = shmat(shrmem_id, (void*)0, 0);
    if((int64_t)shrmem_head == -1){
        puts("Could not connect to server memory, contact admin");
        exit(-1);
    }

    memset(filename, 0, 10);
    memcpy(filename, "/tmp/", 5);
    char* temp = malloc(10);
    memset(temp, 0, 6);

    gen_random(temp, 5);

    strcat(filename, temp);
    strcat(filename, ".txt");

    chat_file = fopen(filename, "w+");

    free(temp);

    if(!chat_file){
        puts("Failed to create a new chatroom, contact admin");
        exit(-1);
    }

    memset(shrmem_head, 0, 0x1000);
    memcpy(shrmem_head+0x10, filename, 14);

    gen_random(shrmem_head+0xd0, 0x10);

    printf("Password to connect to the server: %s\n", (char*)shrmem_head+0xd0);

    int fid = open("/dev/urandom", O_RDONLY);
    memset(key, 0, 8);
    read(fid, key, 8);
    close(fid);

    fclose(chat_file);
    puts("Server setup complete, please enter the server ID and password into a client process to connect!");
    return;
}

void cleanup(int sig_num){
    int err = shmctl(shrmem_id, IPC_RMID, 0); //free the shared memory segment
    if(chat_file != NULL) fclose(chat_file);
    remove(filename);
    exit(0);
}

int filter(char* mes){
    char* whitelist = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ .!?,\n\\";
    int fail = 1;
    int m_count = 0;
    if(strstr(mes, "flag") != NULL || strstr(mes, "sh") !=NULL) {
        fail = 0;
        goto done;
    }
    printf("FILTER STATUS: .");
    usleep(500000);
    printf("\rFILTER STATUS: ..");
    usleep(500000);
    printf("\rFILTER STATUS: ...");
    while(mes[m_count] != '\x00' && m_count < 0xe8){
        uint check = 0;
        for(int w=0; w<59; w++){
            if(mes[m_count] == whitelist[w]) check = 1;
        }
        if(!check){
            printf("restricted character: %c\n", mes[m_count]);
            fail = 0;
            goto done;
        }
        m_count++;
    }
    mes[m_count-2] = 0; //clear out the trailing new line
done:
    return fail;
}

char* parse_message(struct message* message_ptr){
    char* mes = malloc(0x400);
    strncpy(mes, message_ptr->message,0xe8);
    puts("Stating filter");
    int check = filter(mes);
    free(mes);
    printf("\r");
    if(check) {
        puts("FILTER STATUS: PASS");
        return message_ptr->message;
    } else {
        puts("FILTER STATUS: FAIL");
        return NULL;
    }
}

FILE* format_message(char* mes){
    char cmd[0x100];
    int b_read = sprintf(cmd, "echo '%s'", mes+5);
    //confirm that there was an additional newline and if there was over write it
    if(cmd[b_read-2] == 0xa){ 
        cmd[b_read-2] = cmd[b_read-1];
        cmd[b_read-1] = 0; 
    }
    FILE* fp = popen(cmd, "r");
    return fp; 
}

void server_loop(){
    struct message* blank_msg = malloc(sizeof(struct message));
    uint count = 1;
    char* mes;
    char* cmd_output = NULL;
    blank_msg->next = 0;
    int done = 0;
    while (1) {
        struct message* message_head = (struct message*)(shrmem_head+0x100); 
        sleep(2);
        if(memcmp(shrmem_head+0xf0, "\x01", 1) == 0){
            chat_file = fopen(filename, "a");
            if(chat_file == NULL) exit(-1);
            memcpy(shrmem_head, key, 8);
            for(int i = 2; i < 16; i++){
                if(message_head->next == 2){
                    mes = parse_message(message_head);
                    if(mes == NULL) {
                        goto clean_msg;
                    }
                    if(strncmp(mes,"!exit",5) == 0){
                        done = 1;
                        break;
                    }
                    if(strncmp(mes,"!fmt",4) == 0) {
                        FILE* fp = format_message(mes);
                        cmd_output = malloc(0xf0);
                        memset(cmd_output, 0, 0xf0);
                        int num_read = fread(cmd_output, 1, 0xe8, fp);
                        if(cmd_output[num_read-1] != 0xa) cmd_output[num_read-1] = 0xa;
                        pclose(fp);
                        mes = cmd_output;
                    }
                    fprintf(chat_file, "%s\n","----message start----");
                    fprintf(chat_file, "%s",message_head->name);
                    fprintf(chat_file, "%.232s",mes);
                    if(cmd_output != NULL) {
                        free(cmd_output);
                        cmd_output = NULL;
                    }
                    fprintf(chat_file, "%s\n","----message break----");
                }
clean_msg:
                message_head = (struct message*)(shrmem_head+(0x100*i));
                memset(shrmem_head+(0x100*(i-1)),0,0x100);
            }
            fclose(chat_file);
            chat_file = NULL;
            if(done) break;
            memset(shrmem_head+0xf0, 0, 1);
            memset(shrmem_head, 0, 8); 
        }
    }
    free(blank_msg);
    return;
}

int main(int argc, char* argv[]){
    setvbuf(stdin,(char*)0x0,2,0);
    setvbuf(stdout,(char*)0x0,2,0);
    setvbuf(stderr,(char*)0x0,2,0);
    setup();
    signal(SIGINT, cleanup);
    signal(SIGALRM, cleanup);
    alarm(120); //set an alarm to trigger after 2 minutes that cleans everything up
    server_loop();
    cleanup(0);
}
