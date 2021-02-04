

#include <SoftwareSerial.h>


#define DEBUG true

#define LED_PORT 6

#define IDLE 0
#define ENTERING_1 1
#define ENTERING_2 2
#define ENTERING_3 3
#define EXITING_1 4
#define EXITING_2 5
#define EXITING_3 6

#define DAYS_COUNT 5

/*
#define PERIOD 900000
#define PER_COUNT 96
*/

#define PERIOD 5000
#define PER_COUNT 2

SoftwareSerial ESPserial(2, 3); // RX | TX


float periodsPeople[PER_COUNT];

int peopleCount;
int currentPeriod;
int lastPeriodMod;
int currPeriodMod;

int minEntry = 99999;
int maxExit = -99999;

int i;


int state = IDLE;

void setup()

{
  peopleCount = 0;
  currentPeriod = 0;
  lastPeriodMod = 0;
  currPeriodMod = 0;
  
  for(i = 0 ; i < PER_COUNT ; i++)
  {
    periodsPeople[i] = 0;
  }

  Serial.begin(9600); // communication with the host computer
  
  //while (!Serial) { ; }
  
  // Start the software serial for communication with the ESP8266
  
  ESPserial.begin(9600);
  pinMode(LED_PORT,OUTPUT);

  
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

int connectionId;

void loop()
{
   enterExitDetect();
   updatePeriod();
   if(ESPserial.available());
  {
    /////////////////////Recieving from web browser to toggle led
    if(ESPserial.find("+IPD,"))
    {
     delay(10);
     connectionId = ESPserial.read()-48;
     if(ESPserial.find("pin="))
     { 
       Serial.println("recieving data from web browser");
       int pinNumber = (ESPserial.read()-48)*10; 
       pinNumber += (ESPserial.read()-48); 
       digitalWrite(pinNumber, !digitalRead(pinNumber));
     }
   
    /////////////////////Sending data to browser
    
      String add1="<h2>Room Air Conditioner Controller</h2>";
      String count =  "People in the room : " + String(peopleCount, 3) + "<br/>";
      String ave =  "Average of People which have been in the room in the past 5 Days : " + String(periodsPeople[currentPeriod], 3) + "<br/>";
      add1+= count;  
      add1 += ave;
      espsend(add1);
     
     String closeCommand = "AT+CIPCLOSE=";  ////////////////close the socket connection////esp command 
     closeCommand+=connectionId; // append connection id
     closeCommand+="\r\n";
     sendData(closeCommand,3000,DEBUG);
    }
  }
  log();
  
}

char* getState()
{
  switch(state)
  {
    case IDLE:
    {
      return "idle";
      break;
    }
    case ENTERING_1:
    {
      return "entering 1";
      break;
    }
    case ENTERING_2:
    {
      return "entering 2";
      break;
    }
    case ENTERING_3:
    {
      return "entering 3";
      break;
    }
    case EXITING_1:
    {
      return "exiting 1";
      break;
    }
    case EXITING_2:
    {
      return "exiting 2";
      break;
    }
    case EXITING_3:
    {
      return "exiting 3";
      break;
    }
    default:
    {
      return "default";
      break;
    }
  }
}

void log()
{
  Serial.print("period : ");
  Serial.print(currentPeriod);
  Serial.print(" - People in room : ");
  Serial.print(peopleCount);
  Serial.print(" - People record : ");
  Serial.print(periodsPeople[currentPeriod]);
  Serial.print(" - Next period's record : ");
  if(currentPeriod + 1 < PER_COUNT)
  {
    Serial.print(periodsPeople[currentPeriod + 1]);
  }
  else
  {
    Serial.print(periodsPeople[0]);
  }
  Serial.print(" - State : ");
  Serial.print(getState());
  Serial.print("\n");
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
void enterExitDetect()
{
  outSensorValue = readSensor(A1);
  inSensorValue = readSensor(A0);
  switch(state)
  {
    case IDLE:
    {
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
      if(outSensorValue == 0 && inSensorValue == 0)
      {
        state = IDLE;
      }
      else if(outSensorValue == 1 && inSensorValue == 1)
      {
        state = ENTERING_2;
      }
      else if(outSensorValue == 0 && inSensorValue == 1)
      {
        state = IDLE;
      }
      break;
    }
    case ENTERING_2:
    {
      if(outSensorValue == 1 && inSensorValue == 0)
      {
        state = ENTERING_1;
      }
      else if(outSensorValue == 0 && inSensorValue == 1)
      {
        state = ENTERING_3;
      }
      else if(outSensorValue == 0 && inSensorValue == 0)
      {
        state = IDLE;
      }
      break;
    }
    case ENTERING_3:
    {
      if(outSensorValue == 0 && inSensorValue == 0)
      {
        Serial.println("Enered");
        onEnterDetected();
        setLed();
        state = IDLE;
      }
      else if(outSensorValue == 1 && inSensorValue == 1)
      {
        state = ENTERING_2;
      }
      else if(outSensorValue == 1 && inSensorValue == 0)
      {
        state = IDLE;
      }
      break;
    }
    case EXITING_1:
    {
      if(outSensorValue == 0 && inSensorValue == 0)
      {
        state = IDLE;
      }
      else if(outSensorValue == 1 && inSensorValue == 1)
      {
        state = EXITING_2;
      }
      else if(outSensorValue == 1 && inSensorValue == 0)
      {
        state = IDLE;
      }
      break;
    }
    case EXITING_2:
    {
      if(outSensorValue == 1 && inSensorValue == 0)
      {
        state = EXITING_3;
      }
      else if(outSensorValue == 0 && inSensorValue == 1)
      {
        state = EXITING_1;
      }
      else if(outSensorValue == 0 && inSensorValue == 0)
      {
        state = IDLE;
      }
      break;
    }
    case EXITING_3:
    {
      if(outSensorValue == 0 && inSensorValue == 0)
      {
        Serial.println("Exited");
        onExitDetected();
        setLed();
        state = IDLE;
      }
      else if(outSensorValue == 1 && inSensorValue == 1)
      {
        state = EXITING_2;
      }
      else if(outSensorValue == 0 && inSensorValue == 1)
      {
        state = IDLE;
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

void updatePeriod()
{
  if(checkPeriodShift())
  {
    currentPeriod++;
    currentPeriod = currentPeriod % PER_COUNT;
    setPeopleInCurrentPeriod();
    setLed();
  }
}

void setPeopleInCurrentPeriod()
{
  if(periodsPeople[currentPeriod] > 0)
  {
    periodsPeople[currentPeriod] = (periodsPeople[currentPeriod] * (DAYS_COUNT - 1) + peopleCount) / DAYS_COUNT;
  }
  else
  {
    periodsPeople[currentPeriod] = peopleCount;
  }
}

bool checkPeriodShift()
{
  lastPeriodMod = currPeriodMod;
  currPeriodMod = millis() % PERIOD;
  if(lastPeriodMod > currPeriodMod)
  {
    return true;
  }
  return false;
}
            
int readSensor(int pin) {
  int value = analogRead(pin);
  return value < 300;
}


void onEnterDetected(){
  peopleCount++;
}


void onExitDetected(){
   if(peopleCount > 0)
   {
     peopleCount--;
   }
}

void setLed()
{
  if(peopleCount > 0)
  {
    turnOnLed();
  }
  else
  {  
    if(currentPeriod + 1 < PER_COUNT)
    {
      if(periodsPeople[currentPeriod + 1] > 0.5)
      {
        turnOnLed();
      }
      else
      {
        turnOfffLed();
      }
    }
    else
    {
      if(periodsPeople[0] > 0.5)
      {
        turnOnLed();
      }
      else
      {
        turnOfffLed();
      }
    }
  }
}

void turnOfffLed(){
    digitalWrite(LED_PORT, LOW);
}

void turnOnLed(){
    digitalWrite(LED_PORT, HIGH);
}
