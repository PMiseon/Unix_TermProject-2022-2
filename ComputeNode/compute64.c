#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFSIZE 4* 1024


int main(void){

        int fd0, fd1, fd2, fd3;
        int buf0[BUFSIZE],buf1[BUFSIZE],buf2[BUFSIZE],buf3[BUFSIZE];

        fd0 = open("compute64_0",O_RDWR|O_CREAT|O_TRUNC,0644);
        fd1 = open("compute64_1",O_RDWR|O_CREAT|O_TRUNC,0644);
        fd2 = open("compute64_2",O_RDWR|O_CREAT|O_TRUNC,0644);
        fd3 = open("compute64_3",O_RDWR|O_CREAT|O_TRUNC,0644);

        int i;
        for(i=0;i<BUFSIZE;i++){
                buf0[i] = i*4 +0;
                buf1[i] = i*4 +1;
                buf2[i] = i*4 +2;
                buf3[i] = i*4 +3;
        }

        write(fd0,buf0,sizeof(buf0));
        write(fd1,buf1,sizeof(buf1));
        write(fd2,buf2,sizeof(buf2));
        write(fd3,buf3,sizeof(buf3));

}
