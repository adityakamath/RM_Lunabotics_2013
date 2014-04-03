/*********************************************************************************
How it works:
The first byte (command[0]) that is sent bt LW is the sync
bit and the next is the with that determines what the arduino must do
that is, the switch- case bit. 
The last bit is the parity bit
all the bits in between are the bits we can use to do our functions
 *********************************************************************************/


#include <LiquidCrystal.h>



// Variables
char msg[13]="";
unsigned int retVal;
int sevenSegmentPins[8];
int currentMode;
unsigned int freq;
unsigned long duration;
int i2cReadTimeouts = 0;
char spiBytesToSend = 0;
char spiBytesSent = 0;
char spiCSPin = 0;
char spiWordSize = 0;
byte customChar[8];
LiquidCrystal lcd(0,0,0,0,0,0,0);
// Sets the mode of the Arduino (Reserved For Future Use)
void setMode(int mode)
{
  currentMode = mode;
}

// Checks for new commands from LabVIEW and processes them if any exists.
int checkForCommand(void)
{  

  int bufferBytes = Serial.available();

  if(bufferBytes >= COMMANDLENGTH) 
  {
    // New Command Ready, Process It  
    // Build Command From Serial Buffer
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

// Processes a given command
void processCommand(unsigned char command[])
{  
  // Determine Command
  if(command[0] == 0xFF && checksum_Test(command) == 0)
  {
    switch(command[1])
    {    
    /*********************************************************************************
    ** LIFA Maintenance Commands
    *********************************************************************************/
    case 0x00:     // Sync Packet
      Serial.print("sync");
      Serial.flush();        
      break;
    case 0x01:    // Flush Serial Buffer  
      Serial.flush();
      break;
      
    /*********************************************************************************
    ** Low Level - Digital I/O Commands
    *********************************************************************************/
    case 0x02:    // Set Pin As Input Or Output      
      pinMode(command[2], command[3]);
      Serial.write('0');
      break;
    case 0x03:    // Write Digital Pin
      digitalWrite(command[2], command[3]);
       Serial.write('0');
      break;
    case 0x04:    // Write Digital Port 0
      writeDigitalPort(command);
       Serial.write('0');
      break;
    case 0x05:    //Tone          
      freq = ( (command[3]<<8) + command[4]);
      duration=(command[8]+  (command[7]<<8)+ (command[6]<<16)+(command[5]<<24));   
   
      if(freq > 0)
      {
        tone(command[2], freq, duration); 
      }
      else
      {
        noTone(command[2]);
      }    
       Serial.write('0');
      break;
    case 0x06:    // Read Digital Pin
      retVal = digitalRead(command[2]);  
      Serial.write(retVal);    
      break;
    case 0x07:    // Digital Read Port
      retVal = 0x0000;
      for(int i=0; i <=13; i++)
      {
        if(digitalRead(i))
        {
          retVal += (1<<i);
        } 
      }
      Serial.write( (retVal & 0xFF));
      Serial.write( (retVal >> 8));    
      break;
      
    /*********************************************************************************
    ** Low Level - Analog Commands
    *********************************************************************************/
    case 0x08:    // Read Analog Pin    
      retVal = analogRead(command[2]);
      Serial.write( (retVal >> 8));
      Serial.write( (retVal & 0xFF));
      break;
    case 0x09:    // Analog Read Port
      analogReadPort();
      break; 
      
    /*********************************************************************************
    ** Low Level - PWM Commands
    *********************************************************************************/      
    case 0x0A:    // PWM Write Pin
      analogWrite(command[2], command[3]);
       Serial.write('0');
      break;
    case 0x0B:    // PWM Write 3 Pins
      analogWrite(command[2], command[5]);
      analogWrite(command[3], command[6]);
      analogWrite(command[4], command[7]);
       Serial.write('0');
      break;
      
    /*********************************************************************************
    ** Sensor Specific Commands
    *********************************************************************************/      
    case 0x0C:    // Configure Seven Segment Display
      sevenSegment_Config(command);
       Serial.write('0');
      break;
    case 0x0D:    // Write To Seven Segment Display
      sevenSegment_Write(command);
       Serial.write('0');
      break;
 /*********************************************************************************
 **                      Custom Commands 14-29;0x0E-0x1D 
 *********************************************************************************/  
      
     case 0x0E:  // Serial read from LavVIEW
         for(int i=0;i<command[2];i++)     //String is recieved from LW 
         msg[i]=command[i+3];             //Max length 11 charecters
         break; 
        
        
       case 0x0F:  // Serial Write to LabVIEW
         Serial.print("Hello LabVIEW");  //The exact length of this string must be
         break;                         //Given to bites to be read of arduino READ WRITE tool
               
      case 0x10: //Read and write back (serial monitor)
         for(int i=0;i<command[2];i++)     //String is recieved from LW and sent back.
         msg[i]=command[i+3];           //Max length 11 charecters
         Serial.print(msg);
         break;
     /*********************************************************************************
         **                                      LCD
         *********************************************************************************/  
    case 0x1E:  // LCD Init
        lcd.init(command[2], command[3], command[4], command[5], command[6], command[7], command[8], command[9], command[10], command[11], command[12], command[13]);
       
        Serial.write('0');
        break;
      case 0x1F:  // LCD Set Size
        lcd.begin(command[2], command[3]);
        Serial.write('0');
        break;
      case 0x20:  // LCD Set Cursor Mode
        if(command[2] == 0)
        {
          lcd.noCursor(); 
        }
        else
        {
          lcd.cursor(); 
        }
        if(command[3] == 0)
        {
          lcd.noBlink(); 
        }
        else
        {
          lcd.blink(); 
        }
        Serial.write('0');
        break; 
      case 0x21:  // LCD Clear
        lcd.clear();
        Serial.write('0');
        break;
      case 0x22:  // LCD Set Cursor Position
        lcd.setCursor(command[2], command[3]);
        Serial.write('0');
        break;
      case 0x23:  // LCD Print
        lcd_print(command);        
        break;
      case 0x24:  // LCD Display Power
        if(command[2] == 0)
        {
          lcd.noDisplay(); 
        }
        else
        {
          lcd.display();
        }
        Serial.write('0');
        break;
      case 0x25:  // LCD Scroll
        if(command[2] == 0)
        {
          lcd.scrollDisplayLeft(); 
        }
        else
        {
          lcd.scrollDisplayRight();
        }
        Serial.write('0');
        break;
      case 0x26: //  LCD Autoscroll
        if(command[2] == 0)
        {
          lcd.noAutoscroll(); 
        }
        else
        {
          lcd.autoscroll(); 
        }
        Serial.write('0');
        break;
      case 0x27: // LCD Print Direction
        if(command[2] == 0)
        {
          lcd.rightToLeft(); 
        }
        else
        {
          lcd.leftToRight(); 
        }
        Serial.write('0');
        break;
      case 0x28: // LCD Create Custom Char
        for(int i=0; i<8; i++)
        {
          customChar[i] = command[i+3];
        }      
        lcd.createChar(command[2], customChar);
        
        Serial.write('0');
        break;
      case 0x29:  // LCD Print Custom Char
        lcd.write(command[2]);
        Serial.write('0');
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

// Configure digital I/O pins to use for seven segment display
void sevenSegment_Config(unsigned char command[])
{
  // Configure pins as outputs and store in sevenSegmentPins array for use in sevenSegment_Write
  for(int i=2; i<10; i++)
  {
    pinMode(command[i], OUTPUT); 
    sevenSegmentPins[(i-1)] = command[i];
  }  
}

//  Write values to sevenSegment display.  Must first use sevenSegment_Configure
void sevenSegment_Write(unsigned char command[])
{
  for(int i=1; i<9; i++)
  {
    digitalWrite(sevenSegmentPins[(i-1)], command[i]);
  }
}



// Synchronizes with LabVIEW and sends info about the board and firmware (Unimplemented)
void syncLV()
{
  Serial.begin(DEFAULTBAUDRATE); 

  Serial.flush();
}

// Compute Packet Checksum
unsigned char checksum_Compute(unsigned char command[])
{
  unsigned char checksum;
  for (int i=0; i<(COMMANDLENGTH-1); i++)
  {
    checksum += command[i]; 
  }
  return checksum;
}

// Compute Packet Checksum And Test Against Included Checksum
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
  //want to exit this loop every 8ms
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

void lcd_print(unsigned char command[])
{
  if(command[2] != 0)
  {          
    // Base Specified By User
    int base = 0;
    switch(command[2])
    {
      case 0x01:  // BIN
        base = BIN;
        break;
      case 0x02:  // DEC
        base = DEC;
        break;
      case 0x03:  // OCT
        base = OCT;
        break;
      case 0x04:  // HEX
        base = HEX;
        break;
      default:
        break;
    }
    for(int i=0; i<command[3]; i++)
    {
      lcd.print(command[i+4], base);
    } 
  }
  else
  {
    
    for(int i=0; i<command[3]; i++)
    {
      lcd.print((char)command[i+4]);
    
    } 
  }
  Serial.write('0');
}






