#include "NinjaBlockEthernet.h"

//#include <MemoryFree.h> 

byte _mac[] = { 0xCE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

char _packet[PACKET_LEN];

EthernetClient client;
EthernetClient recvclient;

#define ALLNOTNULL(A, B, C) ((A!=NULL) && (B!=NULL) && (C!=NULL))

#define kEthernetBytes 4
#define RETRANSMISSION_TIME 4000

// Like strcpy, except it takes the string from flash memory instead of the more limited SRAM.
// See: http://playground.arduino.cc/Learning/Memory 
size_t strcpyf(char *buffer, const __FlashStringHelper *ifsh) {
	const char PROGMEM *p = (const char PROGMEM *)ifsh;
	size_t n = 0;
	char *b = buffer;
	
	unsigned char c = pgm_read_byte(p++);
	
	while(c != '\0') {
		*b++ = c;
		n++;
		
		c = pgm_read_byte(p++);
	}
	*b = '\0';
	return n;
}

// Lets you specify your own MAC (instead of default one declared within library).
int NinjaBlockClass::begin(byte *mac)
{
	int result = 1;
	if (ALLNOTNULL(host, nodeID, token) // has connection params
		&& (Ethernet.begin(mac)!=0) // has Dynamic IP address
		)
	{
		result = 1;
		Serial.print(F("IP: "));
		for (byte thisByte = 0; thisByte < kEthernetBytes; thisByte++) 
		{
			// print the value of each byte of the IP address:
			Serial.print(Ethernet.localIP()[thisByte], DEC);
			Serial.print('.');
		}
		Serial.println();
	}
	// Default retransmission time was 1000ms, which was often too short.
	W5100.setRetransmissionTime(RETRANSMISSION_TIME);
	return result;
}

// Uses baked-in mac (for back-wards compatibility).
// Sticking with the baked-in MAC will cause problems if two or more
// instances of this library are used in the same local network.
int NinjaBlockClass::begin()
{
	return begin(_mac);
}

//Most Arduinos, 2-byte int range -32,768 to 32,767 (max 6 chars)
//Arduino Duo, 4-byte range -2,147,483,648 to 2,147,483,647 (max 11 chars)
const byte kNumLength = (sizeof(int) * 3)+1;	// +1 is to allow for null terminator.
char strNumber[kNumLength];
//return reference to strNumber
inline char *int2str(int num) {
	return itoa(num, strNumber, 10); // base 10
}

void NinjaBlockClass::httppost(bool isInt, int intData, char *charData)
{	
	Serial.print('_');
    if (!client.connected()) {
        client.flush();
        client.stop();
        client.connect(host,port);
		Serial.print('\'');
    }
	if (client.connected()) {
		char* p_packet = prepareHeaders(true, _packet);
		strcpyf(p_packet, F("Content-Length: ")); 		// 16 chars
		p_packet += 16;
		int2str(calculateNinjaMessageLength(isInt, intData, charData));
		strcpy(p_packet, strNumber); 						
		p_packet += strlen(strNumber);
		strcpyf(p_packet, F("\r\n\r\n"));				// 4 chars.
		p_packet += 4;
		
		copyNinjaMessage(isInt, intData, charData, p_packet);
		client.println(_packet);
		Serial.print(F("Sent="));
		Serial.println(p_packet);
		client.flush();
		client.stop();
	} else {
		Serial.println(F("Send Failed"));
	}
	return;
}

// Prepares all the headers in the packet buffer.
// Returns: Pointer for start of body.
char* NinjaBlockClass::prepareHeaders(bool isPOST, char* packet) {
	char* p_packet = packet;
	if(isPOST) {
		strcpyf(p_packet, F("POST"));										// 4 chars.
		p_packet += 4;
	}
	else { //isGET
		strcpyf(p_packet, F("GET"));										// 3 chars.
		p_packet += 3;
	}
	strcpyf(p_packet, F(" /rest/v0/block/"));								// 16 chars.
	p_packet += 16;
	strcpy(p_packet, nodeID);
	p_packet += strlen(nodeID);
	if(isPOST) {
		strcpyf(p_packet, F("/data"));										// 5 chars.
		p_packet += 5;
	}
	else {
		strcpyf(p_packet, F("/commands"));									// 9 chars.
		p_packet += 9;
	}
	strcpyf(p_packet, F(" HTTP/1.1\r\n"));									// 11 chars.
	p_packet += 11;
	strcpyf(p_packet, F("Host: "));											// 6 chars.
	p_packet += 6;
	strcpy(p_packet, host);													// strlen(host) chars.
	p_packet += strlen(host);
	strcpyf(p_packet, F("\r\nUser-Agent: Ninja Arduino 1.2\r\n"));			// 33 chars.
	p_packet += 33;
	if(isPOST) { // Content-Type only applicable for POSTS, not GETs.
		strcpyf(p_packet, F("Content-Type: application/json\r\n"));			// 32 chars.
		p_packet += 32;
	}
	strcpyf(p_packet, F("Accept: application/json\r\nX-Ninja-Token: "));	// 41 chars.
	p_packet += 41;
	strcpy(p_packet, token);												// strlen(token) chars.
	p_packet += strlen(token);
	strcpyf(p_packet, F("\r\n"));											// 2 chars.
	p_packet += 2;
	return p_packet;
}
	

void NinjaBlockClass::send(char *data)
{
	httppost(false, 0, data);
	//ninjaMessage(false, 0, data);
}

void NinjaBlockClass::send(int data)
{
	httppost(true, data, 0);
//	ninjaMessage(true, data, 0);
}

// Previously would construct ninjaMessage in temp array to be then sent by httppost.
// Wastes memory, so now copies it into the end of the array after httppost has constructed
//  the header. Downside to this approach is Content-Length has to be calculated separately.
void NinjaBlockClass::copyNinjaMessage(bool isInt, int intData, char *charData, char *dst) {
	if (guid != NULL) {
		// strcat is too slow to find end of string O(n) so I keep track of pointer myself.
		char* p_packet = dst;
		
		// GUID.
		strcpyf(p_packet, F("{\"GUID\": \""));				// 10 chars.
		p_packet += 10; 
		strcpy(p_packet, nodeID);						// len(nodeID)
		p_packet += strlen(nodeID);
		*p_packet = '_';								// 1 char.
		p_packet += 1;
		strcpy(p_packet, guid);							// len(guid)
		p_packet += strlen(guid);
		*p_packet = '_';								// 1 char.
		p_packet += 1;
		strcpy(p_packet, int2str(vendorID));			// len(int2str(vendorID)
		p_packet += strlen(p_packet);
		*p_packet = '_';								// 1 char.
		p_packet += 1;
		strcpy(p_packet, int2str(deviceID));			// len(int2str(deviceID)) chars.
		p_packet += strlen(int2str(deviceID));
		
		// G
		strcpyf(p_packet, F("\",\"G\": \""));				// 8 chars.
		p_packet += 8;
		strcpy(p_packet, guid);							// len(guid) chars.
		p_packet += strlen(guid);
		
		// V
		strcpyf(p_packet, F("\",\"V\": "));					// 7 chars.
		p_packet += 7;
		strcpy(p_packet, int2str(vendorID));			// len(int2str(vendorID)) chars.
		p_packet += strlen(p_packet);
		
		// D
		strcpyf(p_packet, F(",\"D\": "));					// 6 chars.
		p_packet += 6;
		strcpy(p_packet, int2str(deviceID));			// len(int2str(deviceID)) chars.
		p_packet += strlen(p_packet);
		
		// DA
		strcpyf(p_packet, F(",\"DA\": "));					// 7 chars.
		p_packet += 7;
		if (isInt) {
			strcpy(p_packet, int2str(intData));			// len(int2str(intData)) chars.
			p_packet += strlen(p_packet);
		} else {
			*p_packet = '\"';							// 1 char.
			p_packet += 1;
			strcpy(p_packet, charData);					// len(charData) chars.
			p_packet += strlen(charData);
			*p_packet = '\"';							// 1 char.
			p_packet += 1;
		}
		*p_packet = '}';								// 1 char.
		p_packet += 1;
		*p_packet = '\0';								// Don't forget to null terminate!
	}
}

// There is a field in the header called "Content-Length", which forces us to calculate
//  the data's length before copying it into the char array.
// Previously this was handled by fragmenting the TCP packet into two parts: header and data.
// The fragmentation adds work to other parts of the system, and makes it trickier to debug in
//  a packet sniffer, so instead I am calculating the length of the POST's data separately.
// This approach is a little bit more processing than preparing the data ahead of time into a
//  temporary char array, but it saves the additional memory of that char array.
int NinjaBlockClass::calculateNinjaMessageLength(bool isInt, int intData, char *charData)
{
	// I'll let the compiler optimise this into fewer calls than first appears.
	int sizeCount = 0;
	sizeCount += 10;							// '{"GUID": "'
	sizeCount += strlen(nodeID) + 1;			// nodeID + '_'
	sizeCount += strlen(guid) + 1;				// guid + '_'
	sizeCount += strlen(int2str(vendorID)) + 1;	// int2str(vendorID) + '_'
	sizeCount += strlen(int2str(deviceID));		// int2str(deviceID)
	sizeCount += 8;								// '","G": "'
	sizeCount += strlen(guid);					// guid
	sizeCount += 7;								// '","V": '
	sizeCount += strlen(int2str(vendorID));		// int2str(vendorID)
	sizeCount += 6;								// ',"D": '
	sizeCount += strlen(int2str(deviceID));		// int2str(deviceID);
	sizeCount += 7;								// ',"DA": '
	if(isInt) {
		sizeCount += strlen(int2str(intData));	// int2str(intData);
	} else {
		sizeCount += 1;							// '"'
		sizeCount += strlen(charData);			// charData
		sizeCount += 1;							// '"'
	}
	sizeCount += 1;								// '}'
	return sizeCount;
}

const char kStrHeaderEnd[] = {'\r', '\n', '\r', '\n'};
const byte kHeaderLength = sizeof(kStrHeaderEnd);

// will keep reading bytes until it has matched the header, or it has read all available bytes
inline void skipHeader(const int bytesAvailable, int &bytesRead) {
	//skip past header
	for (uint8_t matching=0
		; (matching < kHeaderLength) && (bytesRead < bytesAvailable)
		; bytesRead++) {
		matching = ((recvclient.read() == kStrHeaderEnd[matching]) ? matching+1 : 0);
	}
}

const char kCharInvertedCommas	= '\"';
bool NinjaBlockClass::receive(void) {
	bool gotData = false;
	if(!recvclient.connected())
	{
		// connect if not connected
		Serial.print(',');
		recvclient.stop();
		if(recvclient.connect(host,port)==1)
		{
			char* p_packet = prepareHeaders(false, _packet);
			strcpyf(p_packet, F("\r\n"));
			p_packet += 2;
			recvclient.println(_packet);
		}
	}
	if (recvclient.connected())
	{
		gotData = receiveConnected();
	}
	else {
		Serial.println(F("RX: Can't connect."));
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
	int bytesAvailable = recvclient.available();
	if (bytesAvailable > 0)
	{
		int bytesRead = 0;
		skipHeader(bytesAvailable, bytesRead);
		gotHeader = (bytesRead < bytesAvailable); //skipped header without reaching end of available bytes
		if (gotHeader) {
			//reset counts
			bytesAvailable -= bytesRead;
			bytesRead = 0;

			if (bytesAvailable > PACKET_LEN) {
				// Error condition. Potential hang point (infinite loop).
				// Flush and stop the client so the connection can then restart.
				recvclient.flush();
				recvclient.stop();
				Serial.print(F("ERR: DATA_SIZE"));
				return false;
			}

//			char data[DATA_SIZE];		// 
			//read data into array eg. {"DEVICE":[{"G":"0","V":0,"D":1000,"DA":"FFFFFF"}]}
			for (bytesRead=0; bytesRead<bytesAvailable; bytesRead++) {
				_packet[bytesRead] = recvclient.read();
				//Serial.print(_packet[bytesRead]);
			}
			_packet[bytesRead] = '\0'; //terminate _packet as string
			bytesRead = 0;
			char *strVal;
			strVal = valueString("G\":\"", _packet, bytesRead, bytesAvailable);
			if (strVal) {
				strcpy(strGUID, strVal);
				strVal = valueString("V\":", _packet, bytesRead, bytesAvailable);
				if (strVal != NULL) {
					intVID = atoi(strVal);
					strVal = valueString("D\":", _packet, bytesRead, bytesAvailable);
					if (strVal != NULL) {
						intDID = atoi(strVal);

					 	// Serial.print(" strGUID=");
					 	// Serial.println(strGUID);
					 	// Serial.print(" intVID=");
					 	// Serial.println(intVID);
					 	// Serial.print(" intDID=");
					 	// Serial.println(intDID);

						int start = bytesRead;
						strVal = valueString("DA\":\"", _packet, bytesRead, bytesAvailable);
						if (strVal != NULL) {
							strcpy(strDATA, strVal);
							IsDATAString = true;
							gotData = true;
							// Serial.print("strDATA=");
							// Serial.println(strDATA);
						}
						else { // may be an int value
							bytesRead = start; // reset to where we were before attempting (_packet is unmodified if NULL was returned)
							strVal = valueString("DA\":", _packet, bytesRead, bytesAvailable);
							if (strVal) {
								intDATA = atoi(strVal);
								IsDATAString = false;
								gotData = true;
								// Serial.print("intDATA=");
								// Serial.println(intDATA);
							}
						}
					}
				}
			}
		}
	}
	if (gotHeader) {
		//if a header was received, there was some data after (either json, or some html etc)
		//purge and close the stream
		recvclient.flush();
		delay(100);
		recvclient.stop();
		delay(100);
		Serial.print('$');
	}
	return gotData;
}

//Needs to be called periodically from the sketch. Required so DHCP can be
// renewed when the lease expires.
int NinjaBlockClass::maintain(void)
{
	int rc = Ethernet.maintain();
	if(rc != DHCP_CHECK_NONE) {
		Serial.print('#');
		Serial.print(rc);
	}
	return rc;
}

NinjaBlockClass NinjaBlock;
