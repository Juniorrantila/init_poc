#include <stdio.h>
#include <Windows.h>
#include <stdint.h>

#define CHECK_INIT(label, callback) asm (#label": jmp "#callback)

#define DESTROY(label, ret) do {                                 \
    DWORD old;                                                   \
    VirtualProtect(label, 4096, PAGE_EXECUTE_READWRITE, &old);   \
    ((char*) label)[0]  = 0x48;                                  \
    ((char*) label)[1]  = 0xb8;                                  \
    int64_t* val_p      = (int64_t*) &((char*) label)[2];        \
    *val_p              = (int64_t) ret;                         \
    ((char*) label)[10] = 0xC3;                                  \
    VirtualProtect(label, 4096, old, &old);                      \
} while(0)

int init();
int run();
int do_something();
_Noreturn void check_run();
_Noreturn void check_do_something();
_Noreturn void err_no_init();

#define BYTES(thing) (char*)(thing)
char* init_funcs[] = {BYTES(check_run), BYTES(check_do_something), NULL};

int main(){
    init();
    run();
    do_something();
}

int init(){
    
    // do stuff on init
    
    for (uint8_t i = 0; init_funcs[i] != NULL; i++){
        DWORD old;
        VirtualProtect(init_funcs[i], 4096, PAGE_EXECUTE_READWRITE, &old);
        
        init_funcs[i][0] = 0x90; // nop
        init_funcs[i][1] = 0x90;
        init_funcs[i][2] = 0x90;
        init_funcs[i][3] = 0x90;
        init_funcs[i][4] = 0x90;
        
        VirtualProtect(init_funcs[i], 4096, old, &old);
    }
    DESTROY(init, -1);
    
    return 0;
}

int run(){
    CHECK_INIT(check_run, err_no_init);
    printf("Running\n");
    return 0;
}

int do_something(){
    CHECK_INIT(check_do_something, err_no_init);
    printf("Doing something\n");
    return 0;
}

_Noreturn void err_no_init(){
    printf("Err: init() has not been run!\n");
    exit(-1);
}