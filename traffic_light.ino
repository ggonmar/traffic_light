#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Set WiFi credentials
const char* WIFIS[] = {"GD57A", "Poliwifi","LaCasaDeNemo"};
const char* PWDS[] = {"En1lug@rdelaMancha", "fader1946", "Olivia2017"};

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
const int GND = d7;

const int DELAY_TIME = 5000;
const int AMBAR_TIME = 5;

ESP8266WebServer webserver(80);
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

  initialiseWebServer();

  initialiseTrafficLight();
}

void loop() {
  //  printTemperature();
  webserver.handleClient();
  //printTemperature();
  //sequentialDemo();
}

void definePins(){
  pinMode(green, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(GND, OUTPUT);
  digitalWrite(GND, LOW);
  digitalWrite(BUILTIN_LED, HIGH);
  return;
}

// WIFI functions
void connectToWifi(int index){
  WiFi.begin(WIFIS[index], PWDS[index]);

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  // WiFi Connected
  Serial.println("");
  Serial.println("Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

int findValidWifi(){
  int nbVisibleNetworks = WiFi.scanNetworks();
  if (nbVisibleNetworks == 0) {
    Serial.println(F("no networks found. Reset to try again"));
    while (true); // no need to go further, hang in there, will auto launch the Soft WDT reset
  }

  boolean foundValidWifi=false;
  int i, j;
  for (i = 0; i < nbVisibleNetworks; ++i) {
    for(j=0; j<sizeof(WIFIS); j++){
      if(!strcmp(WiFi.SSID(i).c_str(), WIFIS[j]))
      {
        foundValidWifi=true;
        break;
      }
    }
    if(foundValidWifi)
    break;
  }
  Serial.print("Trying to connect to");
  Serial.println(WIFIS[j]);
  return j;
}

// WEBSERVER functions
void initialiseWebServer(){
  webserver.on("/", rootPage);
  webserver.onNotFound(notfoundPage);
  webserver.begin();
  return;
}

void rootPage() {
  if(semaforoAbierto)
  {
    webserver.send(200, "text/html", HTMLcerrado);
    cerrarSemaforo(AMBAR_TIME);
    semaforoAbierto=false;
  }
  else
  {
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
  int colors[]= {green, yellow, red};
  int i=0;
  for (i=0;i<3; i++)
  {
    if(color == colors[i])
    analogWrite(color, 1023);
    else
    analogWrite(colors[i], 0);
  }
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

