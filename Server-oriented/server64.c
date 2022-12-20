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

#define SIZE 16


int main() {

	char name[100];
	int i, j, total_time;
	pid_t pid;
	struct timeval start, end;
	int data;

	int s2c[4][2]; //server to computeNode
	int c2s[4][2]; //computeNode to server

	int io0 = open("ionode64_0", O_RDWR | O_CREAT | O_TRUNC, 0644);
	int io1 = open("ionode64_1", O_RDWR | O_CREAT | O_TRUNC, 0644);

#ifdef TIMES
	gettimeofday(&start, NULL);
#endif

	for (i = 0; i < 4; i++) {

		if (pipe(s2c[i]) == -1) {
			perror("s2cpd");
			exit(1);
		}
		if (pipe(c2s[i]) == -1) {
			perror("c2spd");
			exit(1);
		}

	}
	
	switch (pid = fork()) {

	case -1:
		perror("fork");
		exit(1);
		break;

	case 0: //child
		for (i = 0; i < 4; i++) {
			close(s2c[i][1]);
			close(c2s[i][0]);
		}

		for (i = 0; i < SIZE; i++) {

			for (j = i * 1024; j < (i + 1) * 1024; j++) {
				sprintf(name, "compute64_%d", j % 4);
				int computeNode = open(name, O_RDONLY);
				int data;
				lseek(computeNode, (j / 4) * sizeof(int), SEEK_SET);
				read(computeNode, &data, sizeof(int));
				write(c2s[j % 4][1], &data, sizeof(int));
				close(computeNode);
			}

		}

		exit(0);
		break;


	default: //parent
		for (i = 0; i < 4; i++) {
			close(s2c[i][0]);
			close(c2s[i][1]);
		}

		for (i = 0; i < SIZE; i++) {


			int IOBLOCK[1024] = { 0 };
			for (j = 0; j < 1024; j++) {
				int data;
				read(c2s[j % 4][0], &data, sizeof(int));
				IOBLOCK[j] = data;
			}


			if (i % 2 == 0) { //->ionode#0
				lseek(io0, (i / 2) * 1024 * sizeof(int), SEEK_SET);
				if (write(io0, IOBLOCK, sizeof(int) * 1024) == -1) {
					perror("write ionode");
					exit(1);
				}

			}


			else { //i%2 ==1  -> ionode#1
				lseek(io1, (i / 2) * 1024 * sizeof(int), SEEK_SET);
				if (write(io1, IOBLOCK, sizeof(int) * 1024) == -1) {
					perror("write ionode");
					exit(1);
				}
			}

		}

		break;

	}


#ifdef TIMES
	gettimeofday(&end, NULL);
	total_time = end.tv_usec - start.tv_usec;
#endif
	printf("total time : %d\n", abs(total_time));

}

