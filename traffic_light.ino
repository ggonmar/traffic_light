#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EasyDDNS.h>

// Set WiFi credentials
const String WIFIS[] = {"GD57A", "Poliwifi","LaCasaDeNemo", "JuanPlas", "Polis8"};
const String PWDS[] = {"En1lug@rdelaMancha", "fader1946", "Olivia2017", "internet", "milepolis8"};

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

const int DELAY_TIME = 3000;
const int AMBAR_TIME = 5;
boolean semaforoAbierto=true;
boolean semaforoEncendido=true;

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
  webserver.on("/CHANGESTATUS", HTTP_POST, handleTrafficLightStatus);
  webserver.on("/TURNONANDOFF", HTTP_POST, handleTrafficLightOnAndOff);
  webserver.onNotFound(notfoundPage);
  webserver.begin();
  Serial.println("Webserver is up and running");
  return;
}

String getProperHTML(){
  String htmlCode;
  String currentColor = "red";
  if(semaforoAbierto)
     currentColor="green";

  if(!semaforoEncendido)
     currentColor="off";

  htmlCode =""
"<html>" 
"   <title>Semaforo de Reunion 2.0 </title>" 
"   <head>" 
"   <script>" 
"   function sendJSCall(mode){" 
"       modes=[" 
"         {mode:'green', bg:'green', title:'Abierto', message:'El acceso esta permitido', button:'Cerrar semaforo', disablebutton:false, onoffButton:'Apagar', light:'lime'}," 
"         {mode:'amber', bg:'orange', title:'Procesando...', message:'...', button:'Procesando...', disablebutton:true, onoffButton:'Apagar', light:'gold'}," 
"         {mode:'red', bg:'tomato', title:'Cerrado', message:'El acceso esta restringido', button:'Abrir semaforo', disablebutton:false, onoffButton:'Apagar', light:'red'}," 
"     {mode:'off', bg:'#73828a', title:'OFF', message:'El semaforo esta apagado',button:'Desactivado', disablebutton:true, onoffButton:'Encender', light:''}" 
"       ];" 
"    console.log(document.readyState);" 
"     mode=modes.filter(e => e.mode == mode)[0];" 
"     document.title = '[' + mode.title + '] Semaforo de Reunion 2.0';" 
"     document.body.style.backgroundColor = mode.bg;" 
"     document.getElementById('title').innerHTML = mode.title;" 
"     document.getElementById('message').innerHTML = mode.message;" 
"     document.getElementById('status-button').disabled = mode.disablebutton;" 
"     document.getElementById('status-button').value = mode.button;" 
"     document.getElementById('on-off-button').value = mode.onoffButton;" 
"     modes.map(elems => elems.mode).filter(e => e != 'off').forEach( light => {" 
"                     document.querySelector('#'+light).style.fill = light===mode.mode ? mode.light : 'white'; "
"     });" 
"" 
"   console.log(mode);" 
"   }" 
"   </script>" 
"" 
"   </head>" 
"   <body onload=sendJSCall('"+currentColor+"')> " 
"<!-- <body onload=sendJSCall('red')> -->" 
" <h1 id='title' style='color:white'></h1>" 
"  <div id='message' style='color:white'></div>" 
"  <div style='height:300px; position:absolute; top:10px; left:300px'>" 
"   <svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' width='178pt' height='299pt' viewBox='0 0 178 299' version='1.1'>" 
"   <g id='surface1'>" 
"   <path style='stroke:none;fill-rule:nonzero;fill:rgb(0%,0%,0%);fill-opacity:1;' d='M 73.851562 0.105469 L 104.101562 0.105469 C 119.644531 0.105469 132.246094 12.691406 132.246094 28.21875 L 132.246094 161.167969 C 132.246094 176.695312 119.644531 189.28125 104.101562 189.28125 L 73.851562 189.28125 C 58.308594 189.28125 45.710938 176.695312 45.710938 161.167969 L 45.710938 28.21875 C 45.710938 12.691406 58.308594 0.105469 73.851562 0.105469 Z M 73.851562 0.105469 '/>" 
"   <path id='red'   style=' stroke:none;fill-rule:nonzero;fill:rgb(100%,100%,100%);fill-opacity:1;' d='M 115.773438 151.488281 C 115.773438 166.25 103.792969 178.21875 89.011719 178.21875 C 74.230469 178.21875 62.25 166.25 62.25 151.488281 C 62.25 136.722656 74.230469 124.753906 89.011719 124.753906 C 103.792969 124.753906 115.773438 136.722656 115.773438 151.488281 Z M 115.773438 151.488281 '/>" 
"   <path id='amber' style=' stroke:none;fill-rule:nonzero;fill:rgb(100%,100%,100%);fill-opacity:1;' d='M 115.773438 94.695312 C 115.773438 109.457031 103.792969 121.425781 89.011719 121.425781 C 74.230469 121.425781 62.25 109.457031 62.25 94.695312 C 62.25 79.929688 74.230469 67.960938 89.011719 67.960938 C 103.792969 67.960938 115.773438 79.929688 115.773438 94.695312 Z M 115.773438 94.695312 '/>" 
"   <path id='green' style=' stroke:none;fill-rule:nonzero;fill:rgb(100%,100%,100%);fill-opacity:1;' d='M 115.773438 37.898438 C 115.773438 52.664062 103.792969 64.632812 89.011719 64.632812 C 74.230469 64.632812 62.25 52.664062 62.25 37.898438 C 62.25 23.136719 74.230469 11.167969 89.011719 11.167969 C 103.792969 11.167969 115.773438 23.136719 115.773438 37.898438 Z M 115.773438 37.898438 '/>" 
"   <path style=' stroke:none;fill-rule:nonzero;fill:rgb(0%,0%,0%);fill-opacity:1;' d='M 31.3125 175.207031 C 30.390625 174.839844 22.761719 165.171875 14.273438 153.75 C 0.121094 134.5625 -0.921875 132.714844 0.429688 129.640625 C 1.90625 126.347656 2.089844 127.355469 18.824219 127.355469 L 38.941406 127.355469 L 38.941406 151.425781 C 38.941406 171.695312 38.695312 173.316406 36.359375 174.710938 C 34.265625 175.976562 33.40625 176.039062 31.3125 175.207031 Z M 31.3125 175.207031 '/>" 
"   <path style=' stroke:none;fill-rule:nonzero;fill:rgb(0%,0%,0%);fill-opacity:1;' d='M 31.3125 118.414062 C 30.390625 118.046875 22.761719 108.378906 14.273438 96.953125 C 0.121094 77.769531 -0.921875 75.925781 0.429688 72.847656 C 1.90625 69.554688 2.089844 70.566406 18.824219 70.566406 L 38.941406 70.566406 L 38.941406 94.632812 C 38.941406 114.898438 38.695312 116.523438 36.359375 117.917969 C 34.265625 119.183594 33.40625 119.242188 31.3125 118.414062 Z M 31.3125 118.414062 '/>" 
"   <path style=' stroke:none;fill-rule:nonzero;fill:rgb(0%,0%,0%);fill-opacity:1;' d='M 31.3125 61.621094 C 30.390625 61.25 22.761719 51.585938 14.273438 40.160156 C 0.121094 20.976562 -0.921875 19.132812 0.429688 16.054688 C 1.90625 12.757812 2.089844 13.773438 18.824219 13.773438 L 38.941406 13.773438 L 38.941406 37.84375 C 38.941406 58.105469 38.695312 59.726562 36.359375 61.121094 C 34.265625 62.386719 33.40625 62.449219 31.3125 61.621094 Z M 31.3125 61.621094 '/>" 
"   <path style=' stroke:none;fill-rule:nonzero;fill:rgb(0%,0%,0%);fill-opacity:1;' d='M 146.65625 175.203125 C 147.570312 174.832031 155.230469 165.171875 163.671875 153.742188 C 177.851562 134.5625 178.910156 132.714844 177.539062 129.636719 C 176.066406 126.339844 175.851562 127.355469 159.132812 127.355469 L 139.042969 127.355469 L 139.042969 151.425781 C 139.042969 171.695312 139.296875 173.316406 141.597656 174.710938 C 143.675781 175.96875 144.585938 176.039062 146.65625 175.203125 Z M 146.65625 175.203125 '/>" 
"   <path style=' stroke:none;fill-rule:nonzero;fill:rgb(0%,0%,0%);fill-opacity:1;' d='M 146.65625 118.40625 C 147.570312 118.039062 155.230469 108.378906 163.671875 96.949219 C 177.851562 77.769531 178.910156 75.917969 177.539062 72.839844 C 176.066406 69.554688 175.851562 70.5625 159.132812 70.5625 L 139.042969 70.5625 L 139.042969 94.632812 C 139.042969 114.898438 139.296875 116.523438 141.597656 117.917969 C 143.675781 119.175781 144.585938 119.242188 146.65625 118.40625 Z M 146.65625 118.40625 '/>" 
"   <path style=' stroke:none;fill-rule:nonzero;fill:rgb(0%,0%,0%);fill-opacity:1;' d='M 146.65625 61.621094 C 147.570312 61.246094 155.230469 51.585938 163.671875 40.15625 C 177.851562 20.976562 178.910156 19.125 177.539062 16.046875 C 176.066406 12.757812 175.851562 13.765625 159.132812 13.765625 L 139.042969 13.765625 L 139.042969 37.835938 C 139.042969 58.105469 139.296875 59.726562 141.597656 61.121094 C 143.675781 62.382812 144.585938 62.449219 146.65625 61.621094 Z M 146.65625 61.621094 '/>" 
"   <path style=' stroke:none;fill-rule:nonzero;fill:rgb(0%,0%,0%);fill-opacity:1;' d='M 72.613281 196.003906 L 105.324219 196.003906 C 109.546875 196.003906 112.972656 199.421875 112.972656 203.640625 L 112.972656 291.257812 C 112.972656 295.472656 109.546875 298.894531 105.324219 298.894531 L 72.613281 298.894531 C 68.390625 298.894531 64.964844 295.472656 64.964844 291.257812 L 64.964844 203.640625 C 64.964844 199.421875 68.390625 196.003906 72.613281 196.003906 Z M 72.613281 196.003906 '/>" 
"   </g>" 
"   </svg>" 
" </div>" 
" <div style='padding-top:30px; text-align:center; width:280px'>" 
"     <form action='/CHANGESTATUS' method='POST' onsubmit='sendJSCall(\"amber\")'>" 
"       <input id='status-button' type='submit' value='' style='width:250px; height:150px'>" 
"     </form>"
"     <br>" 
"     <form action='/TURNONANDOFF' method='POST' onsubmit='sendJSCall(\"amber\")'>" 
"       <input id='on-off-button' type='submit' value='Apagar' style='width:150px; height:40px; margin-top:30px'>" 
"     </form>" 
"     </div>" 
"   </body>" 
"</html>";

  return htmlCode;
}

void rootPage() {
  Serial.println("new connection");
  webserver.send(200, "text/html", getProperHTML());
}

void handleTrafficLightStatus(){
    sendJSCall("Processing");
  if(semaforoAbierto)
  {
    Serial.println("Cerrando semaforo");
    cerrarSemaforo(AMBAR_TIME);
    semaforoAbierto=false;
    sendJSCall("Cerrado");
  }
  else
  {
    Serial.println("Abriendo semaforo");
    abrirSemaforo(AMBAR_TIME);
    semaforoAbierto=true;
    sendJSCall("Abierto");
  }

  webserver.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  webserver.send(303);
  Serial.println("Webserver relocation sent");
}

void handleTrafficLightOnAndOff(){
  sendJSCall("Processing");
  if(semaforoEncendido)
  {
    Serial.println("Apagando semaforo");
    turnTrafficLightOff();
    semaforoEncendido=false;
    sendJSCall("Apagado");
  }
  else
  {
    Serial.println("Encendiendo semaforo");
    initialiseTrafficLight();
    semaforoEncendido=true;
    sendJSCall("Encendido");
  }

  webserver.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  webserver.send(303);
  Serial.println("Webserver relocation sent");
}
// Handle 404
void notfoundPage(){
  webserver.send(404, "text/plain", "404: Not found");
}

void sendJSCall(String call)
{
  Serial.println(call);
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
  delay(DELAY_TIME/10);
  transition(yellow, green, red);
  delay(DELAY_TIME/10);
  transition(red, yellow, green);
  delay(DELAY_TIME/10);
  transition(yellow, red, green);
  delay(DELAY_TIME/10);
  transition(green, yellow, red);
  delay(DELAY_TIME/10);
  establishColor(green);
}

void setTrafficLightAsLoading(){
  establishColor(yellow);
  return;
}

void initialiseTrafficLight(){
  sequentialDemo();
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
void all(int value){
  int colors[]= {green, yellow, red};
  int i;
  for (i=0;i<3; i++)
  {
    analogWrite(colors[i], value); 
  }
}

void turnTrafficLightOff(){
  int dim=1023;
  int step=dim/10;
  for(dim = 1023; dim>0; dim=dim-step)
  {
    all(dim);
    delay(DELAY_TIME/10);
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
