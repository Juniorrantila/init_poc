/*
 *  I don't really like how some put if-statements inside functions to   *
 *  test if they should be able to run or not, since this would mean     *
 *  that every time the function gets called, it unnecessary runs extra  *
 *  code.                                                                * 
 *  Here is a proof of concept of self-modifying code which handles this *
 *  by putting a jmp instruction at the start of each relevant function  *
 *  and replacing it with nop instructions once init() is executed.      *
 *                                                                       *
 *  Note: clang and gcc wont compile if optimize level is above 1.       *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

enum prot {
   none,
   READ  = (1<<0),
   WRITE = (1<<1),
   EXEC  = (1<<2),
   RW    = READ|WRITE,
   RWX   = READ|WRITE|EXEC
};

#ifdef _WIN32

   #include <Windows.h>
   
   #define CHECK_INIT(label, callback) \
       __asm__ __volatile__ (#label": jmp "#callback)
   
   int change_prot(void* addr, size_t size, int prot){
      DWORD old; 
      DWORD _prot = PAGE_EXECUTE_READWRITE;
      
      if (prot & RWX) {
          _prot = PAGE_EXECUTE_READWRITE;
      } else if (prot & RW){
          _prot = PAGE_READWRITE;
      } else if (prot & EXEC){
          _prot = PAGE_EXECUTE;
      }
      
      VirtualProtect(addr, size, _prot, &old);
      return 0;
   } 

#else // UNIX based
   
   #include <sys/mman.h>
   #include <unistd.h>

   #define CHECK_INIT(label, callback) \
       __asm__ __volatile__ ("_"#label": jmp _"#callback)

   int change_prot(void* addr, size_t size, int prot){
      size_t page_size = getpagesize();
	   void* page_start = (void*)((size_t)addr & -page_size);
	   int len = (int)((size_t)addr + size-(size_t)page_start);
     
      int _prot = 0;
      if (prot & READ)  _prot |= PROT_READ;
      if (prot & WRITE) _prot |= PROT_WRITE;
      if (prot & EXEC)  _prot |= PROT_EXEC;
      if (mprotect(page_start, len, _prot))
         return 1;
      return 0;
   } 

#endif // UNIX based


// Replaces function with return instruction
#define PREMATURE_RETURN(label, retval) do {                         \
    __asm__ __volatile__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop"); \
    __asm__ __volatile__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop"); \
    change_prot(label, 4096, RW);                                    \
    ((char*) label)[0]  = 0x48;                                      \
    ((char*) label)[1]  = 0xb8;                                      \
    int64_t* val_p      = (int64_t*) &((char*) label)[2];            \
    *val_p              = (int64_t) retval;                          \
    ((char*) label)[10] = 0xC3;                                      \
    change_prot(label, 4096, EXEC);                                  \
} while(0)

int init();
int run();
int do_something();
_Noreturn void check_run();
_Noreturn void check_do_something();
_Noreturn void err_no_init();

char* funcs[] = {(char*)check_run, (char*)check_do_something, NULL};

int main(){
    init();
    run();
    do_something();
}

int init(){
    // do stuff on init
    
    for (uint8_t i = 0; funcs[i] != NULL; i++){
        change_prot(funcs[i], 4096, RW);
        // nop instructions
        funcs[i][0] = 0x90;
        funcs[i][1] = 0x90;
        #ifdef __clang__ // clang uses 5 byte jmp
        funcs[i][2] = 0x90;
        funcs[i][3] = 0x90;
        funcs[i][4] = 0x90;
        #endif // clang
        change_prot(funcs[i], 4096, EXEC);
    }
    PREMATURE_RETURN(init, -1);
    
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
