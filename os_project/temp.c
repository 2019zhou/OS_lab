#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define LONG_SIZE 8

void getdata(pid_t child,long addr,char *str,long len)
{
    char *laddr = str;
    int i = 0, j = len/LONG_SIZE;
    union u{
        long int val;
        char chars[LONG_SIZE];
    }data;
    while(i<j)
    {
        data.val = ptrace(PTRACE_PEEKDATA,child,addr + i*LONG_SIZE,NULL);
        memcpy(laddr,data.chars,LONG_SIZE);
        laddr += LONG_SIZE;
        ++i;
    }
    j = len % LONG_SIZE;
    if(j != 0)
    {
        data.val = ptrace(PTRACE_PEEKDATA,child,addr + i*LONG_SIZE,NULL);
        memcpy(laddr,data.chars,j);
    }
}

int main()
{
    pid_t child;
    child = fork();
    if(child == 0)
    {
        ptrace(PTRACE_TRACEME,0,NULL,NULL);
        execl("/bin/ls","ls",NULL);
    }
    else
    {
        long orig_rax = 0;
        long params[3] = {0};
        int status = 0;
        char *str;
        int toggle = 0;
        while(1)
        {
            wait(&status);
            if(WIFEXITED(status))
                break;
            orig_rax = ptrace(PTRACE_PEEKUSER,child,8*ORIG_RAX,NULL);
            if(orig_rax == SYS_write)
            {
                if(toggle == 0)
                {
                    toggle = 1;
                    params[0] = ptrace(PTRACE_PEEKUSER,child,8*RDI,NULL);
                    params[1] = ptrace(PTRACE_PEEKUSER,child,8*RSI,NULL);
                    params[2] = ptrace(PTRACE_PEEKUSER,child,8*RDX,NULL);
                    //printf("make write call params %ld, %ld, %ld\n",params[0],params[1],params[2]);
                    str = (char*)malloc(params[2]+1);
                    memset(str,0,params[2]+1);
                    getdata(child,params[1],str,params[2]);
                    printf("get str: %s\n",str);
                    free(str);
                }
                else
                {
                    toggle = 0;
                }
            }
            ptrace(PTRACE_SYSCALL,child,NULL,NULL);
        }
    }
    return 0;
}
