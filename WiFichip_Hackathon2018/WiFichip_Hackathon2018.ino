#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#define SPEED 250

#define AP_SSID "TankView360"
#define AP_PASS "exploring the world"
#define SERVER_PORT "80"
#define AP_IP "10.0.0.1"
#define DEBUG false

// port definitions
// connect motor controller pins to Arduino digital pins
// motor one
#define EN_A 2
#define IN_1 3
#define IN_2 4
// motor two
#define EN_B 7
#define IN_3 5
#define IN_4 6
// gps chip (the wire should run from port number to RX pin on GPS chip)
#define GPS_RX 8
// (the wire should run from port number to TX pin on GPS chip)
#define GPS_TX 9

// definitions of serial ports
// Debug port (where to write/read debug)
// WiFi port (where to write/read to WiFi chip)
HardwareSerial *Debug;
HardwareSerial *WiFi = &Serial;

// create a GPS instance over softwareserial
Adafruit_GPS GPS(&SoftwareSerial(GPS_RX, GPS_TX));

// END OF CONFIG!

void debug(String output) {
  if (DEBUG) {
    Debug->println(output);
  }
}

// just send the command, don't touch the output
// returns true to overload the runCommand method
bool runCommand(String command) {
  debug("Running Command: '" + command + "'");
  WiFi->println(command);
  return true;
}

// run command command and search for success.  If error is found first, return false.
// if neither is found, scream.
bool runCommand(String command, String success, String error) {
  // send command to the WiFi
  runCommand(command);
  // search the WiFi for the success string
  // limit to 6000 (~10s) loops to detect failure to find
  String responder = "";
  for (int i = 0; i < 1000; i++) {
    while (WiFi->available() > 0) {
      if (DEBUG)
        Debug->print((char) WiFi->peek());
      responder += (char) WiFi->read();
    }
    for (int j = 0; j < responder.length(); j++) {
      if (responder.substring(j, j + success.length()) == success) {
        debug("Successfully found " + success);
        delay(500);
        return true;
      }
    }
    for (int j = 0; j < responder.length(); j++) {
      if (responder.substring(j, j + error.length()) == error) {
        debug("Found error " + error);
        return false;
      }
    }
    delay(10);
  }
  debug(responder);
  debug(F("Timed out in output scan: chip disconnected?"));

  // did not throw error or success: thats an issue, probably something wrong
  // for now infinite loop/freeze
  for (int i = 0; i < 10; i++) {
    digitalWrite(13, !digitalRead(13));
    delay(2000);
  }
}

void sendData(String toSend, String ip, uint16_t port) {
    // send data...
    debug("Sending data: '" + toSend + "' to " + ip + ":" + port);
    runCommand("AT+CIPSTART=4,\"TCP\",\"" + ip + "\"," + String(port), F("OK"), F("ERROR"));
    runCommand("AT+CIPSEND=4," + String(toSend.length()), F("OK"), F("ERROR"));
    runCommand(toSend, F("SEND OK"), F("SEND FAIL"));
    digitalWrite(13, HIGH);
    delay(200);
    digitalWrite(13, LOW);
}

// retrieve the network IP in station mode (joined another WiFi)
String getIP() {
  String networkIP = F("unknown");
  debug(F("checking IP..."));
  
  runCommand(F("AT+CIPSTA?"));
  // seek forward in response data until "+CIPSTA:ip"
  if (WiFi->find((char *) "+CIPSTA:ip:\"")) {
    // read everything on that line as the networkIP
    networkIP = WiFi->readStringUntil('"');
    if (networkIP != F("0.0.0.0"))
      debug("Found IP:" + networkIP);
    else {
      debug("No IP");
      networkIP = F("unknown");
    }
  }
  return networkIP;
}

String addHTTPHeader(String body) {
   String withHead = F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ");
   withHead += body.length();
   withHead += F("\r\n\r\n");
   withHead += body;
   return withHead;
}

String packetParse(String packet) {
  debug("processing packet: " + packet);
  String GETTest = packet.substring(0, 3);
  if (packet.equals(F("test"))) {
    debug(F("Recieved test query"));
    digitalWrite(13, !digitalRead(13));
    return F("Light Toggled");
  } else if (packet.equals("ip")) {
    return getIP();
  } else if (GETTest.equals(F("GET"))) {
    String data = packet.substring(packet.indexOf(F("/")) + 1, packet.indexOf(F(" HTTP")));
    data.replace(F("+"), F(" "));
    debug("Recieved get request: " + data);

    // check if it has a login?SSID=____&PASS=_____
    if ((data.indexOf(F("login?")) >= 0) && (data.indexOf(F("&")) >= 0) && (data.indexOf(F("SSID=")) >= 0) && (data.indexOf(F("PASS=")) >= 0)) {
      debug(F("login PACKET... attempting parse"));
      String enteredSSID = data.substring(data.indexOf(F("SSID=")) + 5, data.indexOf(F("&")));
      debug("Found SSID: " + enteredSSID);
      String enteredPASS = data.substring(data.indexOf(F("PASS=")) + 5, data.length());
      debug("Found PASS: " + enteredPASS);

      debug(F("Attempting to join..."));
      if (runCommand("AT+CWJAP=\"" + enteredSSID + "\",\"" + enteredPASS + "\"", F("OK"), F("FAIL"))) {
        debug(F("joined, retreiving IP"));
        return addHTTPHeader("SUCCESS! My IP address is now " + getIP());
      } else {
        debug(F("Failed to connect."));
        return addHTTPHeader("I failed to connect to <BR>" + enteredSSID + "<BR>" + enteredPASS);
      }
    }
    return addHTTPHeader(F("<html><head><title>TankView360 Configurator</title></head><body><form action=\"login\"><input name=\"SSID\" placeholder=\"Enter SSID\" type=\"text\"><input name=\"PASS\" placeholder=\"Enter Password\" type=\"text\"><input value=\"GO\" type=\"submit\"></form></body></html>"));
   } else {
    // parse motor commands
    bool valid = false;
    // variables to store the directions of motors
    bool m1a = false;
    bool m1b = false;
    bool m2a = false;
    bool m2b = false;
    for (int i = 0; i < packet.length(); i++) {
      if (packet.charAt(i) == 'a') {
        valid = true;
        m1a = true;
        m1b = false;
      } else if (packet.charAt(i) == 'b') {
        valid = true;
        m1a = false;
        m1b = true;
      } else if (packet.charAt(i) == 'c') {
        valid = true;
        m2a = true;
        m2b = false;
      } else if (packet.charAt(i) == 'd') {
        valid = true;
        m2a = false;
        m2b = true;
      } else if (packet.charAt(i) == 's') {
        valid = true;
        m1a = false;
        m1b = false;
      } else if (packet.charAt(i) == 't') {
        valid = true;
        m2a = false;
        m2b = false;
      }
    }
    if (valid) {
      // update motors
      debug(F("parsed driver packet"));
      if (m1a)
        digitalWrite(IN_1, HIGH);
      else
        digitalWrite(IN_1, LOW);
      if (m1b)
        digitalWrite(IN_2, HIGH);
      else
        digitalWrite(IN_2, LOW);
      if (m2a)
        digitalWrite(IN_3, HIGH);
      else
        digitalWrite(IN_3, LOW);
      if (m2b)
        digitalWrite(IN_4, HIGH);
      else
        digitalWrite(IN_4, m2b);
      analogWrite(EN_A, SPEED);
      analogWrite(EN_B, SPEED);
    } else {
      debug("unrecognized packet: " + packet);
    }
    return F("DONOTRESPOND");
  }
}

void processLine(String l) {
  String testChars = l.substring(0, 4);
  if (testChars.equals(F("+IPD"))) {
    // recieved data packet
    // the id (0, 1, 2, 3, 4 of the connection)
    String id = l.substring(5, 6);
    // packets format:
    // +IPD,0,326:[]
    // 0: id of connection
    // 326: length of data in packet
    // []: packet body
    String packet = l.substring(l.indexOf(F(":")) + 1, l.length());
    // for GET request: [] = GET / HTTP/1.1 [headers]
    String reply = packetParse(packet);
    if (reply != F("DONOTRESPOND")) {
      debug(F("response sending..."));
      runCommand("AT+CIPSEND="+ id + "," + reply.length(), F("OK"), F("ERROR"));
      runCommand(reply, F("SEND OK"), F("SEND FAIL"));
    }
    runCommand("AT+CIPCLOSE=" + id, F("OK"), F("ERROR"));
  }
}

// Take Debug inputs, and send to WiFi chip
void sendSerialCommands() {
  // read in from cable...
  if (Debug->available() > 0) {
    debug(F("Reading input..."));
    // delay until all data should be recieved
    delay(500);
    //read Serial (usb Input)
    String command = Debug->readString();
    //print confirmation to usb
    debug("Sending: " + command);
    //send to Serial1 (WiFi)
    WiFi->println(command);
  }
}

String line = "";
// Take WiFi inputs, and send to Debug
void processWiFiOutput() {
  // read all data in from WiFi...find the newline chars
  // process each line independently
  while (WiFi->available() > 0) {
    // get next char
    char s = WiFi->read();
    // if it is a newline char (\r or \n)
    if (s == 10 || s == 13) {
      // prevent empty line processing
      if (line.length() > 0) {
        // process this line (in line)
        debug("Processing line: " + line);
        processLine(line);
      }
      // reset current line
      line = "";
    } else {
      // if it is not a newline, just toss the char on the line
      line += (char) s;
    }
  }
}

void checkGPS() {
  // if new data was read
  if (GPS.newNMEAreceived()) {
    debug(F("parsed"));
    if (!GPS.parse(GPS.lastNMEA())) {  // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
    }
  }
  if (millis() > 100000000) {
    String GPSData = "";
    if (GPS.fix) {
      GPSData += F("GPS: location: ");
      // print location and speed
      //        Serial.println("$LOC[");
      GPSData += (String) GPS.latitudeDegrees;
      GPSData += ","; 
      GPSData += (String) GPS.longitudeDegrees;
      GPSData += ",";
      GPSData += (String) GPS.speed;
      GPSData += ".";
    //        Serial.println("];");
    } else {
      GPSData += F("GPS: no location");
      GPSData += ",";
      GPSData += (String) GPS.hour + ":" + (String) GPS.minute + ":" + (String) GPS.seconds + "." + (String) GPS.milliseconds;
      GPSData += ",";
      GPSData += "Fix: " + (int) GPS.fix;
    }
    debug(GPSData);
//    sendData(GPSData, "192.168.0.13", 50383);
  }
}

// activate everything
void setup()
{
  if (DEBUG)
    Debug->begin(115200);
  debug(F("Starting setup"));
  WiFi->begin(115200);
  // connect to GPS chip over softwareSerial
  GPS.begin(9600);
  pinMode(13, OUTPUT);
  
  // Setup the wifi in host and client mode.
  // host mode is for configuring TankView, client mode is for
  // being controlled.
  debug(F("initializing WiFi"));
  // restart the wifi chip
  runCommand(F("AT+RST"), F("OK"), F("ERROR"));
  delay(2000);
  // set to host/client mode (enable chip)
  runCommand(F("AT+CWMODE=3"), F("OK"), F("ERROR"));
  // set host IP address
  runCommand(F("AT+CIPAP=\"" AP_IP "\""), F("OK"), F("ERROR"));
  // enable multithreads
  runCommand(F("AT+CIPMUX=1"), F("OK"), F("ERROR"));
  // enable hotspot
  runCommand(F("AT+CWSAP=\"" AP_SSID "\",\"" AP_PASS "\",5,3"), F("OK"), F("ERROR"));
  // starting listener
  runCommand(F("AT+CIPSERVER=1," SERVER_PORT), F("OK"), F("ERROR"));

  debug(F("initializing GPS"));
  // RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // request antenna info
  GPS.sendCommand(PGCMD_ANTENNA);
  
  debug(F("STARTED"));
  digitalWrite(13, HIGH);
  delay(2000);
  digitalWrite(13, LOW);
}

// check and process all Serial inputs (from WiFi or debug)
void loop()
{
  if (DEBUG) {
    sendSerialCommands();
  }
  checkGPS();
  processWiFiOutput();
}

