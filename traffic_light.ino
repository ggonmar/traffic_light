#include <ESP8266WiFi.h> // Include the Wi-Fi library
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EasyDDNS.h>
#include "FS.h"
#include <ArduinoJson.h>

String SSIDS_FILE = "/good_ssids.txt";

String SEMAFORO_HTML_FILE = "/semaforoPage.html";
String ADMIN_HTML_FILE = "/adminPage.html";

struct ssid
{
  String name;
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

const int AMBAR_TIME = 3;
boolean semaforoAbierto = true;
boolean semaforoEncendido = true;

void resetSPIFFS()
{
  Serial.println("");
  Serial.println("ABOUT TO FORMAT SPIFFS");
  SPIFFS.begin();
  SPIFFS.format();
  SPIFFS.end();
  Serial.println("SPIFFS FORMATTED");
  Serial.println("");
}

void definePins()
{
  Serial.println("Define pins");
  pinMode(green, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(GND, OUTPUT);
  digitalWrite(GND, LOW);
  digitalWrite(BUILTIN_LED, HIGH);
  return;
}

// WIFI functions
int numberOfStoredSSIDs(String filename)
{
  SPIFFS.begin();

  int numberOfSSIDs = -1;
  if (SPIFFS.exists(filename))
  {
    File f = SPIFFS.open(filename, "r");
    if (f)
    {
      String s;
      s = f.readStringUntil('\n');
      s.trim();
      numberOfSSIDs = s.toInt();
      //      Serial.println(s);
    }
    f.close();
  }
  SPIFFS.end();
  return numberOfSSIDs;
}

void loadSSIDsFromFile(int numberOfSSIDs, ssid *wifiInfo, String filename)
{
  SPIFFS.begin();

  if (SPIFFS.exists(filename))
  {
    File f = SPIFFS.open(filename, "r");
    if (f)
    {
      String s;
      s = f.readStringUntil('\n');
      s.trim();
      for (int i = 0; i < numberOfSSIDs; i++)
      {
        s = f.readStringUntil('\n');
        s.trim();
        int firstComma = s.indexOf(",");
        int secondComma = s.indexOf(",", firstComma + 1);
        String ssid = s.substring(0, firstComma);
        String pwd = s.substring(firstComma + 1, secondComma);
        bool type = s.substring(secondComma + 1).equals("T");

        wifiInfo[i].name = ssid;
        wifiInfo[i].pwd = pwd;
        wifiInfo[i].type = type;
      }
    }
    f.close();
  }
  SPIFFS.end();
}

void saveSSIDsInFile(int numberOfSSIDs, ssid *wifiInfo, String filename)
{
  SPIFFS.begin();
  File f = SPIFFS.open(filename, "w+");
  if (f)
  {
    f.println(String(numberOfSSIDs));

    for (int i = 0; i < numberOfSSIDs; i++)
      f.println(printEntry(wifiInfo[i]));
  }
  f.close();
  SPIFFS.end();

//  debugPrintSSIDsInFile(filename);
}

void debugPrintSSIDsInFile(String filename)
{
  int number =  numberOfStoredSSIDs(filename);
  ssid existing[20];
  loadSSIDsFromFile(number, existing, filename);
  Serial.println();
  Serial.println("SSIDs in " + filename);
//  for(int i=0; i< number; i++)
//     printEntry(existing[i]);
}

String printEntry(ssid entry)
{
  String s = entry.name + "," + entry.pwd + ",";
  s += entry.type ? "T" : "F";
//  Serial.println(s);
  return s;
}
ssid findValidWifi(String filename)
{
  ssid storedOptions[20];
  int numberOfStoredOptions;

  numberOfStoredOptions = numberOfStoredSSIDs(filename);

  loadSSIDsFromFile(numberOfStoredOptions, storedOptions, filename);

  Serial.println("SSIDs in " + filename);
//  for (int i = 0; i < numberOfStoredOptions; i++)
//    printEntry(storedOptions[i]);

  Serial.println("Finding valid Wifi");

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
  ssid correctOne = {name : "", pwd : "", type : false};
  while (foundValidWifi == false && attempts > 0)
  {
    String candidate_ssid = WiFi.SSID(i);
    Serial.print("Checking ");
    Serial.println(candidate_ssid);
    for (j = 0; j < numberOfStoredOptions; j++)
    {
      if (storedOptions[j].name.equals(candidate_ssid))
      {
        Serial.print("THIS IS IT: ");
        foundValidWifi = true;
        correctOne = storedOptions[j];
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
  Serial.println("Connecting to selected wifi");

  WiFi.begin(wifiData.name, wifiData.pwd);

  //IP Handling
  IPAddress local_IP(192, 168, 1, 23);
  IPAddress gateway(192, 168, 1, 1);
  if (wifiData.type)
  {
    IPAddress local_IP(192, 168, 43, 23);
    IPAddress gateway(192, 168, 43, 150);
  }

  IPAddress subnet(255, 255, 255, 0);

  if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("STA Failed to configure");
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  // WiFi Connected
  Serial.println("");
  Serial.println("Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}
void initializeAsAP()
{
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(IPAddress(192, 168, 1, 23), IPAddress(192, 168, 1, 23), IPAddress(255, 255, 255, 0)) ? "Ready" : "Failed!");
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP("Semaforito_AP") ? "Ready" : "Failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
}
void establishDDNS()
{
  Serial.println("Establishing DynDNS");
  String dyndns = "semaforo.myddns.me";
  EasyDDNS.service("noip");
  EasyDDNS.client(dyndns, "ggonzalezmartin@gmail.com", "PUT0noip!");
  EasyDDNS.onUpdate([&](const char *oldIP, const char *newIP) {
    Serial.print("EasyDDNS - IP Change Detected: ");
    Serial.println(newIP);
  });
  Serial.println("DynDNS " + dyndns + " is up and running");
}
void establishmDNS()
{
  String mDNSname = "semaforito";
  Serial.println("Establishing mDNS " + mDNSname + ".local");
  if (!MDNS.begin(mDNSname, WiFi.localIP()))
  {
    Serial.println("Error setting up MDNS responder");
  }
  MDNS.update();
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("https", "tcp", 80);
  Serial.println("MDNS responder is up and running");
}

// WEBSERVER functions
void initialiseWebServer()
{
  Serial.println("Initialising Webserver");
  webserver.on("/", HTTP_GET, rootPage);
  webserver.on("/CHANGESTATUS", HTTP_POST, handleTrafficLightStatus);
  webserver.on("/TURNONANDOFF", HTTP_POST, handleTrafficLightOnAndOff);
  webserver.on("/ADMIN", HTTP_GET, adminPage);
  webserver.on("/ADMIN_SUBMIT", HTTP_POST, processEntries);
  webserver.onNotFound(notfoundPage);
  webserver.begin();
  Serial.println("Webserver is up and running");
  return;
}

String getAdminHTML()
{
  String htmlCode;
  String storedSSIDsInHTML = "";
  int numberOfStoredOptions;
  ssid storedOptions[20];
  numberOfStoredOptions = numberOfStoredSSIDs(SSIDS_FILE);
  loadSSIDsFromFile(numberOfStoredOptions, storedOptions, SSIDS_FILE);

  for (int i = 0; i < numberOfStoredOptions; i++)
  {
    String standard_checked = storedOptions[i].type ? "" : "checked";
    String phone_checked = storedOptions[i].type ? "checked" : "";
    String n = String(i + 1);
    storedSSIDsInHTML += ""
                         "  <tr id='container_" + n + "'>"
                         "     <td><input type='text' class='ssid' name='ssid_" + n + "' value='" + storedOptions[i].name + "'/></td>"
                         "     <td><input type='password' class='pwd' name='pwd_" + n + "' value='" + storedOptions[i].pwd + "'/></td>" +
                         "     <td><input class='type' type='radio'  name='type_" + n + "' value='standard' " + standard_checked + "><label>Standard</label></input><br>" +
                         "         <input class='radio' type='radio' name='type_" + n + "' value='phone'    " + phone_checked + "><label>Phone (Tether)</label></input>"
                         "  </tr>";
  }

  SPIFFS.begin();
  if (SPIFFS.exists(ADMIN_HTML_FILE))
  {
    File f = SPIFFS.open(ADMIN_HTML_FILE, "r");
    while (f && f.available())
    {
      htmlCode += f.readStringUntil('\n');
    }
    htmlCode.replace("{{existingSSIDs}}", storedSSIDsInHTML);
    f.close();
  }
  SPIFFS.end();

  //  Serial.println(htmlCode);
  return htmlCode;
}

String getProperHTML()
{
  String htmlCode = "";
  String currentColor = "red";

  if (!semaforoEncendido)
    currentColor = "off";
  else if (semaforoAbierto)
    currentColor = "green";

  SPIFFS.begin();

  if (SPIFFS.exists(SEMAFORO_HTML_FILE))
  {
    File f = SPIFFS.open(SEMAFORO_HTML_FILE, "r");
    while (f && f.available())
    {
      htmlCode += f.readStringUntil('\n');
    }
    htmlCode.replace("{{color}}", currentColor);
    f.close();
  }
  SPIFFS.end();

  //  Serial.println(htmlCode);
  return htmlCode;
}

void rootPage()
{
  Serial.println("new connection");
  webserver.send(200, "text/html", getProperHTML());
}
void handleTrafficLightStatus()
{
  sendJSCall("Processing");
  if (semaforoAbierto)
  {
    Serial.println("Cerrando semaforo");
    cerrarSemaforo(AMBAR_TIME);
    semaforoAbierto = false;
    sendJSCall("Cerrado");
  }
  else
  {
    Serial.println("Abriendo semaforo");
    abrirSemaforo(AMBAR_TIME);
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

void adminPage()
{
  Serial.println("new admin connection");
  webserver.send(200, "text/html", getAdminHTML());
}

void processEntries()
{
  
  String ssidsRaw = webserver.arg("plain");
  Serial.println("POST As string:");
  Serial.println(ssidsRaw);

  Serial.println("");
  DynamicJsonDocument doc(1024);

  auto error = deserializeJson(doc, ssidsRaw);

  if (error)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
  }
//  serializeJsonPretty(doc, Serial);

  ssid ssidsToStore[doc.size()];
  for (int i = 0; i < doc.size(); i++)
  {
    //    Serial.println(doc[i]["ssid"].as<String>());
    ssidsToStore[i].name = doc[i]["ssid"].as<String>();
    ssidsToStore[i].pwd = doc[i]["pwd"].as<String>();
    ssidsToStore[i].type = doc[i]["type"];
//    printEntry(ssidsToStore[i]);
  }

  saveSSIDsInFile(doc.size(), ssidsToStore, SSIDS_FILE);

  webserver.sendHeader("Location", "/ADMIN"); // Add a header to respond with a new location for the browser to go to the home page again
  webserver.send(303);
  Serial.println("Webserver relocation sent");
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
  transition(yellow, green, red);
  delay(ambar_wait * 1000);
  transition(red, yellow, green);
  establishColor(red);
}
void abrirSemaforo(int ambar_wait)
{
  transition(yellow, red, green);
  delay(ambar_wait * 1000);
  transition(green, yellow, red);
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
  analogWrite(mid, 500);
  analogWrite(high, 1023);
  analogWrite(low, 0);
  return;
}
void establishColor(int color)
{
  int colors[] = {green, yellow, red};
  int i = 0;
  for (i = 0; i < 3; i++)
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
  int i;
  for (i = 0; i < 3; i++)
  {
    analogWrite(colors[i], value);
  }
}
void turnTrafficLightOff()
{
  int dim = 1023;
  for (dim = 1023; dim > 0; dim--)
  {
    all(dim);
    delay(AMBAR_TIME);
  }
  all(0);
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  //  resetSPIFFS();
  definePins();
  setTrafficLightAsLoading();

  ssid validWifi = findValidWifi(SSIDS_FILE);

  if (validWifi.name != "")
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
