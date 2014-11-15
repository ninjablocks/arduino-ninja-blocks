/*
 *  NinjaBlock ninjablocks.com API wrapper
 *  Author: JP Liew, Pete Moore @askpete
 *  Source: git://github.com/ninjablocks/arduino
 *  This library provides a basic interface to the Ninja Blocks API.
 *  Use it to send sensor data and/or listen for commands. See exampes
 *  for usage or http://help.ninjablocks.com and search for Arduino library.
 */
 
#ifndef ninjablockethernet_h
#define ninjablockethernet_h

#include <utility/w5100.h> // Needed to adjust retransmission time.
#include <Ethernet.h>

#define GUID_LEN	36
#define DATA_LEN	96
#define PACKET_LEN 448

class EthernetClient;

class NinjaBlockClass {

public:
	char *host;
	char *nodeID;
	char *token;
	char *guid;
	int port;
	int vendorID;
	int deviceID;

	char strGUID[GUID_LEN];
	int intVID;
	int intDID;
	int intDATA;
	char strDATA[DATA_LEN];
	bool IsDATAString;

	int begin();
	int begin(byte *mac);
	void send(int data);
	void send(char *data);
	bool receive(void);
	bool decodeJSON();
	int maintain();

private:
	void httppost(bool isInt, int intData, char *charData);
	void copyNinjaMessage(bool isInt, int intData, char *charData, char *dst);
	int  calculateNinjaMessageLength(bool isInt, int intData, char *charData);
	void sendHeaders(bool isPOST, EthernetClient hclient);
	char* prepareHeaders(bool isPOST, char* packet);
	bool receiveConnected(void);
};

extern NinjaBlockClass NinjaBlock; 

#endif
