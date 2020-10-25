#include <ESP8266WiFi.h> // Include the Wi-Fi library
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EasyDDNS.h>
#include "FS.h"
#include <ArduinoJson.h>

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
  bool type;
};

ESP8266WebServer webserver;
MDNSResponder mdns;

const int d0 = 16;
const int d5 = 14;
const int d6 = 12;
const int d7 = 13;

const int green = d6;
const int yellow = d5;
const int red = d0;
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
String PHONE_IP = "192.168.43.23";
String AP_IP = "192.168.1.23";
String DYN_DNS_URL = "semaforo.myddns.me";
String DYN_DNS_USERNAME = "ggonzalezmartin@gmail.com";
String DYN_DNS_PWD = "foobar1";

void convertToIp(String ip, int ipArray[4])
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

void definePins()
{
  Serial.println("[DefinePins]");
  pinMode(green, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(GND, OUTPUT);
  digitalWrite(GND, LOW);
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.println("\n\tPins set.");
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
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, s);

  AMBER_TIME = doc["transition_speed"];
  AMBER_TRANSITION = doc["amber_transition"];
  RED_TO_GREEN = doc["red_to_green"];
  GREEN_TO_RED = doc["green_to_red"];
  AP_NAME = doc["ap_name"].as<String>();
  DEFAULT_IP = doc["default_ip"].as<String>();
  PHONE_IP = doc["phone_ip"].as<String>();
  AP_IP = doc["ap_ip"].as<String>();
  DYN_DNS_URL = doc["dyn_dns_url"].as<String>();
  DYN_DNS_USERNAME = doc["dyn_dns_username"].as<String>();
  DYN_DNS_PWD = doc["dyn_dns_pwd"].as<String>();

  String str = "[loadSettings]"
               "\n\tTransition speed: " +
               String(AMBER_TIME) +
               "\n\tPass by Amber? " + String(AMBER_TRANSITION) +
               "\n\t   When Red to Green? " + String(RED_TO_GREEN) +
               "\n\t   When Green to Red? " + String(GREEN_TO_RED) +
               "\n\tAP Name: " + AP_NAME +
               "\n\tDefault IPs:" +
               "\n\t   Standard: " + DEFAULT_IP +
               "\n\t   Phone (tether): " + PHONE_IP +
               "\n\t   AP: " + AP_IP +
               "\n\tDynDNS:" +
               "\n\t   URL: " + DYN_DNS_URL +
               "\n\t   Username: " + DYN_DNS_USERNAME +
               "\n\t   Password: " + DYN_DNS_PWD;
  Serial.println(str);
}

int loadSSIDs(String filename, ssid availableSlots[])
{
  Serial.println("[loadSSIDs]");
  SPIFFS.begin();
  String s = loadFile(filename);

  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, s);
  if (error)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    deserializeJson(doc, "[]");
  }

  //Serial.println("[loadSSIDs]   In file: " + String(doc.size()) + " elements:");
  for (int i = 0; i < doc.size(); i++)
  {
    availableSlots[i].ssid = doc[i]["ssid"].as<String>();
    availableSlots[i].pwd = doc[i]["pwd"].as<String>();
    availableSlots[i].type = doc[i]["type"];
    //printEntryString(String(i + 1), availableSlots[i].ssid, availableSlots[i].pwd, availableSlots[i].type);
  }

  return doc.size();
}

void debugPrintSSIDsInFile(String filename)
{
  ssid ssidSlots[20];
  int n = loadSSIDs(filename, ssidSlots);
  Serial.println();
  Serial.println("[debugPrintSSIDsInFile]");
  Serial.println("\n\tSSIDs in " + filename + ": " + n + " elements");

  for (int i = 0; i < n; i++)
    printEntry(ssidSlots[i]);
}

String printEntryString(String n, String ssid, String pwd, boolean type)
{
  String s = "\t  " + n + ". " + ssid + " - " + pwd + " (" + type + ")";
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
  ssid ssidSlots[20];
  int n = loadSSIDs(filename, ssidSlots);

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
  int attempts = 20;
  int i = 0;
  int j;
  ssid correctOne = {ssid : "", pwd : "", type : false};
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
  Serial.println("[connectToWifi]");
  Serial.print("\tConnecting to selected wifi: ");
  printEntry(wifiData);

  WiFi.begin(wifiData.ssid, wifiData.pwd);

  //IP Handling
  convertToIp(DEFAULT_IP, ip);
  IPAddress local_IP(ip[0], ip[1], ip[2], ip[3]);
  IPAddress gateway(192, 168, 1, 1);
  if (wifiData.type)
  {
    convertToIp(PHONE_IP, ip);
    IPAddress local_IP(ip[0], ip[1], ip[2], ip[3]);
    IPAddress gateway(192, 168, 43, 150);
  }

  IPAddress subnet(255, 255, 255, 0);

  if (!WiFi.config(local_IP, gateway, subnet))
    Serial.println("STA Failed to configure");

  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  // WiFi Connected
  Serial.println("");
  Serial.println("\tConnected!");
  Serial.print("\t IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("\t MAC: ");
  Serial.println( WiFi.macAddress());
}

void initializeAsAP()
{
  int *ip;
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.print("Setting soft-AP configuration ... ");
  convertToIp(DEFAULT_IP, ip);
  Serial.println(WiFi.softAPConfig(IPAddress(ip[0], ip[1], ip[2], ip[3]), IPAddress(ip[0], ip[1], ip[2], ip[3]), IPAddress(255, 255, 255, 0)) ? "Ready" : "Failed!");
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(AP_NAME + "_AP") ? "Ready" : "Failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
}
void establishDDNS()
{
  Serial.println("[Establishing DynDNS]");
  EasyDDNS.service("noip");
  EasyDDNS.client(DYN_DNS_URL, DYN_DNS_USERNAME, DYN_DNS_PWD);
  EasyDDNS.onUpdate([&](const char *oldIP, const char *newIP) {
    Serial.print("EasyDDNS - IP Change Detected: ");
    Serial.println(newIP);
  });
  Serial.println("\tDynDNS " + DYN_DNS_URL + " is up and running");
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
  Serial.println("\tMDNS responder is up and running on " + AP_NAME +".local");
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
  String storedSSIDsInHTML = "";
  ssid storedOptions[20];
  int n = loadSSIDs(SSIDS_FILE, storedOptions);

  for (int i = 0; i < n; i++)
  {
    String standard_checked = storedOptions[i].type ? "" : "checked";
    String phone_checked = storedOptions[i].type ? "checked" : "";
    String n = String(i + 1);
    storedSSIDsInHTML += ""
                         "  <tr id='container_" +
                         n + "'>" +
                         "     <td><input type='text' class='ssid' name='ssid_" + n + "' value='" + storedOptions[i].ssid + "'/></td>" +
                         "     <td><input type='password' class='pwd' name='pwd_" + n + "' value='" + storedOptions[i].pwd + "'/></td>" +
                         "     <td><input class='type' type='radio'  name='type_" + n + "' value='standard' " + standard_checked + "><label>Standard</label></input><br>" +
                         "         <input class='radio' type='radio' name='type_" + n + "' value='phone'    " + phone_checked + "><label>Phone (Tether)</label></input>" +
                         "  </tr>";
  }
  //  Serial.println(storedSSIDsInHTML);

  htmlCode.replace("{{existingSSIDs}}", storedSSIDsInHTML);
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
  htmlCode.replace("{{dyn_dns_url}}", DYN_DNS_URL);
  htmlCode.replace("{{dyn_dns_username}}", DYN_DNS_USERNAME);
  htmlCode.replace("{{dyn_dns_pwd}}", DYN_DNS_PWD);


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

  webserver.sendHeader("Location", "/"); // Add a header to respond with a new location for the browser to go to the home page again
  webserver.send(303);
  Serial.println("Webserver relocation sent");
}
void handleTrafficLightOnAndOff()
{
  sendJSCall("Processing");
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
  DynamicJsonDocument doc(1024);
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
  if (AMBER_TRANSITION && GREEN_TO_RED)
  {
    transition(yellow, green, red);
    delay(ambar_wait * 1000);
    transition(red, yellow, green);
  }
  establishColor(red);
}
void abrirSemaforo(int ambar_wait)
{
  if (AMBER_TRANSITION && RED_TO_GREEN)
  {
    transition(yellow, red, green);
    delay(ambar_wait * 1000);
    transition(green, yellow, red);
  }
  establishColor(green);
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
  establishColor(green);
  return;
}
void transition(int high, int mid, int low)
{
  analogWrite(mid, 300);
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

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  //resetSPIFFS();
  definePins();
  setTrafficLightAsLoading();

  loadSettings(SETTINGS_FILE);
  ssid validWifi = findValidWifi(SSIDS_FILE);

  Serial.println("\tSelected wifi to connect:");
  printEntry(validWifi);

  if (validWifi.ssid != "")
  {
    connectToWifi(validWifi);
    establishDDNS();
  }
  else
  {
    // BE AP
    initializeAsAP();
  }

  // establishmDNS();
  initialiseWebServer();
  initialiseTrafficLight();
}

void loop()
{
  MDNS.update();
  webserver.handleClient();
}
