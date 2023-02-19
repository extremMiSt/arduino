#include <Wire.h>

//LED driver adress
const uint8_t addr = 0x70;
const int ds3231 = 0x68;

//the display buffer
uint16_t displayBuffer[8];

//time fudging for fast/slow running rtcs
bool fudged = false;

void setup() {

  //start serial for debug
  Serial.begin(2000000);
  while (!Serial) {;}

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
  //handle input/output
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    if (line.startsWith("t")) {
      if (line.length() == 6 && line.charAt(3) == ':') {
        int hz = line.charAt(1) - 48;
        int he = line.charAt(2) - 48;
        int mz = line.charAt(4) - 48;
        int me = line.charAt(5) - 48;
        if (hz * 10 + he < 24 && mz < 6 && me < 10 && hz * 10 + he >= 0 && mz >= 0 && me >= 0) {
          setDS3231time(0, mz*10+me, hz*10+he, 1, 1, 1, 1);
          setBright(calcBright(hz*10+he));
        } else {
          Serial.println("correct: tXX:XX, got: " + line);
        }
      } else {
        Serial.println("correct: tXX:XX, got: " + line);
      }
    }
    if (line.startsWith("f")) {
      int fudge = line.substring(1).toInt();
      if(fudge >= -128 && fudge <=127){
        setDS3231alertSec(fudge);
        Serial.println("fudge is now: " + String(fudge));
      }else{
        Serial.println("fudge value out of bounds");
      }
    }
  }

  //read time
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  //handle fudge
  byte fudge = getDS3231alertSec();
  if(minute == 0 && second == 0 && hour%3==0){
    second = second + fudge%60;
    minute = minute + fudge/60;
    setDS3231time(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
    fudged = true;
  }else{
    fudged = false;
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
  Serial.print(" - fudge: ");
  Serial.println(fudge, DEC);
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

//-------------------RTC stuff ----------------
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(ds3231);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year)
{
  Wire.beginTransmission(ds3231);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(ds3231, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

byte getDS3231alertSec()
{
  Wire.beginTransmission(ds3231);
  Wire.write(0x07); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(ds3231, 1);
  return Wire.read();
}

void setDS3231alertSec(byte fudge)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(ds3231);
  Wire.write(0x07); // set next input to start at the seconds register
  Wire.write(fudge); // set seconds
  Wire.endTransmission();
}

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}
