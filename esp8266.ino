
#define SSID "SSID"
#define PASS "PASSWORD"
#define CONNECT_ATTEMPTS 5

// We use software serial for UNO/Nano/Micro etc...
// We use Serial1 for Leo/Mega the define's do the work
//

// #define LEONARDO
// #define MEGA

#ifdef LEONARDO
// Use Leonardo Serial1 for the ESP8266
#define ESP8266 Serial1

// Hardware reset on pin 8
#define RESET 8
#else
#ifdef MEGA
// Use Mega Serial1 for the ESP8266
#define ESP8266 Serial1
#else
// Software serial on pins 5 & 3
#include <SoftwareSerial.h>
SoftwareSerial ESP8266(5,3);
#endif // MEGA
#endif // LEONARDO


#define DEBUG true
// Default serial port is DEBUGSERIAL
#ifdef DEBUG
#define DEBUGSERIAL Serial
#endif

String sendATCommand(String command, const int timeout, boolean debug)
{
    if (debug) {
#ifdef DEBUGSERIAL
        DEBUGSERIAL.print("Sending Command: \"");
        DEBUGSERIAL.print(command);
        DEBUGSERIAL.println("\"");
#endif
    }

    String response = "";
    command+="\r\n";
    ESP8266.print(command);

    long int time = millis();

    while ( (time+timeout) > millis()) {
        while (ESP8266.available()) {
            char c = ESP8266.read();
            response+=c;
        }
    }
    if (debug) {
#ifdef DEBUGSERIAL
        DEBUGSERIAL.print("Response: \"");
        DEBUGSERIAL.print(response);
        DEBUGSERIAL.println("\"");
#endif
    }

    
    if (response.indexOf("busy p...") > -1) {
      resetModule();
      moduleInit();
    }
    
    if (response.indexOf("FAIL") > -1) {
      resetModule();
      moduleInit();
    }
    
    if (response.endsWith("ERROR\r\n")) {
      resetModule();
      moduleInit();
    }

    return response;
}

// Timeout here is different to sending AT commands,
// if we receive data within timeout, we reset the base time
// we timeout when we reach the timeout, a good value for this
// would be 1000ms
String readData(const int timeout) {
    String response = "";
    long int time = millis();
    while ( (time + timeout) > millis()) {
        while (ESP8266.available()) {
            char c = ESP8266.read();
            response+=c;
            time = millis();
        }
    }
    return response;
}

void resetModule() {
#ifdef DEBUGSERIAL
        DEBUGSERIAL.println("Resetting Module");
#endif
    digitalWrite(4, LOW);
    delay(100);
    digitalWrite(4, HIGH);
    delay(1000);
}

void reboot() {

#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Rebooting...");
#endif
    resetModule();
    
#ifdef LEONARDO
#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Leonardo cannot reboot. Connect pin D8 for hardware reset.");
    digitalWrite(RESET,HIGH);
#endif
    return;
#endif

    delay(1000);
    asm volatile ("  jmp 0");
}

boolean connectWiFi() {
    sendATCommand("AT+CWMODE=1", 1000, DEBUG);
    String cmd = "AT+CWJAP=\"";
    cmd += SSID;
    cmd += "\",\"";
    cmd += PASS;
    cmd += "\"";
    String response = sendATCommand(cmd, 5000, DEBUG);
    if (response.indexOf("OK") != -1) {
#ifdef DEBUGSERIAL
        DEBUGSERIAL.println("OK, WiFi Connected.");
#endif
        return true;
    }
#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Can not connect to WiFi.");
#endif
    return false;
}

void startWebserver() {
#ifdef DEBUGSERIAL
    DEBUGSERIAL.print("Starting webserver on ");
#endif
    String response = sendATCommand("AT+CIFSR",2000, DEBUG);

#ifdef DEBUGSERIAL
    DEBUGSERIAL.println(response);
#endif

    sendATCommand("AT+CIPMUX=1",2000,DEBUG);
    sendATCommand("AT+CIPSERVER=1,80",2000,DEBUG);

#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Ready.");
#endif
}

void moduleInit() {
  
#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Initialising module");
#endif
    String response = sendATCommand("AT+RST", 3000, DEBUG);
    if (response.indexOf("ready") != -1) {

#ifdef DEBUGSERIAL
        DEBUGSERIAL.println("Module ready");
#endif
    } else {

#ifdef DEBUGSERIAL
        DEBUGSERIAL.println("Module failed");
#endif
        reboot();
    }

#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Connecting to access point");
#endif

    boolean connected = false;
    for (int i = 0; i < CONNECT_ATTEMPTS; i++) {
#ifdef DEBUGSERIAL
        DEBUGSERIAL.print("Connection Attempt: ");
        DEBUGSERIAL.println(i+1);
#endif
        if (connectWiFi()) {
            connected = true;
            break;
        }
    }

    if (!connected) {
#ifdef DEBUGSERIAL
        DEBUGSERIAL.println("Connection failed");
#endif
        reboot();
    }

#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Connected to access point");
#endif
    delay(2000);
    startWebserver();
}

void setup() {
#ifdef DEBUGSERIAL
    DEBUGSERIAL.begin(9600);
#endif
    ESP8266.begin(9600); // your esp's baud rate might be different

#ifdef LEONARDO
    // Wait for serial to become live
    while (!DEBUGSERIAL);
    while (!ESP8266);
    pinMode(RESET,OUTPUT);
    digitalWrite(RESET,LOW);
#endif
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    pinMode(6, OUTPUT);
    digitalWrite(6, LOW);

    moduleInit();
}

int lightsValue = 0;

void loop() {
  if (ESP8266.available()) {
    String resp = readData(2000);
    int i = 0,j = resp.indexOf('\n');
    while (j > -1 && j <= resp.length()) {
      String line = resp.substring(i,j);
      
      // Process the line
      if (line.indexOf("GET") > -1) {
        String url = line.substring(line.indexOf("GET") + 4, line.indexOf("HTTP/1.1") - 1);
        int connectionId = line.substring(5, line.indexOf(",", 5)).toInt();
        
/*#ifdef DEBUGSERIAL
          DEBUGSERIAL.print("\nLine: ");
          DEBUGSERIAL.println(line);
          DEBUGSERIAL.print("\nURL: ");
          DEBUGSERIAL.println(url);
#endif*/
        if (url == "/lightsOn") {
          
#ifdef DEBUGSERIAL
          DEBUGSERIAL.println("Lights On");
#endif
          digitalWrite(6, HIGH);
          lightsValue = 255;
        } else if (url == "/lightsOff") {
          
#ifdef DEBUGSERIAL
          DEBUGSERIAL.println("Lights Off");
#endif
          digitalWrite(6, LOW);
          lightsValue = 0;
        } else if (url.startsWith("/lightsOn")) {
          // Figure out the /value part
          String value = url.substring(10);
          int v = value.toInt();
          analogWrite(6, v);
          lightsValue = v;
#ifdef DEBUGSERIAL
          DEBUGSERIAL.print("Set lights value: ");
          DEBUGSERIAL.println(v);
#endif
        }
        
     
        // Send JSON response
        String response = "HTTP/1.1 200 OK\r\nContent-Type: text/json; charset=utf-8\r\nContent-Length: ";
        String content = "{\"stat\":\"ok\",\"brightness\":";
        content+=lightsValue;
        content+="}";
        response+=content.length();
        response+="\r\n\r\n";
        response+=content;
        
        String cipSend = "AT+CIPSEND=";
        cipSend += connectionId;
        cipSend += ",";
        cipSend +=response.length();
     
        sendATCommand(cipSend,1000,DEBUG);
#ifdef DEBUGSERIAL
          DEBUGSERIAL.println(response);
#endif
        ESP8266.print(response);
        resp = readData(5000);
#ifdef DEBUGSERIAL
          DEBUGSERIAL.println(resp);
#endif
        String closeCommand = "AT+CIPCLOSE="; 
        closeCommand+=connectionId;
     
        sendATCommand(closeCommand,1000,DEBUG);
#ifdef DEBUGSERIAL
          DEBUGSERIAL.println("Request served");
#endif
        break;
      }
      
      i = j + 1;
      j = resp.indexOf('\n', i);
      if (j == -1) j = resp.length();
    }
  }
}
