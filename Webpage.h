/// Adding some websocket stuff to assist debugging and demonstrate the "OTHER" debug page // keep this at the end
#include <Arduino.h>
#include "WebServer.h"
#include "WebsocketInterpreter.h"
#include <WebSocketsServer.h> // https://github.com/Links2004/arduinoWebSockets
#include "OTA_WEB.h"
// NB WEBPAGE "DEMOWEBPAGE_HTML" is defined in a char lower down.

WebSocketsServer webSocket = WebSocketsServer(81);
WebServer webserver(80);  // for the webbrowser 


bool _sendNMEA = false;  // makes it easier to check webpage is connected with websocks if it defaults true
bool _SerialPGN = false;
bool _PauseFlag = true;
extern bool ReadDemoDebugMode();

void WebServerSettings();         // with no {} this just defines the function title so I can place SETUP etc  here at the top for visibilty. . 
void handleSettings();         //the actual function code is defined later..
static char msg[129] = { '\0' };  // used in message buildup

void _WebServerSetup() {
  WebServerSettings();
}
void WEBSERVE_LOOP() {
  webserver.handleClient();  // do any webbrowser interfacing in the main ino loop
}
// set read some flags that might be local if this was a proper cpp/h

void SetPauseFlag(bool set) {
  _PauseFlag = set;
}
bool Read_PauseFlag() {
  return _PauseFlag;
}

void SetSerialPGNFLAG(bool set) {
  _SerialPGN = set;
}
bool Read_SerialPGN(void) {
  return _SerialPGN;
}

void SetNMEAFLAG(bool set) {
  _sendNMEA = set;
}
bool Read_sendNMEA(void) {
  return _sendNMEA;
}

static const char PROGMEM DEMOWEBPAGE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
    <title>NMEA 2K  MonitorI</title>
 
 <script>
  var wsMessageArray = "";
  var currentScreenElement = "WEBMONITOR";
  var terminalPauseFlag = true;
  var ActisenseModeFlag = true;
   var TBAModeFlag = true;

  function selectTerminal()
    { currentScreenElement = "WEBMONITOR";
      updateButtonSelect(currentScreenElement); }

  function terminalClear()
    { var outputTable = document.getElementById('terminalOutput');
      var terminalOutputWrapperDiv = document.getElementById('terminalOutputWrapper');
      outputTable.innerHTML = '<tr><td></tr><td>';
      terminalOutputWrapperDiv.scrollTop = terminalOutputWrapperDiv.scrollHeight;
    }

   function terminalPause()
    { if (terminalPauseFlag)
      { terminalPauseFlag = false;
        document.getElementById("connectTerminalButton").innerHTML = "<b>Paused</b>";
        websock.send("WEBMONITOR PAUSE ON");
      }
      else
      { terminalPauseFlag = true;
        document.getElementById("connectTerminalButton").innerHTML = "<b>ON</b>";
        websock.send("WEBMONITOR PAUSE OFF");
      }
    }

    function ActisenseMode() {
      if (ActisenseModeFlag) {
        ActisenseModeFlag = false;
        document.getElementById("ActisenseModeButton").innerHTML = "<b>Actisense</b>";  // button is inverse to what is being shown now
        websock.send("WEBMONITOR Actisense");
      } else 
      { ActisenseModeFlag = true;
        document.getElementById("ActisenseModeButton").innerHTML = "<b>None</b>";
        websock.send("WEBMONITOR NOActisense");
      }
    }
       function TBAMode() {
      if (TBAModeFlag) {
        TBAModeFlag = false;
        document.getElementById("TBAModeButton").innerHTML = "<b>Actisense</b>";  // button is inverse to what is being shown now
        websock.send("WEBMONITOR Actisense");
      } else 
      { TBAModeFlag = true;
        document.getElementById("TBAModeButton").innerHTML = "<b>None</b>";
        websock.send("WEBMONITOR NOActisense");
      }
    }

  function replaceText(){
     //var divID =wsMessageArray[1]; // where to put it ..
     var text =""; 
     for (let i = 2; i < wsMessageArray.length; i++) {
      text += wsMessageArray[i] + " ";
     }
     document.getElementById(wsMessageArray[1]).innerHTML = text;

  }  


 function serialEventHandler()
    {
      if(wsMessageArray[1] === "UART")
      {
        var outputTable = document.getElementById('terminalOutput');
        var terminalOutputWrapperDiv = document.getElementById('terminalOutputWrapper');
        var slicedWsMessageArray = wsMessageArray.slice(2).join(' ');
        outputTable.innerHTML = outputTable.innerHTML + '<tr><td>' + slicedWsMessageArray + '</tr><td>';
        terminalOutputWrapperDiv.scrollTop = terminalOutputWrapperDiv.scrollHeight;
        if(wsMessageArray[2] === "CLEAR")
        {
          terminalClear();
        }
      }
    }



 function start()
    {
      websock = new WebSocket('ws://' + window.location.hostname + ':81/');
      websock.onopen = function(evt)
      {
        console.log('websock open');
        // send PAUSE OFF -- To start any send.. 
        terminalPauseFlag = false;
        document.getElementById("connectTerminalButton").innerHTML = "<b>-Run-</b>";
        websock.send("WEBMONITOR PAUSE OFF");
        //do anything else here for setup if needed 
      };
      websock.onclose = function(evt)
      {
        console.log('websock close');
      };
      websock.onerror = function(evt)
      {
        console.log(evt);
      };
      websock.onmessage = function(evt)
      {
        //console.log(evt);
        wsMessageArray = evt.data.split(" ");
        if(wsMessageArray.length >= 2)
        {
          if(wsMessageArray[0] === "SERIAL")
           {
           serialEventHandler();
          }  
          if(wsMessageArray[0] === "WEBPAGE")
           {
           replaceText();
          }  
        }
      };
      
    }

 </script>

</head>
<style> 
.VertBoxStyle{
    display: block;
    -webkit-appearance: none;
    width: 90%;
    height: 90%;
    background-color: lightblue
    color: white;
    border-radius: 5px;
    font-family: Helvetica;
    margin-left: 5%;
    font-size: 15px;
}
.SettingsTitle{
   width:100%;
   height: 2.5vh;
   font-size: 20px;
 }
.BigBlue {
  background-color: lightgrey;
  width: 70va;
  border: 10px solid blue;
  padding: 10px;
  margin: 5px;
}
.absolute4 {
  position: absolute;
  top: 10px;
  right: 350px;
  width: 100px;
  height: 30px;
  }
.absolute3 {
  position: absolute;
  top: 10px;
  right: 240px;
  width: 100px;
  height: 30px;
  }
.absolute2 {
  position: absolute;
  top: 10px;
  right: 130px;
  width: 100px;
  height: 30px;
  }
.absolute {
  position: absolute;
  top: 10px;
  right: 20px;
  width: 100px;
  height: 30px;
 }
</style>
<body onload="javascript:start();" id="mainBody">
<center><h2>NMEA2000 Monitor</h2><div id= "TEXT0"> Top text</div><br>
<div id= "TEXTIP"> IPdata </div><br>
</center>
<div class= "absolute2" ><button id="connectTerminalButton" class="VertBoxStyle"  onclick="terminalPause()"><b>Pause?</b></button> </div>
<div class= "absolute3" ><button id="ActisenseModeButton" class="VertBoxStyle"  onclick="ActisenseMode()"><b>Actisense</b></button> </div>
<div class= "absolute4" ><button id="TBAModeButton" class="VertBoxStyle"  onclick="TBAMode()"><b>-TBA-</b></button> </div>
<div class= "absolute" ><button id="TerminalClearButton" class="VertBoxStyle"  onclick="terminalClear()"><b>CLEAR</b></button> </div>

<p>This will display received PGN numbers and interpretation where known. This data is also being sent as a UDP send on port 2002.<br>
If enabled, Actisense encoded N2K messages are being sent from the USB port (and UDP and TCP) for decode by the Actisense NMEA reader<br> 
Other Message decodes will be added later. 
</p>
   
<div class= "BigBlue"id="terminalOutputWrapper" style="height: 65vh; overflow:auto; background-color:white; border-radius: 5px;">
      <table id="terminalOutput" style="color:#4E4E56; text-align:left;">
        <tbody><tr>
          <td></td>
        </tr>
      </tbody></table>
    </div>
    <form action="NMEA">
     <center><input class="but" style="width:20%;" type="submit" value="HomePage"></center>
     </form>
    <form action="ota">
     <center><input class="but" style="width:20%;" type="submit" value="OTA UPDATE"></center>
    </form> 
<div id= "TEXT1" >More text, potentialy a footer. <br> </div>

</body>

</html>

)rawliteral";

long max_sketch_size() {
  long ret = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  return ret;
}
void OTASettings() {
    // Upload firmware page
    webserver.on("/ota", HTTP_GET, []() {
    StartWiFiPorts(false);  // DAGNALL, note this is NEEDED here!, not later on!
    String html = ""; 
    html += FPSTR(Header);      
    html += "&nbsp;</div></div><br></a><center> N2000 MONITOR </center><br><br><div class=\"div2\">";
    html += FPSTR(OTA_STYLE);
    html += FPSTR(OTA_UPLOAD);
    webserver.send_P(200, "text/html", html.c_str());
  });
  // Handling uploading firmware file
    webserver.on("/ota", HTTP_POST, []() { 
    StartWiFiPorts(false);  // leaving here just in case it did something..
    webserver.send(200, "text/plain", (Update.hasError()) ? "Update: fail\n" : "Update: OK!\n");
    delay(500);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = webserver.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Firmware update initiated: %s\r\n", upload.filename.c_str());  // dagnall should this be serial2 for debugging?-- but the Stop serial that I re-instated stops these being sent!
      //uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      uint32_t maxSketchSpace = max_sketch_size();
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
      // Store the next milestone to output
      uint16_t chunk_size  = 51200;
      static uint32_t next = 51200;
      // Check if we need to output a milestone (100k 200k 300k)
      if (upload.totalSize >= next) {
        Serial.printf("%dk ", next / 1024);    

      //  if ( Leds41 == false ) { Leds41 = true; }
        next += chunk_size;
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("\r\nFirmware update successful: %u bytes\r\nRebooting...\r\n", upload.totalSize);// dagnall should this be serial2 for debugging?
      } else {
        Update.printError(Serial);
      }
    }
  });
}



void handleDEMODebugWebpage() {
  webserver.send_P(200, "text/html", DEMOWEBPAGE_HTML);
  Serial.println("DEMO Webpage");
}

void handleSettings() {  // this is just a quick and dirty /settings handler.. 
  String web_error = "";

 // Typically.. 
 // if ( webserver.arg("run") == "WEBSOCKS" ) {DemoDebugMode=true; handleDEMODebugWebpage(); return;}
  //if ( webserver.arg("run") == "DEB" ) {DebugMode=true; Send_DebugModeWebPage(); return;}
   // if ( webserver.arg("run") == "SIM" ) {SimMode =true; }  // just a hack to get to the old debug page  
  // PrintAllsettings();
  //if (SettingsChanges){SavetoEPROM();}
  }

void WebServerSettings() {
  Serial.println("******Setting up Webservers******");
  webserver.on("/", handleDEMODebugWebpage);  // this will eventually be the general response
  webserver.on("/NMEA", handleDEMODebugWebpage);  // response to footer "HomePage"
  webserver.on("/settings", handleSettings);   // if I add a settings page for ssid pw ext. 
  OTASettings();  // in OTA-WEB.ino
  webserver.begin();
}

void printDebug() {
  //Serial.println();
  Serial.print(" STATUS Pause<");
  Serial.print(Read_PauseFlag());  //Read_PauseFlag
  Serial.print(">  sendNMEA<");
  Serial.print(Read_sendNMEA());
  Serial.print(">  sendSerialPGN<");
  Serial.print(Read_SerialPGN());

  
  Serial.println(">");
}















