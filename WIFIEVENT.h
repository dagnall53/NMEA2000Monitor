#ifndef _WIFI_EVENT_H_
#define _WIFI_EVENT_H_
#include <stdint.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#define AP_DEFAULT      0xFF
#define AP_ONLY         0x0F
#define AP_AND_STA      0x00
#define STA_SLAVE       0xF0
extern WiFiServer tcpserver;  // allow others to see these! 
extern WiFiClient tcpclient;
extern WiFiClient  VNClient;  
extern WiFiUDP Udp;


// see https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/   for discussion of WIFI EVENTS
// MODE is  "AP_Mode_Factory" (0xFF), "AP_Only"  (0x0F), "AP_and_STA"   (0x00) 

void SetWIFI(const char * SSID_AP, const char * PW_AP, const char * IP_AP, const char * SSID_ST, const char * PW_ST, const char* IP_ST, byte _MODE ); //  _mode usually 00 (AP_and_STA) AP_(SSID/PW/IP)  and EXT(SSID/PW/IP) to be populated with SSID,IP etc.
void SetPorts(int UDP, int TCP);    // to set TCP and UDP ports 
void StartWiFi();                   // used to start WIFI  startup
void CheckWiFi();                  // called in Main loop to do any updates etc. Checks connected etc. 
boolean Test_T_Connection();     // sets test IsTCPClient boolean also returns if connected.
bool SetUPVNClient();            // return true or false and sets up hidden connection 
void VNClientPrint(char * buf);  // use this to send to the VN client 
int Readudpport();    // read the port values
int Readtcpport();    //

bool tcpclientavailable();  // get the tcpclient.available()) 
byte tcpclientread(); // get tcpclient.read()
int UdpparsePacket();//Udp.parsePacket();
int Udpread(char *buf, int len); // Udp.read
                             
void StartWiFi();                   // used to start WIFI  startup

void SetOutOfSetup(bool set);      // used to advise WIFI that Setup() has finished (/is running)
bool ReadOutOfSetup(void);         // to allow main code to read this boolean (? not used ??) 

bool ReadIsConnected(void);        // these allow monitoring of the state oc "connectedness"
bool ReadGatewaySetup(void);       // only set when valid IP for Ext network has been obtained 

String IPADDasString(IPAddress IPAD); //pass back an IP address x.x.x.x  .. or add "extern IPAddress ap_ip;" etc.  
// void StopPorts();// now in main ino.. but they call the StartWiFiPorts().. 
// void StartPorts();
void StartWiFiPorts( bool set);    // equivalent to StartPorts() / StopPorts() WIFI el. calleed  in Setup 


void SendBufToTCP(const char *buf); 
void SendBufToTCPf(const char* fmt, ...) ; //printf type send
void SendBufToUDP(const char *buf); 
void SendBufToUDPf(const char* fmt, ...) ; //printf type send




#endif
