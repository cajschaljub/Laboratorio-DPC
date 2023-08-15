#include <Arduino.h>

// put function declarations here:
void Task_Print1(void *param);
void Task_Print2(void *param);

TaskHandle_t Task_Handle1;
TaskHandle_t Task_Handle2;

int x = 0, y = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  xTaskCreatePinnedToCore(Task_Print1,"Task1",5000,NULL,2,&Task_Handle1, 0);
  xTaskCreatePinnedToCore(Task_Print2,"Task2",5000,NULL,1,&Task_Handle2, 0);
}

void loop() {
  // Nothing Here
}

void Task_Print1(void *param)
{
  (void) param;
  while(1){
   Serial.print("Task1 ");
   
   x++;
   Serial.print("X = ");
   Serial.println(x);
   if(x == 5){
    //vTaskDelete(Task_Handle2);
    vTaskSuspend(Task_Handle2);
   }
   if(x == 8){
    //xTaskCreatePinnedToCore(Task_Print2,"Task2",5000,NULL,1,&Task_Handle2, 0);
    vTaskResume(Task_Handle2);
    x = 0;
   }
   vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  
}

void Task_Print2(void *param)
{
  (void) param;
  y = 0;
  while(1){
    Serial.print("Task2 ");
    y++;
    Serial.print("Y = ");
    Serial.println(y);
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}