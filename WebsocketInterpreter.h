#include <Arduino.h>
#include <Wire.h>




void WebsocketDataSendf(const char *fmt, ...); // simple websocket send
void WebsocketMonitorDataSendf(const char* fmt, ...);// websocket sent woith "SERIAL UART " placed at beginning of message for Monitor display
void _WebsocketsSetup();
void _WebsocketLOOP();
