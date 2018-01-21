#define SSID "Joe"
#define PASS "cruzhacks2018"
#define SERVER_ADDRESS "http://us-central1-cruzhacks2018-17b9d.cloudfunctions.net"
#define SERVER_PORT 80
#define SERVER_PATH "/BikerackActivity?rackID=lockuplookup&slot=1&details=TAMPER"
#define DEBUG true

// port definitions
// first swtich pin (digital pin)
// switches should be connected to SWITCHPORT1, SWITCHPORT1 + 1, SWITCHPORT1 + 2 (until SWITCHCOUNT pins)
#define SWITCHPORT1 5
// how many pins to read
#define SWITCHCOUNT 5

// definitions of serial ports
// Debug port (where to write/read debug)
// WiFi port (where to write/read to WiFi chip)
HardwareSerial *Debug = &Serial;
HardwareSerial *WiFi = &Serial1;

// END OF CONFIG!

int switches[SWITCHCOUNT];

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
// if neither is found, scream for a bit (maybe remove that feature?)
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

// send "toSend" to "ip" on port "port"
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

// keys is an array of keys to be passed in the payload of the get request
// values is an array of values that correspond to the keys
void sendGET(String ip, uint16_t port, int switchNum, int value) {
  String path = SERVER_PATH;
//  String num = "";
//  num += switchNum;
//  // convert state to string type
//  String state = "LOW";
//  if (value == HIGH) {
//    state = "HIGH";
//  }
//  // add components to path
//  path += "switch=";
//  path += num;
//  path += "&state=";
//  path += state;
  String request = "GET " + path + " HTTP/1.1\r\n";
  sendData(request, SERVER_ADDRESS, SERVER_PORT);
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

// activate everything
void setup()
{
  if (DEBUG)
    Debug->begin(115200);
  debug(F("Starting setup"));
  WiFi->begin(115200);
  pinMode(13, OUTPUT);
  
  // Setup the wifi in client mode.
  debug(F("initializing WiFi"));
  // restart the wifi chip
  runCommand(F("AT+RST"), F("OK"), F("ERROR"));
  delay(2000);
  // set to client mode (enable chip)
  runCommand(F("AT+CWMODE=1"), F("OK"), F("ERROR"));
  // join wifi
  runCommand("AT+CWJAP=\"" SSID "\",\"" PASS "\"", F("OK"), F("ERROR"));

  // setup the switches
  debug(F("initializing switch states"));
  for (int i = 0; i < SWITCHCOUNT; i++) {
    switches[i] = LOW;
  }
  
  debug(F("STARTED"));
  // blink the light for yuks (show setup completion)
  digitalWrite(13, HIGH);
  delay(2000);
  digitalWrite(13, LOW);
}

// check and process all Serial inputs (from WiFi or debug)
void loop()
{
  if (DEBUG)
    sendSerialCommands();
  
  // compare the switchStates to its previous state
  for (int thisPin = 0; thisPin < SWITCHCOUNT; thisPin++) {
    // read pin
    int pinState = digitalRead(SWITCHPORT1 + thisPin);
    // compare to stored previous read
    if (pinState != switches[SWITCHPORT1 + thisPin]) {
      // sendUpdates
      sendGET(SERVER_ADDRESS, SERVER_PORT, thisPin, pinState);
      // store this for the next call
      switches[SWITCHPORT1 + thisPin] = pinState;
      // wait a bit because the example i'm copying did so...
      delay(50);
    }
  }  
}

