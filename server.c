/* \file server.h
 * \author Jesse Joseph
 * \email jjoseph@hmc.edu
 * \ID 040161840
 */
#include "server.h"
#define TIMEOUT_ 20
#define BUFFERSIZE_ 512

int pipes[2][2];
#define FDIN_ 0
#define FDOUT_ 1
#define FDERR_ 2
#define PARENT_READ_PIPE_ 0
#define PARENT_WRITE_PIPE_ 1
#define PARENT_READ_FD_ (pipes[PARENT_READ_PIPE_][FDIN_])
#define PARENT_WRITE_FD_ (pipes[PARENT_WRITE_PIPE_][FDOUT_])
#define CHILD_READ_FD_ (pipes[PARENT_WRITE_PIPE_][FDIN_])
#define CHILD_WRITE_FD_ (pipes[PARENT_READ_PIPE_][FDOUT_])

struct termios oldTerm;
int terminalModeChanged = 0;
pid_t pid;
int pidStatus;
int newsockfd;

int main(int argc, char **argv) {
  int encrypt, port, sockfd;
	socklen_t clientLength;
	struct sockaddr_in serverAddress, clientAddress;

  /* Parse arguments with getopt_long(4).
   * Valid arguments: --encrypt, --port=
   */
  char opt;
  int optind;
  struct option longOptions[] = {
    {"port", required_argument, 0, 'p'},
		{"encrypt", optional_argument, &encrypt, 'e'},
    {0, 0, 0, 0}};
	while((opt = getopt_long(argc, argv, "p:e", longOptions, &optind)) != -1) {
		switch(opt) {
			case 'p':
				port = atoi(optarg);
				break;
			case 'e':
				encrypt = 1;
				break;
			case '?':
				exit(1);
			case 0:
				break;
			default:
				break;
    }
  }

  /* Save the old terminal settings */
  if(tcgetattr(FDIN_, &oldTerm) != 0) {
    perror("In call to tcgetattr(FDIN_, &oldTerm):\nFailed to save original terminal settings.\n");
    exit(1);
  }

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("In call to socket(AF_INET, SOCK_STREAM, 0):\nFailed to open socket.\n");
		exit(1);
	}

	bzero((void *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(port);
  if(bind(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
		perror("In call to bind(sockfd, &serverAddress, sizeof(serverAddress)):\nFailed to bind to address.\n");
		exit(1);
	}
	if(listen(sockfd, 3) < 0){
		perror("In call to listen(sockfd, 3):\nFailed to prepare socket to accept connections.\n");
		exit(1);
	}
	clientLength = (socklen_t ) sizeof(clientAddress);

	/* Blocks until a connection is made */
	newsockfd = accept(sockfd, (struct sockaddr *) &clientAddress, &clientLength);
	if(newsockfd < 0) {
		perror("In call to accept(sockfd, (struct sockaddr *) &clientAddress, $clientLength):\nFailed to open socket.\n");
		exit(1);
	}
	if(close(sockfd) < 0) {
		perror("In call to close(sockfd):\nFailed to close connection to socket.\n");
		exit(1);
	}
	serve(newsockfd);
	if(close(newsockfd) < 0) {
		perror("In call to close(nesockfd):\nFailed to close connection to socket.\n");
		exit(1);
	}
	return 0;
}

void serve(int sockfd) {
/* Build our two pipes */
	pipe(pipes[PARENT_READ_PIPE_]);
	pipe(pipes[PARENT_WRITE_PIPE_]);
	
	pid = fork();
	if(pid < 0) {
		perror("In call to fork():\nFailed to fork process.\n");
		exit(1);
	}
	else if (pid == 0) {
		/* child */
		execShell();
	}
	else {
		/* parent */
		/* Set stdin and stdout to be the socket */
		if(dup2(sockfd, FDIN_) < 0) {
			perror("In call to dup2(sockfd, FDIN_):\nIn server process: Failed to redirect stdin to the socket.\n");
			exit(1);
		}
		if(dup2(sockfd, FDOUT_) < 0) {
			perror("In call to dup2(sockfd, FDOUT_):\nIn server process: Failed to redirect stdout to the socket.\n");
			exit(1);
		}
		if(dup2(sockfd, FDERR_) < 0) {
			perror("In call to dup2(sockfd, FDOUT_):\nIn server process: Failed to redirect stdout to the socket.\n");
			exit(1);
		}
		communicateWithShell();
	}
}

void execShell() {
  /* Child process */
  /* Connect the two pipes to the child's input and output. */
  if(dup2(CHILD_READ_FD_, FDIN_) < 0) {
    perror("In call to dup2(CHILD_READ_FD_, FDIN_):\nFailed to connect pipe to child's input.\n");
    exit(1);
  }
  if(dup2(CHILD_WRITE_FD_, FDOUT_) < 0) {
    perror("In call to dup2(CHILD_WRITE_FD_, FDOUT_):\nFailed to connect pipe to child's output.\n");
    exit(1);
  }
	if(dup2(CHILD_WRITE_FD_, FDERR_) < 0) {
    perror("In call to dup2(CHILD_WRITE_FD_, FDERR_):\nFailed to connect pipe to child's output.\n");
    exit(1);
  }
  /* Close file descriptors unneeded by child process */
  if(close(CHILD_READ_FD_) < 0) {
    perror("In call to close(CHILD_READ_FD_):\nFailed to close child's old input port.\n");
    exit(1);
  }
  if(close(CHILD_WRITE_FD_) < 0) {
    perror("In call to close(CHILD_WRITE_FD):\nFailed to close child's old output port.\n");
    exit(1);
  }
  if(close(PARENT_READ_FD_) < 0) {
    perror("In call to close(PARENT_READ_FD_):\nFailed to close parent's old input port.\n");
    exit(1);
  }
  if(close(PARENT_WRITE_FD_) < 0) {
    perror("In call to close(PARENT_WRITE_FD_):\nFailed to close parent's old output port.\n");
    exit(1);
  }
  char *args[] = {"/bin/bash", NULL};
  if(execv("/bin/bash", args) < 0) {
    perror("In call to execv(\"/bin/bash\", args):\nFailed to execute program bash.\n");
    exit(1);
  }
}

void communicateWithShell() {
  /* Parent process */
  signal(SIGCHLD, signalHandler);
	signal(SIGPIPE, signalHandler);
  signal(SIGINT, SIG_IGN);
  if(close(CHILD_READ_FD_) < 0) {
    perror("In call to close(CHILD_READ_FD_):\nFailed to close child pipe input port.\n");
    exit(1);
  }
  if(close(CHILD_WRITE_FD_) < 0) {
    perror("In call to close(CHILD_WRITE_FD_):\nFailed to close child pipe output port.\n");
    exit(1);
  }

  pthread_t readThread;
  pthread_t writeThread;
  if(pthread_create(&writeThread, NULL, forwardDataToShell, NULL) < 0) {
    perror("In call to pthread_create(%writeThread, NULL, forwardDataToShell, &PARENT_WRITE_FD_):\nFailed to create thread.\n");
    exit(1);
  }
  if(pthread_create(&readThread, NULL, readBytesFromShell, NULL) < 0) {
    perror("In call to pthread_create(&readThread, NULL, readBytesFromShell, &PARENT_READ_FD):\nFailed to create thread.\n");
    exit(1);
  }
  if(pthread_join(writeThread, NULL) != 0) {
    perror("In call to pthread_join(writeThread, NULL):\nThread did not terminate successfully.\n");
    exit(1);
  }
  if(pthread_join(readThread, NULL) != 0) {
    perror("In call to pthread_cancel(readThread):\nThread was not cancelled successfully.\n");
    exit(1);
  }
  if(close(PARENT_WRITE_FD_) < 0) {
    perror("In call to close(PARENT_WRITE_FD_):\nFailed to close pipe after normal shell termination.\n");
    exit(1);
  }
  if(close(PARENT_READ_FD_) < 0) {
    perror("In call to close(PARENT_READ_FD_):\nFailed to close pipe after normal shell termination.\n");
    exit(1);
	}
}

void *forwardDataToShell() {
  /* Non-Canonical input:
   * read bytes continuously as they become available
   * and write them back continuously
   */
  int bytesRead = 0;
  char buff[BUFFERSIZE_];
  int rc = 0;
  while(!rc) {
    bytesRead = read(FDIN_, buff, BUFFERSIZE_);
    if(bytesRead < 0) {
			if(kill(pid, SIGTERM) < 0) {
				perror("In call to kill(pid, SIGTERM):\nFailed to send SIGTERM to shell.\n");
				exit(1);
			}
			if(close(PARENT_WRITE_FD_) < 0) {
				perror("In call to close(fd):\r\nFailed to close pipe after normal shell termination.\r\n");
				exit(1);
			}
      exit(2);
    }
    else {
      rc = sendBytesToShell(buff, bytesRead);
    }
  }
	if(close(PARENT_WRITE_FD_) < 0) {
		perror("In call to close(fd):\r\nFailed to close pipe after normal shell termination.\r\n");
		exit(1);
	}
  return NULL;
}

int sendBytesToShell(char *buff, int nBytes) {
  char writeByte;
  char lf = '\n';
  for(int i = 0; i < nBytes; ++i) {
    writeByte = buff[i];
    if(writeByte == '\n' || writeByte == '\r') {
      if(write(PARENT_WRITE_FD_, &lf, 1) < 0) {
				perror("In call to write(fd, &lf, 1):\nWrite to shell failed.\n");
				exit(1);
      }
    }
    else if(writeByte == 0x004) {
      return 1;
    }
    else if(writeByte == 0x003) {
      if(kill(pid, SIGINT) < 0) {
        perror("In call to kill(pid, SIGINT):\nFailed to send signal to child process.\n");
				exit(1);
      }
    }
    else {
      if(write(PARENT_WRITE_FD_, &writeByte, 1) < 0) {
				fprintf(stderr, "In call to write(fd, &writeByte, 1):\nFailed to write char %s to shell.\n", &writeByte);
				perror("");
				exit(1);
      }
    }
  }
  return 0;
}

void *readBytesFromShell() {
  int bytesRead = 0;
  char buff[BUFFERSIZE_];
  while(1) {
    bytesRead = read(PARENT_READ_FD_, buff, BUFFERSIZE_);
    if(bytesRead < 0) {
      perror("In call to read(PARENT_READ_FD_, buff, BUFFERSIZE_):\nRead from shell failed.\n");
      exit(2);
    }
		else if(bytesRead == 0) {
			if(close(PARENT_READ_FD_) < 0) {
				perror("In call to close(PARENT_READ_FD_):\r\nFailed to close pipe after normal shell termination.\r\n");
				exit(1);
			}
			return NULL;
		}
    else {
			if(write(FDOUT_, buff, bytesRead) < 0) {
				perror("In call to write(FDOUT_, buff, bytesRead):\nFailed to write to stdout.\n");
				exit(1);
			}
    }
  }
  return NULL;
}

void signalHandler(int SIGNUM) {
  if(SIGNUM == SIGPIPE) {
		collectShellStatus();
		close(newsockfd);
    exit(2);
  }
	else if(SIGNUM == SIGCHLD) {
		collectShellStatus();
		close(newsockfd);
		exit(0);
  }
  else {
    fprintf(stdout, "received signal: %d\r\n", SIGNUM);
    exit(1);
  }
}

void collectShellStatus() {
	if(wait(&pidStatus) < 0) {
		perror("in call to wait(&pidStatus):\r\nwaitpid failed.\r\n");
		exit(1);
	}
	int pidStopSignal = pidStatus & 0x00FF;
	int pidExitStatus = pidStatus >> 8;
	fprintf(stdout, "EXIT SIGNAL=%d STATUS=%d\n", pidStopSignal, pidExitStatus);
}

