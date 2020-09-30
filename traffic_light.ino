#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EasyDDNS.h>

// Set WiFi credentials
const String WIFIS[] = {"GD57A", "Poliwifi","LaCasaDeNemo"};
const String PWDS[] = {"En1lug@rdelaMancha", "fader1946", "Olivia2017"};

ESP8266WebServer webserver;

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

//const String HTMLcerrado="<html><title> Semaforo de Reunion 1.0 </title><body style='background-color:red'><h1 style='color:white'>Cerrado</h1><div style='color:white'>Por favor, no acceda.<p>Refrescar la pagina para abrir el semaforo</div><img src='https://www.clipartmax.com/png/full/286-2868544_semaphore-red-light-red-traffic-light-icon.png' style='height:300px; position:absolute; top:10px; left:300px'></body></html>";
//const String HTMLabierto="<html><title> Semaforo de Reunion 1.0 </title><body style='background-color:green'><h1 style='color:white'>Abierto</h1><div style='color:white'>Adelante, puede pasar.<p>Refrescar la pagina para cerrar el semaforo</div><img src='https://www.clipartmax.com/png/full/5-52723_clip-art-traffic-light-green-traffic-light-clipart.png' style='height:300px; position:absolute; top:10px; left:300px'></body></html>";


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
  webserver.on("/", HTTP_GET, rootPage);
  webserver.on("/TRAFFICLIGHT", HTTP_POST, handleTrafficLight);
  webserver.onNotFound(notfoundPage);
  webserver.begin();
  Serial.println("Webserver is up and running");
  return;
}

String getProperHTML(){
  String htmlCode;
  
  if(semaforoAbierto)
  {
      htmlCode= "<html><title>[Abierto] Semaforo de Reunion 1.0 </title>";
      htmlCode += "	<body style='background-color:green'>";
}  else
{
    htmlCode = "<html><title>[Cerrado] Semaforo de Reunion 1.0 </title>";
    htmlCode +=  "	<body style='background-color:red'>";
}
  if(semaforoAbierto)
  {
    htmlCode += ""
    "		<h1 id=\"title\" style='color:white'>Abierto</h1>"
    "		<div id=\"message\" style='color:white'>El acceso esta permitido.<p></div>"
    "		<img id=\"image\" src=\"https://www.clipartmax.com/png/full/5-52723_clip-art-traffic-light-green-traffic-light-clipart.png\" style=\"height:300px; position:absolute; top:10px; left:300px\">"
    "    <div style=\"padding-top:50px\">"
    "        <form action=\"/TRAFFICLIGHT\" method=\"POST\">"
    "			       <input id=\"button\" type=\"submit\" value=\"Cerrar semaforo\" style=\"width:250px; height:150px\">"
    "        </form>"
    "    </div>";
  }else
  {
      htmlCode += ""
    "		<h1 id=\"title\" style='color:white'>Cerrado</h1>"
    "		<div id=\"message\" style='color:white'>El acceso esta restringido.<p></div>"
    "		<img id=\"image\" src=\"https://www.clipartmax.com/png/full/286-2868544_semaphore-red-light-red-traffic-light-icon.png\" style=\"height:300px; position:absolute; top:10px; left:300px\">"
    "    <div style=\"padding-top:50px\">"
    "        <form action=\"/TRAFFICLIGHT\" method=\"POST\">"
    "			       <input id=\"button\" type=\"submit\" value=\"Abrir semaforo\" style=\"width:250px; height:150px\">"
    "        </form>"
    "    </div>";

  }
  htmlCode += "  </body></html>";
  return htmlCode;
}
void rootPage() {
  Serial.println("new connection");
  webserver.send(200, "text/html", getProperHTML());
}

void handleTrafficLight(){
  if(semaforoAbierto)
  {
    sendJSCall("Processing");
    Serial.println("Cerrando semaforo");
    cerrarSemaforo(AMBAR_TIME);
    semaforoAbierto=false;
    sendJSCall("Cerrado");
  }
  else
  {
    sendJSCall("Processing");
    Serial.println("Abriendo semaforo");
    abrirSemaforo(AMBAR_TIME);
    semaforoAbierto=true;
    sendJSCall("Abierto");
  }

  webserver.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  webserver.send(303);
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

void sendJSCall(String mode)
{

}
/*
function sendJSCall(mode)
{
    modes=[
    {
mode:"green", bg:"green", title:"Abierto", message:"Adelante", button:"Cerrar semaforo", disabled:false, src:"https://www.clipartmax.com/png/full/5-52723_clip-art-traffic-light-green-traffic-light-clipart.png"},
{mode:"amber", bg:"orange", title:"Procesando...", message:"", button:"Procesando...", disabled:true, src:"https://www.clipartmax.com/png/full/267-2678270_traffic-light-amber-clip-art-at-clker-traffic-light-green-png.png"},
{mode:"red", bg:"red", title:"Cerrado", message:"Prohibido el paso", button:"Abrir semaforo", disabled:false, src:"https://www.clipartmax.com/png/full/286-2868544_semaphore-red-light-red-traffic-light-icon.png"}];

    mode=modes.filter(e => e.mode == mode)[0];
  document.title = "[" + mode.title + "] Semaforo de Reunion 2.0";
  document.body.style.backgroundColor = mode.bg;
  document.getElementById("title").innerHTML = mode.title;
  document.getElementById("message").innerHTML = mode.message;
  document.getElementById("button").disabled = mode.disabled;
  document.getElementById("button").value = mode.button;
  document.getElementById("image").src =mode.src;

console.log(mode);
}
*/

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
