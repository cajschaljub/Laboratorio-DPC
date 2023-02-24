#include <Arduino.h>

#define echo 13
#define trigger 12
int Echo = 0;
int Dist = 0;
int avg = 0;
void setup() {
  // put your setup code here, to run once:
  pinMode(echo, INPUT);
  pinMode(trigger, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  int act = 0, ant = 0;
    digitalWrite(trigger, HIGH);
    delay(1);
    digitalWrite(trigger,LOW);
    Echo = pulseIn(echo,HIGH);
    Dist = Echo * 0.034 / 2;
    ant = Dist;
  for (size_t i = 0; i < 50; i++)
  {
    digitalWrite(trigger, HIGH);
    delay(1);
    digitalWrite(trigger,LOW);
    Echo = pulseIn(echo,HIGH);
    Dist = Echo * 0.034 / 2;
    Serial.print("Crudo ");
    Serial.println(Dist);
    if ((Dist - ant) < 50)
    {
      avg += Dist;
    }
    
    
  }
  
  avg = avg / 50;
  Serial.print("::: AVG: ");
  Serial.println(avg);
}