#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char* filename(char *path){
    char *p = path + strlen(path);
    while (p >= path && *p != '/') {
        p--;
    }
    return p + 1;
}

void find(char *path,char *target){
    int fd;
    struct stat st;
    struct dirent di;
    char buf[512], *p;
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find: path too long\n");
        return;
    }
    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
     switch (st.type) {
        case T_FILE:
            if (strcmp(filename(path), target) == 0) {
                printf("%s\n", path);
            }
            break;
        case T_DIR:
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while (read(fd, &di, sizeof(di)) == sizeof(di)) {
                if (di.inum == 0 || strcmp(di.name, ".") == 0 || strcmp(di.name, "..") == 0)
                    continue;
                memmove(p, di.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf,target);
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "find: need 2 parameter\n");
        exit(1);
    }
    char* target = argv[2];
    find(argv[1],target);
    exit(0);
}