// *************************************
//   StromBewusst - Arduino Controller
// *************************************
// Configuration

// Uncomment these liness if you use an Ethernet Shield
//#define USE_ETHERNET_SHIELD
#define ETHERNET_MAC { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// Uncomment these lines if you use a WiFi Shield
#define USE_WIFI_SHIELD
#define WIFI_SSID "ssid"
#define WIFI_KEY "wpakey"

// TODO: Fix IP/net config for both possible shields

// Uncomment these lines if you use NeoPixel for status visualization
//#define USE_NEOPIXEL
#define NEOPIXEL_PIN 6

#define USE_XS1_SERVER
#define XS1_IP (10, 11, 10, 18)
#define XS1_URL "/control?callback=cname&cmd=set_state_sensor&number=9&value="

// Uncomment these lines if you wish to use the the free strombewusstsein Web Service!
//#define USE_STROMBEWUSST_SERVER
#define STROMBEWUSST_SERVER "se.esse.es"
#define STROMBEWUSST_PORT 8888
#define STROMBEWUSST_KEY "1234567890123456789012"

// TriggerPin - the PIN that gets HIGH when 1.25W/h were used on the line
#define TRIGGER_PIN 8

// TIMEFRAME defines the datastorages time segmenting (256 hits in that time frame max)
#define TIMEFRAME 15

// STORAGE_HOURS defines how many hours of detailed information are stored in RAM
#define STORAGE_HOURS 3

#define SERVER_PORT 80
// *************************************

// Enable as needed, depending on either USE_ETHERNET_SHIELD, USE_WIFI_SHIELD and USE_NEOPIXEL
//#include <Adafruit_NeoPixel.h>
#include <SPI.h>
//#include <Ethernet.h>
//#include <EthernetUdp.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// Our prototypes!
void setup();
void loop();
void networkConnect();
void wifiConnect();
void ethernetConnect();
void serverCheck();
String serverHeader(String);
String requestHeader(String);
bool networkRequestIP(IPAddress, String);

void triggerCheck();
void gotTriggered();

void push();
void xs1Push();
void strombewusstPush();

int storageSum(int);
String generateJSON();

void pointerLoop();
void pointerChanged();
// -- end of prototypes

// Set up the corresponding network device
#ifdef USE_ETHERNET_SHIELD
  #define USE_NETWORK
  EthernetClient client;
  EthernetServer server = EthernetServer(SERVER_PORT);
  EthernetUDP udp;
  byte ethernetMac[] = ETHERNET_MAC;
#endif

#ifdef USE_WIFI_SHIELD
  #define USE_NETWORK
  WiFiClient client;
  WiFiServer server = WiFiServer(SERVER_PORT);
  WiFiUDP udp;
  int connectionStatus = WL_IDLE_STATUS;
#endif

#ifdef USE_NETWORK
  char requestBuffer[128];
#endif

#ifdef USE_STROMBEWUSST_SERVER
  // UDP Format: <32xkey><1xtimeframe><1xticks>
  char udpBuffer[34] = STROMBEWUSST_KEY;
#endif

// TimeStorage - stores the # of bytes needed to store the information
const int timeStorage = 60 / TIMEFRAME * 60 * STORAGE_HOURS;
byte storage[timeStorage];

bool useNetwork = false;
int storagePointer = -1;

void setup() {
  Serial.begin(9600);
  
  pinMode(TRIGGER_PIN, INPUT);
  
  #ifdef USE_NETWORK
    networkConnect();
  #endif
}

void networkConnect()
{
   #ifdef USE_WIFI_SHIELD
    wifiConnect();
  #endif
  #ifdef USE_ETHERNET_SHIELD
    ethernetConnect();
  #endif
  #ifdef USE_NETWORK
    server.begin();
  #endif

  #ifdef USE_STROMBEWUSST_SERVER
    // UDP Format: <32xkey><1xtimeframe><1xticks>
    udpBuffer[32] = TIMEFRAME;
    udp.begin(STROMBEWUSST_PORT);
  #endif 
}

void loop()
{
  // Check if time moved on to a new frame
  pointerLoop();
  
  // Check if we got a signal right now
  triggerCheck();
 
  #ifdef USE_NETWORK 
    // Check if we got an incoming connection to our server
    serverCheck();
  #endif
}

void serverCheck()
{
  #ifdef USE_ETHERNET_SHIELD
    EthernetClient client = server.available();
  #endif
  #ifdef USE_WIFI_SHIELD
    WiFiClient client = server.available();
  #endif
  #ifdef USE_NETWORK
    // Check for a new connection
    char headerBuffer[10];
    if (client)
    {
      Serial.println("new client");
      while (client.connected()) {
        if (client.available()) {          
          // Get the first couple of bytes
          for (int i=0; i<10; i++)
          {
            headerBuffer[i] = client.read();
          }
          client.flush();
          
          String header = headerBuffer;
          if (header.startsWith("GET / "))
          {
            client.print(serverHeader("text/html"));
            client.println("Hello from StromBewusst!");
          }
          
          if(header.startsWith("GET /json "))
          {
            client.print(serverHeader("text/json"));
            client.print(generateJSON());
          }

          delay(1);
          client.stop();
        }
      }
      Serial.println("client disconnected");
    }      
  #endif
}

#ifdef USE_NETWORK
String serverHeader(String contentType)
{
  return
    String("HTTP/1.1 200 OK\r\n")+
    "Content-Type: "+contentType+"\r\n"+
    "Connection: close\r\n\r\n";
}
#endif

// All the Ethernet related methods only needed when the shield is used
#ifdef USE_ETHERNET_SHIELD
void ethernetConnect()
{
  Ethernet.begin(ethernetMac);

  // print our Ethernet shield's IP address
  IPAddress ip = Ethernet.localIP();
  Serial.println(ip);
}
#endif USE_ETHERNET_SHIELD

// All the WiFi related methods only needed when the shield is used
#ifdef USE_WIFI_SHIELD
void wifiConnect()
{
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    while(1);
  } 

  // connect to our network  
  while ( connectionStatus != WL_CONNECTED) { 
    Serial.print("Connecting to SSID: ");
    Serial.println(WIFI_SSID);
    connectionStatus = WiFi.begin(WIFI_SSID, WIFI_KEY);

    // wait 10 seconds for connection
    delay(10000);
  }

  // print our WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}
#endif

void triggerCheck()
{
  // Check if we got an trigger signal
  if (digitalRead(TRIGGER_PIN) == HIGH)
  {
    // Wait until input gets LOW again
    while(digitalRead(TRIGGER_PIN) == HIGH);
    gotTriggered();
  }
}

void gotTriggered()
{
  Serial.println("Trigger got high!");
  storage[storagePointer]++;
}


void pointerLoop()
{
  int pointer = millis() / 1000 / TIMEFRAME % STORAGE_HOURS;
  if (pointer != storagePointer)
  {
    #ifdef USE_NETWORK
      // Only push if this is not the first iteration
      if (storagePointer != -1)
      {
        push();
      }
    #endif
    
    storagePointer = pointer;
    pointerChanged();
  }
}

void pointerChanged()
{
  storage[storagePointer] = 0;
  Serial.println("Last 15 seconds: "+String(storageSum(1)));
  Serial.println("Last 30 seconds: "+String(storageSum(2)));
  Serial.println("Last minute: "+String(storageSum(4)));
  Serial.println("Pointer changed to "+String(storagePointer));
}



void push()
{
   #ifdef USE_STROMBEWUSST_SERVER
    strombewusstPush();
  #endif
  #ifdef USE_XS1_SERVER
    xs1Push();
  #endif
}

#ifdef USE_XS1_SERVER
void xs1Push()
{
  IPAddress ip XS1_IP;
  String url = XS1_URL+String(storageSum(60/TIMEFRAME*5)/5*1.25*60);
  Serial.println("[XS1] Update URL: "+url);
  if (networkRequestIP(ip, url) == true)
  {
    Serial.println("[XS1] updated");
  } else
  {
    Serial.println("[XS1] update failed");
  }
}
#endif

#ifdef USE_NETWORK
bool networkRequestIP(IPAddress ip, String url)
{
  if (client.connect(ip, 80))
  {
    requestHeader(url).toCharArray(requestBuffer, 128);
    client.write(requestBuffer);
    return true;
  } else
  {
    return false;
  }
}

String requestHeader(String url)
{
  return "GET "+url+"HTTP/1.1\r\nConnection: close\r\n\r\n";
}
#endif

#ifdef USE_STROMBEWUSST_SERVER 
void strombewusstPush()
{
  // Set the last byte the the current frame's value
  udpBuffer[33] = storage[storagePointer];
  udp.beginPacket(STROMBEWUSST_SERVER, STROMBEWUSST_PORT);
  udp.write(udpBuffer);
  udp.endPacket();
}
#endif 

#ifdef USE_NETWORK
String generateJSON()
{
  return "{\"history\":{\"15\":"+String(storageSum(1))
          +",\"30\":"+String(storageSum(2))
          +",\"60\":"+String(storageSum(4))
          +"}}";
}
#endif

int storageSum(int frames)
{
  int sum = 0;
  int pos = storagePointer;
  for (int i=0; i<frames; i++)
  {
    if (--pos == -1)
    {
      pos = timeStorage-1;
    }
    sum += (int)storage[pos];
  }
  return sum;
}

