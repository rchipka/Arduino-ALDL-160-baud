#define ALDL_PIN 0

#define SERIAL_DEBUG

// Number of bits per ALDL packet
#define ALDL_NUM_WORDS 23

// Number of bits per ALDL packet
#define ALDL_PACKET_SIZE 9

// Minimum microsecs for a complete packet
#define ALDL_MIN_PACKET_TIME 5915

// Maximum microsecs for a complete packet
#define ALDL_MAX_PACKET_TIME 5925

// Approx "start bit" max microsecs when transmitting a "0"
#define ALDL_0_MIN_LENGTH 360
#define ALDL_0_MAX_LENGTH 370

// Approx "start bit" min microsecs when transmitting a "1"
#define ALDL_1_MIN_LENGTH 1850
#define ALDL_1_MAX_LENGTH 1899


#define READ_INTERVAL 200

volatile int syncCount = 0;
bool inSync = false;
int buf[ALDL_NUM_WORDS];
volatile unsigned int diff = 0;
volatile unsigned long bufIndex = 0;
volatile unsigned long bitIndex = 0;
volatile unsigned long readIndex = 0;
volatile unsigned long current = micros();
volatile unsigned long prev = micros();
volatile unsigned int curBit = 0;
IntervalTimer myTimer;

void setup() {
 pinMode(ALDL_PIN, INPUT);
  myTimer.priority(255);
  while (digitalRead(ALDL_PIN) == LOW) {
    #ifdef SERIAL_DEBUG
    Serial.println("Waiting for line to go HIGH");
    #endif
    delay(350);
  }
  #ifdef SERIAL_DEBUG
  Serial.print("Line is ");
  if (digitalRead(ALDL_PIN) == HIGH)
    Serial.println("HIGH");
  else
    Serial.println("LOW");
  #endif
  #ifdef SERIAL_DEBUG
  Serial.print("Attaching interrupt on pin #");
  Serial.println(ALDL_PIN);
  #endif
  attachInterrupt(ALDL_PIN, interrupt, CHANGE);

   for (int i = 0; i < ALDL_NUM_WORDS; i++) {
    buf[i] = (byte)0x00;
   }
}

void loop() {
}

void readBit() {
  while (readIndex < bufIndex) {
    Serial.print(readIndex+1);
    Serial.print("/");
    Serial.print(bufIndex+1);
    Serial.print(": ");
    Serial.println(buf[readIndex], BIN);
    readIndex++;
  }
}

void interrupt() {
  current = micros();
  diff = current-prev;
  if (diff <= ALDL_0_MAX_LENGTH && diff >= ALDL_0_MIN_LENGTH) {
    curBit = 0;
  }else if (diff <= ALDL_1_MAX_LENGTH && diff >= ALDL_1_MIN_LENGTH) {
    curBit = 1;
  }else{
    if (diff < ALDL_0_MIN_LENGTH)
      return;
    prev = current;
    /*
    if (diff < ALDL_MAX_PACKET_TIME && diff > ALDL_MIN_PACKET_TIME && inSync) {
      //inSync = false;
      //Serial.print("Invalid packet length: ");
      //Serial.println(diff);
    }*/
    return;
  }
  if (inSync) {
    if (bitIndex == 0) {
      if (curBit == 1) {
        myTimer.end();
        inSync = false;
        bufIndex = 0;
        Serial.println("Invalid start bit");
      }else{
        ++bitIndex;
      }
    }else{
      buf[bufIndex] |= curBit << bitIndex;
      if (++bitIndex == 9) {
        bitIndex = 0;
        if (++bufIndex == ALDL_NUM_WORDS) {
          myTimer.end();
          inSync = false;
          //Serial.println("Out of sync");
        }
      }
    }
  }else if (curBit == 1) {
    if (++syncCount == ALDL_PACKET_SIZE) {
      inSync = true;
      readIndex = 0;
      syncCount = 0;
      bitIndex = 0;
      bufIndex = 0;
      myTimer.begin(readBit, READ_INTERVAL);
      Serial.println("Synced");
    }
  }else{
    syncCount = 0;
  }

  prev = current;
}
