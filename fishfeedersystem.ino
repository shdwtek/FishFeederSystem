#include <WiFiNINA.h>

#define LEDPIN 13 // Pin 13: Arduino has an LED connected on pin 13

#define SENSORPIN 4 // Pin 4: Arduino has IR Sensor connected on Pin 4

#define FOOD_THRESHOLD 1

char ssid[] = "SSID";             //  your network SSID (name) between the " "
char pass[] = "PASS";      // your network password between the " "
int keyIndex = 0;                 // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;      //connection status
WiFiServer server(8082);            //server socket

WiFiClient client = server.available();

int solenoid = 12;

// IFTTT Init

int    HTTP_PORT   = 80;
String HTTP_METHOD = "POST";
char   HOST_NAME[] = "maker.ifttt.com";
String PATH_NAME   = "/trigger/send-email/with/key/WEBHOOKKEY"; // change your Webhooks key

bool isSent = false;

// variables will change:
int sensorState = 0; 
int lastState = 0; // variable for reading the pushbutton status

int foodStatus = 0; // variable for showing food level status

void setup() {
  Serial.begin(9600);
  pinMode(solenoid, OUTPUT);
  while (!Serial);
    
  enable_WiFi();
  connect_WiFi();

  server.begin();
  printWifiStatus();  
  
  // initialize the LED pin as an output:
  pinMode(LEDPIN, OUTPUT);      
  // initialize the sensor pin as an input:
  pinMode(SENSORPIN, INPUT);     
  digitalWrite(SENSORPIN, HIGH); // turn on the pullup
}

void loop(){
  
  if (client) {
    printWEB();
  }
  
  // read the state of the pushbutton value:
  sensorState = digitalRead(SENSORPIN);

  // check if the sensor beam is broken
  // if it is, the sensorState is LOW:
  if (sensorState == LOW) {     
    // turn LED on:
    digitalWrite(LEDPIN, HIGH);     
  } 
  else {
    // turn LED off:
    digitalWrite(LEDPIN, LOW); 
  }
  
  if (sensorState && !lastState) {
    Serial.println("Feeder Food Level Ok");
  foodStatus = 1;
  } 
  if (!sensorState && lastState) {
    Serial.println("Feeder Food Level Low");
  foodStatus = 0;
  }
  lastState = sensorState;
  
  if (foodStatus <= FOOD_THRESHOLD) {
    if (isSent == false) { // to make sure that Arduino does not send duplicated emails
      sendEmail(foodStatus);
      isSent = true;
    }
  } else {
    isSent = false; // reset to send if the temperature exceeds threshold again
  }
}



void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

void enable_WiFi() {
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }
}

void connect_WiFi() {
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
}

void printWEB() {

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {

            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
           
            //create the buttons
            client.print("Click <a href=\"/H\">here</a> turn the feeder on<br>");
            client.print("Click <a href=\"/L\">here</a> turn the feeder off<br><br>");
            
            int randomReading = analogRead(A1);
            client.print("Random reading from analog pin: ");
            client.print(randomReading);
           
            
            

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      
       if (currentLine.endsWith("GET /H")) {
        digitalWrite(12, HIGH);
        delay(25);
        digitalWrite(12, LOW);
        }
       
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

// IFTTT Send Email Notification
void sendEmail(float foodStatus) {
  // connect to IFTTT server on port 80:
  if (client.connect(HOST_NAME, HTTP_PORT)) {
    // if connected:
    Serial.println("Connected to server");
    // make a HTTP request:
    String queryString = "?value1=" + String(foodStatus);
    // send HTTP header
    client.println("GET " + PATH_NAME + queryString + " HTTP/1.1");
    client.println("Host: " + String(HOST_NAME));
    client.println("Connection: close");
    client.println(); // end HTTP header

    while (client.connected()) {
      if (client.available()) {
        // read an incoming byte from the server and print it to serial monitor:
        char c = client.read();
        Serial.print(c);
      }
    }

    // the server's disconnected, stop the client:
    client.stop();
    Serial.println();
    Serial.println("disconnected");
  } else {// if not connected:
    Serial.println("connection failed");
  }
}
