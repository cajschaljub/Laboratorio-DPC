#include <Arduino.h>

void Task_Led1(void *param);
void Task_Led2(void *param);
SemaphoreHandle_t mutex;

TaskHandle_t Led1;
TaskHandle_t Led2;
void setup() {
  // put your setup code here, to run once:
  pinMode(4,OUTPUT);
  pinMode(4,OUTPUT);
  Serial.begin(115200);
  xTaskCreatePinnedToCore(Task_Led1,"Task1",5000,NULL,1,&Led1, 0);
  xTaskCreatePinnedToCore(Task_Led2,"Task2",5000,NULL,1,&Led2, 0);
}

void loop() {
  // No Code
}

void Task_Led1(void *param){
  (void) param;
  pinMode(4, OUTPUT);
  int x = 0;
  while (1)
  {
    x++;
    digitalWrite(4,HIGH);
    vTaskDelay(500/portTICK_PERIOD_MS);
    digitalWrite(4,LOW);
    vTaskDelay(500/portTICK_PERIOD_MS);
    if (x == 4)
    {
      vTaskSuspend(Led2);
    } 
    if (x >= 8)
    {
      vTaskResume(Led2);
      x = 0;
    }
    
  }
  
}
void Task_Led2(void *param){
  (void) param;
  pinMode(2, OUTPUT);
  while (1)
  {
    digitalWrite(2,HIGH);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    digitalWrite(2,LOW);
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}