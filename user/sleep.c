#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc != 2) { 
        fprintf(2,"sleep: need a parameter\n");
        exit(1); 
    }

    int time = atoi(argv[1]); // 将字符串转换为整数
    sleep(time); 
    exit(0); 
}