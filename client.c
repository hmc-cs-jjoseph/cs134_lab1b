/* \file client.c
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
int logFlag = 0;
int encryptFlag = 0;
int sockfd, logfd;
MCRYPT encrypt, decrypt;
char key[16];

int main(int argc, char **argv) {
	int port = 0;

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
	char *keyFile;
  char opt;
  int optind;
  struct option longOptions[] = {
    {"log", optional_argument, 0, 'l'},
    {"port", required_argument, 0, 'p'},
		{"encrypt", optional_argument, 0, 'e'},
    {0, 0, 0, 0}};
  while((opt = getopt_long(argc, argv, "lp:e", longOptions, &optind)) != -1) {
    switch(opt) {
      case 'l':
				logFile = optarg;
				logFlag = 1;
      	break;
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
		fprintf(stderr, "Usage: ./client --port=<portno> [--log=<logfile> --encrypt=<keyfile>]\n");
		exit(1);
	}

	sockfd = connectSocket(port);

	/* Open the log file */
	if(logFlag) {
		logfd = creat(logFile, 0666);
		if(logfd < 0) {
			perror("In call to creat(logFile, 0666):\r\nCouldn't open log file.\r\n");
			exit(1);
		}
	}
	
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

	/* Save the old terminal settings */
	if(tcgetattr(FDIN_, &oldTerm) != 0) {
		perror("In call to tcgetattr(FDIN_, &oldTerm):\r\n Failed to save original terminal settings.\r\n");
	}

	/* Change the terminal settings to non-canonical input */
	setTerminalToNonCanonicalInput();

  int flags = fcntl(FDIN_, F_GETFL, 0);
  fcntl(FDIN_, F_SETFL, flags | O_NONBLOCK);

	while(1) {
		sendBytesToSocket();
		recieveBytesFromSocketAndPrint();
	}
	return 0;
}

int connectSocket(int port) {
	/* Build the socket. Server and client must be on the same machine. */
	struct sockaddr_in serverAddress;
	struct hostent *server;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
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

	if(connect(fd,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		perror("In call to (connect(sockfd,(struct sockaddr *)&serverAddress, sizeof(serverAddress)):\nFailed to connect to socket.\n");
		exit(1);
	}
	return fd;
}

void initializeEncryption(int fd, MCRYPT *encrypt_td, MCRYPT *decrypt_td, char *encryptKey) {
	/* Open two separate mcrypt modules, one for encrypting and one for decrypting */
	*encrypt_td = mcrypt_module_open("xtea", NULL, "cfb", NULL);
	if (*encrypt_td == MCRYPT_FAILED) {
		perror("In client: Failed to open mcrypt encryption module for encryption.\n");
		exit(1);
	}
	*decrypt_td = mcrypt_module_open("xtea", NULL, "cfb", NULL);
	if (*decrypt_td == MCRYPT_FAILED) {
		perror("In client: Failed to open mcrypt encryption module for decryption.\n");
		exit(1);
	}
	/* We have two initialization vectors: one for outgoing data and one for incoming
	 * data. The outgoing IV is set by the client and is the first data sent over
	 * the socket. The client then recieves the server's initialization vector.
	 */
	int IVsize = mcrypt_enc_get_iv_size(*encrypt_td); 
	char IV[IVsize];
	char serverIV[IVsize];
	srand(time(0));
	for(int i = 0; i < IVsize; ++i) {
		IV[i] = rand() % 10;
	}
	if(mcrypt_generic_init(*encrypt_td, encryptKey, 16, IV) < 0) {
		perror("In call to mcrypt_generic_init(*encrypt_td, key, 16, IV):\nFailed to initialize encryption.\n");
		exit(1);
	}
	if(send(fd, IV, IVsize, 0) < IVsize) {
		perror("In call to send(sockfd, IV, IVsize, 0):\nFailed to send initialization vector to server.\n");
		exit(1);
	}	
	if(recv(fd, serverIV, IVsize, 0) < IVsize) {
		perror("In call to recv(sockfd, serverIV, IVsize, 0):\nDidn't receive full initialization vector from server.\n");
		exit(1);
	}
	if(mcrypt_generic_init(*decrypt_td, encryptKey, 16, serverIV) < 0) {
		perror("In call to mcrypt_generic_init(encrypt, key, 16, serverIV):\nFailed to initialize encryption.\n");
		exit(1);
	}
}




void sendBytesToSocket() {
	char buff[BUFFERSIZE_];
	int nBytes = read(FDIN_, buff, BUFFERSIZE_);
	if(nBytes > 0) {
		writeBytesToTerminal(buff, nBytes, 0);
		if(encryptFlag) {
			if(mcrypt_generic(encrypt, buff, nBytes) < 0) {
				perror("In call to mcrypt_generic(encrypt, buff, nBytes):\r\nFailed to encrypt outgoing data.\r\n");
				exit(1);
			}
		}
		if(logFlag) {
			logToFile(buff, nBytes, 1);
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
		exit(0);
	}
	else {
		if(logFlag) {
			logToFile(buff, recieved, 0);
		}
		if(encryptFlag) {
			if(mdecrypt_generic(decrypt, buff, recieved) != 0) {
				perror("In call to mdecrypt_generic(decrypt, buff, recieved):\r\nFailed to decrypt cyphertext.\r\n");
				exit(1);
			}
		}
		writeBytesToTerminal(buff, recieved, 1);
	}
}

void logToFile(char *buff, int nBytes, int sentflag) {
	char mode[128];
	bzero(mode, 128);
	if(sentflag) {
		sprintf(mode, "SENT %d BYTES: ", nBytes);
	}
	else {
		sprintf(mode, "RECEIVED %d BYTES: ", nBytes);
	}
	int i;
	for(i = 0; i < 128; ++i) {
		char writeByte = mode[i];
		if(writeByte == 0) {
			break;
		}
		else {
			if(write(logfd, &mode[i], 1) < 0) {
				perror("in call to write(logfd, buff, nBytes):\r\nFailed to write to logfile.\r\n");
				exit(1);
			}
		}
	}
	if(write(logfd, buff, nBytes) < 0) {
		perror("in call to write(logfd, buff, nBytes):\r\nFailed to write to logfile.\r\n");
		exit(1);
	}
	if(write(logfd, "\n", 1) < 0) {
		perror("in call to write(logfd, \"\\n\", 1):\r\nFailed to write to logfile.\r\n");
		exit(1);
	}

}


int writeBytesToTerminal(char *buff, int nBytes, int fromSocket) {
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
    else if(writeByte == 0x004 && fromSocket) {
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


void exitCleanUp() {
  if(terminalModeChanged) {
    if(tcsetattr(FDIN_, TCSANOW, &oldTerm) != 0) {
      perror("In call to tcsetattr(FDIN_, TCSANOW, &oldTerm):\r\nFailed to restore terminal to canonical input.\r\n");
    }
  }
	close(sockfd);
	close_mcrypt();
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

void close_mcrypt() {
	mcrypt_generic_deinit(encrypt);
	mcrypt_generic_deinit(decrypt);
	mcrypt_module_close(encrypt);
	mcrypt_module_close(decrypt);
}
