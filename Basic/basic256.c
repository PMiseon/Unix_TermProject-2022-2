#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

#define BUFSIZE 32*1024

int child(int fd0,int fd1, int i,int fd[2]);

int main(){

	int fd0,fd1,i;
	int buf0[BUFSIZE],buf1[BUFSIZE];
	int total_time;
	struct timeval start, end;
	pid_t pid;

	fd0 = open("ionode256_0",O_RDWR|O_CREAT|O_TRUNC, 0644);
	fd1 = open("ionode256_1",O_RDWR|O_CREAT|O_TRUNC, 0644);
	
#ifdef TIMES
	gettimeofday(&start,NULL);
#endif		
	int main_fd[2],check;
	for(i=0;i<4;i++){
		if(pipe(main_fd)==-1){
			perror("pipe_main");
			exit(1);
		}
		
		pid = fork();
		if(pid==0){
			check = child(fd0,fd1, i,main_fd);
			while(check!=1){
				
				close(main_fd[0]);
				write(main_fd[1],"ok",3);
				
			}
			exit(1);
		}
	}
#ifdef TIMES
	gettimeofday(&end,NULL);
	total_time = end.tv_usec - start.tv_usec;
#endif
	printf("total time : %d\n",total_time);

	exit(0);
	return 0;
}

int child(int fd0, int fd1, int i,int main_fd[2]){
	
	while(1){

		char tmp[3];
		int comfd,data,n;
		char name[BUFSIZ];
		off_t offset;
		pid_t pid;
		
		close(main_fd[1]);	
		read(main_fd[0],tmp,3);

		sprintf(name,"compute256_%d",i);
		if((comfd = open(name,O_RDONLY,0644))==-1){
			perror("compute open");
			exit(1);
		}
		
		// 여기서 fork 한번더 써서 i/o 노드연결
		int io_fd[2],flag=0;
		if(pipe(io_fd) == -1){
			perror("pipe");
			exit(1);
		}

		switch(pid=fork()){

			case -1:
				perror("fork");
				exit(1);
				break;
				
			case 0:
				close(io_fd[1]);
				while((n=read(io_fd[0],&data,sizeof(int)))>0){
					int k;
					for(k=0;k<=64;k++){
						if(k*1024 <= data && data <= ((k+1)*1024-1)){
							if(k%2==0)
								flag=0;
							else
								flag=1;
							break;
						} 
					}

					if(!flag){ 	// I/O node #0
						offset = lseek(fd0, (data-k/2*1024)*sizeof(int) ,SEEK_SET);
						if(write(fd0,&data,sizeof(int))==-1){
							perror("write #0");
							exit(1);
						}
					}else{		// I/O node #1
						offset = lseek(fd1, (data-(k/2+1)*1024)*sizeof(int), SEEK_SET);
						if(write(fd1,&data,sizeof(int))==-1){
							perror("write #1");
							exit(1);
						}
					}
			
				}
				exit(1);
				break;

			default:
				close(io_fd[0]);
				while((n=read(comfd,&data,sizeof(int)))>0){
					if(write(io_fd[1],&data,sizeof(int))==-1){
						perror("write");
						exit(1);
					}
				}
		
		}

		exit(1);
		return 1;
	
	}
}
