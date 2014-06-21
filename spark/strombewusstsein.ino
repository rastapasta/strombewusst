
#define STROMBEWUSST_IP (192,168,178,49)
//(188, 40, 78, 147)
#define STROMBEWUSST_PORT 8888
#define PIN D4


UDP Udp;
byte udpBuffer[26];
IPAddress strombewusstIP STROMBEWUSST_IP;

volatile bool gotSignal = FALSE;
const unsigned int localPort = 8888;
unsigned int currentWatts = 0;
unsigned long lastSignal = 0;
unsigned int lastWatts = 0;

void setup() {
    // Setup the signal processing
    attachInterrupt(PIN, signalInterrupt, FALLING);
    
    // Prepare the UDP buffer
    char buffer[25];
    String sparkID = Spark.deviceID();
    sparkID.toCharArray(buffer, 25);
    for (byte i=0; i<23; i++)
        udpBuffer[i] = buffer[i];
        
    Udp.begin(localPort);
}

void loop() {
    if (gotSignal) {
        gotSignal = FALSE;
        
        RGB.control(true);
        RGB.color(255,0,0);

        processSignal();
        sendUdp();
    }
    
    if (RGB.controlled() && (micros()-lastSignal) > 2000000) {
        RGB.control(false);
    }
}

void signalInterrupt() {
    gotSignal = TRUE;
}

void processSignal()
{
  unsigned long now = micros();
  
  // Fix glimps - there should be at least 5ms between each signal
  if ((now-lastSignal) < 50000)
    return;
  
  lastWatts = currentWatts;  
  currentWatts = 4500000 / ((now-lastSignal)/1000);

  if (currentWatts == 1)
      currentWatts = lastWatts;

  lastSignal = now;
}

void sendUdp() {
    Udp.beginPacket(strombewusstIP, STROMBEWUSST_PORT);
    udpBuffer[24] = currentWatts>>8;
    udpBuffer[25] = currentWatts%256;
    Udp.write(udpBuffer, 26);
    Udp.endPacket();
}

