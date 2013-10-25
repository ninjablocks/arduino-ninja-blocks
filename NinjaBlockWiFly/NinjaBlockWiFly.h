/*
 *  NinjaBlock ninjablocks.com API wrapper
 *  Author: JP Liew, Pete Moore @askpete
 *  Source: git://github.com/ninjablocks/arduino
 *  This library provides a basic interface to the Ninja Blocks API.
 *  Use it to send sensor data and/or listen for commands. See exampes
 *  for usage or http://help.ninjablocks.com and search for Arduino library.
 */
 
#ifndef ninjablockwifly_h
#define ninjablockwifly_h

#include "WiFlyHQ.h"

#define DATA_SIZE  128
#define GUID_LEN	36
#define DATA_LEN	96

class NinjaBlockClass {

public:
	char *host;
	char *nodeID;
	char *token;
	char *guid;
	int port;
	int vendorID;
	int deviceID;
	WiFly client;
	Stream* debugSerial;

	char strGUID[GUID_LEN];
	int intVID;
	int intDID;
	int intDATA;
	char strDATA[DATA_LEN];
	bool IsDATAString;

	int begin();
	void send(int data);
	void send(char *data);
	bool receive(void);
	void httppost(char *postData);
	bool decodeJSON();
	void setClient(WiFly client);
	void setDebugSerial(Stream* debugSerial);

private:
	void ninjaMessage(bool, int intData, char *charData);
	void sendHeaders(bool isPOST, WiFly hclient);
	bool receiveConnected(void);
};

extern NinjaBlockClass NinjaBlock; 

#endif
