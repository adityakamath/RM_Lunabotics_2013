int analogInPin = A0;  
void setup() {
 
  Serial.begin(9600); 
}

void loop() {

float a=0.0;
float b=0.0;
a=analogRead(A0);
b=a*(60/819);
 Serial.println(b);      
          
}

