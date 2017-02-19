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

pid_t pid;
int pidStatus, sockfd, encryptFlag;
MCRYPT encrypt, decrypt;

int main(int argc, char **argv) {
	char *keyFile;
	char key[16];
  int port;

  /* Parse arguments with getopt_long(4).
   * Valid arguments: --encrypt=, --port=
   */
  char opt;
  int optind;
  struct option longOptions[] = {
    {"port", required_argument, 0, 'p'},
		{"encrypt", required_argument, 0, 'e'},
    {0, 0, 0, 0}};
	while((opt = getopt_long(argc, argv, "p:e", longOptions, &optind)) != -1) {
		switch(opt) {
			case 'p':
				port = atoi(optarg);
				break;
			case 'e':
				keyFile = optarg;
				encryptFlag = 1;
				break;
			case '?':
				exit(1);
			case 0:
				break;
			default:
				break;
    }
  }
	if(port == 0) {
		fprintf(stderr, "Usage: ./server --port=<portno> [ --encrypt=<keyfile> ]\n");
		exit(1);
	}
	sockfd = connectSocket(port);

	if(encryptFlag) {
		int keyfd = open(keyFile, O_RDONLY);
		if(keyfd < 0) {
			perror("In call to open(keyFile, O_RDONLY):\nFailed to open key file.\n");
			exit(1);
		}
		if(read(keyfd, key, 16) < 16) {
			perror("In call to read(keyfd, key, 16):\nFailed to get encryption key.\n");
			exit(1);
		}
		if(close(keyfd) < 0) {
			perror("In call to close(keyfd):\nFailed to close keyfile.\n");
			exit(1);
		}
		initializeEncryption(sockfd, &encrypt, &decrypt, key);
	}

	serve(sockfd);
	if(close(sockfd) < 0) {
		perror("In call to close(nesockfd):\nFailed to close connection to socket.\n");
		exit(1);
	}
	return 0;
}

void initializeEncryption(int fd, MCRYPT *encrypt_td, MCRYPT *decrypt_td, char *encryptKey) {
	/* Open two separate mcrypt modules, one for encrypting and one for decrypting */
	*encrypt_td = mcrypt_module_open("xtea", NULL, "cfb", NULL);
	if (*encrypt_td == MCRYPT_FAILED) {
		perror("In server: Failed to open mcrypt encryption module for encryption.\n");
		exit(1);
	}
	*decrypt_td = mcrypt_module_open("xtea", NULL, "cfb", NULL);
	if (*decrypt_td == MCRYPT_FAILED) {
		perror("In server: Failed to open mcrypt encryption module for decryption.\n");
		exit(1);
	}
	/* We have two initialization vectors: one for outgoing data and one for incoming
	 * data. The incoming data initialization vector is created by the client and
	 * is the first data sent over the socket. The outgoing IV is set by the server
	 * and sent to the client.
	 */
	int IVsize = mcrypt_enc_get_iv_size(*encrypt_td); 
	char IV[IVsize];
	char clientIV[IVsize];
	srand(time(0));
	for(int i = 0; i < IVsize; ++i) {
		IV[i] = rand() % 10;
	}
	if(mcrypt_generic_init(*encrypt_td, encryptKey, 16, IV) < 0) {
		perror("In call to mcrypt_generic_init(encrypt, encryptKey, 16, IV):\nFailed to initialize encryption.\n");
		exit(1);
	}
	if(recv(fd, clientIV, IVsize, 0) < IVsize) {
		perror("In call to recv(fd, serverIV, IVsize, 0):\nDidn't receive full initialization vector from server.\n");
		exit(1);
	}
	if(send(fd, IV, IVsize, 0) < IVsize) {
		perror("In call to send(fd, IV, IVsize, 0):\nFailed to send initialization vector to server.\n");
		exit(1);
	}	
	if(mcrypt_generic_init(*decrypt_td, encryptKey, 16, clientIV) < 0) {
		perror("In call to mcrypt_generic_init(encrypt, encryptKey, 16, serverIV):\nFailed to initialize encryption.\n");
		exit(1);
	}
}

int connectSocket(int port) {
	socklen_t clientLength;
	struct sockaddr_in serverAddress, clientAddress;
	int unboundSock = socket(AF_INET, SOCK_STREAM, 0);
	if(unboundSock < 0) {
		perror("In call to socket(AF_INET, SOCK_STREAM, 0):\nFailed to open socket.\n");
		exit(1);
	}
	/* Set SO_REUSEADDR because this server closes after one use, so this will allow
	 * immediate reboot of server.
	 */
	int trueVal = 1;
	if(setsockopt(unboundSock, SOL_SOCKET, SO_REUSEADDR, &trueVal, sizeof(int)) < 0) {
		perror("In call to setsockopt(unboundSock, SOL_SOCKET, SO_REUSEADDR, &1, sizeof(int)):\nFailed to set socket option.\n");
		exit(1);
	}

	bzero((void *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(port);
  if(bind(unboundSock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
		perror("In call to bind(unboundSock, &serverAddress, sizeof(serverAddress)):\nFailed to bind to address.\n");
		exit(1);
	}
	if(listen(unboundSock, 3) < 0){
		perror("In call to listen(unboundSock, 3):\nFailed to prepare socket to accept connections.\n");
		exit(1);
	}
	clientLength = (socklen_t ) sizeof(clientAddress);

	/* Blocks until a connection is made */
	sockfd = accept(unboundSock, (struct sockaddr *) &clientAddress, &clientLength);
	if(sockfd < 0) {
		perror("In call to accept(unboundSock, (struct sockaddr *) &clientAddress, $clientLength):\nFailed to open socket.\n");
		exit(1);
	}
	if(close(unboundSock) < 0) {
		perror("In call to close(unboundSock):\nFailed to close connection to socket.\n");
		exit(1);
	}
	return sockfd;
}

void serve(int fd) {
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
		if(dup2(fd, FDIN_) < 0) {
			perror("In call to dup2(sockfd, FDIN_):\nIn server process: Failed to redirect stdin to the socket.\n");
			exit(1);
		}
		if(dup2(fd, FDOUT_) < 0) {
			perror("In call to dup2(sockfd, FDOUT_):\nIn server process: Failed to redirect stdout to the socket.\n");
			exit(1);
		}
		if(dup2(fd, FDERR_) < 0) {
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
  /* Non-Canonical input
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
			if(encryptFlag) {
				if(mdecrypt_generic(decrypt, buff, bytesRead) < 0) {
					perror("In call to mdecrypt_generic(decrypt, buff, bytesRead):\nDecrypt failed.\n");
					exit(1);
				}
			}
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
			if(encryptFlag) {
				if(mcrypt_generic(encrypt, buff, bytesRead) < 0) {
					perror("In call to mcrypt_generic(encrypt, buff, bytesRead):\nEncrypt failed.\n");
					exit(1);
				}
			}
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
		close(sockfd);
		close_mcrypt();
    exit(2);
  }
	else if(SIGNUM == SIGCHLD) {
		collectShellStatus();
		close(sockfd);
		close_mcrypt();
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
	char statusMessage[128];
	bzero(statusMessage, 128);
	sprintf(statusMessage, "EXIT SIGNAL=%d STATUS=%d\n", pidStopSignal, pidExitStatus);
	int statusLength;
	for( statusLength = 0; statusLength < 128; ++statusLength) {
		if(statusMessage[statusLength] == '\0') {
			break;
		}
	}
	if(encryptFlag) {
		if(mcrypt_generic(encrypt, statusMessage, statusLength) < 0) {
			perror("In call to mcrypt_generic(encrypt, statusMessage, statusLength):\nFailed to encrypt status line.\n");
			exit(1);
		}
	}
	if(write(FDOUT_, statusMessage, statusLength) < 0) {
		perror("In call to write(FDOUT_, statusMessage, statusLength):\nFailed to send status line.\n");
		exit(1);
	}
}

void close_mcrypt() {
	mcrypt_generic_deinit(encrypt);
	mcrypt_generic_deinit(decrypt);
	mcrypt_module_close(encrypt);
	mcrypt_module_close(decrypt);
}
