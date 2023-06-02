#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

typedef void (*Printer)(void*);

struct Review {
    struct Review* next;
    Printer print_func;
    size_t length;
    char* text;
};

struct Rated_Review{
    struct Review* next;
    Printer print_func;
    size_t length;
    size_t rating;
    char* text;
};

//struct ops {
//    void* on_delete;
//};

struct Movie {
    char name[0x20];
    struct Movie* next;
    struct Movie* previous;
    struct Review* review_head;
    ulong num_revs;
//    struct ops operations;
};


struct Movie* head = NULL;
uint count;

void prompt(char* text){
    puts(text);
    printf("> ");
    //fflush(stdin);
    //fflush(stdout);
}

void win(){
    char flag[0x100];
    FILE* fp = fopen("/flag", "r");
    fread(flag, 1,0x100,fp);
    printf("%s\n",flag);
    return;
}

struct Movie* get_movie_at(ulong index){
    struct Movie* current = head;
    index--;
    while((int)index > 0){
        current = current->next;   
        index--;
    }
    if(current == NULL){
        puts("No movies allocated");
        return NULL;
    }
    return current;
}

struct Review* get_review_at(struct Review* current, ulong index){
    for(int i = 0; i < (int)index && current != NULL; i++){
        current = current->next;
    }
    if(current == NULL){
        puts("No reviews allocated");
        return NULL;
    }
    return current;
}

struct Movie* unlink_movie(ulong index){
    struct Movie* to_remove = get_movie_at(index);
    if(to_remove == NULL) return NULL;
    struct Movie* to_ret = to_remove->next;
    struct Movie* prev = to_remove->previous;
    
    //free the reviews
    struct Review* current_rev = to_remove->review_head;
    struct Review* next_rev;
    while(current_rev != NULL){
        next_rev = current_rev->next;
        free(current_rev);
        current_rev = next_rev;
    }

    //if the movie that is currently being removed is the head, need to do some special things
    if(prev == NULL){
        head = to_ret;
        to_ret->previous = NULL;
        free(to_remove);
        count--;
        return head;
    }

    if(to_ret == NULL){
        prev->next = NULL;
        free(to_remove);
        count--;
        return prev;
    }

    prev->next = to_ret;
    to_ret->previous = prev;
    
    free(to_remove);
    to_remove = 0;
    count--;
    return to_ret;
}

void unlink_review(struct Review* previous_rev){
    if(previous_rev->next == NULL){
        free(previous_rev);
        return;
    }
    struct Review* to_remove = previous_rev->next;
    previous_rev->next = to_remove->next;
    free(to_remove);
    return;
}

size_t read_int(){
    size_t to_ret;
    fscanf(stdin, "%zu", &to_ret);
    return to_ret;
}

void* create_movie(){
    struct Movie* new_movie = (struct Movie*)calloc(1,sizeof(struct Movie));
    memset(new_movie, 0, 0x20);
    if(head == NULL){
        new_movie->next = NULL;
        new_movie->previous = NULL;
        head = new_movie;
    } else {
        struct Movie* current = head;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = new_movie;
        new_movie->previous = current;
        new_movie->next = 0;
    }
    prompt("Enter the move title, max length of 31");
    read(0, new_movie->name, 0x1f);
    count++;
    new_movie->review_head = NULL;
    new_movie->num_revs = 0;
    return new_movie;
}

struct Review* get_back(struct Review* base){
    while(base->next != NULL){
        base = base->next;
    }
    return base;
}

void print_review_rated(void* first){
    struct Rated_Review* rev = (struct Rated_Review*)first;
    printf("\trating: %zu\n\treview: ", rev->rating);
    write(1,(void *)rev+0x20,rev->length);
    puts("");
}

void print_review_unrated(void* first){
    struct Review* rev = (struct Review*)first;
    printf("\treview: ");
    write(1,(void *)rev+0x18,rev->length);
    //fwrite((void *)rev+0x18, 1, rev->length, stdout);
    puts("");
}

void* add_review(struct Movie* selection){
    //puts("here");
    int count = 0;
    char answer;
    char b[4];
    struct Review* new_review;
    prompt("Please enter the length of the review");
    ulong length = read_int()&0xFFF8;
    //puts((size_t)length);
    if(length > 0x200){
        puts("too big, exiting");
        exit(420);
    }

    prompt("Would you like to give a rating? (y/n)");
    scanf(" %c", &answer);
    getchar(); //clear out the floating newline
    if(answer == 'y'){
        prompt("please enter a rating (1-5)");
        ulong rating = read_int(); 
        struct Rated_Review* temp = (struct Rated_Review*)calloc(1,sizeof(struct Rated_Review)+length);
        temp->rating = rating;
        temp->length = length;
        temp->print_func = *print_review_rated;
        prompt("Enter your review");
        read(0, (void *)temp+0x20, (ulong)length);
        new_review = (struct Review*)temp;
    } else {
        new_review = (struct Review*)calloc(1,sizeof(struct Review)+length);
        new_review->length = length;
        prompt("enter your review");
        read(0, (void *)new_review+0x18, (ulong)length);
        new_review->print_func = *print_review_unrated;
    }
    if(selection->review_head == NULL){
        puts("creating new head");
        selection->review_head = new_review;
        new_review->next = NULL;
    } else {
        puts("adding to the back");
        struct Review* previous = get_back(selection->review_head);
        previous->next = new_review;
        new_review->next = NULL;
    }
    selection->num_revs++;
    //prompt("please enter the review score");
    return new_review;
}

void print_movie(struct Movie* to_print){
    printf("title: %s\n", to_print->name);
    if(to_print->review_head == NULL) return;
    struct Review* cur_rev = to_print->review_head;
    for(int i = 0; i < to_print->num_revs; i++){
        printf("-----review %u-----\n", i+1);
        (* (cur_rev->print_func))((void*)cur_rev);
        cur_rev = cur_rev->next;
    }
}


void edit_movie(struct Movie* to_edit){
    memset(to_edit->name, 0, 0x20);
    prompt("Please enter a new title");
    read(0, to_edit->name, 0x1f);
}

void edit_review(struct Review* to_edit){
    prompt("enter a new review");
    if(to_edit->print_func == *print_review_unrated){
        read(0, (void*)to_edit+0x18, to_edit->length);
    } else {
        read(0, (void*)to_edit+0x20, to_edit->length);
    }
    return;
}

size_t menu(){
    puts("1. Create Movie");
    puts("2. Edit Movie");
    puts("3. Print Movie");
    puts("4. Delete Movie");
    puts("5. Print Movies");
    puts("6. exit");
    prompt("Enter your selection");
    return read_int();
}

size_t menu2(){
    puts("1. change title");
    puts("2. create review");
    puts("3. edit review");
    puts("4. delete review");
    prompt("Enter your selection");
    return read_int();
}

void edit_movie_menu(struct Movie* to_edit){
    printf("Editing movie: %s\n", to_edit->name);
    ulong choice = menu2();
    switch(choice){
        case 1:
            edit_movie(to_edit);
            break;
        case 2:
            add_review(to_edit);
            break;
        case 3:
            prompt("enter the index of the review you would like to edit");
            struct Review* selected_review = get_review_at(to_edit->review_head, read_int()-1);
            if(selected_review == NULL) break;
            edit_review(selected_review);
            break;
        case 4:
            prompt("enter the index of the review you would like to delete");
            struct Review* selection = get_review_at(to_edit->review_head, read_int()-1);
            if(selection == NULL) break;
            unlink_review(selection);
            to_edit->num_revs--;
            if(to_edit->num_revs == 0){
                to_edit->review_head = NULL;
            }
            break;
    }
}

void setup(){
    setvbuf(stdin,(char*)0x0,2,0);
    setvbuf(stdout,(char*)0x0,2,0);
    setvbuf(stderr,(char*)0x0,2,0);
    return;
}

int main(){
    setup();
    while(1){
        ulong choice = menu();
        switch(choice){
            case 1:
                create_movie();
                break;
            case 2:
                prompt("enter the index of the movie you would like to edit");
                struct Movie* to_edit = get_movie_at(read_int());
                if(to_edit == NULL) break;
                edit_movie_menu(to_edit);
                break;
            case 3:
                prompt("enter the index of the movie you would like to print");
                struct Movie* to_print = get_movie_at(read_int());
                if(to_print == NULL) break;
                print_movie(to_print);
                break;
            case 4:
                prompt("enter the index of the movie you would like to remove");
                unlink_movie(read_int());
                break;
            case 5:
                ;
                ulong temp_index = 1;
                struct Movie* current_movie = head;
                puts("---------------------------------------");
                while(current_movie != NULL){
                    printf("Movie at index: %lu\n", temp_index);
                    puts("---------------------------------------");
                    print_movie(current_movie);
                    puts("---------------------------------------");
                    current_movie = current_movie->next;
                    temp_index++;
                }
                break;
            case 6:
                exit(0);
                break;
            default:
                puts("invalid entry, exiting");
                exit(1);
                break;
        }
    }
}

