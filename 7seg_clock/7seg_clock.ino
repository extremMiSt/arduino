#include <Wire.h>

#define DCF_PIN 2

//dcf state
int start = 0;
int end = 0;
int duration = 0;
 
bool signal = false;
bool sync = false;
int count = -1;
int data[65];
int hour = 0;
int minute = 0;

//LED driver adress
const uint8_t addr = 0x70;
const int ds3231 = 0x68;

//the display buffer
uint16_t displayBuffer[8];

void setup() {

  //start serial for debug
  Serial.begin(2000000);
  while (!Serial) {;}

  //init dcf
  pinMode(DCF_PIN, INPUT);

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
  if(count > 60){ //somehow we got more than 60 bits
    sync = false;
  }
  int DCF_SIGNAL = digitalRead(DCF_PIN);
  
  if (DCF_SIGNAL == HIGH && signal == false){ //rising edge
    signal = true;
    end = millis();
    duration = end - start;

    if (sync == true) {
      data[count] = (bitFromTime(duration));
      Serial.print (data[count]);
    }
    else {
      Serial.print(".");
    }
  }

  if (DCF_SIGNAL == LOW && signal == true) { //falling edge
    signal = false;
    start = millis();

    if (duration >= 1700) {
      count = 0;
      sync = true;
      hour = data[29]*1+data[30]*2+data[31]*4+data[32]*8+data[33]*10+data[34]*20;
      minute = data[21]*1+data[22]*2+data[23]*4+data[24]*8+data[25]*10+data[26]*20+data[27]*40;
      Serial.println("\n" + String(hour) + ":" + String(minute));
    } else {
      count++;
    }
  }
  
  //ouput time
  //serial
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute<10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  //7seg
  displayBuffer[0] = to7seg(minute % 10);
  displayBuffer[1] = to7seg(minute / 10);
  displayBuffer[2] = to7seg(hour % 10);
  displayBuffer[3] = to7seg(hour / 10);
  setBright(calcBright(hour));
  show();

  //wait
  delay(500);
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

//-------------------DFC77 stuff ----------------
int bitFromTime (int duration) {
  if(duration >= 851 && duration <= 950){
    return 0;
  }
  if(duration >= 750 && duration <= 850){
    return 1;
  }
  //else, we have a bad connection
  sync = false;
  return 0;
}
