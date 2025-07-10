#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char *argv[]){
    char buf[64],*xargs[32];
    for(int i=1;i<argc;i++){
        xargs[i-1]=argv[i];
    }
    int base_args = argc - 1;

    int n;
    while((n = read(0, buf, sizeof(buf))) > 0) {
        int m = 0;
        while(m < n) {
            int start = m;
            while(m < n && buf[m] != '\n') {
                m++;
            }
            buf[m] = 0; // 将换行符替换为字符串结束符
            xargs[base_args] = buf + start; 
            xargs[base_args + 1] = 0; // 确保 args 以 NULL 结尾
            
            if(fork() == 0) { 
                exec(xargs[0], xargs); 
                fprintf(2, "exec failed\n");
                exit(1); 
            } else {
                wait(0); 
            }
            m++;
        }
    }

    exit(0);
}