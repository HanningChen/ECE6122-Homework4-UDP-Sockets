#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <iostream>
#include<string>
#include<vector>
#include<math.h>
#include"udpMessage.h"
#include<thread>

using namespace std;
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  
#endif
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#else
/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */

typedef int SOCKET;
#endif

int sockInit(void)
{
#ifdef _WIN32
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
	return 0;
#endif
}

int sockQuit(void)
{
#ifdef _WIN32
	return WSACleanup();
#else
	return 0;
#endif
}

/* Note: For POSIX, typedef SOCKET as an int. */

int sockClose(SOCKET sock)
{

	int status = 0;

#ifdef _WIN32
	status = shutdown(sock, SD_BOTH);
	if (status == 0)
	{
		status = closesocket(sock);
	}
#else
	status = shutdown(sock, SHUT_RDWR);
	if (status == 0)
	{
		status = close(sock);
	}
#endif

	return status;

}

void error(const char* msg)
{
	perror(msg);

	exit(0);
}

void listenServer();

char* sign;

int sockfd;
struct sockaddr_in from;
socklen_t fromlen;


int main(int argc, char* argv[])
{
	int portno, n;
	struct sockaddr_in serv_addr;
	struct hostent* server;
	char buffer[1024];
	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}
	sockInit();
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	server = gethostbyname(argv[1]);
	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	memset((char*)& serv_addr, sizeof(serv_addr), 0);
	serv_addr.sin_family = AF_INET;
	memmove((char*)& serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	//    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	//       error("ERROR connecting");

	thread listenThread(listenServer);
	listenThread.detach();
	while (true)
	{
		udpMessage message;
		printf("Please enter command: ");
		char command[1024];
		memset(command,1024,0);
		fgets(command,1023,stdin);
		/********************/
		message.nVersion = '1';
		/********************/
		if (command[0] == 'q')
		{
			//*sign = 'q';
			break;
		}
		else if (command[0] == 'v')
		{
			message.nVersion = command[2];
			cout << "version changed,Please input other message"<<endl;
			continue;
		}
		else if (command[0] == 't')
		{
			message.nType = command[2];
			int counter1 = 0;
			int point = 4;
			while (command[point] != ' ')
			{
				counter1++;
				point++;
			}
			int tempNum = 0;
			int j = 4;
			while (counter1 > 0)
			{
				
				tempNum = (command[j]-48) * pow(10, counter1 - 1) + tempNum;
				counter1--;
				j++;
			}
			message.lSeqNum = tempNum;
			point++;
			int i = 0;
			while (command[point] != '\n')
			{
				message.chMsg[i] = command[point];
				point++;
				i++;
			}
			/*************************************/
			if (i > 20)
			{
				cout << "The message length out of range, please try again!!! " << endl;
				continue;
			}
			/************************************/
			message.nMsgLen = i;
		}
		n = sendto(sockfd, (const char*)& message, sizeof(message)+1, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		if (n < 0)
			error("ERROR writing to socket");
		//        n = recv(sockfd, buffer, 1023, 0);
	}
	sockClose(sockfd);
	sockQuit();

#ifdef _WIN32
	std::cin.get();
#endif
	return 0;
}

void listenServer()
{
	while (true) {
		char buffer[1024];
		memset(buffer,1024,0);
		udpMessage message;
		int n;
		n = recvfrom(sockfd, buffer, 1023, 0, (struct sockaddr*) & from, &fromlen);
		buffer[n] = 0;
		if (n < 0)
			error("ERROR reading from socket");
		cout<<"Have get Message"<<endl;
		//memset(&message, 0, sizeof(buffer));
		//memcpy(&message, buffer, sizeof(buffer));
		//printf("%s\n", message.chMsg);
		printf("%s \n", buffer);
	}
}
