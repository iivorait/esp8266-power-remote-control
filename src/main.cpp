#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define WIFI_SSID "your-ssid"
#define WIFI_PASSWORD "password"

#define WWW_AUTH_ON true //require username and password when calling the API
#define WWW_USERNAME "admin"
#define WWW_PASSWORD "power"
#define WWW_PORT 80

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define SCREEN_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for address; 0x3D for 128x64, 0x3C for 128x32, sometimes 0x3C for 128x64
#define SCREEN_ROTATION 2 //2 = upside down
#define SCREEN_SDA_PIN D2 // GPIO4
#define SCREEN_SCL_PIN D1 // GPIO5

#define RELAY_PIN 14 // PIN D5 on a NodeMCU V3



Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RESET);
WiFiClient client;
ESP8266WebServer httpserver(WWW_PORT);
String ip;

void displayText(String text, int size) {
  display.clearDisplay();
  display.setRotation(SCREEN_ROTATION); 
  display.setTextSize(size); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(text);
  display.display();
}

void pressButton(bool longPress)
{
  if (longPress) {
    digitalWrite(RELAY_PIN, LOW);
    delay(6000);
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Long press");
  } else {
    digitalWrite(RELAY_PIN, LOW);
    delay(1500);
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Short press");
  }
}

String infoText(){
  String ptr = "<html>\n";
  ptr +="<head>\n";
  ptr +="<title>ESP8266 Power Remote Control</title>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP8266 Power Remote Control</h1>\n";
  String auth_string;
  if(WWW_AUTH_ON) {
    ptr +="<h3>Basic authentication is ON</h3>\n";
    auth_string = " -u \"username:password\"";
  } else {
    ptr +="<h3>Basic authentication is OFF</h3>\n";
    auth_string = "";
  }
  ptr += "<form id=\"web\" action=\"/short_press\" method=\"POST\"> \n";
  ptr += "<fieldset>\n";
  ptr += "<legend>Example cURL commands</legend>\n";
  ptr += "<pre>curl -X POST" + auth_string + " http://" + ip + "/short_press\n";
  ptr += "curl -X POST" + auth_string + " http://" + ip + "/long_press</pre>\n";
  ptr += "</fieldset>\n";
  ptr += "<fieldset>\n";
  ptr += "<legend>Web control</legend>\n";
  ptr += "<input type=\"radio\" id=\"short\" name=\"press\" value=\"short\" checked onchange=\"document.forms.web.action='/short_press'\">\n";
  ptr += "<label for=\"short\">Short press</label><br>\n";
  ptr += "<input type=\"radio\" id=\"long\" name=\"press\" value=\"long\" onchange=\"document.forms.web.action='/long_press'\">\n";
  ptr += "<label for=\"long\">Long press</label><br>\n";
  ptr += "<button>Send</button>\n";
  ptr += "</fieldset>\n";
  ptr += "</form>\n";
  ptr += "<p><a href=\"https://github.com/iivorait/esp8266-power-remote-control\" target=\"_blank\">Application's GitHub project</a></p>";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

void handleRoot() {
  httpserver.send(200, "text/html", infoText()); 
}

void handleShortPress() {
  if(WWW_AUTH_ON && !httpserver.authenticate(WWW_USERNAME, WWW_PASSWORD)) {
    return httpserver.requestAuthentication();
  }
  pressButton(false);
  httpserver.send(200, "text/plain", "short press"); 
}

void handleLongPress() {
  pressButton(true);
  httpserver.send(200, "text/plain", "long press"); 
}

void handle_NotFound(){
  httpserver.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(9600);

  //Configure display pins
  Wire.begin(SCREEN_SDA_PIN, SCREEN_SCL_PIN);

  //Configure relay GPIO mode
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  displayText("Connecting to WIFI", 1);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  ip = WiFi.localIP().toString();
  displayText("Connected\n" + ip + "\nport " + WWW_PORT, 1);

  httpserver.on("/", handleRoot);
  httpserver.on("/short_press", HTTP_POST, handleShortPress);
  httpserver.on("/long_press", HTTP_POST, handleLongPress);
  httpserver.onNotFound(handle_NotFound);

  httpserver.begin();
  Serial.println("HTTP server started");
}

void loop() {
  httpserver.handleClient();
  if (millis() > 86400000) { //restart to refresh network connection
    Serial.println("Daily restart");
    ESP.restart();
  }
}

