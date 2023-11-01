#include <EEPROM.h>
#include <Wire.h>

#define DCF_PIN PD2

int hour = 0;
int minute = 0;

unsigned long last_time = 0;


//LED driver adress
const uint8_t addr = 0x70;
const int ds3231 = 0x68;

//the display buffer
uint16_t displayBuffer[8];

void setup() {
  //start serial for debug
  Serial.begin(2000000);

  //init matrix-led driver
  //Make connection to LED-driver
  Wire.begin();
  
  //send command to start the LED-drivers timer
  Wire.beginTransmission(addr);
  Wire.write(0x20 | 1);
  Wire.endTransmission();

  //send command to set the brightness
  Wire.beginTransmission(addr);
  Wire.write(0xE0 | 15); //0 is minimum, 15 is maximum
  Wire.endTransmission();
  
  //stop blinking
  Wire.beginTransmission(addr);
  Wire.write(0x80 | 0 << 1 | 1); // Blinking / blanking command
  Wire.endTransmission();
  
  //set the display buffer to all on
  for (int i = 0; i < 8; i++) {
    displayBuffer[i] = 0xFFFF;
  }
  show();
}

void loop() {
  unsigned long period = getMspm();
  unsigned long micro = micros();

  if(micro - last_time > period){
    last_time = micro;

    minute += 1;
    if(minute == 60){
      minute = 0;
      hour += 1;
      if(hour == 24){
        hour = 0;
      }
    }

    Serial.println((hour<10?"0":"") + String(hour) + ":" + (minute<10?"0":"") + String(minute) + " - " + String(getMspm()));

    displayBuffer[0] = to7seg(minute % 10);
    displayBuffer[1] = to7seg(minute / 10);
    displayBuffer[2] = to7seg(hour % 10);
    displayBuffer[3] = to7seg(hour / 10);
    setBright(calcBright(hour));
    show();
  }

  while (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    if (line.startsWith("t")) {
      if (line.length() == 6 && line.charAt(3) == ':') {
        int hz = line.charAt(1) - 48;
        int he = line.charAt(2) - 48;
        int mz = line.charAt(4) - 48;
        int me = line.charAt(5) - 48;
        if (hz * 10 + he < 24 && mz < 6 && me < 10 && hz * 10 + he >= 0 && mz >= 0 && me >= 0) {
          hour = hz * 10 + he;
          minute = mz * 10 + me;
          setBright(calcBright(hour));
           Serial.println((hour<10?"0":"") + String(hour) + ":" + (minute<10?"0":"") + String(minute));

        } else {
          Serial.println("correct: tXX:XX, got: " + line);
        }
      } else {
        Serial.println("correct: tXX:XX, got: " + line);
      }
    }
    if (line.startsWith("f")) {
      long msPm2 = line.substring(1).toInt();
      if(msPm2 > 0){
        setMspm(msPm2);
        unsigned long t = getMspm();
        Serial.println("msPm is now: " + String(t));
      }else{
        Serial.println("msPm value out of bounds");
      }
    }
  }

}

//---------------- LED DRIVER STUFF --------------
uint16_t to7seg(uint8_t b) {
  if (b == 0) return 0x003F;
  if (b == 1) return 0x0006;
  if (b == 2) return 0x005B;
  if (b == 3) return 0x004F;
  if (b == 4) return 0x0066;
  if (b == 5) return 0x006D;
  if (b == 6) return 0x007D;
  if (b == 7) return 0x0007;
  if (b == 8) return 0x007F;
  if (b == 9) return 0x006F;
  if (b == 10) return 0x0077;
  if (b == 11) return 0x007C;
  if (b == 12) return 0x0039;
  if (b == 13) return 0x005E;
  if (b == 14) return 0x0079;
  if (b == 15) return 0x0071;
  return 0x40;
}

void setBright(int i) {
  Wire.beginTransmission(addr);
  Wire.write(0xE0 | i); //0 is minimum, 15 is maximum
  Wire.endTransmission();
}

int calcBright(int h) {
  return (int)(cos(h * PI/12.0 -4)*6+6)+1;
}

//send display buffer to LED-driver and make LED turn on and off
void show() {
  Wire.beginTransmission(addr);
  Wire.write(0x00); // start at address 0x0

  for (int i = 0; i < 8; i++) {
    Wire.write(displayBuffer[i] & 0xFF);
    Wire.write(displayBuffer[i] >> 8);
  }
  Wire.endTransmission();
}

unsigned long getMspm(){
  unsigned long t;
  EEPROM.get(100, t);
  if(t > 1000000000){
    return 1000;
  }
  return t;
}

void setMspm(unsigned long mspm){
  EEPROM.put(100, mspm);
}
