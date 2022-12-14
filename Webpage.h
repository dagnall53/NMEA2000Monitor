/// Adding some websocket stuff to assist debugging and demonstrate the "OTHER" debug page // keep this at the end
#include <Arduino.h>
#include "WebServer.h"
#include "WebsocketInterpreter.h"
#include <WebSocketsServer.h> // https://github.com/Links2004/arduinoWebSockets

// NB WEBPAGE "DEMOWEBPAGE_HTML" is defined in a char lower down.

WebSocketsServer webSocket = WebSocketsServer(81);
WebServer webserver(80);  // for the webbrowser 

bool _sendPGN = true;  //  will send PGN summaries. 
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
  var terminalModeFlag = true;


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
        websock.send("WEBMONITOR PAUSE OFF");
      }
      else
      { terminalPauseFlag = true;
        document.getElementById("connectTerminalButton").innerHTML = "<b>ON</b>";
        websock.send("WEBMONITOR PAUSE ON");
      }
    }

    function terminalMode() {
      if (terminalModeFlag) {
        terminalModeFlag = false;
        document.getElementById("ModeTerminalButton").innerHTML = "<b>NMEA</b>";  // button is inverse to what is being shown now
        websock.send("WEBMONITOR PGN");
      } else 
      { terminalModeFlag = true;
        document.getElementById("ModeTerminalButton").innerHTML = "<b>PGN</b>";
        websock.send("WEBMONITOR NMEA");
      }
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
        //do anything here for setup if needed 
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
.absolute3 {
  position: absolute;
  top: 40va;
  right: 240px;
  width: 100px;
  height: 30px;
  border: 1px solid blue;
}
.absolute2 {
  position: absolute;
  top: 40va;
  right: 20px;
  width: 100px;
  height: 30px;
  border: 1px solid blue;
}
.absolute {
  position: absolute;
  top: 40va;
  right: 130px;
  width: 100px;
  height: 30px;
  border: 1px solid blue;
}
</style>
<body onload="javascript:start();" id="mainBody">
<h2>NMEA2000 Monitor</h2>
<div class= "absolute" ><button id="connectTerminalButton" class="VertBoxStyle"  onclick="terminalPause()"><b>Pause?</b></button> </div>
<div class= "absolute2" ><button id="ModeTerminalButton" class="VertBoxStyle"  onclick="terminalMode()"><b>-TBD-</b></button> </div>
<div class= "absolute3" ><button id="TerminalClearButton" class="VertBoxStyle"  onclick="terminalClear()"><b>CLEAR</b></button> </div>

<p>This will display received PGN numbers and interpretation where known. This data is also being sent as a UDP send on port 2002.<br>
Actisense encoded N2K messages are being sent from the USB port for decode by the Actisense NMEA reader<br> 
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
<div>More text, potentialy a footer. <br> </div>

</body>

</html>

)rawliteral";


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















