// Demo: Supplies a Wepage to show what PGN have been seen.
//
// Based on 
//   NMEA2000 library. Bus listener and sender.
//   Sends all bus data to serial in Actisense format.
//   Send all data received from serial in Actisense format to the N2kBus.
//   Use this e.g. with NMEA Simulator (see. http://www.kave.fi/Apps/index.html) to send simulated data to the bus.
//   Meanwhile you can define other stream to different port so that you can send data with NMEA Simulator and listen it on other port with
//   Actisense NMEA Reader.
//   I have tried to get the Actisense Reader to send "actisense over UDP- but this isso far unsucessful.. see notes below. " 
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
 #include "WebsocketInterpreter.h";
//tActisenseReader ActisenseReader;
WiFiUDP Udp;



// Define READ_STREAM to port, where you write data from PC e.g. with NMEA Simulator.
#define READ_STREAM Serial
// Define ForwardStream to port, what you listen on PC side. On Arduino Due you can use e.g. SerialUSB
#define FORWARD_STREAM Serial

Stream *ReadStream = &READ_STREAM;
Stream *ForwardStream = &FORWARD_STREAM;


// Make it easy to log into the local network for testing:
#include "Passwords.h" 
// Add a Passwords.h file in the sketch folder with these correctly filled in
//************************************************************************
//char *ssid = "My Home network";
//char *password = "My Password";
//************************************************************************
void setup() {
  SetWIFI("N2000_Monitor", "", "", ssid, password, "192.168.0.120", 0x00);  // Sets Ap  EXT etc FIXED IP if needed ...
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
    //**********
  // 
  //NMEA2000.SetForwardStream(&Udp); //THIS CRASHES!..but it should forward stream to  port 2000  UDP??  ?see forum posts of 8 dec 2022
  
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndSend);

  // This (next) needs to be commented out for the ActisenseNMEA reader to work .
  // but helps if just looking at serial
  //NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show bus data in clear text
  NMEA2000.SetMsgHandler(&HandleNMEA2000Msg);

  // recieve actisense over wifi and do something out on N2k if needed ??
  //ActisenseReader.SetReadStream(ReadStream);
  //ActisenseReader.SetDefaultSource(75);
  //ActisenseReader.SetMsgHandler(HandleStreamN2kMsg);

  NMEA2000.Open();
}

//NMEA 2000 message handler.. NOTE the Stream part (NMEA2000.SetForwardStream) works independently of this handler!! 
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  // This is called on every N2k message Rx,
  if (Read_PauseFlag()){WebsocketDebugSend("* PGN:%i  (%S)",N2kMsg.PGN,PGNDecode(N2kMsg.PGN)); }  // Could it really be this simple? AVOID USE OF <> as it will screw up the web display in html!!!! 
  //Serial.print("Got PGN:");Serial.println(PGNDecode(N2kMsg.PGN));  //NB The SERIAL PRINT does not interfere with the Actisense Reader in serial connected mode!
  SendBufToUDPf("--PGN<%i><%S>\r\n", N2kMsg.PGN, PGNDecode(N2kMsg.PGN)); // SHOW THE pgn AND DESCRIPTION ON UDP, PORT 2002 
  }

void HandleStreamN2kMsg(const tN2kMsg &N2kMsg) {
  // Is Only called by the Actisense RECEIVE portion!!
  // NMEA2000.SendMsg(N2kMsg,-1);
}

void loop() {
  CheckWiFi();
  NMEA2000.ParseMessages();
  //  ActisenseReader.ParseMessages(); //Switch on if we are using the Actisense streams
  WEBSERVE_LOOP();
  _WebsocketLOOP();
 
}

char * PGNDecode(int PGN) { // decode the PGN to a readable name.. Useful for monitoring the bus? 
  //https://endige.com/2050/nmea-2000-pgns-deciphered/
  switch (PGN) {
    case 65311: return "Magnetic Variation (Raymarine Proprietary)"; break;
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
