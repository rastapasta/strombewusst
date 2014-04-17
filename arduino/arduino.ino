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

#define USE_XS1_SERVER
#define XS1_IP (10, 11, 10, 18)
#define XS1_URL "/control?callback=cname&cmd=set_state_sensor&number=9&value="
#define XS1_TIMEFRAME 2

// Uncomment these lines if you wish to use the the free strombewusstsein Web Service!
#define USE_STROMBEWUSST_SERVER
#define STROMBEWUSST_IP (188, 40, 78, 147)
#define STROMBEWUSST_PORT 8888
#define STROMBEWUSST_KEY {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};

// Trigger interrupt port - the PIN that gets HIGH when 1.25W/h were used on the line
//    0 -> PIN 2
//    1 -> PIN 3
#define TRIGGER_INT 0

// TIMEFRAME defines the datastorages time segmenting (256 hits in that time frame max)
#define TIMEFRAME 15

// STORAGE_MINUTES defines how many minutes of detailed information are stored in RAM
#define STORAGE_MINUTES 10

#define SERVER_PORT 80
//#define LOCAL

// *************************************

#ifdef LOCAL
  #define XS1_IP (192, 168, 178, 21)
  #define STROMBEWUSST_IP (192, 168, 178, 21)
#endif

// Dynamic inclusion taking place

#if defined(ARDUINO) && ARDUINO > 18
  #include <SPI.h>
#endif

#ifdef USE_ETHERNET_SHIELD
  #include <Ethernet.h>
  #ifdef USE_STROMBEWUSST_SERVER
    #include <EthernetUdp.h>
  #endif
#endif

#if defined(USE_WIFI_SHIELD)
  #include <WiFi.h>
  #ifdef USE_STROMBEWUSST_SERVER
    #include <WiFiUdp.h>
  #endif
#endif

// Whyever??? Only prototype needed. TODO.
void networkConnect();

// Set up the corresponding network device
#ifdef USE_ETHERNET_SHIELD
  #define USE_NETWORK
  EthernetClient client;
  EthernetClient connection;
  EthernetServer server = EthernetServer(SERVER_PORT);
  #ifdef USE_STROMBEWUSST_SERVER
    EthernetUDP udp;
  #endif
  byte ethernetMac[] = ETHERNET_MAC;
#endif

#ifdef USE_WIFI_SHIELD
  #define USE_NETWORK
  WiFiClient client;
  WiFiClient connection;
  WiFiServer server = WiFiServer(SERVER_PORT);
  int connectionStatus = WL_IDLE_STATUS;
  #ifdef USE_STROMBEWUSST_SERVER
    WiFiUDP udp;
  #endif
#endif

#ifdef USE_NETWORK
  char readBuffer[13];
  char headerBuffer[10];
  char sendBuffer[128];
#endif

#ifdef USE_STROMBEWUSST_SERVER
  // UDP Format: <16xkey><1xtimeframe><1xticks>
  byte udpBuffer[8] = STROMBEWUSST_KEY;
  IPAddress strombewusstIP STROMBEWUSST_IP;
#endif

// stores the # of bytes needed to store the information
const unsigned int
  storageSize = 60 / TIMEFRAME * STORAGE_MINUTES;

// Gets incremented by the interrupt method gotSignal
volatile byte signals = 0;

byte
  storage[storageSize],
  lastDay = 0xFF,
  lastHour = 0xFF;

unsigned long
  storageRuntime = 0,
  lastTrigger = 0;

unsigned int
  storagePointer = 0,
  storageHour[24],
  storageDay[31];

void setup() {
  Serial.begin(9600);
  Serial.println(F("*** StromBewusst - Arduino FW ***"));

  // Attach an interrupt
  attachInterrupt(TRIGGER_INT, gotSignal, RISING);

  #ifdef USE_NETWORK
    pinMode(4, OUTPUT);     // SD select pin
    digitalWrite(4, HIGH);  // Disable the SD port
    pinMode(10, OUTPUT);    // Ethernet//Wifi select pin
    digitalWrite(10, LOW);  // Explicitly enable network
    networkConnect();
  #endif

  Serial.println(F("[system] end of setup method"));
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
    udpBuffer[6] = (byte)TIMEFRAME;
    udp.begin(STROMBEWUSST_PORT);
  #endif
}

void loop()
{
  // Check if time moved on to a new frame
  pointerLoop();

  #ifdef USE_NETWORK
    // Check if we got an incoming connection to our server
    serverCheck();
    networkCheck();
  #endif
}

#ifdef USE_NETWORK
void networkCheck()
{
  // dump all pending data from network connections to the serial interface
  if (client.available())
  {
    for (byte i=0; i<12; i++)
    {
      readBuffer[i] = client.read();
    }
    client.stop();

    String response = readBuffer;
    if (response.substring(9) == "200")
    {
      Serial.println(F("[network] HTTP push successful"));
    } else
    {
      Serial.print(F("[network] HTTP push failed - response: "));
      Serial.println(response.substring(9));
    }
  }
}
#endif

#ifdef USE_NETWORK
void serverCheck()
{
  connection = server.available();
  // Check for a new connection
  if (connection)
  {
    Serial.println(F("[server] new client"));
    if (connection.available()) {
      // Get the first couple of bytes
      for (int i=0; i<9; i++)
      {
        headerBuffer[i] = connection.read();
      }
      connection.flush();

      Serial.print(F("[server] request: "));
      Serial.println(headerBuffer);

      String header = headerBuffer;
      if (header.startsWith("GET / "))
      {
        connection.println(F("HTTP/1.1 200 OK"));
        connection.println(F("Content-Type: text/html"));
        connection.println(F("Connection: close"));
        connection.println();

        connection.print(F("Angelegte Last: "));
        connection.println(calculateWatt(1));

      } else if (header.startsWith("GET /json"))
      {
        connection.println(F("HTTP/1.1 200 OK"));
        connection.println(F("Content-Type: text/json"));
        connection.println(F("Connection: close"));
        connection.println();
        connection.println(F("{\"foo\":\"bar\"}"));
      } else
      {
        connection.println(F("HTTP/1.1 404 Not found"));
        connection.println();
      }

      delay(10);
      connection.stop();
    }
    Serial.println(F("[server] client disconnected"));
  }
}
#endif

// All the Ethernet related methods only needed when the shield is used
#ifdef USE_ETHERNET_SHIELD
void ethernetConnect()
{
  Serial.println(F("[network] connecting via Ethernet shield"));
  Ethernet.begin(ethernetMac);

  // print our Ethernet shield's IP address
  Serial.print(F("[network] IP address received: "));
  Serial.println(Ethernet.localIP());
}
#endif

// All the WiFi related methods only needed when the shield is used
#ifdef USE_WIFI_SHIELD
void wifiConnect()
{
  Serial.println(F("[network] Connecting via WiFi shield"));

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("[network] WiFi shield not present"));
    while(1);
  }

  // connect to our network
  while ( connectionStatus != WL_CONNECTED) {
    Serial.print(F("[network] Joining network "));
    Serial.println(WIFI_SSID);

    connectionStatus = WiFi.begin(WIFI_SSID, WIFI_KEY);

    // wait 10 seconds for connection
    delay(10000);
  }

  // print our WiFi shield's IP address
  Serial.print(F("[network] IP address received: "));
  Serial.println(WiFi.localIP());
}
#endif

// Simple as it is: increment on each interrupt (rising low -> high on signal pin)
void gotSignal()
{
  signals++;
}


void pointerLoop()
{
  // Tick forward each x-Timeframe seconds
  int pointer = millis() / 1000 / TIMEFRAME % storageSize;
  if (pointer != storagePointer)
  {
    storeSignals();

    storagePointer = pointer;
    storage[storagePointer] = 0;

    pointerChanged();
  }
}

void storeSignals()
{

  storage[storagePointer] = signals;
  storageRuntime += signals;

  byte day = millis() / 1000 / 60 / 24 % 31;
  if (day != lastDay)
  {
    lastDay = day;
    storageDay[day] = 0;
  }
  storageDay[day] += signals;

  byte hour = millis() / 1000 / 60 % 24;
  if (hour != lastHour)
  {
    lastHour = hour;
    storageHour[hour] = 0;
  }
  storageHour[hour] += signals;

  signals = 0;
}

void pointerChanged()
{
  #ifdef USE_NETWORK
    push();
  #endif

  Serial.println(F("[loop] *** Time frame changed ***"));

  Serial.print(F("[data] Last 30 seconds: "));
  Serial.println(storageSum(1));

  Serial.print(F("[data] Last 60 seconds: "));
  Serial.println(storageSum(2));

  Serial.print(F("[data] Last 5 minutes: "));
  Serial.println(storageSum(20));

  Serial.print(F("[data] Currently connected: "));
  Serial.print(calculateWatt(2));
  Serial.println(F(" Watt"));
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
  url += calculateWatt(XS1_TIMEFRAME);

  if (networkRequestIP(ip, url))
  {
    Serial.println(F("[XS1] push!"));
  } else
  {
    Serial.println(F("[XS1] push failed"));
  }
}
#endif

#ifdef USE_NETWORK
bool networkRequestIP(IPAddress ip, String url)
{
  Serial.print(F("[network] HTTP request "));
  Serial.println(url);

  if (client.connect(ip, 80))
  {
    client.print(F("GET "));
    client.print(url);
    client.println(F(" HTTP/1.0"));
    client.println(F("Connection: close"));
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
  Serial.println(F("[strombewusst] push!"));

  // Set the last byte to the latest storage field
  udpBuffer[7] = (byte)storageSum(1);
  udp.beginPacket(strombewusstIP, STROMBEWUSST_PORT);
  udp.write(udpBuffer, 8);
  udp.endPacket();
}
#endif

int storageSum(unsigned int frames)
{
  int sum = 0;
  int pos = storagePointer;
  for (int i=0; i<frames; i++)
  {
    pos--;
    if (pos == -1)
    {
      pos = storageSize-1;
    }
    sum += storage[pos];
  }
  return sum;
}

int calculateWatt(unsigned int minutes)
{
  return storageSum(60 / TIMEFRAME * minutes)*1.25*60/minutes;
}
