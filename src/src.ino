#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <string.h>

#include <mpu6050_esp32.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
char network[] = "breadb";
char password[] = "";
uint8_t scanning = 0;
uint8_t channel = 1; //network channel on 2.4 GHz
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; //6 byte MAC address of AP you're targeting.

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 2000; //periodicity of getting a number fact.
const int BUTTON_TIMEOUT = 1000; //button timeout in milliseconds
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response


const int BOTTOMBUTTON = 45; //used to teach/store
const int CENTERBUTTON = 39; //used to compare/get
const int TOPBUTTON = 38; //restart

const int UNPRESSED = 0;
const int PRESSED = 1;

int topButtonState = UNPRESSED;
int centerButtonState = UNPRESSED;
int bottomButtonState = UNPRESSED;

const int IDLE_STATE = 0;
const int PRE_TEACH_STATE = 2;
const int PRE_MEASURE_STATE = 3;
const int RECORD_TEACH_STATE = 4;
const int GET_STATE = 5;
const int POST_STATE = 6;
const int RECORD_MEASURE_STATE = 7;
const int LETTER_STATE = 8;
const int LAST_STATE1 = 9;
const int LAST_STATE2 = 1;

int systemState = IDLE_STATE;

uint8_t state=0; //state variable
const float ZOOM = 9.81; //for display (converts readings into m/s^2)...used for visualizing only

const uint8_t LOOP_PERIOD = 1000; //milliseconds
uint32_t primary_timer = 0;
float x, y, z; //variables for grabbing x,y,and z values
char z_arr[5000];
char curr_z[100];
char letter[2];

const int long_press_duration = 1500;
int button_start_time;


MPU6050 imu; //imu object called, appropriately, imu


void setup() {
  delay(50); //pause to make sure comms get set up

  // put your setup code here, to run once:
  tft.init();  //init screen
  tft.setRotation(1); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_WHITE, TFT_BLACK); //set color of font to green foreground, black background
  Serial.begin(115200); //begin serial comms

    if (scanning){
      int n = WiFi.scanNetworks();
      Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
        uint8_t* cc = WiFi.BSSID(i);
        for (int k = 0; k < 6; k++) {
          Serial.print(*cc, HEX);
          if (k != 5) Serial.print(":");
          cc++;
        }
        Serial.println("");
      }
    }
  }
  delay(100); //wait a bit (100 ms)

    //if using regular connection use line below:
  WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count<6) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n",WiFi.localIP()[3],WiFi.localIP()[2],
                                            WiFi.localIP()[1],WiFi.localIP()[0], 
                                          WiFi.macAddress().c_str() ,WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }

  pinMode(BOTTOMBUTTON, INPUT_PULLUP); //set input pin as an input!
  pinMode(CENTERBUTTON, INPUT_PULLUP);
  pinMode(TOPBUTTON, INPUT_PULLUP);

  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }

}

void loop() {
  uint8_t bottomButton = digitalRead(BOTTOMBUTTON);
  uint8_t centerButton = digitalRead(CENTERBUTTON);
  uint8_t topButton = digitalRead(TOPBUTTON);

  if (systemState==IDLE_STATE) {

      tft.setCursor(0,0,2);
      tft.println("Welcome to your custom vowel recognition \nsystem! Press the bottom button to teach a new \nvowel or the center \nbutton to compare a vowel gesture.");


      if (bottomButtonState==UNPRESSED && bottomButton==0) {
        bottomButtonState = PRESSED;
        Serial.println("BOTTOM BUTTON HAS BEEN PRESSED"); 
      }      
      else if (bottomButtonState==PRESSED && bottomButton==1) {
        bottomButtonState = UNPRESSED;
        systemState = PRE_TEACH_STATE;
        tft.fillScreen(TFT_BLACK); //fill background
      }

      if (centerButtonState==UNPRESSED && centerButton==0) {
        centerButtonState = PRESSED;
        Serial.println("CENTER BUTTON HAS BEEN PRESSED"); 
      }      
      else if (centerButtonState==PRESSED && centerButton==1) {
        centerButtonState = UNPRESSED;
        systemState = PRE_MEASURE_STATE;
        tft.fillScreen(TFT_BLACK); //fill background

      }

  }

    if (systemState==PRE_MEASURE_STATE) {

      tft.setCursor(0,0,2);
      tft.println("You are in compare mode Press the center button again when you are \nready to make a gesture.");


      if (centerButtonState==UNPRESSED && centerButton==0) {
        centerButtonState = PRESSED;
        Serial.println("CENTER BUTTON HAS BEEN PRESSED"); 
      }      
      else if (centerButtonState==PRESSED && centerButton==1) {
        centerButtonState = UNPRESSED;
        systemState = RECORD_MEASURE_STATE;
        tft.fillScreen(TFT_BLACK); //fill background
      }

    }

    if (systemState==PRE_TEACH_STATE) {

      tft.setCursor(0,0,2);
      tft.println("You are in teach mode! \nPress the bottom button \nagain when you're ready \nto make a gesture.");


      if (bottomButtonState==UNPRESSED && bottomButton==0) {
        bottomButtonState = PRESSED;
        Serial.println("BOTTOM BUTTON HAS BEEN PRESSED"); 
      }      
      else if (bottomButtonState==PRESSED && bottomButton==1) {
        bottomButtonState = UNPRESSED;
        systemState = RECORD_TEACH_STATE;
        tft.fillScreen(TFT_BLACK); //fill background
      }

  }

    if (systemState==RECORD_MEASURE_STATE) {

      tft.setCursor(0,0,2);
      tft.println("Recording gesture. Press center button again to stop.");
      
      //get accels continuously

      imu.readAccelData(imu.accelCount);
      x = imu.accelCount[0] * imu.aRes; //store convert x val in g's
      y = imu.accelCount[1] * imu.aRes; //store convert y val in g's
      z = imu.accelCount[2] * imu.aRes; //store convert z val in g's

      //just store x vals
      sprintf(curr_z, "%f, ", z);
      strcat(z_arr, curr_z);
      Serial.printf("current z vec is %s \n", z_arr);


      while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
      primary_timer = millis();


      if (centerButtonState==UNPRESSED && centerButton==0) {
        centerButtonState = PRESSED;
        Serial.println("CENTER BUTTON HAS BEEN PRESSED"); 
      }      
      else if (centerButtonState==PRESSED && centerButton==1) {
        centerButtonState = UNPRESSED;
        systemState = GET_STATE;
        tft.fillScreen(TFT_BLACK); //fill background
      }

    }

    if (systemState==LETTER_STATE) {

      tft.setCursor(0,0,2);
      tft.println("What letter did you make? SP Top = A, LP Top = E, SP Center = I, LP Center = O, SP Bottom = U, LP Bottom = Y");


      if (topButtonState==UNPRESSED && topButton==0) {
        topButtonState = PRESSED;
        Serial.println("TOP BUTTON HAS BEEN PRESSED");
        button_start_time = millis();
      }      
      else if (topButtonState==PRESSED && topButton==1) {
        topButtonState = UNPRESSED;
        systemState = POST_STATE;
        if (millis() - button_start_time >= long_press_duration) {
          Serial.println("long press detected");
          strcpy(letter, "E");
        }
        else {
          strcpy(letter, "A");
        }
        tft.fillScreen(TFT_BLACK); //fill background
      }

      if (centerButtonState==UNPRESSED && centerButton==0) {
        centerButtonState = PRESSED;
        Serial.println("CENTER BUTTON HAS BEEN PRESSED");
        button_start_time = millis();
      }      
      else if (centerButtonState==PRESSED && centerButton==1) {
        centerButtonState = UNPRESSED;
        systemState = POST_STATE;
        if (millis() - button_start_time >= long_press_duration) {
          Serial.println("long press detected");
          strcpy(letter, "O");
        }
        else {
          strcpy(letter, "I");
        }
        tft.fillScreen(TFT_BLACK); //fill background
      }

      if (bottomButtonState==UNPRESSED && bottomButton==0) {
        bottomButtonState = PRESSED;
        Serial.println("BOTTOM BUTTON HAS BEEN PRESSED");
        button_start_time = millis();
      }      
      else if (bottomButtonState==PRESSED && bottomButton==1) {
        bottomButtonState = UNPRESSED;
        systemState = POST_STATE;
        if (millis() - button_start_time >= long_press_duration) {
          Serial.println("long press detected");
          strcpy(letter, "Y");
        }
        else {
          strcpy(letter, "U");
        }
        tft.fillScreen(TFT_BLACK); //fill background
      }

    }

  if (systemState==RECORD_TEACH_STATE) {

      tft.setCursor(0,0,2);
      tft.println("Recording gesture. Press bottom button again to stop.");

      //get accels continuously

      imu.readAccelData(imu.accelCount);
      x = imu.accelCount[0] * imu.aRes; //store convert x val in g's
      y = imu.accelCount[1] * imu.aRes; //store convert y val in g's
      z = imu.accelCount[2] * imu.aRes; //store convert z val in g's

      //just store z vals
      sprintf(curr_z, "%f, ", z);
      strcat(z_arr, curr_z);
      Serial.printf("current z vec is %s \n", z_arr);


      while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
      primary_timer = millis();


      if (bottomButtonState==UNPRESSED && bottomButton==0) {
        bottomButtonState = PRESSED;
        Serial.println("BOTTOM BUTTON HAS BEEN PRESSED"); 
      }      
      else if (bottomButtonState==PRESSED && bottomButton==1) {
        bottomButtonState = UNPRESSED;
        systemState = LETTER_STATE;
        tft.fillScreen(TFT_BLACK); //fill background
    }

  }

    if (systemState==GET_STATE) {

      //get request to server, response is most likely gesture

      int i = 0; //start at 0
      do {
          char request[1000];
          sprintf(request, "GET http://608dev-2.net/sandbox/sc/divnor80/gestures_server.py?accels=%s HTTP/1.1\r\n", z_arr);
          request_buffer[i] = (request)[i]; //assign s[i] to the string literal index i
      } while(request_buffer[i++]); //continue the loop until the last char is null
      strcat(request_buffer,"Host: 608dev.net\r\n");
      strcat(request_buffer,"\r\n");
      do_http_GET("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
      Serial.println(response_buffer);

      systemState = LAST_STATE2;
      

    }
    

    if (systemState==POST_STATE) {

      //post request to server, body includes accels

      char body[100]; //for body
      sprintf(body,"letter=%s&accels=%s",letter,z_arr);//generate body
      int body_len = strlen(body); //calculate body length (for header reporting)
      sprintf(request_buffer,"POST http://608dev-2.net/sandbox/sc/divnor80/gestures_server.py HTTP/1.1\r\n");
      strcat(request_buffer,"Host: 608dev.net\r\n");
      strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
      sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
      strcat(request_buffer,"\r\n"); //new line from header to body
      strcat(request_buffer,body); //body
      strcat(request_buffer,"\r\n"); //new line
      Serial.println(request_buffer);
      do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);

      //memset everything
      systemState = LAST_STATE1;


    }

    if (systemState==LAST_STATE1) {

      tft.setCursor(0,0,2);
      tft.printf("Letter is %s. Gesture procedure was a %s Press top button to restart.", letter, response_buffer);


      if (topButtonState==UNPRESSED && topButton==0) {
        topButtonState = PRESSED;
        Serial.println("TOP BUTTON HAS BEEN PRESSED"); 
      }      
      else if (topButtonState==PRESSED && topButton==1) {
        topButtonState = UNPRESSED;
        systemState = IDLE_STATE;
        tft.fillScreen(TFT_BLACK); //fill background
        memset(response_buffer, 0, OUT_BUFFER_SIZE);
        memset(request_buffer, 0, IN_BUFFER_SIZE);
        memset(z_arr, 0, 1500);
        memset(curr_z, 0, 500);
        memset(letter, 0, 2);
      }

    }

    if (systemState==LAST_STATE2) {

      tft.setCursor(0,0,2);
      tft.printf("Gesture classified as %s. Press top button to restart.", response_buffer);


      if (topButtonState==UNPRESSED && topButton==0) {
        topButtonState = PRESSED;
        Serial.println("TOP BUTTON HAS BEEN PRESSED"); 
      }      
      else if (topButtonState==PRESSED && topButton==1) {
        topButtonState = UNPRESSED;
        systemState = IDLE_STATE;
        tft.fillScreen(TFT_BLACK); //fill background
        memset(response_buffer, 0, OUT_BUFFER_SIZE);
        memset(request_buffer, 0, IN_BUFFER_SIZE);
        memset(z_arr, 0, 1500);
        memset(curr_z, 0, 500);
        memset(letter, 0, 2);
      }

    }


}


void do_http_GET(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  Serial.println(host);
  Serial.println(request_buffer);
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}  

/*----------------------------------
 * char_append Function:
 * Arguments:
 *    char* buff: pointer to character array which we will append a
 *    char c: 
 *    uint16_t buff_size: size of buffer buff
 *    
 * Return value: 
 *    boolean: True if character appended, False if not appended (indicating buffer full)
 */
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}

void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

char* replaceWord(const char* s, const char* oldW,
                  const char* newW)
{
    char* result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);
  
    // Counting the number of times old word
    // occur in the string
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;
  
            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }
  
    // Making new string of enough length
    result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1);
  
    i = 0;
    while (*s) {
        // compare the substring with the result
        if (strstr(s, oldW) == s) {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else
            result[i++] = *s++;
    }
  
    result[i] = '\0';
    return result;
}
