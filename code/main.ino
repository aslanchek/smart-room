#include <DHT.h>
#include <TroykaMQ.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
 
// Enter your Write API key from ThingSpeak
String apiKey = ""; 

// Enter your wifi ssid and wpa2 key
const char *ssid =  "";
const char *pass =  "";

const char* server = "api.thingspeak.com";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define DHTPIN 2       // Define the digital pin where the DHT sensor is connected
#define ZALOOPTIME 10
#define DHTTYPE DHT22  // DHT11 or DHT22, change this according to your sensor type
#define PIN_MQ135  A0
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(DHTPIN, DHTTYPE);
MQ135 mq135(PIN_MQ135);
WiFiClient client;


void setup() {
  pinMode(14, OUTPUT);
  Serial.begin(9600);  // initialize serial port
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally




  Serial.println("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, pass);
 
  while (WiFi.status() != WL_CONNECTED) 
     {
          delay(500);
          Serial.print(".");
     }
      Serial.println("");
      Serial.println("WiFi connected");



  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.

  display.clearDisplay();
  display.display();
  delay(2000);

  mq135.calibrate();
  // выводим сопротивление датчика в чистом воздухе (Ro) в serial-порт
  digitalWrite(14, HIGH);
  Serial.print("Ro = ");
  Serial.println(mq135.getRo());
  dht.begin();  // Initialize the DHT sensor
}
void loop() {
  // Read temperature and humidity from the DHT sensor
  // In the parameter pass true if temperature is required in fahrenheit, pass false if temperature is required in celsius
  float temperature = 0; 
  float humidity = 0;
  float cogas = 0;

  for(int i = 0; i < ZALOOPTIME; i++){ 
  float temperature1 = dht.readTemperature(false);
  float humidity1 = dht.readHumidity();
  float cogas1 = mq135.readCO2();
  delay(1000);
  temperature += temperature1; 
  humidity += humidity1;
  cogas += cogas1;
  }

  temperature /= ZALOOPTIME; 
  humidity /= ZALOOPTIME;
  cogas /= ZALOOPTIME;


  // Check if the sensor reading is valid (non-NaN)

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.print("CO2 PPM: ");
  display.print(cogas);
  

  Serial.print("Ratio: ");
  Serial.print(cogas);
  // выводим значения газов в ppm
  Serial.print("\tCO2: ");
  Serial.print(cogas);
  Serial.println(" ppm");

  
  if (!isnan(temperature) && !isnan(humidity)) {
    display.setCursor(10, 12);
    display.print("T: ");
    display.printf("%.1f ", temperature);
    display.write(0xF8);
    display.print("C");
    display.setCursor(10, 24);
    display.print("H: ");
    display.printf("%.1f ",humidity);
    display.write(0x25);
    
    

    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print("  °C, H: ");
    Serial.print(humidity);
    Serial.println("%");
  }
  else
  {  
    digitalWrite(14, LOW);
    delay(1000);
    Serial.println("Failed to read from DHT sensor!");
    digitalWrite(14, HIGH);
    
  }

  display.display();


  if (client.connect(server,80))   //   "184.106.153.149" or api.thingspeak.com
                      {  
                            
                             String postStr = apiKey;
                             postStr +="&field1=";
                             postStr += String(temperature);
                             postStr +="&field2=";
                             postStr += String(humidity);
                             postStr +="&field3=";
                             postStr += String(cogas);
                             postStr += "\r\n\r\n";
 
                             client.print("POST /update HTTP/1.1\n");
                             client.print("Host: api.thingspeak.com\n");
                             client.print("Connection: close\n");
                             client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
                             client.print("Content-Type: application/x-www-form-urlencoded\n");
                             client.print("Content-Length: ");
                             client.print(postStr.length());
                             client.print("\n\n");
                             client.print(postStr);
                             Serial.println("%. Send to Thingspeak.");
                        }
          client.stop();
 
          Serial.println("Waiting...");
  delay(10000);  // Delay for 2 seconds before the next reading
  
}
