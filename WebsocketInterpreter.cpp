#include "WebsocketInterpreter.h"
#include <WebSocketsServer.h>  // https://github.com/Links2004/arduinoWebSockets



extern WebSocketsServer webSocket;

String webSocketData = "";

String DataOutput1;
extern void handleTest();
extern void SetNMEAFLAG(bool set);   // or possibly could have just had ext bool ?
extern void SetPauseFlag(bool set);  // but it demonstrates that stuff in the .ino IS NOT "global"

extern bool SendActisenseUDP,SendActisenseTCP,SendActisenseSerial;


void webSocketDataInterpreter(WebSocketsServer &WEBSOCKETOBJECT, String WEBSOCKETDATA) {  //not used ?
  String topLevelToken = "";
  String subLevelToken = "";
  String serialClear = "SERIAL UART CLEAR";
  Serial.println("New data received: " + WEBSOCKETDATA);  //  turn on for debugging!

  if (WEBSOCKETDATA.startsWith("SERIAL_OUT")) {
    topLevelToken = "SERIAL_OUT";
    Serial.println(WEBSOCKETDATA.substring(topLevelToken.length() + 1));
  }

  //Terminal related tasks
  if (WEBSOCKETDATA.startsWith("WEBMONITOR")) {
    //Look at start of line for tokens, add +1 to length to account for space
    topLevelToken = "WEBMONITOR";
    String terminalCommand = WEBSOCKETDATA.substring(topLevelToken.length() + 1);
    if (terminalCommand.startsWith("Actisense")) {
      SendActisenseUDP=true;SendActisenseTCP=true;SendActisenseSerial=true;
    }
    if (terminalCommand.startsWith("NOActisense")) {
      SendActisenseUDP=false;SendActisenseTCP=false;SendActisenseSerial=false;
    }

    if (terminalCommand.startsWith("PAUSE")) {
      subLevelToken = "PAUSE";
      if (terminalCommand.substring(subLevelToken.length() + 1) == "ON") {
        SetPauseFlag(true);
      } else {
        SetPauseFlag(false);
      }
    }
  }
}





static char msg[129] = { '\0' };
char _sendTOTerm[13] = "SERIAL UART ";  // saves adding it 

void WebsocketDataSendf(const char *fmt, ...) {
  va_list args;  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  //Serial.println("\r\n*** WDSEND " );Serial.println(msgOUT);  Serial.println("*********************** " );
  webSocket.broadcastTXT(msg);
  //Serial.println("     Sent Websocket Broadcast!");
}

void WebsocketMonitorDataSendf(const char *fmt, ...) {

  va_list args;  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  // add checksum?
  int len = strlen(msg);
  char msgOUT[141];
  strcpy(msgOUT, _sendTOTerm);
  strcat(msgOUT, msg);
  // debugging !
  //Serial.println("\r\n*** WDSEND " );Serial.println(msgOUT);  Serial.println("*********************** " );
  webSocket.broadcastTXT(msgOUT);
  //Serial.println("     Sent Websocket Broadcast!");
}
void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
  const uint8_t *src = (const uint8_t *)mem;
  Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for (uint32_t i = 0; i < len; i++) {
    if (i % cols == 0) {
      Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    Serial.printf("%02X ", *src);
    src++;
  }
  Serial.printf("\n");
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      // if (!OTA_ON()){
      webSocket.close();
      _WebsocketsSetup();
      Serial.printf("[%u] ATTEMPTING RESTART OF WEBSERVER!\r\n", num);  // not compatible with  OTA?
                                                                        // }
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      webSocketData = String((const char *)payload);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);

      hexdump(payload, length);  /// from debug.h in esp8266

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    case WStype_PONG:
      Serial.printf("WStype [%d] is PONG\r\n", type);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);  // see http://www.martyncurrey.com/esp8266-and-the-arduino-ide-part-9-websockets/
      break;
  }
}

void _WebsocketsSetup() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}
void _WebsocketLOOP() {
  webSocket.loop();  // do any websocket send / rx
  if (webSocketData != "") {
    webSocketDataInterpreter(webSocket, webSocketData);
    Serial.println("Websocket data Handle");
    webSocketData = "";
  }
}
