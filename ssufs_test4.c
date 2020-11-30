#include "ssufs-ops.h"

int main()
{
    char str[25][300];
    int fd[MAX_OPEN_FILES];
    char buf[300];
    memset(buf,0,sizeof(buf));
    ssufs_formatDisk();

    for(int i=0;i<MAX_OPEN_FILES;i++){
        if(i<10)
            sprintf(str[i],"!-----%d 32 Bytes of Data-------!!-------32 Bytes of Data-------!",i);
        else
            sprintf(str[i],"!----%d 32 Bytes of Data-------!!-------32 Bytes of Data-------!",i);
        sprintf(buf,"f%d.txt",i%NUM_INODES);
        ssufs_create(buf);
        fd[i] = ssufs_open(buf);
    }
    ssufs_dump();


    for(int i=0;i<200;i++){
        printf("on fd%d.. Write Data: %d\n", fd[i%MAX_OPEN_FILES] ,ssufs_write(fd[i%MAX_OPEN_FILES], str[i%MAX_OPEN_FILES], 24));
       // if(i%10==0)
            ssufs_dump();
    }


  /*  for(int i=0;i<60;i++){
        printf("%d Seek: %d\n", i%MAX_OPEN_FILES, ssufs_lseek(fd[i%MAX_OPEN_FILES], -40));
        printf("%d Read : %d\n",i,ssufs_read(fd[i%MAX_OPEN_FILES],buf,24));
        printf("Read Data: %s\n", buf);
    }
    ssufs_dump();
    for(int i=0;i<MAX_OPEN_FILES;i++){
        sprintf(buf,"f%d.txt",i);
        ssufs_delete(buf);
        ssufs_dump();
    }*/

}
