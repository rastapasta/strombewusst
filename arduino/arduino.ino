// *************************************
//   StromBewusst - Arduino Controller
// *************************************
// Configuration

// Uncomment this line if you use an Ethernet Shield
#define USE_ETHERNET_SHIELD
#define ETHERNET_MAC { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc }

// Uncomment this line if you use a WiFi Shield
//#define USE_WIFI_SHIELD
#define WIFI_SSID "ssid"
#define WIFI_KEY "wpakey"

// TODO: Fix IP/net config for both possible shields

// Uncomment this line if you would like to use a NeoPixel for status indication
//#define USE_NEOPIXEL
#define NEOPIXEL_PIN 9

#define USE_XS1_SERVER
#define XS1_IP (10, 11, 10, 18)
#define XS1_URL "/control?callback=cname&cmd=set_state_sensor&number=9&value="
#define XS1_TIMEFRAME 3

// Uncomment these lines if you wish to use the the free strombewusstsein Web Service!
//#define USE_STROMBEWUSST_SERVER
#define STROMBEWUSST_SERVER "se.esse.es"
#define STROMBEWUSST_PORT 8888
#define STROMBEWUSST_KEY "1234567890123456789012"

// TriggerPin - the PIN that gets HIGH when 1.25W/h were used on the line
#define TRIGGER_PIN 8

// TIMEFRAME defines the datastorages time segmenting (256 hits in that time frame max)
#define TIMEFRAME 30

// STORAGE_HOURS defines how many hours of detailed information are stored in RAM
#define STORAGE_HOURS 1

#define SERVER_PORT 80
// *************************************

// Enable as needed, depending on either USE_ETHERNET_SHIELD, USE_WIFI_SHIELD and USE_NEOPIXEL
//#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
//#include <WiFi.h>
//#include <WiFiUdp.h>

// Our prototypes!
void setup();
void loop();
void networkConnect();
void wifiConnect();
void ethernetConnect();

void serverCheck();
void networkCheck();

String serverHeader(String);
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
  EthernetClient connection;
  EthernetServer server = EthernetServer(SERVER_PORT);
  EthernetUDP udp;
  byte ethernetMac[] = ETHERNET_MAC;
#endif

#ifdef USE_WIFI_SHIELD
  #define USE_NETWORK
  WiFiClient client;
  WiFiClient connection;
  WiFiServer server = WiFiServer(SERVER_PORT);
  WiFiUDP udp;
  int connectionStatus = WL_IDLE_STATUS;
#endif

#ifdef USE_NETWORK
  char requestBuffer[128];
  boolean lastConnected = false;
#endif

#ifdef USE_STROMBEWUSST_SERVER
  // UDP Format: <32xkey><1xtimeframe><1xticks>
  char udpBuffer[34] = STROMBEWUSST_KEY;
#endif

// TimeStorage - stores the # of bytes needed to store the information
const unsigned int timeStorage = 60 / TIMEFRAME * 60 * STORAGE_HOURS;
byte storage[timeStorage];
unsigned long storageRuntime = 0;
unsigned int storageHour[24];
unsigned int storageDay[31];

byte lastDay = 0xFF;
byte lastHour = 0xFF;

bool useNetwork = false;
int storagePointer = -1;

void setup() {
  Serial.begin(9600);
  Serial.println("*** StromBewusst - Arduino FW ***");
  
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
    networkCheck();
  #endif
}

void networkCheck()
{
  // dump all pending data from network connections to the serial interface
  while (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }

  // stop the client if we lost the connection since the last loop
  if (!client.connected() && lastConnected) {
    Serial.println("[network] closing connection");
    client.stop();
  }
  lastConnected = client.connected();
}

#ifdef USE_NETWORK
void serverCheck()
{
  connection = server.available();
  // Check for a new connection
  char headerBuffer[10];
  if (connection)
  {
    Serial.println("new client");
    while (connection.connected()) {
      if (connection.available()) {          
        // Get the first couple of bytes
        for (int i=0; i<10; i++)
        {
          headerBuffer[i] = connection.read();
        }
        connection.flush();
        Serial.println(headerBuffer);
        String header = headerBuffer;
        if (header.startsWith("GET / "))
        {
          connection.print(serverHeader("text/html"));
          connection.println("Hello from StromBewusst!");
        }
        
        if(header.startsWith("GET /json "))
        {
          connection.print(serverHeader("text/json"));
          connection.print(generateJSON());
        }

        delay(1);
        connection.stop();
      }
    }
    Serial.println("client disconnected");
  }      
}
#endif

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
  Serial.println("[network] connecting via Ethernet shield");
  Ethernet.begin(ethernetMac);

  // print our Ethernet shield's IP address
  Serial.print("[network] IP address received: ");
  Serial.println(Ethernet.localIP());
}
#endif USE_ETHERNET_SHIELD

// All the WiFi related methods only needed when the shield is used
#ifdef USE_WIFI_SHIELD
void wifiConnect()
{
  Serial.println("[network] Connecting via WiFi shield"); 

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("[network] WiFi shield not present"); 
    while(1);
  }

  // connect to our network  
  while ( connectionStatus != WL_CONNECTED) { 
    Serial.print("[network] Joining network ");
    Serial.println(WIFI_SSID);

    connectionStatus = WiFi.begin(WIFI_SSID, WIFI_KEY);

    // wait 10 seconds for connection
    delay(10000);
  }

  // print our WiFi shield's IP address
  Serial.print("[network] IP address received: ");
  Serial.println(WiFi.localIP());
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

  storageRuntime++;

  byte day = millis() / 1000 / 60 / 24 % 31;
  if (day != lastDay)
  {
    lastDay = day;
    storageDay[day] = 1;
  }
  storageDay[day]++;
  
  byte hour = millis() / 1000 / 60 % 24;
  if (hour != lastHour)
  {
    lastHour = hour;
    storageHour[hour] = 0;
  }
  storageHour[hour]++;
}


void pointerLoop()
{
  // Tick forward each x-Timeframe seconds
  int pointer = millis() / 1000 / TIMEFRAME % timeStorage;
  if (pointer != storagePointer)
  {
    storage[storagePointer] = 1;
    storagePointer = pointer;
    pointerChanged();
  }
}

void pointerChanged()
{
  storage[storagePointer] = 0;

  #ifdef USE_NETWORK
    push();
  #endif

  Serial.println("*** Time frame changed ***");
  Serial.println("Last 15 seconds: "+String(storageSum(1)));
  Serial.println("Last 30 seconds: "+String(storageSum(2)));
  Serial.println("Last minute: "+String(storageSum(4)));
  Serial.println();
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
  String url = XS1_URL;
  url += (int)(storageSum(60*XS1_TIMEFRAME/TIMEFRAME)*1.25*60/XS1_TIMEFRAME);
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
  if (client.connected())
  {
    client.stop();
  }
  
  if (client.connect(ip, 80))
  {
    Serial.println("[network] sending request");

    client.print("GET ");
    client.print(url);
    client.println(" HTTP/1.0");
    client.println("Connection: close");
    client.println();
    
    return true;
  } else
  {
    return false;
  }
}
#endif

#ifdef USE_STROMBEWUSST_SERVER 
void strombewusstPush()
{
  // Set the last byte the the last frame's value
  udpBuffer[33] = storageSum(1);
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
    pos--;
    if (pos == -1)
    {
      pos = timeStorage-1;
    }
    sum += (int)storage[pos];
  }
  return sum;
}

