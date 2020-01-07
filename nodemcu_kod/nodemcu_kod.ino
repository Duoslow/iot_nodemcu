/*********
Ugur Yavas
NODEMCU(esp8266)
IOT Ödevi
*********/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

//wifi agi bilgileri
const char* ssid     = "UgurIOT";
const char* password = "uguriot123";

//dht sıcaklık sensoru ayarları
#define DHTPIN 5   
#define DHTTYPE    DHT11
DHT dht(DHTPIN, DHTTYPE);
float t = 0.0;
float h = 0.0;
unsigned long previousMillis = 0;
const long interval = 10000;

//80 portunda webser oluşturduk
AsyncWebServer server(80);


// anasayfamızı kodladık
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Oda Sıcaklığı</h2>
  <p>
    <span class="dht-labels">Sıcaklık</span>
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <span class="dht-labels">Nem Oranı</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
  <div>
  <h2>RGB Led Kontrolü</h2>
    <div>
      <span>Kırmızı:</span><input id="r" type="range" value="0" min="0" max="1023" step="16" onchange="sendRGB();" />
    </div>
    <div>
      <span>Yeşil:</span><input id="g" type="range" value="0" min="0" max="1023" step="16" onchange="sendRGB();" />
    </div>
    <div>
      <span>Mavi:</span><input id="b" type="range" value="0" min="0" max="1023" step="16" onchange="sendRGB();" />
    </div>
    <div>
      <input type="checkbox" id="priz" disabled/><span>Priz Kontrol </span><input type="button" onclick="sendBB();" value="AÇ/KAPA" />
    </div>
  </div>
</body>

<script>

var role = false;
var updateSpeed = 5; //Saniye
setInterval(function(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhr.open("GET", "/temperature", true);
  xhr.send();
}, updateSpeed*100);

setInterval(function(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhr.open("GET", "/humidity", true);
  xhr.send();
}, updateSpeed*100);

function sendRGB(){ //RGB LED KONTROLÜ
  var xhr = new XMLHttpRequest();
  var r = document.getElementById('r').value;
  var g = document.getElementById('g').value;
  var b = document.getElementById('b').value;
  xhr.open("GET", "/rgb?r=" + r + "&g=" + g + "&b=" + b, true);
  xhr.send();
}

function sendBB(){  // RÖLE KONTROLÜ
  var xhr = new XMLHttpRequest();
  if (role) {
    xhr.open("GET", "/button?state=" + role.toString() , true);
    role = false;
  } else {
    xhr.open("GET", "/button?state=" + role.toString() , true);
    role = true;
  }
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if (this.responseText == "true") {
        document.getElementById("priz").checked = true;
      } else {
        document.getElementById("priz").checked = false;
      }
    }
  };
  xhr.send();
}
</script>
</html>
)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  return String();
}


void setup(){
  // diğer ayarlar
  Serial.begin(115200);
  dht.begin();
  pinMode(4, OUTPUT);  
  
  //wifi ayarları
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.println(WiFi.localIP());

  // sitedeki içeriklerin yerlerini ayarladık
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });
  server.on("/button", HTTP_GET, [](AsyncWebServerRequest *request){ //RÖLE KONTROLU
    int params = request->params();
    String butState;
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      butState = (p->value().c_str());
    }
    if(butState == "true") {
      // role ac
      digitalWrite(4,HIGH);
      request->send_P(200, "text/plain", String("true").c_str());
    } else {
      // role kapa
      digitalWrite(4,LOW);
      request->send_P(200, "text/plain", String("false").c_str());
    }
  });
  server.on("/rgb", HTTP_GET, [](AsyncWebServerRequest *request){ //RGB LED KONTROLU
   int params = request->params();
   int rgbData[3];
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      String rgb = (p->value().c_str());
      rgbData[i] = rgb.toInt();
      Serial.println(rgb);
      //  Serial.printf("%s\n",p->value().c_str());
      
    }
    
    Serial.println(rgbData[0]);
    Serial.println(rgbData[1]);
    Serial.println(rgbData[2]);
    analogWrite(12, rgbData[0]);
    analogWrite(15, rgbData[1]);
    analogWrite(14, rgbData[2]);
    request->send_P(200, "text/plain", String("a").c_str());
  });

  // Start server
  server.begin();
}

void loop(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    float newT = dht.readTemperature();
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.println(t);
    }

    float newH = dht.readHumidity();
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.println(h);
    }
  }
}
