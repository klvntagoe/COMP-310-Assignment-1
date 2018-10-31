#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <sched.h>
#include <wordexp.h>
#include <time.h>
#include <fcntl.h>
#include <linux/sched.h>
#include <sys/wait.h>

char pipeName[512] = {'\0'};
int pipeMode;

int my_system(char* line);

int childFunc(void *command);

int main(int argc, char *argv[]){
	long startTime, endTime, difference;
	struct timespec st, et;
	char buff[512];
	#ifdef PIPE
		strcpy(pipeName, argv[1]);
		pipeMode = atoi(argv[2]);
	#endif
	do{
		fgets(buff, 512, stdin);
		buff[strlen (buff) -1] = '\0';

		clock_gettime(CLOCK_REALTIME, &st);

		my_system(buff);

		clock_gettime(CLOCK_REALTIME, &et);
		startTime = (st.tv_sec * 1000000000) + st.tv_nsec;
		endTime = (et.tv_sec * 1000000000) + et.tv_nsec;
		difference = endTime - startTime;
		printf("\nThis process took %ld ns\n\n", difference);
	}while (strlen(buff) > 1 || strcmp(buff, "\n") == 0);
	return 0;
}



int my_system(char* line){
	#ifdef FORK
		int *status;
		pid_t pid;
		pid = fork();
		if (pid == -1){
			perror("Unable to create child process\n");
			exit(1);
		}else if (pid == 0){	//Child process
			execl("/bin/sh", "sh", "-c", line, (char *) NULL);
		}else{	//Parent process
			waitpid(pid, status, 0);
		}
	#elif VFORK
		int *status;
		pid_t pid;
		pid = vfork();
		if (pid == -1){
			perror("Unable to create child process");
			exit(1);
		}else if (pid == 0){	//Child process
			execl("/bin/sh", "sh", "-c", line, (char *) NULL);

		}else{	//Parent process
			waitpid(pid, status, 0);
		}
	#elif CLONE
		int *status;
		pid_t pid;
		void *stack, *stackTop;
		stack = malloc( 1024 * 1024 );
		if (stack == NULL){
			perror("Unable to allocate memory<n");
			exit(1);
		}
		stackTop = stack + (1024 * 1024);
		pid = clone( childFunc, stackTop, SIGCHLD | CLONE_FS, (void *) line);
		if (pid < 0){
			perror("Unable to clone a process\n");
			exit(1);
		}
		waitpid(pid, status, 0);
		free(stack);
	#elif PIPE
		int fd, *status;
		pid_t pid;
		pid = fork();
		if (pid == -1){
			perror("Unable to create child process\n");
			exit(1);
		}else if (pid == 0){	//Child process
			if (pipeMode == 1){	//WRITING TO THE BUFFER
				close(1);
				fd = open(pipeName, O_WRONLY);
				//dup(fd);
				// dup2(fd, 1);
			}else if (pipeMode == 0){	//READING FROM THE BUFFER
				close(0);
				fd = open(pipeName, O_RDONLY);
				//dup(fd);
				// dup2(fd, 0);
			}else{
				perror("Invalid input/output arguement.");
			}
			execl("/bin/sh", "sh", "-c", line, (char *) NULL);
		}else{	//Parent process
			waitpid(pid, status, 0);
		}
	#else
		system(line);
	#endif
	return 0;
}



int childFunc(void *command){
	execl("/bin/sh", "sh", "-c", command, (char *) NULL);
	return 0;
}