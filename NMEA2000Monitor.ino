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
#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Uncomment this and set right CAN TX pin definition, if you use ESP32 and do not have TX on default IO 16
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Uncomment this and set right CAN RX pin definition, if you use ESP32 and do not have RX on default IO 4

#include <Arduino.h>
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <NMEA2000_CAN.h>
//#include <ActisenseReader.h>

//#include "Defines.h"
#include "WIFIEVENT.h"  // my "easy to use" (eventually!) WIFI library..
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include "Webpage.h"
 #include "WebsocketInterpreter.h";
//tActisenseReader ActisenseReader;
WiFiUDP Udp;



// Define READ_STREAM to port, where you write data from PC e.g. with NMEA Simulator.
#define READ_STREAM Serial
// Define ForwardStream to port, what you listen on PC side. On Arduino Due you can use e.g. SerialUSB
#define FORWARD_STREAM Serial

Stream *ReadStream = &READ_STREAM;
Stream *ForwardStream = &FORWARD_STREAM;

unsigned long _loopTiming;
bool SendActisense = true;


void setup() {
   // TO just use AP (N2000_Monitor), (PW 12345678) 
  SetWIFI("N2000_Monitor", "", "", "","", "", 0x0F);  // Sets up  AP only
  // to setup to connect to a Home network use something like this to specify the ssid and pw,, and IP if you want a fixed ip.:
  // SetWIFI("N2000_Monitor", "", "", "SSID", "password", "192.168.0.120", 0x00);  // Sets Ap + EXT etc with FIXED IP if needed ...
  
  
  SetPorts(2002, 3003);  // just set ports -- We set UDP= 2002 here. TCP=3003 (not used at present)
  StartWiFi();
  _WebsocketsSetup();
  _WebServerSetup();

  NMEA2000.SetN2kCANSendFrameBufSize(300);
  NMEA2000.SetN2kCANReceiveFrameBufSize(300);

  if (ReadStream != ForwardStream) READ_STREAM.begin(115200);
    // ***** Comment out for debugging with serial port 
  FORWARD_STREAM.begin(115200);
  NMEA2000.SetForwardStream(ForwardStream);// let it send out Actisense on USB so we can use the Actisense NMEA reader
  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show bus data in clear text (N2K ASCII)
 
 
 // NB.. This crashes ESP32  
 // NMEA2000.SetForwardStream(&Udp);
  
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndSend);

  NMEA2000.SetMsgHandler(&HandleNMEA2000Msg);  // not essential, as stream is dealt with in the ParseMessages

  // recieve actisense over wifi and do something out on N2k if needed ??
  //ActisenseReader.SetReadStream(ReadStream);
  //ActisenseReader.SetDefaultSource(75);
  //ActisenseReader.SetMsgHandler(HandleStreamN2kMsg);

  NMEA2000.Open();

_loopTiming=millis();
}

//NMEA 2000 message handler.. NOTE the Stream part (NMEA2000.SetForwardStream) works independently of this handler!! 
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  // This is called on every N2k message Rx,
  if (!Read_PauseFlag()){WebsocketMonitorDataSendf("* PGN:%i Source:%i (%S)",N2kMsg.PGN,N2kMsg.Source,PGNDecode(N2kMsg.PGN)); }  // 
  if (!SendActisense) {SendBufToUDPf("--PGN<%i><%S>\r\n", N2kMsg.PGN, PGNDecode(N2kMsg.PGN));}// SHOW THE pgn AND DESCRIPTION ON UDP, PORT 2002 
  else {_SendInActisenseFormat(N2kMsg);}
  //if (SendActisense && !SendN2KASCII ) {WriteBufToUDP((N2kMsg.Data,N2kMsg.DataLen);} // works, but is not same data as shows on serial port 
   }

void HandleStreamN2kMsg(const tN2kMsg &N2kMsg) {
  // Is Only called by the Actisense RECEIVE portion!!
  // NMEA2000.SendMsg(N2kMsg,-1);
}
extern IPAddress sta_ip;   // identify sta_ip so we can access it in the webpage updates every second. 
extern char ssidAP[16];      // ssid of the network to create in AP mode
extern char ssidST[16];      // ssid of the network to connect in STA mode

void loop() {
  CheckWiFi();
  NMEA2000.ParseMessages();
  //  ActisenseReader.ParseMessages(); //Switch on if we are using the Actisense streams
  WEBSERVE_LOOP();
  _WebsocketLOOP();
 if((_loopTiming-millis())>=1000){  // inserted as a test of how to change Text in an identified "id" in a div on a webpage using websocks.
   _loopTiming=millis();
   WebsocketDataSendf("WEBPAGE TEXT0 Running Time:%i ",millis()/1000);  // Spacing Critical between "WEBPAGE" and "ID"- there must be only one space!
    if(ReadIsConnected()&&ReadGatewaySetup()){
      WebsocketDataSendf("WEBPAGE TEXTIP SSID_AP:<b><i>%s</i></b>   Connected to:<b><i>%s</i></b> @IP:<b><i>%i.%i.%i.%i</i></b>  ", 
                   ssidAP,ssidST, sta_ip[0],sta_ip[1],sta_ip[2],sta_ip[3]);}
      else { WebsocketDataSendf("WEBPAGE TEXTIP AP:<b><i>%s</i></b>   ",ssidAP);}             

   WebsocketDataSendf("WEBPAGE TEXT1 <small><center> External-Network_IP:<b><i> %i.%i.%i.%i</i></b> </small></center>", sta_ip[0],sta_ip[1],sta_ip[2],sta_ip[3]);
 }
}

char * PGNDecode(int PGN) { // decode the PGN to a readable name.. Useful for monitoring the bus? 
  //https://endige.com/2050/nmea-2000-pgns-deciphered/
  // see also https://canboat.github.io/canboat/canboat.xml#pgn-list 
  switch (PGN) {
    case 65311: return "Magnetic Variation (Raymarine Proprietary)"; break;
    case 126720: return "Raymarine Device ID"; break;
    case 126992: return "System Time"; break;
    case 126993: return "Heartbeat"; break;
    case 127237: return "Heading/Track Control"; break;
    case 127245: return "Rudder"; break;
    case 127250: return "Vessel Heading"; break;
    case 127251: return "Rate of Turn"; break;
    case 127258: return "Magnetic Variation"; break;
    case 127488: return "Engine Parameters, Rapid Update"; break;
    case 127508: return "Battery Status"; break;
    case 127513: return "Battery Configuration Status"; break;
    case 128259: return "Speed, Water referenced"; break;
    case 128267: return "Water Depth"; break;
    case 128275: return "Distance Log"; break;
    case 129025: return "Position, Rapid Update"; break;
    case 129026: return "COG & SOG, Rapid Update"; break;
    case 129029: return "GNSS Position Data"; break;
    case 129033: return "Local Time Offset"; break;
    case 129044: return "Datum"; break;
    case 129283: return "Cross Track Error"; break;
    case 129284: return "Navigation Data"; break;
    case 129285: return "Navigation — Route/WP information"; break;
    case 129291: return "Set & Drift, Rapid Update"; break;
    case 129539: return "GNSS DOPs"; break;
    case 129540: return "GNSS Sats in View"; break;
    case 130066: return "Route and WP Service — Route/WP— List Attributes"; break;
    case 130067: return "Route and WP Service — Route — WP Name & Position"; break;
    case 130074: return "Route and WP Service — WP List — WP Name & Position"; break;
    case 130306: return "Wind Data"; break;
    case 130310: return "Environmental Parameters"; break;
    case 130311: return "Environmental Parameters"; break;
    case 130312: return "Temperature"; break;
    case 130313: return "Humidity"; break;
    case 130314: return "Actual Pressure"; break;
    case 130316: return "Temperature, Extended Range"; break;
    case 129038: return "AIS Class A Position Report"; break;
    case 129039: return "AIS Class B Position Report"; break;
    case 129040: return "AIS Class B Extended Position Report"; break;
    case 129041: return "AIS Aids to Navigation (AtoN) Report"; break;
    case 129793: return "AIS UTC and Date Report"; break;
    case 129794: return "AIS Class A Static and Voyage Related Data"; break;
    case 129798: return "AIS SAR Aircraft Position Report"; break;
    case 129809: return "AIS Class B “CS” Static Data Report, Part A"; break;
    case 129810: return "AIS Class B “CS” Static Data Report, Part B"; break;
  }

  return "unknown";
}


/// try to explicitly send Actisense "raw" message. Function basically taken from N2kMsg.cpp - where it seems to crash when sending to Udp
// added _ to names and function to ensure they  does not clash with "proper" definitions
#define _Escape 0x10  
#define _StartOfText 0x02
#define _EndOfText 0x03
#define _MsgTypeN2k 0x93

 void _AddByteEscapedToBuf(unsigned char byteToAdd, uint8_t &idx, unsigned char *buf, int &byteSum)
{
  buf[idx++]=byteToAdd;
  byteSum+=byteToAdd;

  if (byteToAdd == _Escape) {
    buf[idx++]=_Escape;
  }
}

//*****************************************************************************
// // Actisense Format:
// // <10><02><93><length (1)><priority (1)><PGN (3)><destination (1)><source (1)><time (4)><len (1)><data (len)><CRC (1)><10><03>
void _SendInActisenseFormat(const tN2kMsg &N2kMsg)  
{
  unsigned long _PGN= N2kMsg.PGN;
  unsigned long _MsgTime=N2kMsg.MsgTime;
  uint8_t msgIdx=0;
  int byteSum = 0;
  uint8_t CheckSum;
  unsigned char _ActisenseMsgBuf[400];

  //if (port==0 || !IsValid()) return;
  // Serial.print("freeMemory()="); Serial.println(freeMemory());

  _ActisenseMsgBuf[msgIdx++]=_Escape;
  _ActisenseMsgBuf[msgIdx++]=_StartOfText;
  _AddByteEscapedToBuf(_MsgTypeN2k,msgIdx,_ActisenseMsgBuf,byteSum);
  _AddByteEscapedToBuf(N2kMsg.DataLen+11,msgIdx,_ActisenseMsgBuf,byteSum); //length does not include escaped chars
  _AddByteEscapedToBuf(N2kMsg.Priority,msgIdx,_ActisenseMsgBuf,byteSum);
  _AddByteEscapedToBuf(_PGN & 0xff,msgIdx,_ActisenseMsgBuf,byteSum); _PGN>>=8;
  _AddByteEscapedToBuf(_PGN & 0xff,msgIdx,_ActisenseMsgBuf,byteSum); _PGN>>=8;
  _AddByteEscapedToBuf(_PGN & 0xff,msgIdx,_ActisenseMsgBuf,byteSum);
  _AddByteEscapedToBuf(N2kMsg.Destination,msgIdx,_ActisenseMsgBuf,byteSum);
  _AddByteEscapedToBuf(N2kMsg.Source,msgIdx,_ActisenseMsgBuf,byteSum);
  // Time?
  _AddByteEscapedToBuf(_MsgTime & 0xff,msgIdx,_ActisenseMsgBuf,byteSum); _MsgTime>>=8;
  _AddByteEscapedToBuf(_MsgTime & 0xff,msgIdx,_ActisenseMsgBuf,byteSum); _MsgTime>>=8;
  _AddByteEscapedToBuf(_MsgTime & 0xff,msgIdx,_ActisenseMsgBuf,byteSum); _MsgTime>>=8;
  _AddByteEscapedToBuf(_MsgTime & 0xff,msgIdx,_ActisenseMsgBuf,byteSum);
  _AddByteEscapedToBuf(N2kMsg.DataLen,msgIdx,_ActisenseMsgBuf,byteSum);


  for (int i = 0; i < N2kMsg.DataLen; i++) _AddByteEscapedToBuf(N2kMsg.Data[i],msgIdx,_ActisenseMsgBuf,byteSum);
  byteSum %= 256;

  CheckSum = (uint8_t)((byteSum == 0) ? 0 : (256 - byteSum));
  _ActisenseMsgBuf[msgIdx++]=CheckSum;
  if (CheckSum==0x10) _ActisenseMsgBuf[msgIdx++]=CheckSum;

  _ActisenseMsgBuf[msgIdx++] = 0x10;
  _ActisenseMsgBuf[msgIdx++] = _EndOfText;
 // will do (write) something with (_ActisenseMsgBuf,msgIdx) once this gets here!
  Serial.write(_ActisenseMsgBuf,msgIdx); //tested on serial to see if this is the same data as the forward stream Serial. and Actisense NMEA Reader DOES understand it 
  //WriteBufToUDP(_ActisenseMsgBuf,msgIdx);


 // if ( port->availableForWrite()>msgIdx ) {  // 16.7.2017 did not work yet
 //   port->write(_ActisenseMsgBuf,msgIdx);
 // }
 //   Serial.print("Actisense data:");
    //PrintBuf(msgIdx,ActisenseMsgBuf);
 //   Serial.print("\r\n");
}