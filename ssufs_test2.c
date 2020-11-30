#include "ssufs-ops.h"

int main()
{
    char str[] = "!-------32 Bytes of Data-------!!-------32 Bytes of Data-------!!-------32 Bytes of Data-------!";
    char buf[300];
    memset(buf,0,sizeof(buf));
    ssufs_formatDisk();

    ssufs_create("f1.txt");
    int fd1 = ssufs_open("f1.txt");

    for(int i=0;i<20;i++){
        printf("Write Data: %d\n", ssufs_write(fd1, str, 72));
        ssufs_dump();
        
    }
    
    printf("Seek: %d\n", ssufs_lseek(fd1, -240));
    for(int i=0;i<20;i++){
        printf("%d Read : %d\n",i,ssufs_read(fd1,buf,72));
        printf("Read Data: %s\n", buf);
    }
    ssufs_dump();
    ssufs_delete("f1.txt");
    ssufs_dump();
}
