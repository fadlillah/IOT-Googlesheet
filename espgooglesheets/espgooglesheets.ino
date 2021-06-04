#include <ESP8266WiFi.h>
#include "DHTesp.h"
#include "HTTPSRedirect.h"
#include "DebugMacros.h"

DHTesp dht;
const char* ssid = "....";
const char* password = "1234567890";
//string data
String sheetHumid = "";
String sheetTemp = "";

//google sheet host
const char* host = "script.google.com";
// script.google.com/macros/s/AKfycbyh1Q9iCr02IHCF5Gx1T06HFyrtbSUI869S7DRNqr4-ppHdtJHm2QMtkgb-lz-5nLxC/exec
const char* GScriptId = "AKfycbyh1Q9iCr02IHCF5Gx1T06HFyrtbSUI869S7DRNqr4-ppHdtJHm2QMtkgb-lz-5nLxC"; 
const int httpsPort = 443; 
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec?value=Temperature"; 
String url2 = String("/macros/s/") + GScriptId + "/exec?cal";
String payload_base =  "{\"command\": \"appendRow\", \
                    \"sheet_name\": \"TempSheet\", \
                       \"values\": ";
String payload = "";

HTTPSRedirect* client = nullptr;
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void setup() {
  delay(1000);
  pinMode(BUILTIN_LED, OUTPUT);     
  Serial.begin(9600);
  dht.setup(D4, DHTesp::DHT11);
  setup_wifi();
  // Use HTTPSRedirect class to create a new TLS connection
 
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  Serial.print("Connecting to ");
  Serial.println(host);          //try to connect with "script.google.com"
  Serial.print("connection host success");
  // Try to connect for a maximum of 5 times then exit
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }
// Finish setup() function in 1s since it will fire watchdog timer and will reset the chip.
//So avoid too many requests in setup()
  Serial.println("\nWrite into cell 'A1'");
  Serial.println("------>");
  // fetch spreadsheet data
  client->GET(url, host);
  
  Serial.println("\nGET: Fetch Google Calendar Data:");
  Serial.println("------>");
  // fetch spreadsheet data
  client->GET(url2, host);
 Serial.println("\nStart Sending Sensor Data to Google Spreadsheet");
  // delete HTTPSRedirect object
  delete client;
  client = nullptr;
  Serial.println("setupp success......");
  delay(3000);
}
void loop() {
    float t = dht.getTemperature(); //membaca data temperature sensor dht11
    float h = dht.getHumidity();
    if (isnan(t)&&isnan(h)) {                                                
      Serial.println(F("Failed to read from Temperature sensor!"));
      return;
    }                              
  Serial.print("Humidity: ");  Serial.print(h);
  sheetHumid = String(h) + String("%");                                         //convert integer humidity to string humidity
  Serial.print("%  Temperature: ");  Serial.print(t);  Serial.println("Â°C ");
  sheetTemp = String(t) + String("Â°C");
  static int error_count = 0;
  static int connect_count = 0;
  const unsigned int MAX_CONNECT = 20;
  static bool flag = false;

  payload = payload_base + "\"" + sheetTemp + "," + sheetHumid + "\"}";

  if (!flag) {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }

  if (client != nullptr) {
    if (!client->connected()) {
      client->connect(host, httpsPort);
      client->POST(url2, host, payload, false);
      Serial.print("Sent : ");  Serial.println("Temp and Humid");
    }
  }
  else {
    DPRINTLN("Error creating client object!");
    error_count = 5;
  }

  if (connect_count > MAX_CONNECT) {
    connect_count = 0;
    flag = false;
    delete client;
    return;
  }

  Serial.println("POST or SEND Sensor data to Google Spreadsheet:");
  if (client->POST(url2, host, payload)) {
    ;
  }
  else {
    ++error_count;
    DPRINT("Error-count while connecting: ");
    DPRINTLN(error_count);
  }

  if (error_count > 3) {
    Serial.println("Halting processor...");
    delete client;
    client = nullptr;
    Serial.printf("Final free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("Final stack: %u\n", ESP.getFreeContStack());
    Serial.flush();
    ESP.deepSleep(0);
  }
delay(3000); 
}
