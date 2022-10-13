#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <string.h>
#include <ESPAsyncWebServer.h>
#include<sstream>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

hw_timer_t * timer = NULL;
Adafruit_MPU6050 mpu;
const char *ssid =  "Farhana";     // Enter your WiFi Name
const char *pass =  "1234567890"; // Enter your WiFi Password
AsyncWebServer server(80);
int determineFace(sensors_event_t a);
int setTime(int face);
void displayTimer(int );
void initTimer();
int countTimer(int face);
void gethttpData();
int face0Time = 25;
int face1Time = 20;
int face2Time= 5;
int face3Time = 10 ;

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";
const char* PARAM_INPUT_4 = "input4";
volatile int seconds = 60 ;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
void setup(void) {
    initTimer();
  



  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");
if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.println("");
    delay(100);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");              // print ... till not connected
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address is : ");
  Serial.println(WiFi.localIP());
  server.begin();
  Serial.println("Server started");

  gethttpData();


  

}


void loop() {
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);



 int face = determineFace(a);

  displayTimer(countTimer(face));
  

  delay(500);
}

int determineFace(sensors_event_t a){
 if(a.acceleration.y < -5 ) return 1;
if(a.acceleration.y > 5) return 2;
if ((a.acceleration.y > -5 && a.acceleration.y < 5 ) && a.acceleration.z <-5) return 3;
 if(a.acceleration.y > -5 && a.acceleration.y < 5  )return 0 ;
else return 4 ;

}




int setTime(int face){
  if (face == 0 ) return face0Time;
  else if (face == 1) return face1Time;
  else if (face == 2 ) return face2Time; 
  else if (face == 3 ) return face3Time;
  
  else return 70 ;

}

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  seconds--;
  portEXIT_CRITICAL_ISR(&timerMux);
 
}
void initTimer(){
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
}

int  countTimer(int face){
 
  static int currentFace = face;
  static int currentTime = setTime(currentFace);
  if (face !=currentFace ){
    currentFace = face ;
    currentTime = setTime(currentFace);
    seconds = 60 ;
    Serial.print("face change");
    timerAlarmEnable(timer);
  }
  if(seconds <=0){
    seconds = 60 ;
    currentTime-- ;
  }
  if(currentTime == -1 ){
    timerAlarmDisable(timer);
    seconds = 0 ;
    currentTime = 0;
  }
  return currentTime;
}

void displayTimer(int minutes){
  
display.clearDisplay();
 display.setTextSize(4);
 display.setCursor(0,2);
 display.setTextColor(WHITE);
 display.print(minutes);
 display.print(":");
 display.print(seconds);
 if (millis() <= 10000) {display.setCursor(0,50);
 display.setTextSize(1);
 display.print(WiFi.localIP());}
 display.display();




}
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h2>value should not be more than 60 minutes</h2>
  <form action="/get">
    top face: <input type="number" name="input1">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    right face: <input type="number" name="input2">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    left face: <input type="number" name="input3">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    bottom face: <input type="number" name="input4">
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void gethttpData(){
  

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      face0Time = inputMessage.toInt();
      inputParam = PARAM_INPUT_1;

    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      face1Time = inputMessage.toInt();
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      face2Time = inputMessage.toInt();
    }else if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      inputParam = PARAM_INPUT_4;
      face3Time = inputMessage.toInt();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    
    int value = inputMessage.toInt();
    Serial.println(value);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
  
}