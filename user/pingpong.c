#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p1[2], p2[2];  // 两个管道，p1用于父进程向子进程发送数据，p2用于子进程向父进程发送数据
    char buffer[4];
    pipe(p1);
    pipe(p2);

    if(fork()==0){ //子进程
        read(p1[0],buffer,4);
        printf("%d: received %s\n",getpid(),buffer);
        write(p2[1],"pong",strlen("pong"));
        exit(0);
    }
    else{ //父进程
        write(p1[1],"ping",strlen("ping"));
        wait(0);
        read(p2[0],buffer,4);
        printf("%d: received %s\n",getpid(),buffer);
        exit(0);
    }
    
}