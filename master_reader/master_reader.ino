#include <Wire.h>

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
}

void loop()
{
  Wire.requestFrom(2, 3);
  int c[3];  // request 6 bytes from slave device #2
  while(Wire.available())    // slave may send less than requested
  { 
    for(int i=0;i<3;i++){
    c[i] = Wire.read(); // receive a byte as character
    }
    int d=c[0]*100+c[1]*10+c[2];
    Serial.println(d);
  }

  delay(500);
}
