#include <stdint.h>
#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>  // https://github.com/Links2004/arduinoWebSockets

#include "WIFIEVENT.h"
#define AP_DEFAULT 0xFF
#define AP_ONLY 0x0F
#define AP_AND_STA 0x00
#define STA_SLAVE 0xF0

// *********** This code is planned to be a personal "library" to simplify connecting to WIFI and setting up
// often used functions.
// it is NOT yet ready to be a Library!, but it works and was helpful for my demo.
// https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
// see also .. https://randomnerdtutorials.com/solved-reconnect-esp32-to-wifi/   for a simple set of commands, rather than this full set.

// these can be in Defines if needed for other parts of the program -- but I would prefer they were local and hermetic to this library
// ------------ Logically I  prefer these to be hermetically defined locally,
//  but it seems to work. if you define like this extern IPAddress apip; NO () ! !!!  -----
IPAddress ap_ip(192, 168, 4, 1);            // the IP address in AP mode. Defaults can be changed!
const IPAddress sub255(255, 255, 255, 0);   // the default Subnet Mask in in SoftAP mode
IPAddress udp_ap(0, 0, 0, 0);               // the IP address to send UDP data in SoftAP mode
IPAddress udp_st(0, 0, 0, 0);               // the IP address to send UDP data in Station mode
IPAddress sta_ip(0, 0, 0, 0);               // the IP address (or received from DHCP if 0.0.0.0) in Station mode
IPAddress gateway(0, 0, 0, 0);              // the IP address for Gateway in Station mode
IPAddress subnet(0, 0, 0, 0);               // the SubNet Mask in Station mode
IPAddress VNhost(192, 168, 4, 1);           // the VN multiplexer to connect in Slave mode
const IPAddress def_ap_ip(192, 168, 4, 1);  // the default IP address in AP mode
int udpport, tcpport;

char ssidAP[16];      // ssid of the network to create in AP mode
char passwordAP[16];  // password for above mode
char ipadAP[16];      // ip address for above mode
char ssidST[16];      // ssid of the network to connect in STA mode
char passwordST[16];  // password for above mode
char ipadST[16];      // ip address for connection in STA mode ("0.0.0.0" means DHCP)
boolean UseDHCP = true;
boolean APDefault = true;  // when false we can set a different IP in AP mode (not 192.168.4.1)
boolean IsVNAvailable;

// these can be in Defines if needed for other parts of the program -- but I would prefer they were local and hermetic to this library
//.NOTE .***************************************************************
//*** Here the functions used to advise Settings status
// Replace these - or do them properly elsewhere (e.g with a TFT display )
void AdviseNotConnectedYet() {}                                   // flashes LEDS if we are not connected
void AdviseAPStarted() {}                                         // advises the AP is set up
void AdviseChangeInClientsConnectedToAP(uint8_t AP_connected) {}  //Advises any change in th enumber of clients connected to the AP
void AdviseConnectedToExtNetwork(IPAddress _LocalIP) {}           //advises that the unit has connected to an external network, and shows the IP.

bool IsTcpClient = false;
byte MAIN_MODE;                  // INTERNAL WIFI EVENT byte for the wifi modes.. AP_AND_STA, AP_ONLY,
boolean SoftAPstarted;           // to force SoftAP when Station fails
boolean IsConnected = false;     // used when Mode (was MAIN_MODE) (Here used as variable.. MAIN_MODE)== AP_AND_STA to flag connection success
boolean GateWayIsSet = false;    // to monitor when Gateway has been set up .. used in conjunction with IsConnected
boolean TCPisAvailable = false;  // to monitor the setting up of TCP lnks, so they can be re-established after EXT network loss and reconnection.


const size_t MaxClients = 10;
extern WiFiUDP Udp;
WiFiClient tcpclient;

const unsigned int tcp_def = 3000;          // default (factory) value; zero means TCP is OFF
WiFiServer tcpserver(tcp_def, MaxClients);  // the port may (will!) be changed later in StartWiFiPorts()
WiFiClient VNClient(tcp_def);               // will set to 3001 ?


static char msg[300] = { '\0' };  // used in message buildup
bool ReadIsConnected(void) {
  return IsConnected;
}
bool ReadGatewaySetup(void) {
  return GateWayIsSet;
}

String IPADDasString(IPAddress IPAD) {
  String st = "";
  st = IPAD[0];
  st += ".";
  st += IPAD[1];
  st += ".";
  st += IPAD[2];
  st += ".";
  st += IPAD[3];
  return st;
}

//******************************************************************
//         TCP  and UDP port sending
//      Here to avoid moving away from the IP information that may be
//      needed during sends.
//******************************************************************
void SendBufToTCP(const char* buf) {  // simple version of TCP and UDP sending NO operations on the incoming (buf) data!
  if (IsTcpClient) {
    tcpclient.print(buf);
  }
}

void WriteBufToTCP(const uint8_t* buffer, size_t size) {  // simple version of TCP write
  if (IsTcpClient) {
    tcpclient.write(buffer, size);
  }
}

//forward declaration
void _debugPrint(const uint8_t* buffer, size_t size, String msg);

void WriteBufToUDP(const uint8_t* buffer, size_t size) {  //Writes to BOTH specific UDP IP and the general AP UDP Ip
                                                          // this shows the data to be the same!
                                                          //_debugPrint(buffer, size,"UDP");
  if ((MAIN_MODE == AP_AND_STA) && IsConnected) {
    Udp.beginPacket(udp_st, udpport);
    Udp.write(buffer, size);
    Udp.endPacket();
  }
  Udp.beginPacket(udp_ap, udpport);
  Udp.write(buffer, size);
  Udp.endPacket();
}

void SendBufToUDP(const char* buf) {  //Send to BOTH specific UDP IP and the general AP UDP Ip
  if ((MAIN_MODE == AP_AND_STA) && IsConnected) {
    Udp.beginPacket(udp_st, udpport);
    Udp.print(buf);
    Udp.endPacket();
  }
  Udp.beginPacket(udp_ap, udpport);
  Udp.print(buf);
  Udp.endPacket();
}

void SendBufToUDPf(const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  // add checksum?
  int len = strlen(msg);
  SendBufToUDP(msg);
}


void SendBufToTCPf(const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  // add checksum?
  int len = strlen(msg);
  SendBufToTCP(msg);
}

///************  WIFI "STUFF"


void StartWiFiPorts(bool set) {  // START / STOP  WIFI TCP UDP PORTS
  // =============== conditional turns on or off the ports, but only if they have values
  if ((udpport != 0) && set) {
    Udp.begin(udpport);
    Serial.printf("UDP PORT STARTED :%i \r\n", udpport);

  } else {
    Udp.stop();
    Serial.println("UDP PORT STOP ");
  }
  if ((tcpport != 0) && set) {
    tcpserver.begin(tcpport);
    tcpserver.setNoDelay(true);
    tcpclient.setNoDelay(true);
    Serial.printf("TCP PORT STARTED : port %i  \r\n", tcpport);
  } else {
    tcpserver.stop();
    Serial.println("TCP PORT STOP");
  }
}

void SetPorts(int UDP, int TCP) {
  udpport = UDP;
  tcpport = TCP;
  Serial.printf("  Ports SET:  TCP:%i, UDP %i \r\n", tcpport, udpport);
}
int Readudpport() {
  return udpport;
}
int Readtcpport() {
  return tcpport;
}

void SetWIFI(const char* SSID_AP, const char* PW_AP, const char* IP_AP, const char* SSID_ST, const char* PW_ST, const char* IP_ST, byte _MODE) {  // sets defaults if blank
  if (SSID_AP != "") {
    strcpy(ssidAP, SSID_AP);
  } else {
    strcpy(ssidAP, "NMEA2000Monitor");
  }
  if (PW_AP != "") {
    strcpy(passwordAP, PW_AP);
  } else {
    strcpy(passwordAP, "12345678");
  }
  if (IP_AP != "") {
    strcpy(ipadAP, IP_AP);
  } else {
    strcpy(ipadAP, "192.168.4.1");
  }
  ap_ip.fromString(ipadAP);
  if (SSID_ST != "") {
    strcpy(ssidST, SSID_ST);
  } else {
    _MODE = AP_ONLY;  // NO SSID, so AP ONLY
    strcpy(ssidST, "EXT NETWORK");
  }
  if (PW_ST != "") {
    strcpy(passwordST, PW_ST);
  } else {
    strcpy(passwordST, "Password");
  }
  if (IP_ST != "") {
    strcpy(ipadST, IP_ST);
    sta_ip.fromString(ipadST);
    UseDHCP = false;
  }

  if ((_MODE != 0xFF) && (_MODE != 0x0F) && (_MODE != 0xF0)) { _MODE == AP_AND_STA; }  // default

  Serial.println("* Setting UP WiFi");
  if (_MODE == AP_AND_STA) {
    Serial.printf("  WIFI set for AP+EXT  \r\n ");
  }
  if (_MODE == AP_ONLY) {
    Serial.printf("  WIFI set for AP only \r\n");
  }
  if (_MODE == STA_SLAVE) {
    Serial.printf("  WIFI set for Host Connection\r\n");
  }


  Serial.printf("  AP SSID:%s PW:%s IP:%s   \r\n", ssidAP, passwordAP, ipadAP);
  Serial.printf("  EXT SSID:%s PW:%s IP:%s  \r\n ", ssidST, passwordST, ipadST);
  Serial.printf("  Ports  TCP:%i, UDP %i \r\n", tcpport, udpport);
}

void StartSoftAP_New() {
  if (APDefault == false) {
    gateway = ap_ip;
    gateway[3] = 0x01;
    Serial.print("*   Configuring IP address as: ");
    Serial.println(ap_ip);
    Serial.print("*   Configuring Gateway as: ");
    Serial.println(gateway);
    Serial.print("*   Configuring SubNet as: ");
    Serial.println(sub255);
    WiFi.softAPConfig(ap_ip, gateway, sub255);
    delay(100);
  }
  udp_ap = ap_ip;
  udp_ap[3] = 0xFF;
  boolean result = WiFi.softAP(ssidAP, passwordAP);
  delay(100);
  if (result == true) {
    if (!SoftAPstarted) {
      AdviseAPStarted();

    }  // only need to do this once.
    SoftAPstarted = true;
    Serial.println("*  SoftAP created!");
    Serial.print("*   IP address: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("*   UDP broadcast: ");
    Serial.println(udp_ap);
  } else {
    Serial.println("AP creation Failed!");
  }
}

String _show_Status(int value) {  // used only in debug for help
  String _message;
  _message = "Unknown";
  switch (value) {
    case WL_IDLE_STATUS: _message = "temporary status assigned when WiFi.begin() is called"; break;
    case WL_NO_SSID_AVAIL: _message = "No SSID are available"; break;
    case WL_SCAN_COMPLETED: _message = "scan networks is completed"; break;
    case WL_CONNECTED: _message = "when connected to a WiFi network"; break;
    case WL_CONNECT_FAILED: _message = "when the connection fails for all the attempts"; break;
    case WL_CONNECTION_LOST: _message = "when the connection is lost"; break;
    case WL_DISCONNECTED: _message = "when disconnected from a network"; break;
  }
  return _message;
}

IPAddress Get_UDP_IP(IPAddress ip, IPAddress mk) {  // ..needed in ext STA Connect
  uint16_t ip_h = ((ip[0] << 8) | ip[1]);           // high 2 bytes
  uint16_t ip_l = ((ip[2] << 8) | ip[3]);           // low 2 bytes
  uint16_t mk_h = ((mk[0] << 8) | mk[1]);           // high 2 bytes
  uint16_t mk_l = ((mk[2] << 8) | mk[3]);           // low 2 bytes
  // reverse the mask
  mk_h = ~mk_h;
  mk_l = ~mk_l;
  // bitwise OR the net IP with the reversed mask
  ip_h = ip_h | mk_h;
  ip_l = ip_l | mk_l;
  // ip to return the result
  ip[0] = highByte(ip_h);
  ip[1] = lowByte(ip_h);
  ip[2] = highByte(ip_l);
  ip[3] = lowByte(ip_l);
  return ip;
}

void EXT_Station_Connect() {
  if (MAIN_MODE == STA_SLAVE) {
    SetUPVNClient();
    WiFi.mode(WIFI_STA);
  }
  if (GateWayIsSet && IsConnected) { return; }
  if (MAIN_MODE == AP_ONLY) { return; }
  int _STATUS = WiFi.status();
  if (_STATUS != WL_CONNECTED) {
    GateWayIsSet = false;
    TCPisAvailable = false;
    if (IsConnected) {  // do this part only if we have been connected before!
      IsConnected = false;
      WiFi.disconnect();  // it is essential that this is not repeatedly called while re-connecting.
      WiFi.reconnect();
      AdviseNotConnectedYet();
    }
    return;
  }
  WiFi.setAutoReconnect(true);  //  CONNECTED!!  TURN ON the auto reconnect -- we may have been in Ap only mode previously!
  subnet = WiFi.subnetMask();
  gateway = WiFi.gatewayIP();
  IPAddress Zeros(0, 0, 0, 0);
  if (UseDHCP == true) {
    sta_ip = WiFi.localIP();
  } else {  // configure IP after DHCP
    WiFi.config(sta_ip, gateway, subnet);
    delay(100);
  }
  udp_st = Get_UDP_IP(sta_ip, subnet);
  if (_STATUS == WL_CONNECTED) {  // are we connected ?
    IsConnected = true;
  } else {
    IsConnected = false;
  }
  if (WiFi.gatewayIP() != Zeros) {  // have a valid gateway?
    GateWayIsSet = true;
  } else {
    GateWayIsSet = false;
  }
  if (GateWayIsSet && IsConnected) {
    Serial.println("* Connected to " + String(ssidST) + "!");
    Serial.print("*   IP address: ");
    Serial.println(sta_ip);
    Serial.print("*   UDP broadcasting: ");
    Serial.println(udp_st);
    Serial.print("*   Gateway: ");
    Serial.println(gateway);
    Serial.print("*   SubnetMask: ");
    Serial.println(subnet);
    delay(100);
    StartWiFiPorts(true);
    AdviseConnectedToExtNetwork(WiFi.localIP());
  }
}

// see also https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
//File > Examples > WiFi > WiFiClientEvents.
//Based on  wifiClientEvent.ino but // Serial.print and (lots of)  added actions to enable fixed IP, flashing leds  etc

void OnWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_WIFI_READY:
      // Serial.println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      // Serial.println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
      // Serial.println("WiFi client started");
      break;
    case SYSTEM_EVENT_STA_STOP:
      // Serial.println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      // Serial.println("Connected to access point");
      EXT_Station_Connect();  //  do any fixed ip stuff etc. this prints out the connection stuff
                              //NOTE  without the EXT_Station_Connect being run, the gateways etc will not be correct

      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      if (IsConnected) {
        // Serial.println("STA_Disconnected (LOST ext network)");
        // we HAD been connected! This wil be turned off by the EXT_Station_Connect()// so we only get a single message!
        EXT_Station_Connect();
        GateWayIsSet = false;  // optional?
        TCPisAvailable = false;
        // NB... DO not leave in 115200 mode during the 500ms of the flash!

        AdviseNotConnectedYet();
      }
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      // Serial.println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      //NOTE  without the StartStation being run, the gateways etc will not be correct
      // Serial.println("* (re-connected) got IP from connected AP");
      EXT_Station_Connect();  // includes new STA_SLAVE mode

      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      // Serial.println("Lost IP address and IP address is reset to 0");
      // new stuff
      IsVNAvailable = false;
      IsConnected = false;
      WiFi.disconnect();
      WiFi.begin(ssidST);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      // Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      EXT_Station_Connect();  //  do any fixed ip stuff etc. this prints out the connection stuff
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      // Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      // Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      // Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      EXT_Station_Connect();  //??  do any fixed ip stuff etc. this prints out the connection stuff
      break;
    case SYSTEM_EVENT_AP_START:
      // Serial.println("---WiFi access point started---");
      StartSoftAP_New();
      break;

    case SYSTEM_EVENT_AP_STOP:
      // Serial.println("WiFi access point  stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      // Serial.println("Client connected");
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      // Serial.println("Client disconnected");
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      // Serial.println("Assigned IP address to client");
      EXT_Station_Connect();  //  do any fixed ip stuff etc. this prints out the connection stuff
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      // Serial.println("Received probe request");
      break;
    case SYSTEM_EVENT_GOT_IP6:
      // Serial.println("IPv6 is preferred");
      break;
    case SYSTEM_EVENT_ETH_START:
      // Serial.println("Ethernet started");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      // Serial.println("Ethernet stopped");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      // Serial.println("Ethernet connected");
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      // Serial.println("Ethernet disconnected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      // Serial.println("Obtained IP address");
      break;
    default: break;
  }
  //   here showing if devices connect / disconnect to its AP.. Useful for debug and also for TCP to send to different Ip addresses later?
  //Serial.print("** Stations Connected to AP:"); Serial.println(WiFi.softAPgetStationNum());
  AdviseChangeInClientsConnectedToAP(WiFi.softAPgetStationNum());
  delay(100);  // to allow serial Debugs print time to send..
}

bool SetUPVNClient() {  // return true or false and sets up connection
  if (IsVNAvailable) { return true; }
  if (!VNClient.connect(VNhost, 3001)) {  // not while as that is blocking!
    delay(10);
    if (GateWayIsSet) {
      // Serial.println("* gateway is setup");
    } else {
      // Serial.println("* gateway NOT set");
    }
    return false;
  } else {
    VNClient.print("Thank you NMEA server!");
    Serial.println("Connected as client to the NMEA server");
    IsVNAvailable = true;
    return true;
  }
}

void VNClientPrint(char* buf) {
  VNClient.print(buf);
}

void StartWiFi() {
  SoftAPstarted = false;
  Serial.println("STARTING WIFI");
  WiFi.onEvent(OnWiFiEvent);
  delay(100);
  SoftAPstarted = false;
  GateWayIsSet = false;
  IsConnected = false;
  //new __ Is this duplicated?
  if (MAIN_MODE == STA_SLAVE) {
    SetUPVNClient();
    WiFi.mode(WIFI_STA);
  }
  //****  Either set AP+STA mode **************
  if (MAIN_MODE == AP_AND_STA) {  // AP+STA Station mode
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(false);  // else it will still try to auto-connect to the LAST known "EXT" network, which might not exist ?
    delay(100);                    //
    //  original "SetupStation();"  // sends the crucial wifi.begin to try to connect to STA.. sent ONLY ONCE.
    WiFi.begin(ssidST, passwordST);
    delay(100);
    StartSoftAP_New();  // start the ap.. no wifi events will trigger it!
    //  Nothing else really needed!
  }
  //****  or set AP  mode **************
  if (MAIN_MODE == AP_ONLY) {  // create an Access Point only
    Serial.println("* Setting AP-MODE as " + String(ssidAP) + " ...");
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.setAutoReconnect(false);  // or else it will still try to auto-connect to the LAST known "EXT" network
    delay(100);
  }
  // **** setup AP and Report the AP connection status ******
  // Serial.println(" WIFI STARTED");
  MDNS.begin(ssidAP);
  MDNS.begin("NMEA_MASTER");
  MDNS.addService("http", "tcp", 80);  // I hope to change these later
}

boolean Test_T_Connection() {    // sets IsTCPClient boolean.
  if (!tcpclient.connected()) {  // if client (still) not connected
    //Serial.println("**********TCP_client Connected************");
    tcpclient = tcpserver.available();  // try to connect
    return false;
  } else {
    //Serial.println("********* No TCP client ***********");
    return true;
  }
}

//These could probably be removed now..
//I wrote these for TCP reading to avoid having tcpclient a global.
bool tcpclientavailable() {  // get the tcpclient.available())
  return tcpclient.available();
}
byte tcpclientread() {  // get tcpclient.read()
  return tcpclient.read();
}
//I wrote these for UDP reading to avoid having Udp a global.
int UdpparsePacket() {  //Udp.parsePacket();
  return Udp.parsePacket();
}
int Udpread(char* buf, int len) {  // Udp.read
  return Udp.read(buf, len);
}


void CheckWiFi() {
  if (!GateWayIsSet) { EXT_Station_Connect(); }                      // keep trying to connect until it gets the gateway !
  if (GateWayIsSet && !IsVNAvailable && (MAIN_MODE == STA_SLAVE)) {  // ?? and only slave mode .. ? to be tested ?  this is the NON blocking VNClient setup// VNClientTest();
    SetUPVNClient();
    return;
  }
  if (tcpport > 0) {
    IsTcpClient = Test_T_Connection();
  } else {
    IsTcpClient = false;
  }
}
