#include <ESP8266WiFi.h> // Include the Wi-Fi library
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EasyDDNS.h>
#include "FS.h"
#include <ArduinoJson.h>
#include "TinyUPnP.h"

String SSIDS_FILE = "/ssids.json";
String SETTINGS_FILE = "/settings.json";
String DEFAULT_SETTINGS_FILE = "/settings.default.json";
String SEMAFORO_HTML_FILE = "/semaforoPage.html";
String SSID_SETTINGS_HTML_FILE = "/ssidsSettingsPage.html";
String SETTINGS_HTML_FILE = "/settingsPage.html";

struct ssid
{
  String ssid;
  String pwd;
  String type;
  String ip;
  String gateway;
  String mask;
};

MDNSResponder mdns;

const int d0 = 16;
const int d5 = 14;
const int d6 = 12;
const int d7 = 13;
const int d4 = 2;
const int d3 = 0;

const int green = d6;
const int yellow = d5;
const int red = d0;
const int button = d3;

int colors[] = {green, yellow, red};
const int GND = d7;
boolean semaforoAbierto = true;
boolean semaforoEncendido = true;

int AMBER_TIME = 3;
boolean AMBER_TRANSITION = true;
boolean RED_TO_GREEN = true;
boolean GREEN_TO_RED = true;
String AP_NAME = "semaforito";
String DEFAULT_IP = "192.168.1.23";
String DEFAULT_GATEWAY = "192.168.1.1";
String DEFAULT_MASK = "255.255.255.0";
String PHONE_IP = "192.168.43.23";
String PHONE_GATEWAY = "192.168.43.1";
String PHONE_MASK = "255.255.255.0";
String AP_IP = "192.168.1.23";
String DDNS_SERVICE = "";
String DDNS_URL = "";
String DDNS_USERNAME = "";
String DDNS_PWD = "";
int UPNP_LISTEN_PORT = 5123;
int UPNP_LEASE_TIME = 360000;
boolean UPNP_ENABLED = false;
boolean DDNS_ENABLED = false;

TinyUPnP *tinyUPnP = new TinyUPnP(UPNP_LEASE_TIME);
ESP8266WebServer webserver(UPNP_LISTEN_PORT);

void definePins()
{
  Serial.println("[DefinePins]");
  pinMode(green, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(GND, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);

  pinMode(button, INPUT_PULLUP);
  digitalWrite(GND, LOW);
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.println("\tPins set.");
}

void convertToIp(String ip, int ipArray[])
{
  int firstSplit = ip.indexOf(".");
  int secondSplit = ip.indexOf(".", firstSplit + 1);
  int thirdSplit = ip.indexOf(".", secondSplit + 1);

  ipArray[0] = ip.substring(0, firstSplit).toInt();
  ipArray[1] = ip.substring(firstSplit + 1, secondSplit).toInt();
  ipArray[2] = ip.substring(secondSplit + 1, thirdSplit).toInt();
  ipArray[3] = ip.substring(thirdSplit + 1).toInt();

//  Serial.println(String(ipArray[0]) + ":" + String(ipArray[1]) + ":" + String(ipArray[2]) + ":" + String(ipArray[3]));
}

void resetSPIFFS()
{
  Serial.println("[resetSPIFFS]");
  Serial.println("\n\tABOUT TO FORMAT SPIFFS");
  SPIFFS.begin();
  SPIFFS.format();
  SPIFFS.end();
  Serial.println("\n\tSPIFFS FORMATTED");
  Serial.println("");
}

String loadFile(String filename)
{
  SPIFFS.begin();
  String s;

  if (SPIFFS.exists(filename))
  {
    File f = SPIFFS.open(filename, "r");
    if (f)
    {
      while (f.available())
      {
        s += f.readStringUntil('\n');
      }
    }
    else
      Serial.println("\t\tPointer to file could not open.");
    f.close();
  }
  else
    Serial.println("\t\tFile " + filename + " does not exist.");
  SPIFFS.end();
  return s;
}

void saveToFile(String filename, String settings)
{
  SPIFFS.begin();
  File f = SPIFFS.open(filename, "w+");
  if (f)
    f.println(settings);
  f.close();
  SPIFFS.end();
  Serial.println(settings);
}

void loadSettings(String filename)
{
  Serial.println("[loadSettings]");
  String s = loadFile(filename);
  DynamicJsonDocument doc(2048);
  auto error = deserializeJson(doc, s);
  Serial.println(s);

  if(s.length()>0)
  {
    //TODO insert control for if s is shizze (no file loaded?)
    AMBER_TIME = doc["transition_speed"];
    AMBER_TRANSITION = doc["amber_transition"];
    RED_TO_GREEN = doc["red_to_green"];
    GREEN_TO_RED = doc["green_to_red"];
    DEFAULT_IP = doc["default_ip"].as<String>();
    DEFAULT_GATEWAY = doc["default_gateway"].as<String>();
    DEFAULT_MASK = doc["default_mask"].as<String>();
    PHONE_IP = doc["phone_ip"].as<String>();
    PHONE_GATEWAY = doc["phone_gateway"].as<String>();
    PHONE_MASK = doc["phone_mask"].as<String>();
    AP_NAME = doc["ap_name"].as<String>();
    AP_IP = doc["ap_ip"].as<String>();
    DDNS_ENABLED = doc["ddns_enabled"];
    DDNS_SERVICE = doc["ddns_service"].as<String>();
    DDNS_URL = doc["ddns_url"].as<String>();
    DDNS_USERNAME = doc["ddns_username"].as<String>();
    DDNS_PWD = doc["ddns_pwd"].as<String>();
    UPNP_ENABLED = doc["upnp_enabled"];
    UPNP_LISTEN_PORT = doc["listen_port"];
  }
  String str = "\n\tTransition speed: " +
               String(AMBER_TIME) +
               "\n\tPass by Amber? " + String(AMBER_TRANSITION) +
               "\n\t   When Red to Green? " + String(RED_TO_GREEN) +
               "\n\t   When Green to Red? " + String(GREEN_TO_RED) +
               "\n\tAP Name: " + AP_NAME +
               "\n\tDefault IPs:" +
               "\n\t   Standard: " + DEFAULT_IP +
               "\n\t   Phone (tether): " + PHONE_IP +
               "\n\t   AP IP: " + AP_IP +
               "\n\tDynDNS:" +
               "\n\t   Enabled: " + String(DDNS_ENABLED) +
               "\n\t   Service: " + DDNS_SERVICE +
               "\n\t   URL: " + DDNS_URL +
               "\n\t   Username: " + DDNS_USERNAME +
               "\n\t   Password: " + mask(DDNS_PWD) +
               "\n\tuPnP: " +
               "\n\t   Enabled: " + String(UPNP_ENABLED);
               "\n\t   Listen port: " + String(UPNP_LISTEN_PORT);
  Serial.println(str);
}

String mask(String str)
{
  return "*******";
}

int loadSSIDs(String filename, ssid availableSlots[])
{
  Serial.println("[loadSSIDs]");
  SPIFFS.begin();
  String s = loadFile(filename);

  DynamicJsonDocument doc(2048);
  auto error = deserializeJson(doc, s);
  if (error)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    Serial.println(s);
    return 0;
  }

  //Serial.println("[loadSSIDs]   In file: " + String(doc.size()) + " elements:");
  for (int i = 0; i < doc.size(); i++)
  {
    availableSlots[i].ssid = doc[i]["ssid"].as<String>();
    availableSlots[i].pwd = doc[i]["pwd"].as<String>();
    availableSlots[i].type = doc[i]["type"].as<String>();
    if (availableSlots[i].type == "standard")
    {
      availableSlots[i].ip = DEFAULT_IP;
      availableSlots[i].gateway = DEFAULT_GATEWAY;
      availableSlots[i].mask = DEFAULT_MASK;
    }
    else if (availableSlots[i].type == "phone")
    {
      availableSlots[i].ip = PHONE_IP;
      availableSlots[i].gateway = PHONE_GATEWAY;
      availableSlots[i].mask = PHONE_MASK;
    }
    else if (availableSlots[i].type == "custom")
    {
      availableSlots[i].ip = doc[i]["ip"].as<String>();
      availableSlots[i].gateway = doc[i]["gateway"].as<String>();
      availableSlots[i].mask = doc[i]["mask"].as<String>();
    }
    Serial.println("\tElligible SSIDs:");
    printEntryString(String(i + 1), availableSlots[i].ssid, mask(availableSlots[i].pwd), availableSlots[i].type);
  }

  return doc.size();
}

void debugPrintSSIDsInFile(String filename)
{
  ssid ssidSlots[10];
  int n = loadSSIDs(filename, ssidSlots);
  Serial.println();
  Serial.println("[debugPrintSSIDsInFile]");
  Serial.println("\n\tSSIDs in " + filename + ": " + n + " elements");

  for (int i = 0; i < n; i++)
    printEntry(ssidSlots[i]);
}

String printEntryString(String n, String ssid, String pwd, boolean type)
{
  String s = "\t  " + n + ". " + ssid + " - " + mask(pwd) + " (" + type + ")";
  Serial.println(s);
  return s;
}
String printEntry(ssid entry)
{
  String s = printEntryString("", entry.ssid, entry.pwd, entry.type);
  return s;
}
ssid findValidWifi(String filename)
{
  ssid ssidSlots[10];
  ssid correctOne = {ssid : "", pwd : "", type : "standard"};
  int n = loadSSIDs(filename, ssidSlots);
  if(n==0){
      Serial.println("\tNo SSIDs stored on file.");
      return correctOne;
  } 

  int nbVisibleNetworks = 0;
  while (nbVisibleNetworks == 0)
  {
    int nbVisibleNetworks = WiFi.scanNetworks();
    if (nbVisibleNetworks == 0)
    {
      Serial.println("no networks found. Reset to try again");
    }
    if (nbVisibleNetworks > 0)
      break;
  }

  boolean foundValidWifi = false;
  int attempts = 10;
  int i = 0;
  int j;
  while (foundValidWifi == false && attempts > 0)
  {
    String candidate_ssid = WiFi.SSID(i);
    Serial.println("\tChecking " + candidate_ssid);
    for (j = 0; j < n; j++)
    {
      //      Serial.println("\tComparing to " + ssidSlots[j].ssid + " (" + ssidSlots[j].ssid.equals(candidate_ssid) + ")");
      if (ssidSlots[j].ssid.equals(candidate_ssid))
      {
        correctOne = ssidSlots[j];
        Serial.println("\t  THIS IS IT: " + correctOne.ssid + "(" + correctOne.type + ")");
        foundValidWifi = true;
        break;
      }
    }

    i++;
    attempts--;
    if (foundValidWifi || attempts == 0)
      return correctOne;
  }
}

void connectToWifi(ssid wifiData)
{
  int ip[4];
  int gt[4];
  int mask[4];
  Serial.println("[connectToWifi]");
  Serial.print("\tConnecting to selected wifi: ");
  printEntry(wifiData);

  WiFi.begin(wifiData.ssid, wifiData.pwd);

  establishDDNS();

  //IP Handling
  convertToIp(wifiData.ip, ip);
  convertToIp(wifiData.gateway, gt);
  convertToIp(wifiData.mask, mask);

  IPAddress local_IP(ip[0], ip[1], ip[2], ip[3]);
  IPAddress gateway(gt[0], gt[1], gt[2], gt[3]);
  IPAddress subnet(mask[0], mask[1], mask[2], mask[3]);
  IPAddress dns(8, 8, 8, 8);
  if (!WiFi.config(local_IP, dns, gateway, subnet))
    Serial.println("STA Failed to configure");

  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  // WiFi Connected
  Serial.println("");
  Serial.println("\tConnected!");
  Serial.print("\t  ");
  Serial.println(wifiData.ssid);
  Serial.print("\t  IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("\t  MAC: ");
  Serial.println(WiFi.macAddress());
}
String printIP(int ip[4])
{
  String s="";
  for(int i=0; i<4; i++)
    s += String(ip[i]);
  return s;
} 
void initializeAsAP()
{
  int ip[4];
  digitalWrite(BUILTIN_LED, LOW);
  Serial.print("Setting soft-AP configuration ... ");
  convertToIp(AP_IP, ip);
  Serial.println(WiFi.softAPConfig(IPAddress(ip[0], ip[1], ip[2], ip[3]), IPAddress(ip[0], ip[1], ip[2], ip[3]), IPAddress(255, 255, 255, 0)) ? "Ready" : "Failed!");
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(AP_NAME + "_AP") ? "Ready" : "Failed!");
  Serial.print("Soft-AP IP address: ");
  Serial.println(WiFi.softAPIP());
}
void establishDDNS()
{
  Serial.println("[Establishing DDNS]");
  if(!DDNS_ENABLED)
  {
    Serial.println("\tDDNS is disabled");
    return;
  }
  if(DDNS_SERVICE =="" || DDNS_URL =="" || DDNS_USERNAME =="" || DDNS_PWD ==""){ 
    Serial.println("\tMissing credentials, please refer to Settings page");
    return;
  }

  HTTPClient http;
  http.begin("http://ipv4bot.whatismyipaddress.com/");
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      Serial.print("\tHTTP request OK: ");
      Serial.println(http.getString());
    }
    else
      Serial.println("\tHTTP Request NOT OK: " + String(httpCode));
  }
  else
  {
    http.end();
    Serial.println("\tHTTP Request Failed: " + String(httpCode));
    return;
  }
  http.end();

  EasyDDNS.service(DDNS_SERVICE);
  EasyDDNS.client(DDNS_URL, DDNS_USERNAME, DDNS_PWD);
  Serial.println("\tURL: " + DDNS_URL);
  Serial.println("\tUSERNAME: " + DDNS_USERNAME);
  Serial.println("\tPWD: " + DDNS_PWD);
  EasyDDNS.onUpdate([&](const char *oldIP, const char *newIP) {
    Serial.print("EasyDDNS - IP Change Detected: ");
    Serial.println(newIP);
  });
  Serial.println("\tDynDNS " + DDNS_URL + " is up and running");
}

void establishUPnP()
{
  Serial.println("[Establishing UPnP]");

  if(!UPNP_ENABLED)
  {
    Serial.println("\tUPnP is disabled");
    return;
  }

  portMappingResult portMappingAdded;
  tinyUPnP->addPortMappingConfig(WiFi.localIP(), UPNP_LISTEN_PORT, RULE_PROTOCOL_TCP, UPNP_LEASE_TIME, AP_NAME);
  Serial.print("\tService name: ");
  Serial.println(AP_NAME);
  Serial.print("\tListen Port: ");
  Serial.println(UPNP_LISTEN_PORT);
  Serial.print("\tLease Time: ");
  Serial.println(UPNP_LEASE_TIME);

  portMappingAdded = tinyUPnP->commitPortMappings();
  Serial.println("\ttinyUPnP -> printAllPortMappings after trying to set uPnP");
  tinyUPnP->printPortMappingConfig();

  ssdpDeviceNode *ssdpDeviceNodeList = tinyUPnP->listSsdpDevices();
  tinyUPnP->printSsdpDevices(ssdpDeviceNodeList);
}

void establishmDNS()
{
  Serial.println("[Establishing mDNS]");
  Serial.println("\tSetting up on " + AP_NAME + ".local");
  if (!MDNS.begin(AP_NAME, WiFi.localIP()))
  {
    Serial.println("Error setting up MDNS responder");
  }
  MDNS.update();
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("https", "tcp", 80);
  Serial.println("\tMDNS responder is up and running on " + AP_NAME + ".local");
}

// WEBSERVER functions
void initialiseWebServer()
{
  Serial.println("[Initialise Webserver]");
  webserver.on("/", HTTP_GET, rootPage);
  webserver.on("/CHANGESTATUS", HTTP_POST, handleTrafficLightStatus);
  webserver.on("/TURNONANDOFF", HTTP_POST, handleTrafficLightOnAndOff);
  webserver.on("/SETTINGS", HTTP_GET, settingsPage);
  webserver.on("/SETTINGS_SUBMIT", HTTP_POST, processSettings);
  webserver.on("/SETTINGS_RESET", HTTP_POST, resetSettings);
  webserver.on("/FACTORY_RESET", HTTP_POST, factoryReset);
  webserver.on("/SSID_SETTINGS", HTTP_GET, ssidSettingsPage);
  webserver.on("/SSID_SETTINGS_SUBMIT", HTTP_POST, processSSIDS);
  webserver.onNotFound(notfoundPage);
  webserver.begin();
  Serial.println("\tWebserver is up and running");
  Serial.println("--------------");
  Serial.println("");
  return;
}

String getTrafficLightHTML()
{
  String htmlCode = loadFile(SEMAFORO_HTML_FILE);
  String currentColor = "red";

  if (!semaforoEncendido)
    currentColor = "off";
  else if (semaforoAbierto)
    currentColor = "green";

  htmlCode.replace("{{color}}", currentColor);

  //  Serial.println(htmlCode);
  return htmlCode;
}
String getSSIDSettingsHTML()
{
  //  Serial.println("Composing HTML");
  String htmlCode = loadFile(SSID_SETTINGS_HTML_FILE);
  ssid storedOptions[10];

  String ssidsJson = loadFile(SSIDS_FILE);
//  Serial.println("SSIDS Json file read: ");
//  Serial.println(ssidsJson);

  htmlCode.replace("{{existingSSIDs}}", ssidsJson);
  htmlCode.replace("{{default_wifi_ip}}", DEFAULT_IP);
  htmlCode.replace("{{default_wifi_gateway}}", DEFAULT_GATEWAY);
  htmlCode.replace("{{default_wifi_mask}}", DEFAULT_MASK);
  htmlCode.replace("{{default_phone_ip}}", PHONE_IP);
  htmlCode.replace("{{default_phone_gateway}}", PHONE_GATEWAY);
  htmlCode.replace("{{default_phone_mask}}", PHONE_MASK);

  //  Serial.println("HTML Code read: ");
  //  Serial.println(htmlCode);
  return htmlCode;
}
String getGeneralSettingsHTML()
{
  String htmlCode = loadFile(SETTINGS_HTML_FILE);
  htmlCode.replace("{{transition_speed}}", String(AMBER_TIME));
  htmlCode.replace("{{amber_transition}}", AMBER_TRANSITION ? "checked" : "");
  htmlCode.replace("{{red_to_green}}", RED_TO_GREEN ? "checked" : "");
  htmlCode.replace("{{green_to_red}}", GREEN_TO_RED ? "checked" : "");
  htmlCode.replace("{{default_ip}}", DEFAULT_IP);
  htmlCode.replace("{{phone_ip}}", PHONE_IP);
  htmlCode.replace("{{ap_ip}}", AP_IP);
  htmlCode.replace("{{ap_name}}", AP_NAME);
  htmlCode.replace("{{ddns_enabled}}", DDNS_ENABLED ? "checked" : "");
  htmlCode.replace("{{ddns_service}}", DDNS_SERVICE);
  htmlCode.replace("{{ddns_url}}", DDNS_URL);
  htmlCode.replace("{{ddns_username}}", DDNS_USERNAME);
  htmlCode.replace("{{ddns_pwd}}", DDNS_PWD);
  htmlCode.replace("{{upnp_enabled}}", UPNP_ENABLED ? "checked" : "");
  htmlCode.replace("{{listen_port}}", String(UPNP_LISTEN_PORT));

  //  Serial.println(htmlCode);
  return htmlCode;
}

void rootPage()
{
  Serial.println("new connection");
  webserver.send(200, "text/html", getTrafficLightHTML());
}
void handleTrafficLightStatus()
{
  sendJSCall("Processing");
  toggleTrafficLightStatus();
  webserver.sendHeader("Location", "/"); // Add a header to respond with a new location for the browser to go to the home page again
  webserver.send(303);
  Serial.println("Webserver relocation sent");
}
void handleTrafficLightOnAndOff()
{
  sendJSCall("Processing");
  toggleTrafficLightOnOff();
  webserver.sendHeader("Location", "/"); // Add a header to respond with a new location for the browser to go to the home page again
  webserver.send(303);
  Serial.println("Webserver relocation sent");
}

void ssidSettingsPage()
{
  Serial.println("new ssid settings connection");
  webserver.send(200, "text/html", getSSIDSettingsHTML());
}

void settingsPage()
{
  Serial.println("new general settings connection");
  webserver.send(200, "text/html", getGeneralSettingsHTML());
}

void processSettings()
{
  setSettings(webserver.arg("plain"));
}

void factoryReset()
{
  resetSPIFFS();
  webserver.sendHeader("Location", "/SETTINGS");
  webserver.send(303);

}
void resetSettings()
{
  setSettings(loadFile(DEFAULT_SETTINGS_FILE));
}

void setSettings(String values)
{
  saveToFile(SETTINGS_FILE, values);
  loadSettings(SETTINGS_FILE);
  webserver.sendHeader("Location", "/SETTINGS");
  webserver.send(303);
}

void processSSIDS()
{
  String ssidsRaw = webserver.arg("plain");
  DynamicJsonDocument doc(2048);
  saveToFile(SSIDS_FILE, ssidsRaw);
  webserver.sendHeader("Location", "/SSID_SETTINGS");
  webserver.send(303);
}

void notfoundPage()
{
  webserver.send(404, "text/plain", "404: Not found");
}

void sendJSCall(String call)
{
  Serial.println(call);
}

// TRAFFIC LIGHT functions
// Traffic light logic
void cerrarSemaforo(int ambar_wait)
{
  if (GREEN_TO_RED)
  {
    transition(yellow, green, red);
    delay(ambar_wait * 1000);
    transition(red, yellow, green);
  }
  establishColor(red);
}
void abrirSemaforo(int ambar_wait)
{
  if (RED_TO_GREEN)
  {
    transition(yellow, red, green);
    delay(ambar_wait * 1000);
    transition(green, yellow, red);
  }
  establishColor(green);
}
void toggleTrafficLightStatus(){
  if (semaforoAbierto)
  {
    Serial.println("Cerrando semaforo");
    cerrarSemaforo(AMBER_TIME);
    semaforoAbierto = false;
    sendJSCall("Cerrado");
  }
  else
  {
    Serial.println("Abriendo semaforo");
    abrirSemaforo(AMBER_TIME);
    semaforoAbierto = true;
    sendJSCall("Abierto");
  }
}

void toggleTrafficLightOnOff(){
    if (semaforoEncendido)
  {
    Serial.println("Apagando semaforo");
    turnTrafficLightOff();
    semaforoEncendido = false;
    sendJSCall("Apagado");
  }
  else
  {
    Serial.println("Encendiendo semaforo");
    initialiseTrafficLight();
    semaforoEncendido = true;
    sendJSCall("Encendido");
  }
}
void sequentialDemo()
{
  int sequential_delay = 300;
  establishColor(green);
  delay(sequential_delay);
  transition(yellow, green, red);
  delay(sequential_delay);
  transition(red, yellow, green);
  delay(sequential_delay);
  transition(yellow, red, green);
  delay(sequential_delay);
  transition(green, yellow, red);
  delay(sequential_delay);
  establishColor(green);
}
void setTrafficLightAsLoading()
{
  establishColor(yellow);
  return;
}
void initialiseTrafficLight()
{
  sequentialDemo();
  if (semaforoAbierto)
    establishColor(green);
  else
    establishColor(red);
  return;
}
void transition(int high, int mid, int low)
{
  if (AMBER_TRANSITION)
    analogWrite(mid, 300);
  else
    analogWrite(mid, 0);
  analogWrite(high, 1023);
  analogWrite(low, 0);
  return;
}
void establishColor(int color)
{
  int colors[] = {green, yellow, red};
  for (int i = 0; i < 3; i++)
  {
    if (color == colors[i])
      analogWrite(color, 1023);
    else
      analogWrite(colors[i], 0);
  }
}
void all(int value)
{
  int colors[] = {green, yellow, red};
  for (int i = 0; i < 3; i++)
  {
    analogWrite(colors[i], value);
  }
}
void turnTrafficLightOff()
{
  for (int dim = 1023; dim > 0; dim--)
  {
    all(dim);
    delay(AMBER_TIME);
  }
  all(0);
}
boolean connectToWifi()
{
  ssid validWifi = findValidWifi(SSIDS_FILE);

  if (validWifi.ssid != "")
  {
    // Connect to wifi
    Serial.println("\tSelected wifi to connect:");
    printEntry(validWifi);
    connectToWifi(validWifi);
    return true;
  }
  else
  {
    // Initialise as AP
    Serial.println("\tNo wifi elligible to connect was found.");
    initializeAsAP();
    return false;
  }
}
void setup()
{
  WiFi.setAutoConnect(0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println("---------------------");
  Serial.println("---------------------");
  Serial.println("[Initialising system]");
  Serial.println("---------------------");
  Serial.println("---------------------");
  //resetSPIFFS();
  definePins();
  setTrafficLightAsLoading();

  loadSettings(SETTINGS_FILE);
  bool connected = connectToWifi();

  establishmDNS();
  initialiseWebServer();
  initialiseTrafficLight();

  if (connected)
  {
    establishUPnP();
    establishDDNS();
  }
}

void handleButtonPush(){
  int btn_Status = HIGH;
  btn_Status = digitalRead (button);
  if(btn_Status == LOW)
  {
  Serial.println("[buttonPush]");
  Serial.println("\tSwitching traffic light status");
   toggleTrafficLightStatus();
  }
}

void loop()
{
  MDNS.update();
  
  webserver.handleClient();

  handleButtonPush();
  if(UPNP_ENABLED)
    tinyUPnP->updatePortMappings(UPNP_LEASE_TIME);
  if(DDNS_ENABLED)
    EasyDDNS.update(UPNP_LEASE_TIME);
}
