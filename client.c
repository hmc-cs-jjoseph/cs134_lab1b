/* \file lab1.h
 * \author Jesse Joseph
 * \email jjoseph@hmc.edu
 * \ID 040161840
 */
#include "client.h"
#define FDIN_ 0
#define FDOUT_ 1
#define FDERR_ 2
#define BUFFERSIZE_ 512

struct termios oldTerm;
int terminalModeChanged = 0;
int sockfd;
int logfd;
int shouldLog;

int main(int argc, char **argv) {
	int port;
	struct sockaddr_in serverAddress;
	struct hostent *server;

  /* First, we register a cleanup function to make sure
   * that the terminal is returned to canonical input 
   * mode on program exit.
   */
  if(atexit(exitCleanUp) != 0) {
    perror("In call to atexit(exitCleanUp): Failed to register cleanup routine");
    exit(1);
  }

  /* Parse arguments with getopt_long(4).
   */
	char *logFile;
  char opt;
  int optind;
  struct option longOptions[] = {
    {"log", optional_argument, 0, 'l'},
    {"port", required_argument, 0, 'p'},
    {0, 0, 0, 0}};
  while((opt = getopt_long(argc, argv, "lp:", longOptions, &optind)) != -1) {
    switch(opt) {
      case 'l':
				shouldLog = 1;
				logFile = optarg;
      	break;
			case 'p':
				port = atoi(optarg);
				break;
      case '?':
				exit(1);
      case 0:
        break;
      default:
				break;
    }
  }

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("in call to socket(AF_INET, SOCK_STREAM, 0):\r\nError opening socket.\r\n");
		exit(1);
	}
	server = gethostbyname("localhost");
	if(server == NULL) {
		perror("In call to gethostbyname(\"localhost\"):\r\nNo such host.\r\n");
		exit(1);
	}

	bzero((char *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &serverAddress.sin_addr.s_addr, server->h_length);
	serverAddress.sin_port = htons(port);

	if(connect(sockfd,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		perror("In call to (connect(sockfd,(struct sockaddr *)&serverAddress, sizeof(serverAddress)):\nFailed to connect to socket.\n");
		exit(1);
	}
	
  /* Save the old terminal settings */
  if(tcgetattr(FDIN_, &oldTerm) != 0) {
    perror("In call to tcgetattr(FDIN_, &oldTerm):\r\n Failed to save original terminal settings.\r\n");
    exit(1);
  }

	/* Change the terminal settings to non-canonical input */
	setTerminalToNonCanonicalInput();

  int flags = fcntl(FDIN_, F_GETFL, 0);
  fcntl(FDIN_, F_SETFL, flags | O_NONBLOCK);


	if(sockfd < 0) {
		perror("In call to socket(AF_INET, SOCK_STREAM, 0):\r\nFailed to open socket.\r\n");
		exit(1);
	}

	if(shouldLog) {
		logfd = creat(logFile, 0666);
		if(logfd < 0) {
			perror("In call to creat(logFile, 0666):\r\nCouldn't open log file.\r\n");
			exit(1);
		}
	}

	while(1) {
		sendBytesToSocket();
		recieveBytesFromSocketAndPrint();
	}
	return 0;
}

void sendBytesToSocket() {
	char buff[BUFFERSIZE_];
	int nBytes = read(FDIN_, buff, BUFFERSIZE_);
	if(nBytes > 0) {
		if(shouldLog) {
			logToFile(buff, nBytes, 1);
		}
		if(write(FDOUT_, buff, nBytes) < 0) {
			perror("In call to write(FDOUT_, buff, nBytes):\r\nFailed to echo to terminal.\r\n");
			exit(1);
		}
		int sent;
		int bytesToWrite = nBytes;
		while(bytesToWrite > 0) {
			sent = send(sockfd, &buff[nBytes - bytesToWrite], 1, MSG_DONTWAIT);
			if(sent > 0) {
				bytesToWrite--;
			}
			else if (sent < 0){
				if(errno != EAGAIN || errno != EWOULDBLOCK) {
					perror("In call to send(sockfd, &buff[nBytes - bytesToWrite], 1, MSG_DONTWAIT):\r\nFailed to send data to socket.\r\n");
					exit(1);
				}
			}
		}
	}
	else if(nBytes == 0) {
		//fprintf(stdout, "In call to read(FDIN_, buff, BUFFERSIZE_):\r\nRecieved EOF from terminal.\r\n");
		//exit(0);
	}
	else {
		if(errno != EAGAIN) {
			perror("In call to read(FDIN_, buff, BUFFERSIZE_):\r\nError reading from terminal.\r\n");
			exit(1);
		}
	}
}

void recieveBytesFromSocketAndPrint() {
	char buff[BUFFERSIZE_];
	int recieved = recv(sockfd, buff, BUFFERSIZE_, MSG_DONTWAIT);
	if(recieved < 0) {
		if(errno != EAGAIN || errno != EWOULDBLOCK) {
			perror("In call to recv(sockfd, buff, BUFFERSIZE_, MSG_DONTWAIT):\r\nRecieve error.\r\n");
			exit(1);
		}
	}
	else if(recieved == 0) {
		//fprintf(stdout, "Server closed.\r\n");
	}
	else {
		if(shouldLog) {
			logToFile(buff, recieved, 0);
		}
		fprintf(stdout, ">> "); 
		writeBytesToTerminal(buff, recieved);
	}
}

void logToFile(char *buff, int nBytes, int sentflag) {
	char *mode;
	int modeBytes;
	if(sentflag) {
		mode = "Sent:\r\n";
		modeBytes = 7;
	}
	else {
		mode = "Recieved:\r\n";
		modeBytes = 11;
	}
	if(write(logfd, mode, modeBytes) < 0) {
		perror("in call to write(logfd, buff, nBytes):\r\nFailed to write to logfile.\r\n");
		exit(1);
	}
	if(write(logfd, buff, nBytes) < 0) {
		perror("in call to write(logfd, buff, nBytes):\r\nFailed to write to logfile.\r\n");
		exit(1);
	}
}


int writeBytesToTerminal(char *buff, int nBytes) {
  char writeByte;
  char crlf[2] = {'\r', '\n'};
  for(int i = 0; i < nBytes; ++i ) {
    writeByte = buff[i];
    if(writeByte == '\n' || writeByte == '\r') {
      if(write(FDOUT_, crlf,  2) < 0) {
				perror("In call to write(FDOUT_, crlf, 2):\nFailed to write to terminal.\n");
				exit(1);
      }
    } 
    else if(writeByte == 0x004){
      exit(0);
    }
    else {
      if(write(FDOUT_, &writeByte, 1) < 0) {
				perror("In call to write(FDOUT_, &writeByte, 1):\nFailed to write to terminal.\n");
				exit(1);
      }
    }
  }
  return 0;
}

void signalHandler(int SIGNUM) {
  if(SIGNUM == SIGPIPE) {
    exit(2);
  }
	else if(SIGNUM == SIGCHLD) {
		exit(0);
  }
  else {
    fprintf(stdout, "received signal: %d\r\n", SIGNUM);
    exit(1);
  }
}

void exitCleanUp() {
  if(terminalModeChanged) {
    if(tcsetattr(FDIN_, TCSANOW, &oldTerm) != 0) {
      perror("In call to tcsetattr(FDIN_, TCSANOW, &oldTerm):\r\nFailed to restore terminal to canonical input.\r\n");
    }
  }
	close(sockfd);
}

void setTerminalToNonCanonicalInput() {
  /* Create a new termios object to define our desired 
   * terminal settings.
   */
  struct termios newTerm;
  newTerm.c_iflag = IUTF8 | ISTRIP;
  newTerm.c_oflag = 0;
  newTerm.c_lflag = 0;
  if(tcsetattr(FDIN_, TCSANOW, &newTerm) != 0) {
		fprintf(stderr,"In client:\r\n");
    perror("In call to tcsetattr(FDIN_, TCSANOW, &newTerm):\nFailed to set terminal to non-canonical input.\n");
    exit(1);
  }
  terminalModeChanged = 1;
}
