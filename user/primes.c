#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void seive(int *left_pipe) {
    int prime;
    if (read(left_pipe[0], &prime, sizeof(int)) != sizeof(int)) {
        close(left_pipe[0]);
        return;
    }

    printf("prime %d\n",prime);

    int right_pipe[2];
    pipe(right_pipe);

    if (fork() == 0) {
        // 子进程：继续筛选
        close(right_pipe[1]);
        close(left_pipe[0]);
        seive(right_pipe);
        exit(0);
    }
    else{
        // 父进程：过滤当前 prime 的倍数，写入 right_pipe
        close(right_pipe[0]);
        int num;
        while (read(left_pipe[0], &num, sizeof(int)) == sizeof(int)) {
            if (num % prime != 0) {
                write(right_pipe[1], &num, sizeof(int));
            }
        }

        close(left_pipe[0]);
        close(right_pipe[1]);

        wait(0);
        exit(0);
    }
}


int main(int argc, char *argv[]) {
    int left_pipe[2];
    pipe(left_pipe);

    if(fork()==0){
        close(left_pipe[1]);
        seive(left_pipe);
        exit(0);
    }
    else{
        close(left_pipe[0]);
        for(int i=2;i<=35;i++){
            write(left_pipe[1],&i,sizeof(int));
        }
        close(left_pipe[1]);
        wait(0);
        exit(0);
    }
}