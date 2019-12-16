#pragma once
typedef struct udpMessage {
	unsigned char nVersion;
	unsigned char nType;
	unsigned short nMsgLen;
	unsigned long lSeqNum;
	char chMsg[1000];
};