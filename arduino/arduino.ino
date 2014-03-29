// *************************************
//   StromBewusst - Arduino Controller
// *************************************
// Configuration

// Uncomment these liness if you use an Ethernet Shield
//#define USE_ETHERNET_SHIELD
#define ETHERNET_MAC { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// Uncomment these lines if you use a WiFi Shield
//#define USE_WIFI_SHIELD
#define WIFI_SSID "ssid"
#define WIFI_KEY "wpakey"

// TODO: Fix IP/net config for both possible shields

// Uncomment these lines if you use NeoPixel for status visualization
//#define USE_NEOPIXEL
#define NEOPIXEL_PIN 6

//#define USE_XS1_SERVER
#define XS1_IP (10, 11, 10, 18)
#define XS1_URL "/control?callback=cname&cmd=set_state_sensor&number=9&value="

// Uncomment these lines if you wish to use the the free strombewusstsein Web Service!
//#define USE_STROMBEWUSST_SERVER
#define STROMBEWUSST_SERVER "se.esse.es"
#define STROMBEWUSST_PORT 8888
#define STROMBEWUSST_KEY "foobar"

// TriggerPin - the PIN that gets HIGH when 1.25W/h were used on the line
#define TRIGGER_PIN 8

// TIMEFRAME defines the datastorages time segmenting (256 hits in that time frame max)
#define TIMEFRAME 60

// STORAGE_HOURS defines how many hours of detailed information are stored in RAM
#define STORAGE_HOURS 12

#define SERVER_PORT 80

// *************************************

// Enable as needed, depending on either USE_ETHERNET_SHIELD, USE_WIFI_SHIELD and USE_NEOPIXEL
//#include <Adafruit_NeoPixel.h>
//#include <SPI.h>
//#include <Ethernet.h>
//#include <EthernetUdp.h>
//#include <WiFi.h>
//#include <WiFiUdp.h>

// Our prototypes!
void setup();
void loop();
void networkConnect();
void wifiConnect();
void ethernetConnect();

void serverCheck();
void triggerCheck();
void gotTriggered();

void xs1Push();
void strombewusstPush();

int storageSum(int frames);

void pointerLoop();
void pointerChanged();

#ifdef USE_ETHERNET_SHIELD
  #define USE_NETWORK
  EthernetServer server = EthernetServer(SERVER_PORT);
  EthernetUDP udp;
  byte ethernetMac[] = ETHERNET_MAC;
#endif

#ifdef USE_WIFI_SHIELD
  #define USE_NETWORK
  WiFiServer server = WiFiServer(SERVER_PORT);
  WiFiUDP udp;
  int connectionStatus = WL_IDLE_STATUS;
#endif

#ifdef USE_STROMBEWUSST_SERVER
  // UDP Format: <32xkey><1xtimeframe><1xticks>
  char udpBuffer[34] = STROMBEWUSST_KEY;
#endif

// TimeStorage - stores the # of bytes needed to store the information
const int timeStorage = 60 / TIMEFRAME * 60 * STORAGE_HOURS;
byte storage[timeStorage];

bool useNetwork = false;
int storagePointer = 0;

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

  #ifdef USE_STROMBEWUSST_SERVER
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
    if (client)
    {
      Serial.println("Got a connection!");
      client.stop();
    }
  #endif
}


// All the Ethernet related methods only needed when the shield is used
#ifdef USE_ETHERNET_SHIELD
void ethernetConnect()
{
  Ethernet.begin(ethernetMac);

  // print our Ethernet shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.println(Ethernet.localIP());
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
    #ifdef USE_STROMBEWUSST_SERVER
      strombewusstPush();
    #endif
    #ifdef USE_XS1_SERVER
      xs1Push();
    #endif
    storagePointer = pointer;
    pointerChanged();
  }
}

void xs1Push()
{
  // Make sure we have either the Ethernet or WiFi shield enabled
  #ifdef USE_NETWORK  

  #endif
}  
  
void strombewusstPush()
{
  // Make sure we have either the Ethernet or WiFi shield enabled
  #ifdef USE_NETWORK  
    // Set the last byte the the current frame's value
    udpBuffer[33] = storage[storagePointer];
    udp.beginPacket(STROMBEWUSST_SERVER, STROMBEWUSST_PORT);
    udp.write(udpBuffer);
    udp.endPacket();
  #endif
}

void pointerChanged()
{
  storage[storagePointer] = 0;
  Serial.println("Last 5 seconds: "+String(storageSum(1)));
  Serial.println("Last 30 seconds: "+String(storageSum(6)));
  Serial.println("Last minute: "+String(storageSum(12)));
  Serial.println("Pointer changed to "+String(storagePointer));
}


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

