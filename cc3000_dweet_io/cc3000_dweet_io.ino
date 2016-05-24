
// Libraries
#include <Adafruit_CC3000.h>
#include <SPI.h>
#include "DHT.h"
#include <avr/wdt.h>

// Define CC3000 chip pins
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

// DHT sensor
#define DHTPIN 7
#define DHTTYPE DHT11

// Create CC3000 instances
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2);
                                         
// DHT instance
DHT dht(DHTPIN, DHTTYPE);

// WLAN parameters
#define WLAN_SSID       "paras"
#define WLAN_PASS       "parasvohra966"
#define WLAN_SECURITY   WLAN_SEC_WPA2

// Dweet parameters
#define thing_name  "x123123x"

// Variables to be sent
int temperature;
int humidity;
int gas = A0;
int gasValue = 0;
int methaneGas = A1;
int methaneGasValue = 0;
//int light;

uint32_t ip;

void setup(void)
{
  // Initialize
  Serial.begin(115200);
  
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
 
  // Connect to WiFi network
  Serial.print(F("Connecting to WiFi network ..."));
  cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
  Serial.println(F("done!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }  
}

void loop(void)
{ 
  
  // Measure from DHT
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  temperature = (int)t;
  humidity = (int)h;

   gasValue = analogRead(gas);

   methaneGasValue = analogRead(methaneGas);

  // Measure light level
  //float sensor_reading = analogRead(A0);
  //light = (int)(sensor_reading/1024*100);
  //Serial.println(F("Measurements done"));
  
  // Start watchdog 
  wdt_enable(WDTO_8S); 
  
  // Get IP
   uint32_t ip = 877089256;
  Serial.print(F("www.dweet.io -> "));
  //while  (ip == 0)  {
    //if  (!  cc3000.getHostByName("www.dweet.io", &ip))  {
      //Serial.println(F("Couldn't resolve!"));
    //}
    //delay(500);
  //}  
  cc3000.printIPdotsRev(ip);
  Serial.println(F(""));
  
  // Reset watchdog
     wdt_reset();
  
  // Check connection to WiFi
  Serial.print(F("Checking WiFi connection ..."));
  if(!cc3000.checkConnected()){while(1){}}
  Serial.println(F("done."));
  wdt_reset();
  
  // Send request
  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
  if (client.connected()) {
    Serial.print(F("Sending request... "));
    
    client.fastrprint(F("GET /dweet/for/"));
    client.print(thing_name);
    client.fastrprint(F("?temperature="));
    client.print(temperature);
    client.fastrprint(F("&humidity="));
    client.print(humidity);
    client.fastrprint(F("&gasValue="));
    client.print(gasValue);
    
    client.fastrprint(F("&methaneGasValue="));
    client.print(methaneGasValue);



    
    //client.fastrprint(F("&light="));
    //client.print(light);
    client.fastrprintln(F(" HTTP/1.1"));
    
    client.fastrprintln(F("Host: dweet.io"));
    client.fastrprintln(F("Connection: close"));
    client.fastrprintln(F(""));
    
    Serial.println(F("done."));
  } else {
    Serial.println(F("Connection failed"));    
    return;
  }
  
  // Reset watchdog
  wdt_reset();
  
  Serial.println(F("Reading answer..."));
  while (client.connected()) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
  }
  Serial.println(F(""));
  
  // Reset watchdog
  wdt_reset();
   
  // Close connection and disconnect
  client.close();
  Serial.println(F("Closing connection"));
  Serial.println(F(""));
  
  // Reset watchdog & disable
  wdt_reset();
  wdt_disable();
 
  // Wait 60 seconds until next update
  wait(60000);
    
}

// Wait for a given time 
void wait(int total_delay) {
  
  int number_steps = (int)(total_delay/5000);
  wdt_enable(WDTO_8S);
  for (int i = 0; i < number_steps; i++){
    //Serial.println(F("Waiting for 5 seconds ..."));
    delay(5000);
    wdt_reset();
  }
  wdt_disable();
}
