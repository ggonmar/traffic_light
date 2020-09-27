#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Set WiFi credentials
#define WIFI_SSID "GD57A"
#define WIFI_PASS "En1lug@rdelaMancha"
//#define WIFI_SSID "Poliwifi"
//#define WIFI_PASS "fader1946"

const String HTMLcerrado="<html><title> Semaforo de Reunion 1.0 </title><body style='background-color:red'><h1 style='color:white'>Cerrado</h1><div style='color:white'>Por favor, no acceda.<p>Refrescar la pagina para abrir el semaforo</div><img src='https://www.clipartmax.com/png/full/286-2868544_semaphore-red-light-red-traffic-light-icon.png' style='height:300px; position:absolute; top:10px; left:300px'></body></html>";
const String HTMLabierto="<html><title> Semaforo de Reunion 1.0 </title><body style='background-color:green'><h1 style='color:white'>Abierto</h1><div style='color:white'>Adelante, puede pasar.<p>Refrescar la pagina para cerrar el semaforo</div><img src='https://www.clipartmax.com/png/full/5-52723_clip-art-traffic-light-green-traffic-light-clipart.png' style='height:300px; position:absolute; top:10px; left:300px'></body></html>";


SHT3X sht30(0x45);


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
// Handle Root
void rootPage() {
  if(semaforoAbierto)
  {
    cerrarSemaforo(AMBAR_TIME);
    semaforoAbierto=false;
    webserver.send(200, "text/html", HTMLcerrado);
  }
  else
  {
    abrirSemaforo(AMBAR_TIME);
    semaforoAbierto=true;
    webserver.send(200, "text/html", HTMLabierto);
  }
}
 
// Handle 404
void notfoundPage(){
  webserver.send(404, "text/plain", "404: Not found");
}

void setup() {  
  Serial.begin(115200);

  pinMode(BUILTIN_LED, OUTPUT);  // initialize onboard LED as output
  pinMode(green, OUTPUT);  // initialize onboard LED as output
  pinMode(yellow, OUTPUT);  // initialize onboard LED as output
  pinMode(red, OUTPUT);  // initialize onboard LED as output
  pinMode(GND, OUTPUT);  // initialize onboard LED as output


  establishColor(green);  
  digitalWrite(GND, LOW);
  digitalWrite(BUILTIN_LED, HIGH);



  //Begin WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(100); }
 
  // WiFi Connected
  Serial.println("");
  Serial.println("Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
   Serial.print("MAC: ");
   Serial.println(WiFi.macAddress());
  // Start Web Server
  webserver.on("/", rootPage);
  webserver.onNotFound(notfoundPage);
  webserver.begin();
}

void loop() {
//  printTemperature();
     webserver.handleClient();
    //printTemperature();
    //sequentialDemo(); 
}

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

void transition(int high, int mid, int low){
   analogWrite(mid, 500);
   analogWrite(high, 1023);
   analogWrite(low, 0);
   return;
}

void establishColor(int color){
  if(color==green)
  {
     analogWrite(green, 1023);
     analogWrite(red, 0);
     analogWrite(yellow, 0);
  }
  else if(color==yellow)
  {
     analogWrite(green, 0);
     analogWrite(red, 0);
     analogWrite(yellow, 1023);
}
  else {
     analogWrite(green, 0);
     analogWrite(red, 1023);
     analogWrite(yellow, 0);
}
  }

void printTemperature()
{
  if(sht30.get()==0){
    Serial.print("Temperature in Celsius : ");
    Serial.println(sht30.cTemp);
    }
  else
  {
    Serial.println("Error!");
  }
}
