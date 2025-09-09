#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

// Pin Definitions
#define CAMERA_SENSOR_CS 10
#define LEFT_MOTOR_PWM 5
#define RIGHT_MOTOR_PWM 6
#define LEFT_MOTOR_DIR 7
#define RIGHT_MOTOR_DIR 8
#define DISTANCE_SENSOR_TRIG 9
#define DISTANCE_SENSOR_ECHO 11
#define COLOR_SENSOR_LEFT A0
#define COLOR_SENSOR_RIGHT A1
#define SERVO_GRIPPER_1 12
#define SERVO_GRIPPER_2 13
#define BORDER_COLOR_SENSOR A2

// Queue and Semaphore Handles
QueueHandle_t lineDetectionQueue;
QueueHandle_t colorBorderQueue;
QueueHandle_t distanceSensorQueue;
SemaphoreHandle_t errorHandlerMutex;

// Structs for data passing
struct LineData {
  int leftPosition;
  int rightPosition;
};

struct ColorData {
  bool isBorderDetected;
  int colorValue;
};

// Task Handles
TaskHandle_t xCameraSensorTask;
TaskHandle_t xLeftMotorTask;
TaskHandle_t xRightMotorTask;
TaskHandle_t xDistanceSensorTask;
TaskHandle_t xGripperControlTask;
TaskHandle_t xBorderColorSensorTask;
TaskHandle_t xErrorHandlerTask;

// Error Tracking
enum ErrorType {
  NO_ERROR,
  CAMERA_SENSOR_ERROR,
  MOTOR_CONTROL_ERROR,
  DISTANCE_SENSOR_ERROR,
  GRIPPER_ERROR,
  BORDER_SENSOR_ERROR
};

// Camera Sensor Task
void vCameraSensorTask(void *pvParameters) {
  LineData lineData;
  
  for (;;) {
    // Simulate ICSP camera sensor reading
    // In a real implementation, replace with actual sensor reading
    lineData.leftPosition = analogRead(CAMERA_SENSOR_CS);
    lineData.rightPosition = analogRead(CAMERA_SENSOR_CS + 1);
    
    // Send line detection data to motor tasks
    xQueueSend(lineDetectionQueue, &lineData, portMAX_DELAY);
    
    vTaskDelay(pdMS_TO_TICKS(50)); // 50ms delay
  }
}

// Left Motor Task
void vLeftMotorTask(void *pvParameters) {
  LineData lineData;
  ColorData borderData;
  
  for (;;) {
    // Receive line detection data
    if (xQueueReceive(lineDetectionQueue, &lineData, portMAX_DELAY) == pdTRUE) {
      // Implement line following logic
      int error = lineData.leftPosition - 512; // Assuming center is 512
      int motorSpeed = map(abs(error), 0, 512, 0, 255);
      
      // Motor control
      if (error > 0) {
        digitalWrite(LEFT_MOTOR_DIR, HIGH);
      } else {
        digitalWrite(LEFT_MOTOR_DIR, LOW);
      }
      analogWrite(LEFT_MOTOR_PWM, motorSpeed);
    }
    
    // Check for border
    if (xQueueReceive(colorBorderQueue, &borderData, 0) == pdTRUE) {
      if (borderData.isBorderDetected) {
        // Stop motor
        analogWrite(LEFT_MOTOR_PWM, 0);
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(20)); // 20ms delay
  }
}

// Right Motor Task
void vRightMotorTask(void *pvParameters) {
  LineData lineData;
  ColorData borderData;
  
  for (;;) {
    // Receive line detection data
    if (xQueueReceive(lineDetectionQueue, &lineData, portMAX_DELAY) == pdTRUE) {
      // Implement line following logic
      int error = lineData.rightPosition - 512; // Assuming center is 512
      int motorSpeed = map(abs(error), 0, 512, 0, 255);
      
      // Motor control
      if (error > 0) {
        digitalWrite(RIGHT_MOTOR_DIR, HIGH);
      } else {
        digitalWrite(RIGHT_MOTOR_DIR, LOW);
      }
      analogWrite(RIGHT_MOTOR_PWM, motorSpeed);
    }
    
    // Check for border
    if (xQueueReceive(colorBorderQueue, &borderData, 0) == pdTRUE) {
      if (borderData.isBorderDetected) {
        // Stop motor
        analogWrite(RIGHT_MOTOR_PWM, 0);
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(20)); // 20ms delay
  }
}

// Distance Sensor Task
void vDistanceSensorTask(void *pvParameters) {
  long duration, distance;
  
  for (;;) {
    // Trigger ultrasonic sensor
    digitalWrite(DISTANCE_SENSOR_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(DISTANCE_SENSOR_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(DISTANCE_SENSOR_TRIG, LOW);
    
    // Measure distance
    duration = pulseIn(DISTANCE_SENSOR_ECHO, HIGH);
    distance = duration * 0.034 / 2; // Convert to cm
    
    if (distance < 10) { // Box detected within 10 cm
      // Send distance data to queue
      xQueueSend(distanceSensorQueue, &distance, portMAX_DELAY);
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay
  }
}

// Gripper Control Task
void vGripperControlTask(void *pvParameters) {
  long distance;
  
  for (;;) {
    // Wait for distance data
    if (xQueueReceive(distanceSensorQueue, &distance, portMAX_DELAY) == pdTRUE) {
      // Close gripper
      for (int pos = 0; pos <= 180; pos += 1) {
        analogWrite(SERVO_GRIPPER_1, map(pos, 0, 180, 0, 255));
        analogWrite(SERVO_GRIPPER_2, map(pos, 0, 180, 0, 255));
        vTaskDelay(pdMS_TO_TICKS(15));
      }
      
      // Lift gripper
      for (int pos = 0; pos <= 90; pos += 1) {
        analogWrite(SERVO_GRIPPER_1, map(pos, 0, 90, 0, 255));
        vTaskDelay(pdMS_TO_TICKS(15));
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(50)); // 50ms delay
  }
}

// Border Color Sensor Task
void vBorderColorSensorTask(void *pvParameters) {
  ColorData colorData;
  
  for (;;) {
    // Read color sensor
    colorData.colorValue = analogRead(BORDER_COLOR_SENSOR);
    
    // Detect border (adjust threshold as needed)
    colorData.isBorderDetected = (colorData.colorValue > 700);
    
    if (colorData.isBorderDetected) {
      // Send stop signal to motors
      xQueueSend(colorBorderQueue, &colorData, portMAX_DELAY);
    }
    
    vTaskDelay(pdMS_TO_TICKS(50)); // 50ms delay
  }
}

// Error Handler Task
void vErrorHandlerTask(void *pvParameters) {
  ErrorType currentError = NO_ERROR;
  
  for (;;) {
    if (xSemaphoreTake(errorHandlerMutex, portMAX_DELAY) == pdTRUE) {
      // Simulate error checking (replace with actual error detection)
      // This is a placeholder for actual error detection mechanisms
      
      switch (currentError) {
        case CAMERA_SENSOR_ERROR:
          // Handle camera sensor error
          break;
        case MOTOR_CONTROL_ERROR:
          // Handle motor control error
          break;
        case DISTANCE_SENSOR_ERROR:
          // Handle distance sensor error
          break;
        case GRIPPER_ERROR:
          // Handle gripper error
          break;
        case BORDER_SENSOR_ERROR:
          // Handle border sensor error
          break;
        default:
          break;
      }
      
      xSemaphoreGive(errorHandlerMutex);
    }
    
    vTaskDelay(pdMS_TO_TICKS(200)); // 200ms delay
  }
}

void setup() {
  // Initialize pins
  pinMode(CAMERA_SENSOR_CS, INPUT);
  pinMode(LEFT_MOTOR_PWM, OUTPUT);
  pinMode(RIGHT_MOTOR_PWM, OUTPUT);
  pinMode(LEFT_MOTOR_DIR, OUTPUT);
  pinMode(RIGHT_MOTOR_DIR, OUTPUT);
  pinMode(DISTANCE_SENSOR_TRIG, OUTPUT);
  pinMode(DISTANCE_SENSOR_ECHO, INPUT);
  pinMode(SERVO_GRIPPER_1, OUTPUT);
  pinMode(SERVO_GRIPPER_2, OUTPUT);
  
  // Create queues
  lineDetectionQueue = xQueueCreate(1, sizeof(LineData));
  colorBorderQueue = xQueueCreate(1, sizeof(ColorData));
  distanceSensorQueue = xQueueCreate(1, sizeof(long));
  
  // Create mutex
  errorHandlerMutex = xSemaphoreCreateMutex();
  
  // Create tasks
  xTaskCreate(vCameraSensorTask, "Camera Sensor", 128, NULL, 3, &xCameraSensorTask);
  xTaskCreate(vLeftMotorTask, "Left Motor", 128, NULL, 2, &xLeftMotorTask);
  xTaskCreate(vRightMotorTask, "Right Motor", 128, NULL, 2, &xRightMotorTask);
  xTaskCreate(vDistanceSensorTask, "Distance Sensor", 128, NULL, 1, &xDistanceSensorTask);
  xTaskCreate(vGripperControlTask, "Gripper Control", 128, NULL, 2, &xGripperControlTask);
  xTaskCreate(vBorderColorSensorTask, "Border Color Sensor", 128, NULL, 1, &xBorderColorSensorTask);
  xTaskCreate(vErrorHandlerTask, "Error Handler", 128, NULL, 4, &xErrorHandlerTask);
  
  // Start the scheduler
  vTaskStartScheduler();
}

void loop() {
  // Empty. FreeRTOS will manage tasks
}