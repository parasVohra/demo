#include <SoftwareSerial.h>
//https://dweet.io/dweet/for/x99999999?ramp=2&sig=88
#define SSID        "paras"
#define PASS        "parasvohra966"

#define URL        "www.dweet.io"                                   
#define PORT        80//443 is for secure socket which seems not to be supported at this point on the ESP8266

#define thing_name  "x99999999"
#define MAX_SERVER_CONNECT_ATTEMPTS 5

#define MODULE_HW_RESET_PIN 2

SoftwareSerial dbgSerial(0, 1); // (RX, TX)

#define BUFFER_SIZE 512
char buffer[BUFFER_SIZE];

int Ramp = 22; //Ramp, Sig and freq are variables used to generate output to dweet.io for freeboard.io, etc.
float Sig = 0;
float freq = 1/8.1737;

// By default we are looking for OK\r\n
char OKrn[] = "OK\r\n";
byte wait_for_esp_response(int timeout, char* term=OKrn) {
  unsigned long t=millis();
  bool found=false;
  int i=0;
  int len=strlen(term);
  // wait for at most "timeout" milliseconds
  // or if OK\r\n is found
  while(millis()<t+timeout) {
    if(Serial.available()) {
      buffer[i++]=Serial.read();
      if(i>=len) {
        if(strncmp(buffer+i-len, term, len)==0) {
          found=true;
          break;
        }
      }
    }
  }
  buffer[i]=0;
  dbgSerial.println(buffer);
  return found;
}

byte StartModule()
{
  //reset and test if the module is ready ------------------------------------------------------------------------------------------------------
  bool module_responding =  true;
  bool connected_to_access_point = false; 
  dbgSerial.println("Starting module");
  while(!module_responding){
    //software reset
    Serial.println("AT+RST");//reset module (works with both ESP-01 and ESP-03 module)
    
    //hardware reset // faster, works with ESP-01 module on ESP8266 Arduino WiFi Sheiled rev. 3.0 from www.8266.rocks, but not with ESP-03 module 
    //digitalWrite(MODULE_HW_RESET_PIN,HIGH);
    //delay(200);
    //digitalWrite(MODULE_HW_RESET_PIN,LOW);
  
    if (wait_for_esp_response(5000, "ready")){ //watch out for the case of the r in ready - varies with ESP8266 firmware version
      dbgSerial.println("Module is responding");
      module_responding = true;
    }
    else{
      dbgSerial.println("Module not responding to reset");
      delay(1000);
    }
  }
  //-------------------------------------------------------------------------------------------------------------------------------------------
  
  Serial.println("AT+GMR");
  wait_for_esp_response(1000);
  
  Serial.println("AT+CWMODE=1");
  wait_for_esp_response(1000);
  
  Serial.println("AT+CIPMUX=0");
  wait_for_esp_response(1000);
  
  dbgSerial.println(F("Connecting to WiFi access point..."));
  String cmd = "AT+CWJAP=\"";
  cmd += SSID;
  cmd += "\",\"";
  cmd += PASS;
  cmd += "\"";
  dbgSerial.println(cmd);
  Serial.println(cmd);
  connected_to_access_point = wait_for_esp_response(9000);
  if(!connected_to_access_point){
    dbgSerial.println(F("Attempt to connect to access point failed. Restarting module."));
    return false; 
  }
  else
  {
    dbgSerial.println(F("CONNECTED TO ACCESS POINT"));
  }
  //}
  bool connected_to_dweet = false;
  int connection_attempts = 0;
  dbgSerial.println(F("Connecting to dweet..."));
  while((!connected_to_dweet)&&(connection_attempts < MAX_SERVER_CONNECT_ATTEMPTS)){
    cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += URL;//"www.dweet.io";
    cmd += "\",";
    cmd += PORT;
    Serial.println(cmd);
    //dbgSerial.println(cmd);
    connected_to_dweet = wait_for_esp_response(9000);//this needs to change - look for  something in server response that indicates valid connection
    connection_attempts += 1;
    if (!connected_to_dweet)
    {
      dbgSerial.println(F("Attempt to connect to dweet did not succeed"));
    }
    else
    {
      dbgSerial.println(F("CONNECTED TO DWEET"));
    }
  }
  return connected_to_dweet;
}


void setup()
{
  // Set up module hardware reset pin and reset
  digitalWrite(MODULE_HW_RESET_PIN,LOW);
  pinMode(MODULE_HW_RESET_PIN,OUTPUT);
    
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  //Serial.setTimeout(5000);
  dbgSerial.begin(115200);//(4800);//9600);  //consider baud limitations for software serial
  dbgSerial.println("ESP8266 dweet.io & freeboard.io Example - www.8266.rocks");

  while(!StartModule()){
    delay(1000);
    dbgSerial.println(F("***Calling StartModule Again***"));
  }
  
  //----------------------------------------------------------------------
  //delay(5000); // this is a bandaid for the delayed "linked" response from some firmware versions 
               // for the ESP-03
}

void loop()
{
  int len;
  char ramp_buf[10];
  char sig_buf[10];
  String sig_string;

  len = 80;
  
  dtostrf(Sig, 9, 3, sig_buf);//Special appraoch required for floating point value here.
                              //Have to use dtostrf instead of sprintf since sprintf doesn't
                              //support floats on Arduino. 
  sig_string = String(sig_buf); //convert from character buffer array to string so
                                //white space can be cleared with .trim().
  sig_string.trim();
  
  len += sig_string.length();
  len += sprintf (ramp_buf, "%d", Ramp);
  len += (sizeof(thing_name) - 1);//sizeof will return 11 for a 10 character thing name because of null terminator
                                  //so subtract 1

  //form and send the HTTP POST message
  Serial.print("AT+CIPSEND=");
  Serial.println(len);
  if (!wait_for_esp_response(9000,"> ")){
    Serial.println("AT+CIPCLOSE");
    dbgSerial.println("send timeout - resetting wifi module");
    delay(1000);
    StartModule();
  }
  
  Serial.print(F("POST /dweet/for/"));  
  //Serial.print(F("GET /dweet/for/"));// both POST and GET work
  Serial.print(thing_name);
  Serial.print(F("?Ramp="));
  Serial.print(ramp_buf);
  Serial.print(F("&Sig="));
  Serial.print(sig_string);
  Serial.print(F(" HTTP/1.1\r\n"));
  Serial.print(F("Host: dweet.io\r\n"));
  //Serial.print(F("Connection: close\r\n"));//in some cases connection needs to be closed after POST
  Serial.print(F("Connection: keep-alive\r\n"));//in this case we want to keep the connection alive
  Serial.print(F("\r\n"));
  
  wait_for_esp_response(9000,"}}}\r\nOK");
  
  delay(1000);
 
  Ramp+=17;
  if(Ramp>99){
    Ramp=10;
  }
  Sig = sin(2 * PI * freq * millis() / 1000);
  
}


