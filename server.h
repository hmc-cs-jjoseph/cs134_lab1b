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
 * the child shell to exit. I don't believe this is what was 
 * intended by the assignment
 */

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
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
#include <mcrypt.h>

int main(int argc, char **argv);

/* \brief Opens mcrypt modules and communicates with client connected over fd to share
 * 		public Initialization vector. 2 separate mcrypt modules are opened: one for encrypting
 * 		outgoing data and one for decrypting incoming data.
 */
void initializeEncryption(int fd, MCRYPT *encrypt_td, MCRYPT *decrypt_td, char *encryptKey);

/* \brief Opens a socket at localhost on port. Blocks while listening for an incoming connection.
 * \return Returns a file descriptor for the newly opened socket.
 */
int connectSocket(int port);

/* \brief Main functionality for the server. Forks a child process. The child executes a bash
 * 		bash shell, with input and output redirected to the parent process.
 * 		The parent process sends data from the client to the shell and sends shell data back
 * 		to the client connected over fd.
 */
void serve(int fd);

/* \brief Redirects stdin and stdout to pipes and then executes a bash shell.
 */
void execShell();

/* \brief From parent process, sends data to shell and data from shell to client
 */
void communicateWithShell();

/* \brief Executes in a thread to forward client data to the shell
 */
void *forwardDataToShell();

/* \brief Executes in a thread to forward shell data to client
 */
void *readBytesFromShell();

/* \brief Writes contents of buff to shell
 * \detail Handles sending signals to shell
 */
int sendBytesToShell(char *buff, int nBytes);

/* \brief Signal handler designed to catch SIGCHLD and SIGPIPE 
 * \detail Exit handling happens here, because the server closes
 * 		either on SIGCHLD or SIGPIPE. 
 */
void signalHandler(int SIGNUM);

/* \brief Collects the exit status of the shell instance 
 */
void collectShellStatus();

/* \brief closes mcrypt modules for encrypt and decrypt tds
 */
void close_mcrypt();
