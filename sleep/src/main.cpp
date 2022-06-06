#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MLX90614.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip

#include <WiFi.h>
#include <HttpClient.h>
#include <iostream>
#include <WString.h>

//  Variables
#define PulseSensorPurplePin 39      // Pulse Sensor PURPLE WIRE connected to ANALOG PIN 0
#define LED_PIN 13   //  The on-board Arduion LED
#define TFT_GREY 0x5AEB // New colour
#define BUTTON1PIN 35
// #define BUTTON2PIN 0

// WI-FI setup
char ssid[] = "UCInet Mobile Access";    // your network SSID (name) 
char pass[] = ""; // your network password (use for WPA, or use as key for WEP) 

// Name of the server we want to connect to
const char kHostname[] = "34.221.193.205";
// Path to download (this is the bit after the hostname in the URL
// that you want to download
const char kPath[] = "/?var=120";

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

int user_id = 1;
String session_id = "3";


// sensor setup
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
TFT_eSPI tft = TFT_eSPI();
int Signal;                // holds the incoming raw data. Signal value can range from 0-1024
int Threshold = 3000;            // Determine which Signal to "count as a beat", and which to ingore.
char currentMode;
unsigned long startSleepTime = 0;
unsigned long endSleepTime = 0;
unsigned int counter = 1;
std::vector<int> signalMeasurements;
int totalSignal = 0;
unsigned int totalBeats = 0;
unsigned long pulseStartTime;
unsigned long pulseEndTime;
double oldAvgSignal;
std::vector<unsigned long> timeMeasurements;
double objectTemp;
double ambientTemp;
unsigned int bpm;
bool is_sleeping = true;


void IRAM_ATTR button1Press()
{
  if (currentMode == 's')
  {
    currentMode = 't';
  }
  else if (currentMode == 't')
  {
    currentMode = 'p';
  }
  else if (currentMode == 'p')
  {
    currentMode = 's';
  }
}

void button2Press()
{
  // Serial.println("B2 pressed");
  // is_sleeping = !is_sleeping;
  // if (is_sleeping)
  // {
  //   session_id = "";
  //   WiFiClient c;
  //   HttpClient http(c);
  //   String path = "/new_session?user_id=" + String(user_id) + "&time=" + String(millis());
  //   int err =0;
  //   err = http.get(kHostname, 5000, path.c_str());

  //   if (err == 0)
  //   {
  //     Serial.println("startedRequest ok");

  //     err = http.responseStatusCode();
  //     if (err >= 0)
  //     {
  //       Serial.print("Got status code: ");
  //       Serial.println(err);

  //       // Usually you'd check that the response code is 200 or a
  //       // similar "success" code (200-299) before carrying on,
  //       // but we'll print out whatever response we get

  //       err = http.skipResponseHeaders();
  //       if (err >= 0)
  //       {
  //         int bodyLen = http.contentLength();
  //         Serial.print("Content length is: ");
  //         Serial.println(bodyLen);
  //         Serial.println();
  //         Serial.println("Body returned follows:");
        
  //         // Now we've got to the body, so we can print it out
  //         unsigned long timeoutStart = millis();
  //         char c;
  //         // Whilst we haven't timed out & haven't reached the end of the body
  //         while ( (http.connected() || http.available()) &&
  //               ((millis() - timeoutStart) < kNetworkTimeout) )
  //         {
  //             if (http.available())
  //             {
  //                 c = http.read();
  //                 // Print out this character
  //                 Serial.print(c);
  //                 session_id = session_id + String(c);
  //                 Serial.println(session_id);
                
  //                 bodyLen--;
  //                 // We read something, reset the timeout counter
  //                 timeoutStart = millis();
  //             }
  //             else
  //             {
  //                 // We haven't got any data, so let's pause to allow some to
  //                 // arrive
  //                 delay(kNetworkDelay);
  //             }
  //         }
  //       }
  //       else
  //       {
  //         Serial.print("Failed to skip response headers: ");
  //         Serial.println(err);
  //       }
  //     }
  //     else
  //     {    
  //       Serial.print("Getting response failed: ");
  //       Serial.println(err);
  //     }
  //   }
  //   else
  //   {
  //     Serial.print("Connect failed: ");
  //     Serial.println(err);
  //   }
  //   http.stop();
  // }
  
}

// The SetUp Function:
void setup() {
  pinMode(LED_PIN, OUTPUT);         // pin that will blink to your heartbeat!
  Serial.begin(9600);         // Set's up Serial Communication at certain speed.
  currentMode = 's';
  mlx.begin();
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  pinMode(BUTTON1PIN, INPUT);
  // pinMode(BUTTON2PIN, INPUT);
  attachInterrupt(BUTTON1PIN, button1Press, RISING);
  // attachInterrupt(BUTTON2PIN, button2Press, RISING);
  pulseStartTime = millis();

  // connect to Wi-fi

  // We start by connecting to a WiFi network
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());

}

// The Main Loop Function
void loop() {
  tft.setCursor(65, 60, 2);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);
  
  Signal = analogRead(PulseSensorPurplePin);  // Read the PulseSensor's value.
  //Serial.println(Signal);
  if (signalMeasurements.size() != 10)
  {
    signalMeasurements.push_back(Signal);
  }
  else
  {
    totalSignal = 0;
    signalMeasurements.erase(signalMeasurements.begin());
    signalMeasurements.push_back(Signal);
    for (int i = 0; i<signalMeasurements.size();i++)
    {
      totalSignal += signalMeasurements[i];
    }
    double avgSignal = totalSignal/10;
    //tft.println(avgSignal);
    if (avgSignal > Threshold && oldAvgSignal < Threshold)
    {
      digitalWrite(LED_PIN, HIGH);
      //totalBeats++;
      if (timeMeasurements.size() != 10)
      {
        timeMeasurements.push_back(millis());
      }
      else
      {
        timeMeasurements.erase(timeMeasurements.begin());
        timeMeasurements.push_back(millis());
        bpm =9*1000*60/(timeMeasurements[9]-timeMeasurements[0]);
      }
    }
    else
    {
      digitalWrite(LED_PIN, LOW);
    }
    oldAvgSignal = avgSignal;
  }
  ambientTemp = mlx.readAmbientTempC();
  objectTemp = mlx.readObjectTempC();
  /*pulseEndTime = millis();
  if ((pulseEndTime-pulseStartTime) >= 60000)
  {
    Serial.print("Current beats per minute: ");
    Serial.println(totalBeats);
    totalBeats = 0;
    pulseStartTime = millis();
  }*/


  if (currentMode == 's')
  {
    tft.println("Ready to Sleep");
  }
  else if (currentMode == 't')
  {
    tft.print("Object = "); tft.println(objectTemp);
    tft.setCursor(65,75,2);
    tft.print("Ambient = "); tft.println(ambientTemp);
  }
  else if (currentMode == 'p')
  {
    tft.println(bpm);
  }

  if (counter % 20 == 0)
  {
      tft.fillScreen(TFT_BLACK);
  }
  if (counter % 50 == 0) {
    if (is_sleeping) {
      WiFiClient c;
      HttpClient http(c);
      String path = "/?temp=" + String(objectTemp) + "&heart_rate=" + String(bpm) + "&user_id=" + String(user_id) + "&session_id=" + session_id + "&time=" + String(millis());
      int err  =0;
      err = http.get(kHostname, 5000, path.c_str());
    }
  }
  counter++;
  delay(20);
}