//int input = 0;

void setup() {              

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  Serial.begin(9600); 
}

void loop() {
    char input = Serial.read();
    if (input == '0'){
      digitalWrite(4,HIGH);
      digitalWrite(5,LOW);
    }
 
    if (input == '1'){
     
    digitalWrite(4,LOW);
    digitalWrite(5,HIGH);
  }

}
