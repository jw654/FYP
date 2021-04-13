#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#include <math.h>
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


const char* ssid = "VM8559363";
const char* password = "Martial!11";

AsyncWebServer server(80);
AsyncEventSource events("/events");
Adafruit_BME680 bme; 
float temperature;
float humidity;
float pressure;
float gas_resistance;
float air_quality;
unsigned long start_time = 0;  
unsigned long time_delay = 3000;

float humidity_score, gas_score;
float gas_reference = 250000;
float humidity_reference = 40;
int   getgasreference_count = 0;

float ideal_humidity = 50;
float optimal_gas_resistance = 200;
float humidity_weighting = 0.25; // humidty is 25% of air quality score
float gas_weighting = 0.75; // gas is 75% of air quality score


double gas_lambda = (log(500) - log(28)) / 100;



void getBME680Readings(){
  
  unsigned long end_time = bme.beginReading();
  if (end_time == 0) {
    Serial.println(F("Couldn't start reading :("));
    return;
  }
  if (!bme.endReading()) {
    Serial.println(F("Couldn't complete readings :("));
    return;
  }
 
 
  humidity = bme.humidity;
  temperature = bme.temperature;
  gas_resistance = bme.gas_resistance / 1000.0;
  pressure = bme.pressure / 100.0;


  
  if (humidity <= ideal_humidity){
      humidity_score = -(500/ideal_humidity)*humidity + 500;
    }
  else {
     humidity_score = (500/(500 - ideal_humidity))*(humidity - ideal_humidity);
    }

  
  gas_score = 500 * exp(-gas_resistance * gas_lambda);


  air_quality = humidity_weighting * humidity_score + gas_weighting * gas_score;
}

String processor(const String& var){
  getBME680Readings();
  if(var == "TEMPERATURE"){
    return String(temperature);
  }
  else if(var == "HUMIDITY"){
    return String(humidity);
  }
  else if(var == "PRESSURE"){
    return String(pressure);
  }
 else if(var == "GAS"){
    return String(gas_resistance);
  }
  else if(var == "Air Quality"){
    return String(air_quality);
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Real Time Air Quality</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    .card.pressure { color: #3fca6b; }
    .card.gas { color: #d62246; }
    .card.aq { color: #90ee90; margin-bottom: 20px; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>Real Time Air Quality</h3>
  </div>
  <div class="content">
   <div class = cards>
  <div class="card aq">
            <h4><i class="fas fa-seedling"></i> Air Quality</h4><p><span class="reading"><span id="air">%Air Quality%</span></p>
        </div>
    </div>
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p><span class="reading"><span id="temp">%TEMPERATURE%</span> &deg;C</span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> HUMIDITY</h4><p><span class="reading"><span id="hum">%HUMIDITY%</span> &percnt;</span></p>
      </div>
      <div class="card pressure">
        <h4><i class="fas fa-angle-double-down"></i> PRESSURE</h4><p><span class="reading"><span id="pres">%PRESSURE%</span> hPa</span></p>
      </div>
      <div class="card gas">
        <h4><i class="fas fa-wind"></i> GAS</h4><p><span class="reading"><span id="gas">%GAS%</span> K&ohm;</span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 var last_quality = new String("Initial LQ");
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);


 source.addEventListener('air_quality', function(e) {
  console.log("air_quality", e.data);
  document.getElementById("air").innerHTML = e.data;
 }, false);
 
 source.addEventListener('temperature', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp").innerHTML = e.data;
 }, false);
 
 source.addEventListener('humidity', function(e) {
  console.log("humidity", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);
 
 source.addEventListener('pressure', function(e) {
  console.log("pressure", e.data);
  document.getElementById("pres").innerHTML = e.data;
 }, false);
 
 source.addEventListener('gas', function(e) {
  console.log("gas", e.data);
  document.getElementById("gas").innerHTML = e.data;
 }, false);

 source.addEventListener('air_quality_category', function(e) {
  console.log("air_quality_category", e.data);
  if (e.data == "Hazardous air quality" && (e.data != last_quality)) {
      alert("Warning: Air quality is hazardous");
    }
  last_quality = e.data;
 }, false);
}
</script>
</body>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(10000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Init BME680 sensor
  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1);
  }
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_16X);
  bme.setHumidityOversampling(BME680_OS_4X);
  bme.setPressureOversampling(BME680_OS_8X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();


  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
}



void loop() {
  if ((millis() - start_time) > time_delay) {
    
    getBME680Readings();
    Serial.printf("Temperature = %.2f ºC \n", temperature);
    Serial.printf("Humidity = %.2f % \n", humidity);
    Serial.printf("Pressure = %.2f hPa \n", pressure);
    Serial.printf("Gas Resistance = %.2f KOhm \n", gas_resistance);
    Serial.printf("Air quality = %.2f \n", air_quality);
    Serial.printf("Hum Score = %.2f \n", humidity_score);
    Serial.printf("Gas Score = %.2f \n", gas_score);
    Serial.println();


    String air_quality_category;


    //oled display 
      display.clearDisplay();
  
  // display air quality
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Air quality ");
   display.print('\n');
  if (air_quality>=301) {
    display.setTextSize(1);
    air_quality_category = "Hazardous air quality";
    display.print(air_quality_category);
  }
  else if (air_quality>=201 && air_quality<301 ) {
    display.setTextSize(1);
    air_quality_category = "Very unhealthy air quality";
    display.print(air_quality_category);
  }
  else if (air_quality>=151 && air_quality<201 ) {
    display.setTextSize(1);
    air_quality_category = "Unhealthy air quality";
    display.print(air_quality_category);
  }
  else if (air_quality>=101 && air_quality<151 ) {
    display.setTextSize(1);
    air_quality_category = "Unhealthy for sensitive groups air quality";
    display.print(air_quality_category);
  }
    else if (air_quality>=51 && air_quality<101 ) {
    display.setTextSize(1);
    air_quality_category = "Moderate air quality";
    display.print(air_quality_category);
  }
  else {
      display.setTextSize(1);
      air_quality_category = "Good air quality";
      display.print(air_quality_category);
  }
 display.print('\n');
  display.print(air_quality);
  display.setTextSize(1);
  display.setCursor(0,10);
  display.print(" ");
  display.print('\n');
  display.setTextSize(1);
  
  
  display.display();
    // Send Events to the Web Server with the Sensor Readings
    events.send("ping",NULL,millis());
    events.send(String(temperature).c_str(),"temperature",millis());
    events.send(String(humidity).c_str(),"humidity",millis());
    events.send(String(pressure).c_str(),"pressure",millis());
    events.send(String(gas_resistance).c_str(),"gas",millis());
    events.send(String(air_quality).c_str(),"air_quality",millis());
    events.send(String(air_quality_category).c_str(),"air_quality_category",millis());
    
    start_time = millis();
  }
}
