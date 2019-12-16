#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <iostream>
#include"udpMessage.h"
#include<thread>
#include<vector>

using namespace std;

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
#include <Ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  
#include <unistd.h> 

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
	// Make sure any open sockets are closed before calling exit
	exit(1);
}

void listenMessage();

int CompMessageNum = 0;
socklen_t fromlen;
int sockfd;
vector<udpMessage> compositeMessage;
vector<struct sockaddr_in> from;

int main(int argc, char* argv[])
{
	int newsockfd, portno;
	//socklen_t fromlen;
	char buffer[1024];
	struct sockaddr_in serv_addr;
	//vector<struct sockaddr_in> from;
	int n;

	if (argc < 2)
	{
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}
	sockInit();
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	memset((char*)& serv_addr, sizeof(serv_addr), 0);
	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	
	if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		error("ERROR on binding");
	}
	
	printf("Waiting on messages...\n");

	thread listenThread(listenMessage);
	listenThread.detach();
	while (true) {
		char command;
		cout << "Please enter command:" << endl;
		cin >> command;
		if (command == '0')
		{
			if (compositeMessage.empty())
			{
				cout << "The compositeMessage is empty" << endl;
				continue;
			}
			vector<struct sockaddr_in>::iterator it;
			vector<struct udpMessage>::iterator it_Message;
			it_Message = compositeMessage.begin();
			int tempLength = (*it_Message).nMsgLen;
			char bufferComp[tempLength];
			memset(bufferComp,tempLength, 0);
			strcpy(bufferComp,(*it_Message).chMsg);
			for(it_Message = compositeMessage.begin();it_Message != compositeMessage.end();it_Message++)
			{
				if (it_Message == compositeMessage.begin())
					continue;
				strcat(bufferComp,(*it_Message).chMsg);
			}
			for (it = from.begin(); it != from.end(); it++)
			{
				printf("%s \n",bufferComp);
				n = sendto(sockfd, (const char*)bufferComp,CompMessageNum, 0, (struct sockaddr*)&(*it), fromlen);
				if (n < 0)
				error("sendto");
			}
			//it = from.begin();
			//n = sendto(sockfd, (const char*)bufferComp, CompMessageNum, 0, (struct sockaddr*) & (*it), fromlen);
			compositeMessage.clear();
			from.clear();
		}
		else if (command == '1')
		{
			compositeMessage.clear();
			from.clear();
		}
		else if (command == '2')
		{
			vector<udpMessage>::iterator it_Message;
			if(compositeMessage.empty())
			{
				cout<< "The composite message is empty"<<endl;
				continue;
			}
			cout << "Message size is: "<< CompMessageNum <<endl;
			cout << "Composite Msg: ";
			for (it_Message = compositeMessage.begin(); it_Message != compositeMessage.end(); it_Message++)
			{
			     printf("%s  ", (*it_Message).chMsg);
			}
			cout << endl;
		}
		else if(command == 'q')
		  break;
	}
	sockClose(newsockfd);
	sockClose(sockfd);
	sockQuit();
#ifdef _WIN32
	std::cin.get();
#endif
	return 0;
}

void listenMessage() {
	fromlen = sizeof(struct sockaddr_in);
	while (true)
	{
		struct sockaddr_in tempfrom;
		char buffer[1024];
		memset(buffer, 1024, 0);
		int n;
		n = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*) & tempfrom, &fromlen);
		if (n < 0)
			error("recvfrom");
		if (n < 0)
			error("sendto");
		//buffer[n] = 0;
		udpMessage message;
		memset(&message, 0, sizeof(buffer));
		memcpy(&message, buffer, sizeof(buffer));
		message.chMsg[message.nMsgLen] = 0;
		/*******************************************************/
		vector<struct sockaddr_in>::iterator it_detect;
		int mark_from = 0;
		for (it_detect = from.begin(); it_detect != from.end(); it_detect++)
		{
			if (it_detect->sin_addr.s_addr == tempfrom.sin_addr.s_addr && it_detect->sin_port == tempfrom.sin_port)
			{
				mark_from = 1;
				break;
			}
		}
		if(mark_from == 0)
			from.push_back(tempfrom);
		/*******************************************************/
		if (message.nVersion == 0)
			continue;
		if (message.nType == '0')
		{
			compositeMessage.clear();
			//from.clear();
			CompMessageNum = 0;
		}
		else if (message.nType == '1')
		{
			compositeMessage.clear();
			//from.clear();
			CompMessageNum = 0;
			//from.push_back(tempfrom);
			int length = message.nMsgLen;
			compositeMessage.push_back(message);
			CompMessageNum = CompMessageNum + length;
		}
		else if (message.nType == '2')
		{
			int length = message.nMsgLen;
			vector<udpMessage>::iterator it_Message;
			
			if(compositeMessage.empty())
			{
				cout<<"The first insertion"<<endl;
				compositeMessage.push_back(message);
				CompMessageNum = message.nMsgLen;
				cout <<"Current message length is:"  <<CompMessageNum << endl;
				//continue;
			}
			else {
				int mark = 0;//lSeqNum already in mark is 1 else mark is 0

				for (it_Message = compositeMessage.begin(); it_Message != compositeMessage.end(); it_Message++)
				{
					if ((*it_Message).lSeqNum == message.lSeqNum)
					{
						mark = 1;
						break;
					}
					if ((*it_Message).lSeqNum > message.lSeqNum)
						break;
				}
				if (mark == 1)
				{
					CompMessageNum = CompMessageNum - (*it_Message).nMsgLen;
					compositeMessage.erase(it_Message);
					compositeMessage.insert(it_Message, message);
					CompMessageNum = CompMessageNum + message.nMsgLen;
				}
				else
				{
					compositeMessage.insert(it_Message, message);
					//from.push_back(tempfrom);
					CompMessageNum = CompMessageNum + message.nMsgLen;
				}
			}
		}
		else if (message.nType == '3')
		{
			if (compositeMessage.empty())
			{
				cout << "The composite message is zero, no message to send out" << endl;
				continue;
			}
			vector<struct sockaddr_in>::iterator it;
			vector<udpMessage>::iterator it_Message;
			it_Message = compositeMessage.begin();
			int tempLenghth = (*it_Message).nMsgLen;
			char bufferComp[tempLenghth];
			memset(bufferComp, tempLenghth, 0);
			strcpy(bufferComp, (*it_Message).chMsg);
			for (it_Message = compositeMessage.begin(); it_Message != compositeMessage.end(); it_Message++)
			{
				if (it_Message == compositeMessage.begin())
					continue;
				strcat(bufferComp, (*it_Message).chMsg);
			}
			for (it = from.begin(); it != from.end(); it++)
			{
				n = sendto(sockfd, (const char*)bufferComp, CompMessageNum, 0, (struct sockaddr*) & (*it), fromlen);
				if (n < 0)
					error("sendto");
			}
			//it = from.begin();
			//n = sendto(sockfd, (const char*)bufferComp, CompMessageNum, 0, (struct sockaddr*) & (*it), fromlen);
			if (n < 0)
				error("sendto");
			compositeMessage.clear();
			from.clear();
			CompMessageNum = 0;
		}
		if (CompMessageNum > 20)
		{
			vector<struct sockaddr_in>::iterator it;
			vector<udpMessage>::iterator it_Message;
			it_Message = compositeMessage.begin();
			int tempLenghth = (*it_Message).nMsgLen;
			char bufferComp[tempLenghth];
			memset(bufferComp, tempLenghth, 0);
			strcpy(bufferComp, (*it_Message).chMsg);
			for (it_Message = compositeMessage.begin(); it_Message != compositeMessage.end(); it_Message++)
			{
				if (it_Message == compositeMessage.begin())
					continue;
				strcat(bufferComp, (*it_Message).chMsg);
			}
			int remain_length = CompMessageNum - 20;
			udpMessage newMessage;
			for (int i = 0; i < remain_length; i++)
			{
				newMessage.chMsg[i] = bufferComp[20 + i];
			}
			newMessage.chMsg[remain_length] = 0;
			newMessage.lSeqNum = 0;
			newMessage.nMsgLen = remain_length;
			newMessage.nType = '2';
			newMessage.nVersion = '1';
			compositeMessage.clear();
			CompMessageNum = newMessage.nMsgLen;
			compositeMessage.push_back(newMessage);
			char sendMessage[20];
			memset(sendMessage,20,0);
			for (int i = 0; i < 20; i++)
			{
				sendMessage[i] = bufferComp[i];
			}
			for (it = from.begin(); it != from.end(); it++)
			{
				n = sendto(sockfd, (const char*)sendMessage, 20, 0, (struct sockaddr*) & (*it), fromlen);
				if (n < 0)
					error("sendto");
			}
			if (n < 0)
				error("sendto");
		}
	}
}
