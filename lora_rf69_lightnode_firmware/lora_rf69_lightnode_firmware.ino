//
// CHANGE THIS WHEN PROGRAMMING A DEVICE!!!!
//

#define DEVICE_ID          255
#define BROADCASTS_DEMO    1


  




























#include <SPI.h>
#include <RH_RF69.h>
#include <Adafruit_NeoPixel.h>

#define BROADCAST_ID   0

#define PIN            6
#define MAXPIXELS      3

#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
#define LED           13

#define EFFECT_NONE 0
#define EFFECT_FADEOUT 1
#define EFFECT_FLASH 2
#define EFFECT_INTERPOLATE 3

long NT = 0;
long NT2 = 0;
long NT3 = 0;
uint32_t democounter = 0;
uint8_t buf[500];
char tmp[10] = { 0, };
char recvbuf[300] = { 0, };
uint16_t recvpos = 0;
int got_any_serial_command = 0;
uint8_t mode = 0;

uint8_t effect = EFFECT_NONE;
uint8_t effectparam1 = 0;
uint8_t effectparam2 = 0;

int16_t flashphase = 0;

int32_t cr = 0, cg = 0, cb = 0;
int32_t tr = 0, tg = 0, tb = 0;

RH_RF69 rf69(RFM69_CS, RFM69_INT);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(MAXPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() 
{
  SerialUSB.begin(115200);

  delay(2000);
  SerialUSB.print("Lora node #");
  SerialUSB.print(DEVICE_ID);
  SerialUSB.print("\n");

  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(64);

  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(250, 0, 0));
  pixels.show();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);

  pixels.clear();
  pixels.show();

  digitalWrite(LED_BUILTIN, LOW);

  if (!rf69.init()) {
    while(true) {
      SerialUSB.println("init failed");
      delay(1000);
    }
  }

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(433.0)) {
    while(true) {
      SerialUSB.println("setFrequency failed");
      delay(1000);
    }
  }
  
  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(14, true);

  // The encryption key has to be the same as the one in the client
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
}

void parseCommandSetColor(char *cmd) {
  // C _ I H _ R G B
  tr = cmd[5];
  tg = cmd[6];
  tb = cmd[7];
  flashphase = 0;
}

void parseCommandSetEffect(char *cmd) {
  // E _ I H _ E A B
  effect = cmd[5];
  effectparam1 = cmd[6];
  effectparam2 = cmd[7];
}

void parseCommand(char *cmd, int len, int rebroadcast) {
  if (cmd[1] != '_' || cmd[4] != '_') {
    return;
  }

  uint8_t cmdId = cmd[0];
  uint8_t targetDeviceId = cmd[2];
  uint8_t hop = cmd[3];

  if (targetDeviceId == DEVICE_ID || targetDeviceId == BROADCAST_ID) {
    // message is for us.
    if (cmdId == 'C') {
      parseCommandSetColor(cmd);
    } else if (cmdId == 'E') {
      parseCommandSetEffect(cmd);
    }
  }

  if (hop > 0 && targetDeviceId != DEVICE_ID && rebroadcast) {
    // If we have more hops left, and the message isn't for us, re-broadcast it.
    SerialUSB.print("Rebroadcast\n");
    cmd[3] = hop - 1;
    rf69.send((uint8_t *)&cmd, len);
    rf69.waitPacketSent();
  }
}

void sendEffect() {
  int e = rand() % 3;

  memset(&buf, 0, 500);
  buf[0] = 'E';
  buf[1] = '_';
  buf[2] = 0; // Broadcast
  buf[3] = 2; // 2 Hops
  buf[4] = '_';

  if (e == 0) {
    buf[5] = 3; // Interpolate
    buf[6] = 1 + (rand() % 4);
    buf[7] = 0;
  }

  if (e == 1) {
    buf[5] = 1; // Fade to black
    buf[6] = 1 + (rand() % 4);
    buf[7] = 0;
  }

  if (e == 2) {
    buf[5] = 2; // Flash, then fade to black
    buf[6] = 1 + (rand() % 4);
    buf[7] = 20 + (rand() % 50);
  }

  SerialUSB.print("Sending effect message: ");
  for(int j=0; j<8; j++) {
    sprintf(tmp, " %02X", buf[j]);
    SerialUSB.print(tmp);
  }
  SerialUSB.print("\n");
  
  rf69.send((uint8_t *)&buf, 8);
  rf69.waitPacketSent();
  
  // also handle it locally
  parseCommand((char *)&buf, 8, false);
}

void demoTick() {
  if (!BROADCASTS_DEMO) {
    // We're not supposed to run a demo
    return;
  }

  if (got_any_serial_command) {
    // If we got a serial command, stop running demo mode
    return;
  }

  uint8_t r=0, g=0, b=0;
  
  uint8_t c = rand() % 3;
  if (c == 0) r = 100;
  if (c == 1) g = 100;
  if (c == 2) b = 100;

  c = rand() % 3;
  if (c == 0) r = 255;
  if (c == 1) g = 255;
  if (c == 2) b = 255;

  digitalWrite(LED_BUILTIN, HIGH);
  delay(10);
  digitalWrite(LED_BUILTIN, LOW);

  if (democounter % 20 == 0) {
    sendEffect();
  }

  uint8_t target = rand() % 10;
 //  if (target > 8) target = 0;
  target = democounter % 10;

  memset(&buf, 0, 500);
  buf[0] = 'C';
  buf[1] = '_';
  buf[2] = target;
  buf[3] = 1;
  buf[4] = '_';
  buf[5] = r;
  buf[6] = g;
  buf[7] = b;

  SerialUSB.print("Sending demo color message: ");
  for(int j=0; j<8; j++) {
    sprintf(tmp, " %02X", buf[j]);
    SerialUSB.print(tmp);
  }
  SerialUSB.print("\n");

  rf69.send((uint8_t *)&buf, 8);
  rf69.waitPacketSent();

  // also handle it locally
  parseCommand((char *)&buf, 8, false);

  democounter ++;
}

void effectTick() {
  if (effect == EFFECT_FADEOUT) {
    if (flashphase == 0) {
      cr = tr;
      cg = tg;
      cb = tb;
    } else {
      for(int s=0; s<effectparam1; s++) {
        if (cr > 0) cr --;
        if (cg > 0) cg --;
        if (cb > 0) cb --;
      }
    }

  } else if (effect == EFFECT_FLASH) {
  
    if (flashphase <= effectparam2) {
      int32_t bs = 255 / (1 + effectparam2);
      int32_t bri = 255 - (bs * flashphase);
      if (bri < 0) bri = 0;
      cr = tr + bri;
      cg = tg + bri;
      cb = tb + bri;
    } else {
      for(int s=0; s<effectparam1; s++) {
        if (cr > 0) cr --;
        if (cg > 0) cg --;
        if (cb > 0) cb --;
      }
    }

  } else if (effect == EFFECT_INTERPOLATE) {
  
    for(int s=0; s<effectparam1; s++) {
      if (cr < tr) cr ++;
      if (cg < tg) cg ++;
      if (cb < tb) cb ++;

      if (cr > tr) cr --;
      if (cg > tg) cg --;
      if (cb > tb) cb --;
    }
  
  } else { // EFFECT_NONE, just set color
    
    cr = tr;
    cg = tg;
    cb = tb;  
    
  }

  if (flashphase < 1024) {
    flashphase ++;
  }

  // Render
  pixels.clear();
  int32_t r, g, b;
  r = cr;
  g = cg;
  b = cb;
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  for(int k=0; k<MAXPIXELS; k++) {
    pixels.setPixelColor(k, pixels.Color(r, g, b));
  }
  pixels.show();
}

int ishex(char c) {
  if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
    return 1;
  }

  return 0;
}

int hexval(char c) {
  if (c >= '0' && c <= '9') {
    c -= '0';
    return c;
  }
  
  if (c >= 'A' && c <= 'F') {
    c -= 'A';
    c += 10;
    return c;
  }

  if (c >= 'a' && c <= 'f') {
    c -= 'a';
    c += 10;
    return c;
  }

  return 0;
}


void handleSerialCommand(char *cmd) {

  int len = 0;
  memset(&buf, 0, 500);
  
  SerialUSB.print("Got serial command: ");
  SerialUSB.print(cmd);
  SerialUSB.print("\n");


  int offset = 0;
  int highnibble = 1;
  
  for(int i=0; i<strlen(cmd); i++) {
    uint8_t b = cmd[i];
    if (ishex(b)) {
      b = hexval(b);
      if (highnibble) {
        buf[offset] |= b << 4;
        highnibble = 0;
      } else {
        buf[offset] |= b;
        highnibble = 1;
        offset ++;
        len ++;
      }
    }
  }

  if (len > 0) {
    digitalWrite(LED_BUILTIN, HIGH);
    SerialUSB.print("Handling serial message: ");
    for(int j=0; j<len; j++) {
      sprintf(tmp, " %02X", buf[j]);
      SerialUSB.print(tmp);
    }
    SerialUSB.print("\n");

    // handle it locally first.
    parseCommand((char *)&buf, len, false);

    // then broadcast it.
    rf69.send((uint8_t *)&buf, len);
    rf69.waitPacketSent();

    got_any_serial_command = 1;  
    SerialUSB.print("Stopping automatic messaging...\n");
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void loop()
{
  long T = millis();
  if (T > NT) {
    NT = T + 1000;
    digitalWrite(LED_BUILTIN, HIGH);
    SerialUSB.print("Idle.");
    SerialUSB.print(" id:");SerialUSB.print(DEVICE_ID, DEC);
    SerialUSB.print(" c:");SerialUSB.print(cr, DEC);
    SerialUSB.print(",");SerialUSB.print(cg, DEC);
    SerialUSB.print(",");SerialUSB.print(cb, DEC);
    SerialUSB.print(" t:");SerialUSB.print(tr, DEC);
    SerialUSB.print(",");SerialUSB.print(tg, DEC);
    SerialUSB.print(",");SerialUSB.print(tb, DEC);
    SerialUSB.print(" e:");SerialUSB.print(effect, DEC);
    SerialUSB.print(",");SerialUSB.print(effectparam1, DEC);
    SerialUSB.print(",");SerialUSB.print(effectparam2, DEC);
    SerialUSB.print(" fp:");SerialUSB.print(flashphase, DEC);
    SerialUSB.print("\n");
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  if (T > NT2) {
    NT2 = T + 16;
    effectTick();
  }
  
  if (T > NT3) {
    int D = 250; // random(10, 30) * 100;
    NT3 = T + D;
    demoTick();
  }

  if (rf69.available()) {
    // Should be a message for us now
    uint8_t len = 250;
    memset(buf, 0, len);
    if (rf69.recv(buf, &len))
    {
      digitalWrite(LED_BUILTIN, HIGH);

      if (!got_any_serial_command) {
        SerialUSB.print("Got lora request:");
      } else {
        SerialUSB.print("Ignoring lora request:");
      }
      for(int j=0; j<len; j++) {
        sprintf(tmp, " %02X", buf[j]);
        SerialUSB.print(tmp);
      }
      SerialUSB.print("\n");
      if (!got_any_serial_command) {
        // but if we got any serial commands, just ignore lora messages.
        parseCommand((char *)&buf, len, true);
      }

      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  
  while (SerialUSB.available()) {
    uint8_t b = SerialUSB.read();
    if (b == '\n') {
      recvbuf[recvpos] = 0;
      handleSerialCommand((char *)&recvbuf);
      recvpos = 0;
    } else {
      if (recvpos < 300) {
        recvbuf[recvpos] = b;
        recvpos ++;
      }
    }
  }
}
