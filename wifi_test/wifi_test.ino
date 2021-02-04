#include <SoftwareSerial.h>


#define DEBUG true

#define IDLE 0
#define ENTERING_1 1
#define ENTERING_2 2
#define ENTERING_3 3
#define EXITING_1 4
#define EXITING_2 5
#define EXITING_3 6

SoftwareSerial ESPserial(2, 3); // RX | TX


int entries[100];
int exits[100];

int minEntry = 99999;
int maxExit = -99999;

void setup()

{

  Serial.begin(9600); // communication with the host computer
  
  //while (!Serial) { ; }
  
  // Start the software serial for communication with the ESP8266
  
  ESPserial.begin(9600);
  pinMode(11,OUTPUT);    /////used if connecting a LED to pin 11
  digitalWrite(11,LOW);

  
  sendData("AT+RST\r\n",2000,DEBUG); // reset module
  sendData("AT+CWMODE=2\r\n",1000,DEBUG); // configure as access point
  sendData("AT+CIFSR\r\n",1000,DEBUG); // get ip address
  sendData("AT+CIPMUX=1\r\n",1000,DEBUG); // configure for multiple connections
  sendData("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // turn on server on port 80
  
  Serial.println("");
  
  Serial.println("Remember to to set Both NL & CR in the serial monitor.");
  
  Serial.println("Ready");
  
  Serial.println("");

}

float sensetemp() ///////function to sense temperature.
 {
  int val = analogRead(A0);
  float mv = ( val/1024.0)*5000; 
  float celcius = mv/10;
  return(celcius);
 }


int connectionId;

void loop()
{
  enterExitDetect();
   if(ESPserial.available());
  {
    /////////////////////Recieving from web browser to toggle led
    if(ESPserial.find("+IPD,"))
    {
     delay(300);
     connectionId = ESPserial.read()-48;
     if(ESPserial.find("pin="))
     { 
       Serial.println("recieving data from web browser");
       int pinNumber = (ESPserial.read()-48)*10; 
       pinNumber += (ESPserial.read()-48); 
       digitalWrite(pinNumber, !digitalRead(pinNumber));
     }
   
    /////////////////////Sending data to browser
    else
    {
      String webpage = "<h1>Hello World</h1>";
      espsend(webpage);
     }
    
     if(sensetemp() != 0)
     {
       String add1="<button onclick=>Turn On</button>";
      String two =  String(sensetemp(), 3);
      add1+= two;
      add1+="&#x2103";   //////////Hex code for degree celcius
      espsend(add1);
     }
    
     else
     {
      String c="sensor is not conneted";
      espsend(c);                                     
     } 
     
     String closeCommand = "AT+CIPCLOSE=";  ////////////////close the socket connection////esp command 
     closeCommand+=connectionId; // append connection id
     closeCommand+="\r\n";
     sendData(closeCommand,3000,DEBUG);
    }
  }

}

  //////////////////////////////sends data from ESP to webpage///////////////////////////
 
 void espsend(String d)
         {
             String cipSend = " AT+CIPSEND=";
             cipSend += connectionId; 
             cipSend += ",";
             cipSend +=d.length();
             cipSend +="\r\n";
             sendData(cipSend,1000,DEBUG);
             sendData(d,1000,DEBUG); 
         }

//////////////gets the data from esp and displays in serial monitor///////////////////////         
String sendData(String command, const int timeout, boolean debug)
            {
                String response = "";
                ESPserial.print(command);
                long int time = millis();
                while( (time+timeout) > millis())
                {
                   while(ESPserial.available())
                      {
                         char c = ESPserial.read(); // read the next character.
                         response+=c;
                      }  
                }
                
                if(debug)
                     {
                     Serial.print(response); //displays the esp response messages in arduino Serial monitor
                     }
                return response;
            }



int outSensorValue = 0;
int inSensorValue = 0;
int state = IDLE;
void enterExitDetect()
{
  outSensorValue = readSensor(A1);
  inSensorValue = readSensor(A0);
  switch(state)
  {
    case IDLE:
    {
      Serial.println("Idle");
      if(outSensorValue == 1 && inSensorValue == 0)
      {
        state = ENTERING_1;
      }
      else if(outSensorValue == 0 && inSensorValue == 1)
      {
        state = EXITING_1;
      }
      break;
    }
    case ENTERING_1:
    {
      Serial.println("Entering 1");
      if(outSensorValue == 0 && inSensorValue == 0)
      {
        state = IDLE;
      }
      else if(outSensorValue == 1 && inSensorValue == 1)
      {
        state = ENTERING_2;
      }
      break;
    }
    case ENTERING_2:
    {
      Serial.println("Entering 2");
      if(outSensorValue == 1 && inSensorValue == 0)
      {
        state = ENTERING_1;
      }
      else if(outSensorValue == 0 && inSensorValue == 1)
      {
        state = ENTERING_3;
      }
      break;
    }
    case ENTERING_3:
    {
      Serial.println("Entering 3");
      if(outSensorValue == 0 && inSensorValue == 0)
      {
        Serial.println("Enered");
        state = IDLE;
      }
      else if(outSensorValue == 1 && inSensorValue == 1)
      {
        state = ENTERING_2;
      }
      break;
    }
    case EXITING_1:
    {
      Serial.println("Exiting 1");
      if(outSensorValue == 0 && inSensorValue == 0)
      {
        state = IDLE;
      }
      else if(outSensorValue == 1 && inSensorValue == 1)
      {
        state = EXITING_2;
      }
      break;
    }
    case EXITING_2:
    {
      Serial.println("Exiting 2");
      if(outSensorValue == 1 && inSensorValue == 0)
      {
        state = EXITING_3;
      }
      else if(outSensorValue == 0 && inSensorValue == 1)
      {
        state = EXITING_1;
      }
      break;
    }
    case EXITING_3:
    {
      Serial.println("Exiting 3");
      if(outSensorValue == 0 && inSensorValue == 0)
      {
        Serial.println("Exited");
        state = IDLE;
      }
      else if(outSensorValue == 1 && inSensorValue == 1)
      {
        state = EXITING_2;
      }
      break;
    }
    default:
    {
      state = IDLE;
      break;
    }
  }
  
}
            
int readSensor(int pin) {
  int value = analogRead(pin);
  return value < 300;
}
void onEnterDetected(int et){
  if (et < minEntry){
      minEntry = et;
  }
  for (int i = 0; i <= 100; i++) {
    if (entries[i] != 0){
      entries[i] = et;
    }
  }    
}


void onExitDetected(int et){
   if (et > maxExit){
      maxExit = et;
  }
  for (int i = 0; i <= 100; i++) {   
    if (entries[i] != 0){
      entries[i] = et;
    }
  }    
}

void observeTime(){
  // change this
  int currentTime = 0;

  if (minEntry < currentTime && maxExit > currentTime){
    turnOnLed();
  }else{
    turnOffLed();
  }
}

void turnOffLed(){
    digitalWrite(LED_BUILTIN, LOW);
}

void turnOnLed(){
    digitalWrite(LED_BUILTIN, HIGH);
}
