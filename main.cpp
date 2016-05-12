#define ALDL_PIN 0

#define SERIAL_DEBUG

// Number of bytes per ALDL frame
#define ALDL_FRAME_BUF_SIZE 50

// Number of bits per ALDL byte
#define ALDL_BYTE_SIZE 8

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

int frame[ALDL_FRAME_BUF_SIZE];

volatile unsigned int byteIndex = 0;
volatile unsigned int bitIndex = 0;
volatile unsigned int bitTime = 0;
volatile unsigned int curBit = 0;
volatile unsigned long curTime = micros();
volatile unsigned long prevTimeTime = micros();

void setup() {
    // Clear the frame buffer
    for (int i = 0; i < ALDL_NUM_WORDS; i++) {
        frame[i] = (byte)0x00;
    }

    pinMode(ALDL_PIN, INPUT);
    attachInterrupt(ALDL_PIN, interrupt, CHANGE);
}

void interrupt() {
    if (!readBit()) return;

    if (bitIndex == 0) {
        startBit();
    } else {
        dataBit();
    }
}

bool readBit() {
    curTime = micros();
    bitTime = curTime - prevTime;

    if (bitTime <= ALDL_0_MAX_LENGTH && bitTime >= ALDL_0_MIN_LENGTH) {
        curBit = 0;
    } else if (bitTime <= ALDL_1_MAX_LENGTH && bitTime >= ALDL_1_MIN_LENGTH) {
        curBit = 1;
    } else if (bitTime < ALDL_0_MIN_LENGTH) {
        // Too short to be a bit
        // Could be noise so we don't reset prevTime
        return false;
    } else {
        // Too long to be a bit
        prevTime = curTime;
        return false;
    }

    prevTime = curTime;
}

void syncBit() {
    if (curBit != 1) {
        bitIndex = 0;
        return;
    }

    if (++bitIndex > ALDL_BYTE_SIZE) {
        bitIndex = 0;
        byteIndex = 0;
    }
}

void startBit() {
    if (curBit == 1) {
        // Start bit is "1" which means
        // we're receiving a "sync" bit
        syncBit();
        return;
    }

    ++bitIndex;
}

// Add data bit to the packet framefer
void dataBit() {
    frame[byteIndex] |= curBit << bitIndex;

    if (++bitIndex > ALDL_BYTE_SIZE) {
        // We have a complete data byte
        if (++byteIndex >= ALDL_FRAME_BUF_SIZE) {
            byteIndex = 0;
        }

        bitIndex = 0;
    }
}
