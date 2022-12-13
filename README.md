# NMEA2000Monitor
A simple monitor for N2k messages, built on the ActisenseListnerSender example.
This monitor uses a simple Websocks Webpage (based on the dEEbugger "Terminal window" (https://github.com/S-March/dEEbugger_Public))
You can connect to the AP N2000_Monitor (PW 12345678), and point your browser to 192.168.4.1 
and you should get this webpage.  
YOU WILL NEED TO ADD A Passwords.h file with..
char *ssid = "Your network";
char *password = "Your Password";
THIS IS A WORK IN PROGRESS! 
the code uses a simplified WIFI setup so you can then point your webbrowser to 192.168.0.120 (or your choice) and this display should result 
![image](https://user-images.githubusercontent.com/6950560/207415204-0383b3fe-49f5-498a-8e43-9e8bbdb6a2bb.png)

