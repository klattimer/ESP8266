
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

// This server always sends the same response, nomatter what the request
// that's the JSON value of the light right now.
void response() {

}

void reboot() {
#ifdef LEONARDO
#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Leonardo cannot reboot. Connect pin D8 for hardware reset.");
    digitalWrite(RESET,HIGH);
#endif
    return;
#endif

#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Rebooting...");
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
    String response = sendATCommand("AT+CIFSR",1000, DEBUG);

#ifdef DEBUGSERIAL
    DEBUGSERIAL.println(response);
#endif

    sendATCommand("AT+CIPMUX=1",2000,DEBUG);
    sendATCommand("AT+CIPSERVER=1,80",2000,DEBUG);

#ifdef DEBUGSERIAL
    DEBUGSERIAL.print("Ready.");
#endif
}

void setup() {
    pinMode(LED,OUTPUT);

    digitalWrite(LED,LOW);
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

#ifdef DEBUGSERIAL
    DEBUGSERIAL.println("Initialising module");
#endif
    String response = sendATCommand("AT+RST", 1000, DEBUG);
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

void loop() {

}
