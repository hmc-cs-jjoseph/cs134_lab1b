/* \file lab1.h
 * \author Jesse Joseph
 * \email jjoseph@hmc.edu
 * \ID 040161840
 * \brief Interface for lab1.c
 *
 * The lab1a program converts the terminal to non-canonical input
 * and echos written characters back.
 *
 * Adding the --shell option opens a bash shell in a child process,
 * and writing commands into the lab1a program interface functions
 * similarly to writing them into the bash prompt. Output from the 
 * bash shell is sent back to the terminal.
 *
 * To quit the program:
 * Send the EOF character with ctrl-D.
 *
 * Sending ctrl-c sends a SIGINT to the child shell, which forces
 * the child shell to exit.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <netdb.h>
#include <mcrypt.h>

int main(int argc, char **argv);

/* \brief Opens a socket at localhost on port. Blocks while listening for an incoming connection.
 * \return Returns a file descriptor for the newly opened socket.
 */
int connectSocket(int port);

/* \brief Opens mcrypt modules and communicates with client connected over fd to share
 * 		public Initialization vector. 2 separate mcrypt modules are opened: one for encrypting
 * 		outgoing data and one for decrypting incoming data.
 */
void initializeEncryption(int fd, MCRYPT *encrypt_td, MCRYPT *decrypt_td, char *encryptKey);

/* \brief Cleanup function to restore terminal settings and collect shell exit status
 */
void exitCleanUp();

/* \brief Signal handler designed to catch SIGCHLD and SIGPIPE 
 */
void signalHandler(int SIGNUM);

/* \brief Changes terminal into byte-at-a-time non canonical input mode.
 */
void setTerminalToNonCanonicalInput();

/* \brief Reads bytes from terminal and sends them one by one over the socket connection. Also
			echos typed characters back to the terminal.
 * \detail Does not block if there is no data at the terminal
 */
void sendBytesToSocket();

/* \brief Reads bytes from socket and writes them out to the terminal
 * \detail Does not block if there is no data at the socket
 */
void recieveBytesFromSocketAndPrint();

/* \brief Logs data to logfile, stamped with tag to determine if the client
 * 		recieved this data from the server or sent it.
 * \detail If the --encrypt= option was used, then this logs encrypted data.
 */
void logToFile(char *buff, int nBytes, int sendMode);

/* \brief writes data in buff out to the terminal.
 */
int writeBytesToTerminal(char *buff, int nBytes, int fromSocket);

/* \brief closes mcrypt modules for encrypt and decrypt tds
 */
void close_mcrypt();
