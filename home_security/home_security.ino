#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//Sensor variables
int Status = 12; //d6
int sensor = 13; //d7
int resenceEventCount = 0;
const int maxEventCount = 5;

//wifi variables
const char* ssid = "<Wifi SSID>";
const char* password = "<Wifi Password>";
const int WiFiConnectionOverCount = 10;
const int WiFiConnectionRetryCount = 4;
unsigned long oldTime = 0;
unsigned long delta = 0;
// Time delta to check if nodeMCU is connected to wifi else retry to connect
//const unsigned long wificheckdelta = 3600000; // 1 hr
const unsigned long wificheckdelta = 21600000; // 6 hr
//const unsigned long wificheckdelta = 30000; //1/2 min

//ifttt variables
//const char* serverName = "http://maker.ifttt.com/trigger/<REPLACE_WITH_YOUR_EVENT>/with/key/<REPLACE_WITH_YOUR_API_KEY>";


//basic setup of sensor, connection to wifi
void setup() {
  pinMode(sensor, INPUT); // declare sensor as input
  pinMode(Status, OUTPUT);  // declare LED as output
  Serial.begin(9600);
  //LED blink to show that circuit is working and getting ready
  delay(3000);
  digitalWrite (Status, HIGH);
  delay(5000);
  digitalWrite (Status, LOW);
  // Function to connect to the wifi
  bool wifiConnection = ConnectToWiFi();
  int ReconectionTried = 0;
  while ( !wifiConnection && ReconectionTried != WiFiConnectionRetryCount) {
    ReconectionTried += 1;
    Serial.println("Retrying for wifi connection.");
    wifiConnection = ConnectToWiFi();
  }
  //LED blink rapidly to show that Sensor is arming and you can clear the area to avoid false detection
  armingSignal(750, 40);
}



//function to send alert using ifttt webhook
void send_alert(String message) {
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String httpRequestData = "value1=" + message;
  int httpResponseCode = http.POST(httpRequestData);
  Serial.print("send alert response = ");
  Serial.println(httpResponseCode);
  String response = http.getString();
  Serial.println(response);
  http.end();
}

//function to connect to Wifi
bool ConnectToWiFi() {
  bool returnVal = false;
  if (WiFi.status() != WL_CONNECTED) {
    //wifi connection code
    Serial.println();
    Serial.print("Connecting Wifi to ");
    Serial.println( ssid );
    WiFi.begin(ssid, password);
    Serial.println();
    Serial.print("Connecting");
    int connectionTried = 0;
    // print . untill wifi is connected. Check https://www.arduino.cc/en/Reference/WiFiStatus
    while ( WiFi.status() != WL_CONNECTED && connectionTried != WiFiConnectionOverCount) {
      //      while ( WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
      connectionTried += 1;
    }
    wl_status_t wifiStatus = WiFi.status();
    Serial.print("Wifi status:");
    Serial.println(wifiStatus);
    if (WiFi.status() == WL_CONNECTED) {
      returnVal = true;
      Serial.println("Wifi is connected");
      delay(2000);
      armingSignal(150, 20);
      send_alert("NodeMCU is connected to wifi");
    }
    else {
      Serial.println("Wifi is disconnected");
      WiFi.disconnect();
    }
  }
  else {
    Serial.println("wifi is already connected");
    returnVal = true;
  }
  return returnVal;
}

//calculate time delta for next wifi check
unsigned long CalculateDeltaTime() {
  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - oldTime;
  //  oldTime = currentTime;
  Serial.print("deltatime:");
  Serial.println(deltaTime);
  return deltaTime;
}

//blink led for 30 second
void armingSignal(int delayTime, int maxCount) {
  Serial.println("inside arming signal function");
  bool toggle = true;
  int count =  0;
  while (count < maxCount) {
    count += 1;
    delay(delayTime);
    if (toggle) {
      Serial.println("High");
      digitalWrite (Status, HIGH);
    }
    else {
      Serial.println("LOW");
      digitalWrite (Status, LOW);
    }
    toggle = !toggle;
  }
  digitalWrite (Status, LOW);
}

//actual sensor run and alert mechanisam call
void loop() {
  //sensor check
  Serial.print("resenceEventCount = ");
  Serial.println(resenceEventCount);

  //read sensor value
  long state = digitalRead(sensor);
  delay(500);
  if (state == HIGH) {
    digitalWrite (Status, HIGH);
    Serial.println("Motion detected!");
    if (resenceEventCount == 0) {
      send_alert("Motion detected at home for first times");
    }
    else {
      send_alert("Motion detected at home for " + String(resenceEventCount + 1) + " times");
    }
    //Once PIR sensor detect motion it will remain high for few second, Compensating that with delay.
    delay(5000);
    resenceEventCount += 1;
    //if recent event excide then maxEventCount add 15 mins delay
    if (resenceEventCount == maxEventCount) {
      resenceEventCount = 0;
      digitalWrite (Status, LOW);
      send_alert("Motion detection halted for 5 mins");
      // delay(30000);
      delay(300000);
    }
  }
  else {
    digitalWrite (Status, LOW);
    Serial.println("Motion absent!");
  }
  //wifi connection check
  delta = 0;
  delta = CalculateDeltaTime();
  if (delta > wificheckdelta) {
    send_alert("trying to reconnect to wifi after 6 hr");
    bool loopwifiConnection = ConnectToWiFi();
    oldTime = millis();
    delta = 0;
    int loopReconectionTried = 0;
    while ( !loopwifiConnection && loopReconectionTried != WiFiConnectionRetryCount) {
      loopReconectionTried += 1;
      Serial.println("Retrying for wifi connection in loop.");
      loopwifiConnection = ConnectToWiFi();
    }
  }
}
