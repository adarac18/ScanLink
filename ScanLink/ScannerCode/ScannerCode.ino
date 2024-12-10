#include <collections.h>
#include <fmath.h>
#include <quirc_internal.h>
#include "quirc.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

/* ======================================== Wi-Fi Credentials */
const char* ssid = "*****";
const char* password = "*****"; 
const char* serverURL = "http://172.20.10.9:5000/verify"; // Flask server address
/* ======================================== */

/* ======================================== */ //LED Pins
const int GLED = 13; //GPIO 13
const int RLED = 2; //GPIO 2
/* ======================================== */


// creating a task handle
TaskHandle_t QRCodeReader_Task; 

/* ======================================== Select camera model */
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WITHOUT_PSRAM
//#define CAMERA_MODEL_M5STACK_WITHOUT_PSRAM
#define CAMERA_MODEL_AI_THINKER

//#define LED_FLASH_GPIO 4
/* ======================================== */

/* ======================================== GPIO of camera models */
#if defined(CAMERA_MODEL_WROVER_KIT)
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM    21
  #define SIOD_GPIO_NUM    26
  #define SIOC_GPIO_NUM    27
  
  #define Y9_GPIO_NUM      35
  #define Y8_GPIO_NUM      34
  #define Y7_GPIO_NUM      39
  #define Y6_GPIO_NUM      36
  #define Y5_GPIO_NUM      19
  #define Y4_GPIO_NUM      18
  #define Y3_GPIO_NUM       5
  #define Y2_GPIO_NUM       4
  #define VSYNC_GPIO_NUM   25
  #define HREF_GPIO_NUM    23
  #define PCLK_GPIO_NUM    22

#elif defined(CAMERA_MODEL_ESP_EYE)
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM    4
  #define SIOD_GPIO_NUM    18
  #define SIOC_GPIO_NUM    23
  
  #define Y9_GPIO_NUM      36
  #define Y8_GPIO_NUM      37
  #define Y7_GPIO_NUM      38
  #define Y6_GPIO_NUM      39
  #define Y5_GPIO_NUM      35
  #define Y4_GPIO_NUM      14
  #define Y3_GPIO_NUM      13
  #define Y2_GPIO_NUM      34
  #define VSYNC_GPIO_NUM   5
  #define HREF_GPIO_NUM    27
  #define PCLK_GPIO_NUM    25

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_M5STACK_WITHOUT_PSRAM)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       17
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "Camera model not selected"
#endif
/* ======================================== */

/* ======================================== Variables declaration */
struct QRCodeData
{
  bool valid;
  int dataType;
  uint8_t payload[1024];
  int payloadLen;
};

struct quirc *q = NULL;
uint8_t *image = NULL;  
camera_fb_t * fb = NULL;
struct quirc_code code;
struct quirc_data data;
quirc_decode_error_t err;
struct QRCodeData qrCodeData;  
String QRCodeResult = "";
/* ======================================== */
// Initialize Wi-Fi connection
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}


/* ________________________________________________________________________________ VOID SETTUP() */
void setup() {
  // Disable brownout detector.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  /* ---------------------------------------- Init serial communication speed (baud rate). */
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  /* ---------------------------------------- */

  //Initialize LEDs
  //pinMode(LED_FLASH_GPIO, OUTPUT);
 // digitalWrite(LED_FLASH_GPIO, LOW);
  pinMode(GLED, OUTPUT);
  pinMode(RLED, OUTPUT);

  //Verify Green and Red LEDs are functional
    for(int i = 0; i < 3; i++){
      digitalWrite(GLED, LOW);
      digitalWrite(RLED, LOW);
      delay(500);
      digitalWrite(GLED, HIGH);
      digitalWrite(RLED, HIGH);
      delay(500);
    }
    //turn off LEDs after verified
    digitalWrite(GLED, LOW);
    digitalWrite(RLED, LOW);


  // Connect to WiFi
  connectToWiFi();


  /* ---------------------------------------- Camera configuration. */
  Serial.println("Start configuring and initializing the camera...");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 15;
  config.fb_count = 1;
  
  #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
  #endif

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
  
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
  
  Serial.println("Configure and initialize the camera successfully.");
  Serial.println();

  /* ---------------------------------------- */

  /* ---------------------------------------- create "QRCodeReader_Task" using the xTaskCreatePinnedToCore() function */
  xTaskCreatePinnedToCore(
             QRCodeReader,          /* Task function. */
             "QRCodeReader_Task",   /* name of task. */
             10000,                 /* Stack size of task */
             NULL,                  /* parameter of the task */
             1,                     /* priority of the task */
             &QRCodeReader_Task,    /* Task handle to keep track of created task */
             0);                    /* pin task to core 0 */
  /* ---------------------------------------- */

// Call the function with the QR code data
      checkQRCodeInDatabase(QRCodeResult);
           // checkQRCodeInDatabase("45678, Ben Smith");

      

}
/* ________________________________________________________________________________ */
void loop() {
  if (QRCodeResult != "") {
    // When QR code is decoded, check it in the database
    checkQRCodeInDatabase(QRCodeResult);
     QRCodeResult = ""; // Reset after checking
  }

  delay(100);  // Avoid blocking other tasks
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ The function to be executed by "QRCodeReader_Task" */
// This function is to instruct the camera to take or capture a QR Code image, then it is processed and translated into text.
void QRCodeReader( void * pvParameters ){
  /* ---------------------------------------- */
  Serial.println("QRCodeReader is ready.");
  Serial.print("QRCodeReader running on core ");
  Serial.println(xPortGetCoreID());
  Serial.println();
  /* ---------------------------------------- */

  /* ---------------------------------------- Loop to read QR Code in real time. */
  while(1){
      q = quirc_new();
      if (q == NULL){
        Serial.print("can't create quirc object\r\n");  
        continue;
      }
    
      fb = esp_camera_fb_get();
      if (!fb)
      {
        Serial.println("Camera capture failed");
        continue;
      }   
      
      quirc_resize(q, fb->width, fb->height);
      image = quirc_begin(q, NULL, NULL);
      memcpy(image, fb->buf, fb->len);
      quirc_end(q);
      
      int count = quirc_count(q);
      if (count > 0) {
        quirc_extract(q, 0, &code);
        err = quirc_decode(&code, &data);
    
        if (err){
          Serial.println("Decoding FAILED");
          QRCodeResult = "Decoding FAILED";
        } else {
          Serial.printf("Decoding successful:\n");
          dumpData(&data);
        } 
        Serial.println();
      } 
      
      esp_camera_fb_return(fb);
      fb = NULL;
      image = NULL;  
      quirc_destroy(q);

      vTaskDelay(100 / portTICK_PERIOD_MS); //Add delay (100 ms)
  }
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Function to display the results of reading the QR Code on the serial monitor. */
void dumpData(const struct quirc_data *data)
{
  //Serial.printf("Version: %d\n", data->version);
  //Serial.printf("ECC level: %c\n", "MLHQ"[data->ecc_level]);
  //Serial.printf("Mask: %d\n", data->mask);
 // Serial.printf("Length: %d\n", data->payload_len);
  Serial.printf("Payload: %s\n", data->payload);
  QRCodeResult = (const char *)data->payload;
  


}

/* ======================================== */

void checkQRCodeInDatabase(String qrCodeData) {
  HTTPClient http;
    qrCodeData.trim();
  // Split the qrCodeData into id and name
  int commaIndex = qrCodeData.indexOf(',');
  if (commaIndex == -1) {
   Serial.println("Error: QR code data is not formatted correctly.");
   return;
  }

  String id = qrCodeData.substring(0, commaIndex);
  String name = qrCodeData.substring(commaIndex + 2);

  // URL encode the name (replace spaces with %20)
  String encodedName = name;
  encodedName.replace(" ", "%20");

  // Remove any leading/trailing spaces
  id.trim();
  name.trim();

  // Construct the URL with id and name as separate query parameters
  String url = String(serverURL) + "?id=" + id + "&name=" + encodedName;

  // Debug print the constructed URL
  Serial.println("Constructed URL: " + url);

  http.begin(url); // Initialize the HTTP client


  int httpCode = http.GET(); // Make a GET request

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString(); // Get the response body
    Serial.println("Server response: " + payload);
    if (payload == "verified") {
      Serial.println("QR code data exists in the database.");
      digitalWrite(GLED, HIGH);//turns on Green LED
      digitalWrite(RLED, LOW); //turns off Red LED
      delay(2000);
      digitalWrite(GLED, LOW); //Turns off Red LED
    } else {
      Serial.println("QR code data not found in the database.");
      digitalWrite(RLED, HIGH); //turns on Red LED
      digitalWrite(GLED, LOW); //turns off Green LED
      delay(2000);
      digitalWrite(RLED, LOW); //turns off Red LED
    }
  } else {
    Serial.println("HTTP request failed with error code: " + String(httpCode));
  }

  http.end(); // End the HTTP request
}
/* ======================================== */
