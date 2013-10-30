#include "NinjaBlockWiFly.h"

#undef DEBUG_SERIAL
#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#define DPRINT(A) debugSerial->print(A)
#define DPRINTLN(A) debugSerial->println(A)
#else
#define DPRINT(A)
#define DPRINTLN(A)
#endif

#define ALLNOTNULL(A, B, C) ((A!=NULL) && (B!=NULL) && (C!=NULL))
#define kEthernetBytes 4

int NinjaBlockClass::begin()
{
	DPRINTLN(F("$$$"));
	int result = 0;
	if (ALLNOTNULL(host, nodeID, token))
	{
		if( !client.open(host, port) ) {
			//it can't connect
			result = 0;
			DPRINTLN(F("!26"));
		}
		else {
			result = 1;
		}
	} else {
		DPRINTLN(F("!32"));
	}
	return result;
}

void NinjaBlockClass::httppost(char *postData)
{	
	DPRINT(F("->"));
    if( client.open(host,port) ) {
		sendHeaders(true,client);
		client.print(F("Content-Length: "));
		client.println(strlen(postData));
		client.println();
		client.println(postData);	
		DPRINTLN(F("P"));
		//DPRINTLN(postData);		
		client.flush();
		client.close();
	} else {
		DPRINTLN(F("!51"));
	}
	return;
}

void NinjaBlockClass::sendHeaders(bool isPOST, WiFly hclient) {
	char strData[DATA_LEN];
	if (isPOST)  
		strcpy_P(strData,PSTR("POST"));
	else 
		strcpy_P(strData,PSTR("GET"));
	strcat_P(strData,PSTR(" /rest/v0/block/"));
	strcat_P(strData, nodeID);
	if (isPOST)  
		strcat_P(strData,PSTR("/data"));
	else 
		strcat_P(strData, PSTR("/commands"));
	strcat_P(strData, PSTR(" HTTP/1.1\r\n"));
	hclient.print(strData);
	strcpy_P(strData,PSTR("Host: ")); 
	strcat(strData ,host);
	strcat_P(strData, PSTR("\r\n"));
	hclient.print(strData);
	hclient.print(F("User-Agent: Ninja Arduino 1.1\r\n\
Content-Type: application/json\r\n\
Accept: application/json\r\n"));
	strcpy_P(strData,PSTR("X-Ninja-Token: "));
	strcat_P(strData, token);
	strcat_P(strData,PSTR("\r\n"));
	hclient.print(strData);
}

void NinjaBlockClass::send(char *data)
{
	ninjaMessage(false, 0, data);
}

void NinjaBlockClass::send(int data)
{
	ninjaMessage(true, data, 0);
}

//Most Arduinos, 2-byte int range -32,768 to 32,767 (max 6 chars)
//Arduino Duo, 4-byte range -2,147,483,648 to 2,147,483,647 (max 11 chars)
const char kNumLength = sizeof(int) * 3;
char strNumber[kNumLength];
//return reference to strNumber
char *int2str(int num) {
	return itoa(num, strNumber, 10); // base 10
}
char strSend[DATA_SIZE];
void addStringAndUnderscore(char * str) {
	strcat(strSend, str);
	strcat_P(strSend, PSTR("_"));
}
void addStringAndUnderscore_P(prog_char * str) {
	strcat_P(strSend, str);
	strcat_P(strSend, PSTR("_"));
}
void NinjaBlockClass::ninjaMessage(bool isInt, int intData, char *charData) {
	if (guid != NULL) {
		strcpy_P(strSend,PSTR("{\"GUID\": \""));
		addStringAndUnderscore_P(nodeID);
		addStringAndUnderscore(guid);
		addStringAndUnderscore(int2str(vendorID));
		strcat(strSend, int2str(deviceID));
		strcat_P(strSend, PSTR("\",\"G\": \""));
		strcat(strSend, guid);
		strcat_P(strSend, PSTR("\",\"V\": "));
		strcat(strSend, int2str(vendorID));
		strcat_P(strSend, PSTR(",\"D\": "));
		strcat(strSend, int2str(deviceID));
		strcat_P(strSend, PSTR(",\"DA\": "));
		if (isInt) {
			strcat(strSend, int2str(intData));
		} else {
			strcat_P(strSend, PSTR("\""));
			strcat(strSend, charData);
			strcat_P(strSend, PSTR("\""));
		}
		strcat_P(strSend, PSTR("}"));
		httppost(strSend);
	}
}


const char kStrHeaderEnd[] = {'\r', '\n', '\r', '\n'};
const byte kHeaderLength = sizeof(kStrHeaderEnd);
const char kCharInvertedCommas	= '\"';
bool NinjaBlockClass::receive(void) {
	bool gotData = false;
	if(!client.isConnected())
	{
		// connect if not connected
		DPRINT(F("-"));
		//client.close();
		if(client.open(host,port)==1)
		{
			DPRINTLN(F("-"));
			sendHeaders(false, client);
			client.println();
		} else {
			DPRINTLN(F("!161"));
		}
	}
	if (client.isConnected())
	{
		gotData = receiveConnected();
	}
	return gotData;
}

// giving a name prefix, eg. -> G":" <-, skip past the value after it, and insert a string terminator
// returns NULL, or the beginning of a string within data that has been null-terminated
char * valueString(const char *name, char *data, int &index, const int length) {
	char *result = NULL;
	uint8_t nameLength = strlen(name);
	for (uint8_t matching=0
		; (matching < nameLength) && (index < length)
		; index++) {
		matching = ((data[index] == name[matching]) ? matching+1 : 0);
	}
	if (index < length) {
		//if searching for a string seek end of string ("), otherwise (,) ends an int
		char endChar = (data[index-1]==kCharInvertedCommas) ? kCharInvertedCommas : ',';
		int start = index;
		while ((index < length) && (data[index] != endChar)) {
			index++;
		}
		if (index < length) {
			data[index] = '\0'; // insert string terminator after value (string or int)
			result = &data[start];
		}
	}
	return result;
}

bool NinjaBlockClass::receiveConnected(void) {
	bool gotHeader = false;
	bool gotData = false;
	int bytesRead = 0;
	int index = 0;
	char data[DATA_SIZE];
	IsTick = false;

	//skip past header
	uint8_t matching=0;
	char ch;
	while(client.available() ) {
		delayMicroseconds(100);
		ch = client.read();
		//DPRINT(ch);

		if( !gotHeader ) {
			//skip header first
			matching = 0;
			while( ch == kStrHeaderEnd[matching] ) {
				matching++;
				if( matching == kHeaderLength ) {
					//DPRINT(F("EOH"));
					gotHeader = true;
					break;
				}
				delayMicroseconds(100);
				ch = client.read();
				//DPRINT(ch);
			}
		} 
		else {
			//skipped header; now get the message
			//DPRINT("index="); DPRINTLN(index);			
			//message still coming
			//read data into array eg. {"DEVICE":[{"G":"0","V":0,"D":1000,"DA":"FFFFFF"}]}
			bytesRead++;
			if( index < (DATA_SIZE - 1) ) {
				data[index++] = ch;
			}
		}
	}
	if( gotHeader ) {
		char *strVal;
		bytesRead = 0;
		data[index] = 0;//null terminate it
		DPRINT(F("<-"));
		strVal = valueString("G\":\"", data, bytesRead, index);
		if (strVal) {
			strcpy(strGUID, strVal);
			strVal = valueString("V\":", data, bytesRead, index);
			if (strVal != NULL) {
				intVID = atoi(strVal);
				strVal = valueString("D\":", data, bytesRead, index);
				if (strVal != NULL) {
					intDID = atoi(strVal);

				 	// DPRINT(F(" strGUID="));
				 	// DPRINTLN(strGUID);
				 	// DPRINT(F(" intVID="));
				 	// DPRINTLN(intVID);
				 	// DPRINT(F(" intDID="));
				 	// DPRINTLN(intDID);

					int start = bytesRead;
					strVal = valueString("DA\":\"", data, bytesRead, index);
					if (strVal != NULL) {
						strcpy(strDATA, strVal);
						IsDATAString = true;
						gotData = true;
						DPRINTLN(F("sD"));
						// DPRINTLN(strDATA);
					}
					else { // may be an int value
						bytesRead = start; // reset to where we were before attempting (data is unmodified if NULL was returned)
						strVal = valueString("DA\":", data, bytesRead, index);
						if (strVal) {
							intDATA = atoi(strVal);
							IsDATAString = false;
							gotData = true;
							DPRINTLN(F("iD"));
							// DPRINTLN(intDATA);
						}
					}
				}
			}
		} 
		if( !gotData ) {
			//this is a tick from the cloud
			DPRINTLN(F("T"));
			IsTick = true;
		}
		//if a header was received, there was some data after (either json, or some html etc)
		//purge and close the stream
		client.flush();
		delay(100);
		client.close();
		delay(100);
	}
	return gotData || IsTick;
}

NinjaBlockClass NinjaBlock;
