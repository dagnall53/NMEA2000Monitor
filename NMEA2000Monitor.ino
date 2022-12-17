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
#include "WebsocketInterpreter.h"
#include "WIFIEVENT.h"  // my "easy to use" (eventually!) WIFI library..
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include "Webpage.h"

//tActisenseReader ActisenseReader;
WiFiUDP Udp;



// Define READ_STREAM to port, where you write data from PC e.g. with NMEA Simulator.
#define READ_STREAM Serial
// Define ForwardStream to port, what you listen on PC side. On Arduino Due you can use e.g. SerialUSB
#define FORWARD_STREAM Serial

Stream *ReadStream = &READ_STREAM;
Stream *ForwardStream = &FORWARD_STREAM;

unsigned long _loopTiming;
unsigned long _debugTiming;
// for testing, lots of options for sending.. 
bool SendActisenseUDP = true;
bool SendActisenseTCP = true;
bool SendActisenseSerial = true; //(needs UDP as well) // my n2K message "triggered" version
bool SendActisenseSerialStream=false;  // the "background streaming" approach to USB "


void setup() {
  Serial.begin(115200);
  // TO just use AP (N2000_Monitor), (PW 12345678)
  SetWIFI("N2000_Monitor", "", "", "", "", "", 0x0F);  // Sets up  AP only
  // to setup to connect to a Home network use something like this to specify the ssid and pw,, and IP if you want a fixed ip.:
  // SetWIFI("N2000_Monitor", "", "", "SSID", "password", "192.168.0.120", 0x00);  // Sets Ap + EXT etc with FIXED IP if needed ...
  SetWIFI("N2000_Monitor", "", "", "VM7135825", "cxy6cfhdMqwg", "192.168.0.120", 0x00);  // Sets Ap + EXT etc with FIXED IP if needed ...

  SetPorts(2000, 3000);  // just set ports --
  StartWiFi();
  _WebsocketsSetup();
  _WebServerSetup();

  NMEA2000.SetN2kCANSendFrameBufSize(300);
  NMEA2000.SetN2kCANReceiveFrameBufSize(300);

  if (ReadStream != ForwardStream) READ_STREAM.begin(115200);

  FORWARD_STREAM.begin(115200);  // or no serial!
  // ***** Comment out for debugging with less data on serial port
  if (SendActisenseSerialStream){
  NMEA2000.SetForwardStream(ForwardStream);}  // let it send out Actisense on USB so we can use the Actisense NMEA reader
  //NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);  // Show bus data in clear text (N2K ASCII)


  // NB.. This crashes ESP32
  // NMEA2000.SetForwardStream(&Udp);

  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndSend);

  NMEA2000.SetMsgHandler(&HandleNMEA2000Msg);  // not essential, as stream is dealt with in the ParseMessages

  // recieve actisense over wifi and do something out on N2k if needed ??
  //ActisenseReader.SetReadStream(ReadStream);
  //ActisenseReader.SetDefaultSource(75);
  //ActisenseReader.SetMsgHandler(HandleStreamN2kMsg);

  NMEA2000.Open();

  _loopTiming = millis();
}

//NMEA 2000 message handler.. NOTE the Stream part (NMEA2000.SetForwardStream) works independently of this handler!!
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  // This is called on every N2k message Rx,

  if (!Read_PauseFlag()) { WebsocketMonitorDataSendf("* PGN:%i Source:%i (%S)", N2kMsg.PGN, N2kMsg.Source, PGNDecode(N2kMsg.PGN)); }  //
  if (!SendActisenseUDP) { SendBufToUDPf("--PGN<%i><%S>\r\n", N2kMsg.PGN, PGNDecode(N2kMsg.PGN)); }                                   // SHOW THE pgn AND DESCRIPTION ON UDP, PORT 2002
  else {
    _SendinActisenseFormat(N2kMsg, true);
  }
  if (!SendActisenseTCP) { SendBufToTCPf("--PGN<%i><%S>\r\n", N2kMsg.PGN, PGNDecode(N2kMsg.PGN)); }  // SHOW THE pgn AND DESCRIPTION ON UDP, PORT 2002
  else {
    _SendinActisenseFormat(N2kMsg, false);
  }
}

void HandleStreamN2kMsg(const tN2kMsg &N2kMsg) {
  // Is Only called by the Actisense RECEIVE portion!!
  // NMEA2000.SendMsg(N2kMsg,-1);
}

extern IPAddress sta_ip;  // identify sta_ip so we can access it in the webpage updates every second.
extern char ssidAP[16];   // ssid of the network to create in AP mode
extern char ssidST[16];   // ssid of the network to connect in STA mode

void Animate() {  // inserted as a test of how to change Text in an identified "id" in a div on a webpage using websocks.
  WebsocketDataSendf("WEBPAGE TEXT1 <small><center> External-Network_IP:<b><i> %i.%i.%i.%i</i></b> </small></center>", sta_ip[0], sta_ip[1], sta_ip[2], sta_ip[3]);
  WebsocketDataSendf("WEBPAGE TEXT0 Running Time:%i ", millis() / 1000);  // Spacing Critical between "WEBPAGE" and "ID"- there must be only one space!
  if (ReadIsConnected() && ReadGatewaySetup()) {
    WebsocketDataSendf("WEBPAGE TEXTIP SSID_AP:<b><i>%s</i></b>   Connected to:<b><i>%s</i></b> @IP:<b><i>%i.%i.%i.%i</i></b>  ",
                       ssidAP, ssidST, sta_ip[0], sta_ip[1], sta_ip[2], sta_ip[3]);
  } else {
    WebsocketDataSendf("WEBPAGE TEXTIP AP:<b><i>%s</i></b>   ", ssidAP);
  }
  Serial.println(".");  // keep alive for serial port
}

extern bool IsTcpClient;
void loop() {
  CheckWiFi();
  NMEA2000.ParseMessages();
  //  ActisenseReader.ParseMessages(); //Switch on if we are using the Actisense streams
  WEBSERVE_LOOP();
  _WebsocketLOOP();
  if (_loopTiming <= millis()) {
    _loopTiming = millis() + 1000;
    Animate();  // inserted as a test of how to change Text in an identified "id" in a div on a webpage using websocks.
    //debug
    // Serial.printf(" Tcpclient <%i> \r\n",IsTcpClient);
  }
}

char *PGNDecode(int PGN) {  // decode the PGN to a readable name.. Useful for monitoring the bus?
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


// Endian swap test // covering all options! 
 unsigned char _ActisenseMsgBuf[400];
void EndianSwap( size_t size, bool _byte, bool _order) {
  unsigned char temp[400];
  unsigned char _tempbyte;
  for (int i = 0; i <= size; i++) {
    if (_order) {
      temp[i] = _ActisenseMsgBuf[size - i];
    } else {
      temp[i] = _ActisenseMsgBuf[i];
    }
    if (_byte) {
      _tempbyte = 0;
      if ((temp[i] & 128) == 128) { _tempbyte = _tempbyte | 1; }
      if ((temp[i] & 64) == 64) { _tempbyte = _tempbyte | 2; }
      if ((temp[i] & 32) == 32) { _tempbyte = _tempbyte | 4; }
      if ((temp[i] & 16) == 16) { _tempbyte = _tempbyte | 8; }
      if ((temp[i] & 8) == 8) { _tempbyte = _tempbyte | 16; }
      if ((temp[i] & 4) == 4) { _tempbyte = _tempbyte | 32; }
      if ((temp[i] & 2) == 2) { _tempbyte = _tempbyte | 64; }
      if ((temp[i] & 1) == 1) { _tempbyte = _tempbyte | 128; }
      temp[i] = _tempbyte;      
    }
  }
  for (int i = 0; i <= size; i++) {
    _ActisenseMsgBuf[i]=temp[i];
  }

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
void _SendinActisenseFormat(const tN2kMsg &N2kMsg, bool udp) {  // to udp if bool is true else TCP
  unsigned long _PGN = N2kMsg.PGN;
  unsigned long _MsgTime = N2kMsg.MsgTime;
  uint8_t msgIdx = 0;
  int byteSum = 0;
  uint8_t CheckSum;
 // unsigned char _ActisenseMsgBuf[400];

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
  // can now  do (write) something with (_ActisenseMsgBuf,msgIdx) once we  get here!
  if(SendActisenseSerial && udp ){Serial.write( _ActisenseMsgBuf,msgIdx);} // only if UDP is on.. (reduce traffic in tests)
  // playing with endianness
 // Serial.println();
  //_debugPrint(_ActisenseMsgBuf, msgIdx,"start"); // Testing each endianswap in turn.. not (false,true) // not true,false // not true true //blast!!
  // EndianSwap( msgIdx, true, true);
  // _debugPrint(_ActisenseMsgBuf, msgIdx," None");


  if (udp) {
    WriteBufToUDP(_ActisenseMsgBuf, msgIdx);
  } else {
    SendBufToTCP("\r\n");
    WriteBufToTCP(_ActisenseMsgBuf, msgIdx);
    SendBufToTCP(">\r\n");
  }



  // if ( port->availableForWrite()>msgIdx ) {  // 16.7.2017 did not work yet
  //   port->write(_ActisenseMsgBuf,msgIdx);
  // }
  //   Serial.print("Actisense data:");
  //PrintBuf(msgIdx,ActisenseMsgBuf);
  //   Serial.print("\r\n");
}