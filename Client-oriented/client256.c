#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/sem.h>

#define SIZE 1024
#define LEN 16

void handler(int sig){
        ;
}
int child(int, pid_t cpid[], int[]);
void parent(int, int, int p[4][2]);
int main(){
        char name[BUFSIZ];
        int i, total_time;
        int fd[2], p[4][2], fd0, fd1;
        struct sigaction pact, cact;
        pid_t pid, r_pid, w_pid, cpid[4];
        struct timeval start, end;

#ifdef TIMES
        gettimeofday(&start,NULL);
#endif
        fd0 = open("ionode256_0",O_RDWR|O_CREAT|O_TRUNC, 0644);
        fd1 = open("ionode256_1",O_RDWR|O_CREAT|O_TRUNC, 0644);

        for(i = 0; i < 4; i++){   // 이름있는 파이프 생성
                sprintf(name, "./CC%d-FIFO", i);
                if(mkfifo(name, 0666) == -1){
                        perror("mkfifo");
                        exit(1);
                }
        }

        pact.sa_handler = handler;
        sigaction(SIGUSR1, &pact, NULL);

        for(i=0;i<4;i++){

                if(pipe(fd) == -1){
                        perror("pipe");
                        exit(1);
                }
                if(pipe(p[i]) == -1){
                        perror("pipe i:");
                        exit(1);
                }

                pid = fork();
                if(pid == 0){
                        cact.sa_handler = handler;
                        sigaction(SIGUSR1, &cact, NULL);
                        close(fd[0]);

                        w_pid = getpid();
                        if(write(fd[1], &w_pid, sizeof(w_pid)) < 0)
                                perror("write:");

                        kill (getppid(), SIGUSR1);
                        pause();

                        child(i, cpid, p[i]);
                        exit(1);
                        break;

                }
                else if(pid > 0){
                        close(fd[1]);
                        pause();
                        read(fd[0], &r_pid, sizeof(r_pid));
                        cpid[i] = r_pid;
                        kill(pid, SIGUSR1);
                }
        }

        parent(fd0, fd1, p);

#ifdef TIMES
        gettimeofday(&end,NULL);
        total_time = end.tv_sec - start.tv_sec;
#endif
        printf("total time : %d\n",total_time);

        return 0;
}
void parent(int fd0, int fd1, int p[4][2]) {

        int buf[LEN*SIZE], ch;
        fd_set set, master;
        int i, j, k, data, flag;

        for (i=0; i<4; i++) close(p[i][1]);

        FD_ZERO(&master);
        for (i=0; i<4; i++) FD_SET(p[i][0], &master);

        while (set = master, select(p[3][0]+1, &set, NULL, NULL, NULL) > 0) {

                for (i=0; i<4; i++) {

                        if (FD_ISSET(p[i][0], &set)) {
                                if (read(p[i][0], buf, sizeof(buf)) > 0) {

                                        for(j = 0; j < LEN*SIZE; j++){
                                                data = buf[j];

                                                for(k = 0; k < 4*LEN; k++){
                                                        if(k*SIZE <= data && data < (k+1)*SIZE){
                                                                if(k%2 == 0) flag = 0;
                                                                else flag = 1;
                                                                break;
                                                        }
                                                }
                                                if(!flag){ // io node #0
                                                        lseek(fd0, (data - (k/2)*SIZE)*sizeof(int), SEEK_SET);
                                                        if(write(fd0, &data, sizeof(int)) == -1){
                                                                perror("write #0");
                                                        }
                                                }
                                                else{ // io node #1
                                                        lseek(fd1, data - ((k/2+1)*SIZE)*sizeof(int), SEEK_SET);
                                                        if(write(fd1 , &data, sizeof(int)) == -1){
                                                                perror("write #1");
                                                        }
                                                }


                                        }
                                }
                        }
                        if (waitpid (-1, NULL, WNOHANG) == -1) return;
                }
        }
}
int child(int n, pid_t cpid[4], int p[2]){
        int comfd, pip[4], combuf[LEN*SIZE], mbuf[LEN*SIZE], ybuf[4][(LEN/4)*SIZE];
        int readbuf[(LEN/4)*SIZE], rpbuf[(LEN/4)*SIZE];
        char name[BUFSIZ];
        int i, j, k, m, cnt[3];
        struct sigaction act, cact;
        pid_t r_pid[4], pid;

        cpid[n] = getpid();

        for(i = 0; i < 4; i++){
                sprintf(name, "./CC%d-FIFO", i);
                if(n == i){
                        if((pip[i] = open(name, O_RDONLY)) == -1){
                                perror("open pip[n]");
                                exit(1);
                        }
                }
                else{
                        if((pip[i] = open(name, O_WRONLY)) == -1){
                                perror("open pip[i]");
                                exit(1);
                        }
                }
        }

        act.sa_handler = handler;
        sigaction(SIGUSR1, &act, NULL);

        if(n == 3){
                sleep(1);
                for(i = 0; i < 4; i++)
                        r_pid[i] = cpid[i];

                for(i = 0; i < 3; i++){

                        if(write(pip[i],r_pid, sizeof(r_pid)) < 0)
                                perror("node 3 write:");

                        kill(cpid[i], SIGUSR1);
                        pause();
                }
        }
        else{
                pause();

                if(read(pip[n], r_pid, sizeof(r_pid)) < 0)
                        perror("from node 3 read failed:");

                kill(r_pid[3], SIGUSR1);
        }


        sprintf(name,"compute256_%d",n); // 파일 열기
        if((comfd = open(name,O_RDONLY,0644))==-1){
                perror("compute open");
                exit(1);
        }

        read(comfd, combuf, sizeof(combuf));
        for(i = 0; i < 3; i++) cnt[4] = 0;
        for(i = 0; i < LEN*SIZE; i++){

                m = combuf[i];
                if(0 <= m && m <= (LEN*SIZE - 1)){ // compute0
                        ybuf[0][cnt[0]++] = m;
                        if(n == 0)
                                mbuf[m%(LEN*SIZE)] = m;
                        }
                else if(LEN*SIZE <= m && m <= (2*LEN*SIZE -1)){ //compute1
                        ybuf[1][cnt[1]++] = m;
                        if(n == 1)
                                mbuf[m%(LEN*SIZE)] = m;
                        }
                else if(2*LEN*SIZE <= m && m <= (3*LEN*SIZE -1)){ //compute2
                        ybuf[2][cnt[2]++] = m;
                        if(n == 2)
                                mbuf[m%(LEN*SIZE)] = m;
                        }
                        else if(3*LEN*SIZE <= m && m <= (4*LEN*SIZE - 1)){ //compute3
                                ybuf[3][cnt[3]++] = m;
                                if(n == 3)
                                        mbuf[m%(LEN*SIZE)] = m;
                        }
        }

        if(n == 0){ // 이름있는 파이프에 각각 쓰기

                sleep(1);
                for(i = 0; i < 4; i++){
                       if(n != i){


                               if(write(pip[i], ybuf[i], sizeof(ybuf[i])) < 0)
                                       perror("node 0 write failed\n");


                                kill(r_pid[i], SIGUSR1);
                                pause();

                        }
                }
        }
        else{
                pause();

                if(read(pip[n], rpbuf, sizeof(rpbuf)) < 0)
                        perror("from node 0 read Failed:");

                for(i = 0; i < (LEN/4)*SIZE; i++){ // mbuf 정리
                        mbuf[rpbuf[i]%(LEN*SIZE)] = rpbuf[i];
                }

                kill(r_pid[0], SIGUSR1);
        }
         if(n == 1){
                sleep(1);

                for(i = 0; i < 4; i++){
                        if(n != i){
                                if(write(pip[i], ybuf[i], sizeof(ybuf[i])) < 0)
                                        perror("node 0 write failed\n");

                                kill(r_pid[i], SIGUSR1);
                                pause();
                        }
                }
        }
        else{
                pause();

                if(read(pip[n], rpbuf, sizeof(rpbuf)) < 0)
                        perror("from node 0 read Failed:");

                for(i = 0; i <  (LEN/4)*SIZE; i++){ // mbuf 정리
                        mbuf[rpbuf[i]%(LEN*SIZE)] = rpbuf[i];
                }

                kill(r_pid[1], SIGUSR1);
        }

        if(n == 2){
                sleep(1);

                for(i = 0; i < 4; i++){
                        if(n != i){
                                if(write(pip[i], ybuf[i], sizeof(ybuf[i])) < 0)
                                        perror("node 0 write failed\n");

                                kill(r_pid[i], SIGUSR1);
                                pause();
                        }
                }
        }
        else{
                pause();

                if(read(pip[n], rpbuf, sizeof(rpbuf)) < 0)
                        perror("from node 0 read Failed:");

                for(i = 0; i <  (LEN/4)*SIZE; i++){ // mbuf 정리
                        mbuf[rpbuf[i]%(LEN*SIZE)] = rpbuf[i];
                }

                kill(r_pid[2], SIGUSR1);
        }
        if(n == 3){
                sleep(1);

                for(i = 0; i < 4; i++){
                        if(n != i){
                                if(write(pip[i], ybuf[i], sizeof(ybuf[i])) < 0)
                                        perror("node 0 write failed\n");

                                kill(r_pid[i], SIGUSR1);
                                pause();
                        }
                }
        }
        else{
                pause();

                if(read(pip[n], rpbuf, sizeof(rpbuf)) < 0)
                        perror("from node 0 read Failed:");

                for(i = 0; i <  (LEN/4)*SIZE; i++){ // mbuf 정리
                        mbuf[rpbuf[i]%(LEN*SIZE)] = rpbuf[i];
                }

                kill(r_pid[3], SIGUSR1);
        }

        close(p[0]);
        write(p[1], mbuf, sizeof(mbuf));
        sleep(1);
        return 0;
}
