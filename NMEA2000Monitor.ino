// Demo: Supplies a Wepage to show what PGN have been seen.
//
// Based on
//   NMEA2000 library. Bus listener and sender.
//   Sends all bus data to serial in Actisense format.
//   Send all data received from serial in Actisense format to the N2kBus.
//   Use this e.g. with NMEA Simulator (see. http://www.kave.fi/Apps/index.html) to send simulated data to the bus.
//   Meanwhile you can define other stream to different port so that you can send data with NMEA Simulator and listen it on other port with
//   Actisense NMEA Reader.
//   I have tried to get the Actisense Reader to send "actisense over UDP- but this is -so far- unsucessful.. see notes below. "
//   ******************


//#define N2k_CAN_INT_PIN 21
#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Uncomment this and set right CAN TX pin definition, TX on default IO 16
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Uncomment this and set right CAN RX pin definition, RX on default IO 4

#include <Arduino.h>
#include <Update.h>
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <NMEA2000_CAN.h>
//#include <ActisenseReader.h>

//#include "Defines.h"
#include "WebsocketInterpreter.h"
#include "WIFIEVENT.h"  // my "easy to use" (eventually!) WIFI library..
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include "Webpage.h"

//tActisenseReader ActisenseReader;
const size_t MaxClients = 10;
 WiFiUDP Udp;
 WiFiClient tcpclient;

const unsigned int tcp_def = 3000;          // default (factory) value; zero means TCP is OFF
WiFiServer tcpserver(tcp_def, MaxClients);  // the port may (will!) be changed later in StartWiFiPorts()




// Define READ_STREAM to port, where you write data from PC e.g. with NMEA Simulator.
#define READ_STREAM Serial
// Define ForwardStream - Where to send the stream to port, what you listen on PC side. On Arduino Due you can use e.g. SerialUSB
#define FORWARD_STREAM tcpclient // tcpclient? partially works? 

Stream *ReadStream = &READ_STREAM;
Stream *ForwardStream = &FORWARD_STREAM;

unsigned long _loopTiming;
unsigned long _debugTiming;
// for testing, lots of options for sending..
bool SendActisenseUDP = false;
bool SendActisenseTCP = false;
bool SendActisenseSerial = true;         //(needs UDP as well) // my n2K message "triggered" version
bool SendActisenseStream = false;  // the "background streaming" approach to USB "


void setup() {
  Serial.begin(115200);
  // TO just use AP - default - (N2000_Monitor), (PW 12345678)
      SetWIFI("N2000_Monitor", "", "", "", "", "", 0x0F);  // 0x0F Sets up  AP only 192.168.4.1 access
  // to setup to connect to a Home network use something like this to specify the ssid and pw,, and IP if you want a fixed ip.:
  // SetWIFI("N2000_Monitor", "", "", "SSID", "password", "192.168.0.120", 0x00);  //0x00 Sets Ap + EXT etc with FIXED IP if needed ...
 
  SetPorts(2002, 3003);  // just set ports --
  StartWiFi();
  _WebsocketsSetup();
  _WebServerSetup();

  NMEA2000.SetN2kCANSendFrameBufSize(300);
  NMEA2000.SetN2kCANReceiveFrameBufSize(300);

  if (ReadStream != ForwardStream) READ_STREAM.begin(115200);

  // not if tcp FORWARD_STREAM.begin(115200);  // or no serial!
  // ***** Comment out for debugging with less data on serial port
  // if (SendActisenseStream) {
  //   NMEA2000.SetForwardStream(ForwardStream);
  // }  // let it send out Actisense on USB so we can use the Actisense NMEA reader
  //NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);  // Show bus data in clear text (N2K ASCII)

  // NB.. This crashes ESP32
  // NMEA2000.SetForwardStream(&Udp);

  NMEA2000.SetForwardStream(&tcpclient);
  
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndSend);

  NMEA2000.SetMsgHandler(&HandleNMEA2000Msg);  // not essential, as stream is dealt with in the ParseMessages

  // recieve actisense ( example is from serial.. but we would want wifi) and do something out on N2k if needed ??
  //ActisenseReader.SetReadStream(ReadStream);
  //ActisenseReader.SetDefaultSource(75);
  //ActisenseReader.SetMsgHandler(HandleStreamN2kMsg);

  NMEA2000.Open();

  _loopTiming = millis();
  Serial.printf("GateWayIsSet <%d>  IsConnected <%d>", ReadGatewaySetup() ,ReadIsConnected());
  Serial.println(" Setup Complete");
}

//NMEA 2000 message handler.. NOTE the Stream part (NMEA2000.SetForwardStream) works independently of this handler!!
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  // This is called on every N2k message Rx,

  if (!Read_PauseFlag()) { WebsocketMonitorDataSendf("* PGN:%i Source:%i (%S)", N2kMsg.PGN, N2kMsg.Source, PGNDecode(N2kMsg.PGN)); }  //
  if (!SendActisenseUDP) { SendBufToUDPf("--PGN<%i><%S>\r\n", N2kMsg.PGN, PGNDecode(N2kMsg.PGN)); } // SHOW THE pgn AND DESCRIPTION ON UDP, PORT 2002
  if (!SendActisenseTCP) { SendBufToTCPf("--PGN<%i><%S>\r\n", N2kMsg.PGN, PGNDecode(N2kMsg.PGN)); } // SHOW THE pgn AND DESCRIPTION ON tcp, PORT 3003
  _SendinActisenseFormat(N2kMsg, SendActisenseUDP, SendActisenseTCP, SendActisenseSerial);
}

void HandleStreamN2kMsg(const tN2kMsg &N2kMsg) {
  // Is Only called by the Actisense RECEIVE portion!!
  // NMEA2000.SendMsg(N2kMsg,-1);
}

extern IPAddress sta_ip;  // identify sta_ip so we can access it in the webpage updates every second.
extern char ssidAP[16];   // ssid of the network to create in AP mode
extern char ssidST[16];   // ssid of the network to connect in STA mode

void Animate() {  // inserted as a test of how to change Text in an identified "id" in a div on a webpage using websocks.
  WebsocketDataSendf("WEBPAGE TEXT1 <center> <b>Actisense Settings: UDP{%i} TCP{%i}  Serial{%i}</b> </center>",SendActisenseUDP,SendActisenseTCP,SendActisenseSerial );
  WebsocketDataSendf("WEBPAGE TEXT0 Running Time:%i ", millis() / 1000);  // Spacing Critical between "WEBPAGE" and "ID"- there must be only one space!
  if (ReadIsConnected() && ReadGatewaySetup()) {
    WebsocketDataSendf("WEBPAGE TEXTIP SSID_AP:<b><i>%s</i></b>   Connected to:<b><i>%s</i></b> @IP:<b><i>%i.%i.%i.%i</i></b>  ",
                       ssidAP, ssidST, sta_ip[0], sta_ip[1], sta_ip[2], sta_ip[3]);
  } else {
    WebsocketDataSendf("WEBPAGE TEXTIP AP:<b><i>%s</i></b>   ", ssidAP);
  }
  //Serial.println(".");  // keep alive for serial port
}

extern bool IsTcpClient;
void loop() {
  CheckWiFi();
  NMEA2000.ParseMessages();
  //  ActisenseReader.ParseMessages(); //Switch on if we are using the Actisense streams >n2k
  WEBSERVE_LOOP();
  _WebsocketLOOP();
  if (_loopTiming <= millis()) {
    _loopTiming = millis() + 1000;
    Animate();  // inserted as a test of how to change Text in an identified "id" in a div on a webpage using websocks.
    //debug
    // Serial.printf(" Tcpclient <%i> \r\n",IsTcpClient);
  }
}

char * PGNDecode(int PGN) {  // decode the PGN to a readable name.. Useful for monitoring the bus?
  //https://endige.com/2050/nmea-2000-pgns-deciphered/
  // see also https://canboat.github.io/canboat/canboat.xml#pgn-list
  switch (PGN) {
    case 65311: return (char*) "Magnetic Variation (Raymarine Proprietary)"; break;
    case 60928: return (char*) "Address claimed message"; break;
    case 126720: return (char*) "Raymarine Device ID"; break;
    case 126992: return (char*) "System Time"; break;
    case 126993: return (char*) "Heartbeat"; break;
    case 127237: return (char*) "Heading/Track Control"; break;
    case 127245: return (char*) "Rudder"; break;
    case 127250: return (char*) "Vessel Heading"; break;
    case 127251: return (char*) "Rate of Turn"; break;
    case 127258: return (char*) "Magnetic Variation"; break;
    case 127488: return (char*) "Engine Parameters, Rapid Update"; break;
    case 127508: return (char*) "Battery Status"; break;
    case 127513: return (char*) "Battery Configuration Status"; break;
    case 128259: return (char*) "Speed, Water referenced"; break;
    case 128267: return (char*) "Water Depth"; break;
    case 128275: return (char*) "Distance Log"; break;
    case 129025: return (char*) "Position, Rapid Update"; break;
    case 129026: return (char*) "COG & SOG, Rapid Update"; break;
    case 129029: return (char*) "GNSS Position Data"; break;
    case 129033: return (char*) "Local Time Offset"; break;
    case 129044: return (char*) "Datum"; break;
    case 129283: return (char*) "Cross Track Error"; break;
    case 129284: return (char*) "Navigation Data"; break;
    case 129285: return (char*) "Navigation — Route/WP information"; break;
    case 129291: return (char*) "Set & Drift, Rapid Update"; break;
    case 129539: return (char*) "GNSS DOPs"; break;
    case 129540: return (char*) "GNSS Sats in View"; break;
    case 130066: return (char*) "Route and WP Service — Route/WP— List Attributes"; break;
    case 130067: return (char*) "Route and WP Service — Route — WP Name & Position"; break;
    case 130074: return (char*) "Route and WP Service — WP List — WP Name & Position"; break;
    case 130306: return (char*) "Wind Data"; break;
    case 130310: return (char*) "Environmental Parameters"; break;
    case 130311: return (char*) "Environmental Parameters"; break;
    case 130312: return (char*) "Temperature"; break;
    case 130313: return (char*) "Humidity"; break;
    case 130314: return (char*) "Actual Pressure"; break;
    case 130316: return (char*) "Temperature, Extended Range"; break;
    case 129038: return (char*) "AIS Class A Position Report"; break;
    case 129039: return (char*) "AIS Class B Position Report"; break;
    case 129040: return (char*) "AIS Class B Extended Position Report"; break;
    case 129041: return (char*) "AIS Aids to Navigation (AtoN) Report"; break;
    case 129793: return (char*) "AIS UTC and Date Report"; break;
    case 129794: return (char*) "AIS Class A Static and Voyage Related Data"; break;
    case 129798: return (char*) "AIS SAR Aircraft Position Report"; break;
    case 129809: return (char*) "AIS Class B “CS” Static Data Report, Part A"; break;
    case 129810: return (char*) "AIS Class B “CS” Static Data Report, Part B"; break;
  }

  return (char*) "unknown";
}


/// try to explicitly send Actisense "raw" message. Function basically taken from N2kMsg.cpp - where it seems to crash when sending to Udp
// added _ to names and function to ensure they  does not clash with "proper" definitions
#define _Escape 0x10
#define _StartOfText 0x02
#define _EndOfText 0x03
#define _MsgTypeN2k 0x93

void _AddByteEscapedToBuf(unsigned char byteToAdd, uint8_t &idx, unsigned char *buf, int &byteSum) {
  buf[idx++] = byteToAdd;
  byteSum += byteToAdd;

  if (byteToAdd == _Escape) {
    buf[idx++] = _Escape;
  }
}

//*****************************************************************************
void _PrintActisenseinTXT(const tN2kMsg &N2kMsg) {
  Serial.print(N2kMillis());
  Serial.print(F(" : "));
  Serial.print(F("Pri:"));
  Serial.print(N2kMsg.Priority);
  Serial.print(F(" PGN:"));
  Serial.print(N2kMsg.PGN);
  Serial.print(F(" Source:"));
  Serial.print(N2kMsg.Source);
  Serial.print(F(" Dest:"));
  Serial.print(N2kMsg.Destination);
  Serial.print(F(" Len:"));
  Serial.print(N2kMsg.DataLen);
  Serial.print(F(" Data:"));
  for (int i = 0; i < N2kMsg.DataLen; i++) {
    if (i > 0) { Serial.print(F(",")); };
    // Print bytes as hex.
    Serial.print(N2kMsg.Data[i], 16);
  }
  Serial.println(F(""));
}





void _debugPrint(const uint8_t *buffer, size_t size, String msg) {
  Serial.print(msg);
  Serial.print(":");
  for (int i = 0; i <= size; i++) {
    if (i > 0) { Serial.print(F(",")); };
    // Print bytes as hex.
    Serial.print(buffer[i], 16);
  }
  Serial.println(F(""));
}


//*****************************************************************************
// // Actisense Format:
// // <10><02><93><length (1)><priority (1)><PGN (3)><destination (1)><source (1)><time (4)><len (1)><data (len)><CRC (1)><10><03>
void _SendinActisenseFormat(const tN2kMsg &N2kMsg, bool udpsend, bool tcpsend, bool serialsend) {  // to udp if bool is true else TCP

  if (!udpsend && !tcpsend && !serialsend) { return; }

  unsigned long _PGN = N2kMsg.PGN;
  unsigned long _MsgTime = N2kMsg.MsgTime;
  uint8_t msgIdx = 0;
  int byteSum = 0;
  uint8_t CheckSum;
  unsigned char _ActisenseMsgBuf[400];


  //if (port==0 || !IsValid()) return;
  // Serial.print("freeMemory()="); Serial.println(freeMemory());

  _ActisenseMsgBuf[msgIdx++] = _Escape;
  _ActisenseMsgBuf[msgIdx++] = _StartOfText;
  _AddByteEscapedToBuf(_MsgTypeN2k, msgIdx, _ActisenseMsgBuf, byteSum);
  _AddByteEscapedToBuf(N2kMsg.DataLen + 11, msgIdx, _ActisenseMsgBuf, byteSum);  //length does not include escaped chars
  _AddByteEscapedToBuf(N2kMsg.Priority, msgIdx, _ActisenseMsgBuf, byteSum);
  _AddByteEscapedToBuf(_PGN & 0xff, msgIdx, _ActisenseMsgBuf, byteSum);
  _PGN >>= 8;
  _AddByteEscapedToBuf(_PGN & 0xff, msgIdx, _ActisenseMsgBuf, byteSum);
  _PGN >>= 8;
  _AddByteEscapedToBuf(_PGN & 0xff, msgIdx, _ActisenseMsgBuf, byteSum);
  _AddByteEscapedToBuf(N2kMsg.Destination, msgIdx, _ActisenseMsgBuf, byteSum);
  _AddByteEscapedToBuf(N2kMsg.Source, msgIdx, _ActisenseMsgBuf, byteSum);
  // Time?
  _AddByteEscapedToBuf(_MsgTime & 0xff, msgIdx, _ActisenseMsgBuf, byteSum);
  _MsgTime >>= 8;
  _AddByteEscapedToBuf(_MsgTime & 0xff, msgIdx, _ActisenseMsgBuf, byteSum);
  _MsgTime >>= 8;
  _AddByteEscapedToBuf(_MsgTime & 0xff, msgIdx, _ActisenseMsgBuf, byteSum);
  _MsgTime >>= 8;
  _AddByteEscapedToBuf(_MsgTime & 0xff, msgIdx, _ActisenseMsgBuf, byteSum);
  _AddByteEscapedToBuf(N2kMsg.DataLen, msgIdx, _ActisenseMsgBuf, byteSum);


  for (int i = 0; i < N2kMsg.DataLen; i++) _AddByteEscapedToBuf(N2kMsg.Data[i], msgIdx, _ActisenseMsgBuf, byteSum);
  byteSum %= 256;

  CheckSum = (uint8_t)((byteSum == 0) ? 0 : (256 - byteSum));
  _ActisenseMsgBuf[msgIdx++] = CheckSum;
  if (CheckSum == 0x10) _ActisenseMsgBuf[msgIdx++] = CheckSum;

  _ActisenseMsgBuf[msgIdx++] = 0x10;
  _ActisenseMsgBuf[msgIdx++] = _EndOfText;



  if (serialsend) { Serial.write(_ActisenseMsgBuf, msgIdx); }
  if (tcpsend) { WriteBufToTCP(_ActisenseMsgBuf, msgIdx); }
  if (udpsend) { WriteBufToUDP(_ActisenseMsgBuf, msgIdx); }
}