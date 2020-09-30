#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EasyDDNS.h>

// Set WiFi credentials
const String WIFIS[] = {"GD57A", "Poliwifi","LaCasaDeNemo"};
const String PWDS[] = {"En1lug@rdelaMancha", "fader1946", "Olivia2017"};

const String HTMLcerrado="<html><title> Semaforo de Reunion 1.0 </title><body style='background-color:red'><h1 style='color:white'>Cerrado</h1><div style='color:white'>Por favor, no acceda.<p>Refrescar la pagina para abrir el semaforo</div><img src='https://www.clipartmax.com/png/full/286-2868544_semaphore-red-light-red-traffic-light-icon.png' style='height:300px; position:absolute; top:10px; left:300px'></body></html>";
const String HTMLabierto="<html><title> Semaforo de Reunion 1.0 </title><body style='background-color:green'><h1 style='color:white'>Abierto</h1><div style='color:white'>Adelante, puede pasar.<p>Refrescar la pagina para cerrar el semaforo</div><img src='https://www.clipartmax.com/png/full/5-52723_clip-art-traffic-light-green-traffic-light-clipart.png' style='height:300px; position:absolute; top:10px; left:300px'></body></html>";


//SHT3X sht30(0x45);


const int d0=16;
const int d5=14;
const int d6=12;
const int d7=13;

const int green = d6;
const int yellow = d5;
const int red = d0;
int colors[]= {green, yellow, red};

const int GND = d7;

const int DELAY_TIME = 5000;
const int AMBAR_TIME = 5;
boolean semaforoAbierto=true;

//IP Handling
IPAddress local_IP(192, 168, 1, 23);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


void setup() {
  Serial.begin(115200);
  definePins();
  setTrafficLightAsLoading();

  int wifiIndex =findValidWifi();
  connectToWifi(wifiIndex);

  establishDDNS();

  initialiseWebServer();

  initialiseTrafficLight();
}

void loop() {
  webserver.handleClient();
}

void definePins(){
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
int findValidWifi(){
  Serial.println("Finding valid Wifi");

  int nbVisibleNetworks =0;
  while( nbVisibleNetworks == 0)
  {
    int nbVisibleNetworks = WiFi.scanNetworks();
    if (nbVisibleNetworks == 0) {
      Serial.println("no networks found. Reset to try again");
    }
    if(nbVisibleNetworks > 0)
        break;
  }

  boolean foundValidWifi=false;
  int i=0;
  int j;
  while(foundValidWifi != true)
  {
    String candidate_ssid = WiFi.SSID(i);
    Serial.print("Checking ");
    Serial.println(candidate_ssid);
    for(j=0; j<sizeof(WIFIS); j++)
    {
      if(WIFIS[j].equals(candidate_ssid))
      {
        Serial.print("THIS IS IT: ");
        foundValidWifi=true;
        break;
      }
    }

    if(foundValidWifi == true)
       break;
    i=i+1;
  }
  Serial.println(WIFIS[j]);
  return j;
}

void connectToWifi(int index){
  Serial.println("Connecting to selected wifi");

  WiFi.begin(WIFIS[index], PWDS[index]);

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  // WiFi Connected
  Serial.println("");
  Serial.println("Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

void establishDDNS(){
  Serial.println("Establishing DynDNS");
  EasyDDNS.service("noip");
  EasyDDNS.client("semaforo.myddns.me", "ggonzalezmartin@gmail.com", "PUT0noip!");
  EasyDDNS.onUpdate([&](const char* oldIP, const char* newIP){
  Serial.print("EasyDDNS - IP Change Detected: ");
  Serial.println(newIP);
  });
}
// WEBSERVER functions
void initialiseWebServer(){
  Serial.println("Initialising Webserver");
  webserver.on("/", rootPage);
  webserver.onNotFound(notfoundPage);
  webserver.begin();
  Serial.println("Webserver is up and running");
  return;
}

void rootPage() {
  Serial.println("new connection");
  if(semaforoAbierto)
  {
    Serial.println("Cerrando semaforo");
    webserver.send(200, "text/html", HTMLcerrado);
    cerrarSemaforo(AMBAR_TIME);
    semaforoAbierto=false;
  }
  else
  {
    Serial.println("Abriendo semaforo");
    webserver.send(200, "text/html", HTMLabierto);
    abrirSemaforo(AMBAR_TIME);
    semaforoAbierto=true;
  }
}

// Handle 404
void notfoundPage(){
  webserver.send(404, "text/plain", "404: Not found");
}

// TRAFFIC LIGHT functions

// Traffic light logic
void cerrarSemaforo(int ambar_wait){
  transition(yellow, green, red);
  delay(ambar_wait*1000);
  transition(red, yellow, green);
  establishColor(red);
}

void abrirSemaforo(int ambar_wait){
  transition(yellow, red, green);
  delay(ambar_wait*1000);
  transition(green, yellow, red);
  establishColor(green);
}

void sequentialDemo(){
  establishColor(green);
  delay(DELAY_TIME);
  cerrarSemaforo(1);
  delay(DELAY_TIME);
  abrirSemaforo(5);
}

void setTrafficLightAsLoading(){
  establishColor(yellow);
  return;
}

void initialiseTrafficLight(){
  establishColor(green);
  return;
}

void transition(int high, int mid, int low){
  analogWrite(mid, 500);
  analogWrite(high, 1023);
  analogWrite(low, 0);
  return;
}

void establishColor(int color){
  int i;
  for (i=0;i<3; i++)
  {
    if(color == colors[i])
      analogWrite(color, 1023);
    else
      analogWrite(colors[i], 0);
  }
}

void turnOff()
{
  int i;
  for(i=0; i< sizeof(colors); i++)
    analogWrite(colors[i], 0);
}

void blink(int color){
  turnOff();
  establishColor(color);
  delay(300);
  turnOff();
}



/*
// Temperature Logic
void printTemperature()
{
if(sht30.get()==0)
{
Serial.print("Temperature in Celsius : ");
Serial.println(sht30.cTemp);
}
else
Serial.println("Error!");
}
*/
