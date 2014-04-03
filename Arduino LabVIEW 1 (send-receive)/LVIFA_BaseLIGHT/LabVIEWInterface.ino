
long int x;
long int h;
unsigned int retVal;
int currentMode;
unsigned int freq;
unsigned long duration;
byte customChar[8];

void setMode(int mode)
{
  currentMode = mode;
}


int checkForCommand(void)
{  

  int bufferBytes = Serial.available();

  if(bufferBytes >= COMMANDLENGTH) 
  {
    for(int i=0; i<COMMANDLENGTH; i++)
    {
      currentCommand[i] = Serial.read();       
    }     
    processCommand(currentCommand);     
    return 1;
  }
  else
  {
    return 0; 
  }
}


void processCommand(unsigned char command[])
{  

  if(command[0] == 0xFF && checksum_Test(command) == 0)
  {
    switch(command[1])
    {    
   case 0x00:     // Sync Packet
      Serial.print("sync");
      Serial.flush();        
      break;
    case 0x01:    // Flush Serial Buffer  
      Serial.flush();
      break;
      
               
      case 0x10: //Read and write back (serial monitor)
         if(command[2]>0)
         {
         char msg[13]="";
         for(int i=0;i<command[2];i++)     //String is recieved from LW and sent back.
         msg[i]=command[i+3]; 
         
         //Max length 11 charecters
           Serial.print(msg);
           
          x=atol(msg);
         h=x%10;
          
           if(h==3)
           {
         digitalWrite(8,HIGH); 
       digitalWrite(13,HIGH);  
       }     //do something depepnding on string
         else
         {
         digitalWrite(8,LOW);
        digitalWrite(13,LOW);
         }

      }
         break;

            
        
    /*********************************************************************************
    ** Continuous Aquisition
    *********************************************************************************/        
      case 0x2A:  // Continuous Aquisition Mode On
        acqMode=1;
        contAcqPin=command[2];
        contAcqSpeed=(command[3])+(command[4]<<8);  
        acquisitionPeriod=1/contAcqSpeed;
        iterationsFlt =.08/acquisitionPeriod;
        iterations=(int)iterationsFlt;
        if(iterations<1)
        {
          iterations=1;
        }
        delayTime= acquisitionPeriod; 
       if(delayTime<0)
       {
         delayTime=0;
       }   
        break;   
      case 0x2B:  // Continuous Aquisition Mode Off
        acqMode=0;      
        break;  
     case 0x2C:  // Return Firmware Revision
         Serial.write(byte(FIRMWARE_MAJOR));  
         Serial.write(byte(FIRMWARE_MINOR));   
        break;  
     case 0x2D:  // Perform Finite Aquisition
         Serial.write('0');
         finiteAcquisition(command[2],(command[3])+(command[4]<<8),command[5]+(command[6]<<8));
        break;   
  

    /*********************************************************************************
    ** Unknown Packet
    *********************************************************************************/
    default:      // Default Case
      Serial.flush();
      break;     
    }
  }
  else{  
    // Checksum Failed, Flush Serial Buffer
    Serial.flush(); 
  }   
}

/*********************************************************************************
**  Functions
*********************************************************************************/

// Writes Values To Digital Port (DIO 0-13).  Pins Must Be Configured As Outputs Before Being Written To
void writeDigitalPort(unsigned char command[])
{
  digitalWrite(13, (( command[2] >> 5) & 0x01) );
  digitalWrite(12, (( command[2] >> 4) & 0x01) );
  digitalWrite(11, (( command[2] >> 3) & 0x01) );
  digitalWrite(10, (( command[2] >> 2) & 0x01) );
  digitalWrite(9, (( command[2] >> 1) & 0x01) );
  digitalWrite(8, (command[2] & 0x01) );
  digitalWrite(7, (( command[3] >> 7) & 0x01) );
  digitalWrite(6, (( command[3] >> 6) & 0x01) );
  digitalWrite(5, (( command[3] >> 5) & 0x01) );
  digitalWrite(4, (( command[3] >> 4) & 0x01) );
  digitalWrite(3, (( command[3] >> 3) & 0x01) );
  digitalWrite(2, (( command[3] >> 2) & 0x01) );
  digitalWrite(1, (( command[3] >> 1) & 0x01) );
  digitalWrite(0, (command[3] & 0x01) ); 
}

// Reads all 6 analog input ports, builds 8 byte packet, send via RS232.
void analogReadPort()
{
  // Read Each Analog Pin
  int pin0 = analogRead(0);
  int pin1 = analogRead(1);
  int pin2 = analogRead(2);
  int pin3 = analogRead(3);
  int pin4 = analogRead(4);
  int pin5 = analogRead(5);

  //Build 8-Byte Packet From 60 Bits of Data Read
  char output0 = (pin0 & 0xFF);   
  char output1 = ( ((pin1 << 2) & 0xFC) | ( (pin0 >> 8) & 0x03) );
  char output2 = ( ((pin2 << 4) & 0xF0) | ( (pin1 >> 6) & 0x0F) );
  char output3 = ( ((pin3 << 6) & 0xC0) | ( (pin2 >> 4) & 0x3F) );    
  char output4 = ( (pin3 >> 2) & 0xFF);    
  char output5 = (pin4 & 0xFF);
  char output6 = ( ((pin5 << 2) & 0xFC) | ( (pin4 >> 8) & 0x03) );
  char output7 = ( (pin5 >> 6) & 0x0F );

  // Write Bytes To Serial Port
  Serial.print(output0);
  Serial.print(output1);
  Serial.print(output2);
  Serial.print(output3);
  Serial.print(output4);
  Serial.print(output5);
  Serial.print(output6);
  Serial.print(output7);
}

void syncLV()
{
  Serial.begin(DEFAULTBAUDRATE); 

  Serial.flush();
}


unsigned char checksum_Compute(unsigned char command[])
{
  unsigned char checksum;
  for (int i=0; i<(COMMANDLENGTH-1); i++)
  {
    checksum += command[i]; 
  }
  return checksum;
}

int checksum_Test(unsigned char command[])
{
  unsigned char checksum = checksum_Compute(command);
  if(checksum == command[COMMANDLENGTH-1])
  {
    return 0; 
  }
  else
  {
    return 1;
  }
}


void sampleContinously()
{
   
  for(int i=0; i<iterations; i++)
  {
     retVal = analogRead(contAcqPin);
     if(contAcqSpeed>1000) //delay Microseconds is only accurate for values less that 16383
     {
       Serial.write( (retVal >> 2));
       delayMicroseconds(delayTime*1000000); //Delay for neccesary amount of time to achieve desired sample rate    
     }
     else
     {
        Serial.write( (retVal & 0xFF) );
        Serial.write( (retVal >> 8));   
        delay(delayTime*1000);
     }
  }
}
void finiteAcquisition(int analogPin, float acquisitionSpeed, int numberOfSamples)
{
  
   acquisitionPeriod=1/acquisitionSpeed;
   
  for(int i=0; i<numberOfSamples; i++)
  {
     retVal = analogRead(analogPin);
    
     if(acquisitionSpeed>1000)
     {
       Serial.write( (retVal >> 2));
       delayMicroseconds(acquisitionPeriod*1000000);
     }
     else
     {
       Serial.write( (retVal & 0xFF) );
       Serial.write( (retVal >> 8));
       delay(acquisitionPeriod*1000);
     }
  }
}

