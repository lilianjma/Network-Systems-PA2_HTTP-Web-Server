#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include  <signal.h>

#define MAXLINE 			4096 /*max text line length*/
#define LISTENQ 			200	 /*maximum number of client connections*/
#define ROOT				"./www/"
#define GET_METHOD	 		"GET"
#define VERSION_0 			"HTTP/1.0"
#define VERSION_1 			"HTTP/1.1"
#define CRLF 				"\r\n"

int serverfd = -1;

void INThandler(int);
void command_handler(int socketfd, char buf[]);

/**
 * Method: 	main
 * Uses: 	Recieve connection requests
 * 			Fork for every connection
 * 			Parse connections
 */
int main(int argc, char **argv)
{
	int listenfd, connfd, n;
	pid_t childpid;
	socklen_t clilen;
	int portno;
	char buf[MAXLINE];
	struct sockaddr_in cliaddr, servaddr;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);

	signal(SIGINT, INThandler);

	// Create a socket for the soclet
	// If sockfd<0 there was an error in the creation of the socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Problem in creating the socket");
		exit(2);
	}
	serverfd = listenfd;

	// preparation of the socket address
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(portno);

	// bind the socket
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	// listen to the socket by creating a connection queue, then wait for clients
	listen(listenfd, LISTENQ);

	printf("%s\n", "Server running...waiting for connections.");

	while (1)
	{
		clilen = sizeof(cliaddr);
		// accept a connection
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

		printf("%s\n", "Received request...");

		if ((childpid = fork()) == 0)
		{ // if it’s 0, it’s child process

			printf("%s\n", "Child created for dealing with client requests");

			// close listening socket
			close(listenfd);

			while ((n = recv(connfd, buf, MAXLINE, 0)) > 0)
			{
				printf("String received from the client: ");
				puts(buf);
				command_handler(connfd, buf);
				printf("exited commandhandler\n");
				bzero(buf, sizeof(buf));
				
			}

			if (n < 0)
				printf("%s\n", "Read error");
			
			exit(0);
		}
		// close socket of the server
		printf("Socket closed\n");
		close(connfd);
	}
}

void  INThandler(int sig)
{
    signal(sig, SIG_IGN);
	printf("\nServer exiting...\n");
	if (serverfd != -1) {
		// wait(10);
        close(serverfd);
        printf("Server socket closed.\n");
    }
	 exit(0);
}

int wordcount(char* bufptr) {
	/* count the number of words in the command line */
	char *ptrcpy = bufptr;
	int count = 0;
	int inWord = 0;

	while (*ptrcpy != '\0')
	{
		if (*ptrcpy == ' ')
		{
			inWord = 0;
		}
		else if (!inWord)
		{
			inWord = 1;
			count++;
		}
		ptrcpy++;
	}

	return count;
}

/********************** REQUEST HEADER FORMAT **********************
 * HTTP/1.1 200 OK \r\n
 * Content-Type: <> \r\n # Tells about the type of content and the formatting of <file contents> 
 * Content-Length:<> \r\n # Numeric value of the number of bytes of <file contents>
 * \r\n<file contents>
*/

/**
 * Method:	command_handler
 * Uses:	
 */
void command_handler(int connfd, char* buf) {
	
	char method[12]; char uri[256]; char version[12]; // User input buffers
	char filepath[512];
	printf("In commandhandler\n");

	// Error if incorrect number of args
	// int argc = wordcount(buf);
	// if (argc != 3) {
	// 	char* response = "400 Bad Request\r\n\r\n";
	// 	send(connfd, response, strlen(response), 0);
	// 	return;
	// }

    // Parse request from client
    sscanf(buf, "%s %s %s", method, uri, version);

	// Error if not using GET
	if (strcmp(method, GET_METHOD)) {
		char* response = "405 Method Not Allowed\r\n\r\n";
		send(connfd, response, strlen(response), 0);
		return;
	}

	// Error if wrong versions
	if ((strcmp(version, VERSION_0) != 0) && (strcmp(version, VERSION_1) != 0)) {
		char* response = "505 HTTP Version Not Supported\r\n\r\n";
		send(connfd, response, strlen(response), 0);
		return;
	}

	// Create file path
	strcpy(filepath, ROOT);
    strcat(filepath, uri);

    // Open file and send response
    int fd = open(filepath, O_RDONLY);
	printf("%s\n", filepath);

    if (fd == -1) { // File not found
        char* response = "HTTP/1.1 404 Not Found\r\n\r\n"; // TODO do i need to add "\r\nContent-Length: 0"?
		send(connfd, response, strlen(response), 0);
    }
	else { 			// File found
		// Get file size
		ssize_t filesize = lseek(fd, 0, SEEK_END);
		if (filesize == -1) {
			perror("Failed to determine file size");
			close(fd);
			return;
		}
	
		// Set file pointer to beginning
		if (lseek(fd, 0, SEEK_SET) == -1) {
			perror("Failed to reset file pointer");
			close(fd);
			return;
		}
	
		// Create header and get header length
		char header[256];
		sprintf(header, "HTTP/1.1 200 OK\r\nContent-Length: %zd\r\nContent-Type: text/html\r\n\r\n", filesize);
		size_t header_len = strlen(header);

		// Allocate memory for the file content + null terminator + header length
		char* resp_buf = (char*)malloc(filesize + 1 + header_len);
		if (!resp_buf) {
			perror("Memory allocation failed");
			close(fd);
			return;
		}

		memcpy(resp_buf, header, header_len);

		pread(fd, resp_buf+header_len, filesize + 1, 0);
		printf("%s\n", resp_buf);
		send(connfd, resp_buf, header_len + filesize + 1, 0);
		free(resp_buf);
    }
	close(fd);
	printf("File, %s, closed\n", filepath);
}
