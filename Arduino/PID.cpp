#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
float txValue = 0;
float set_temperature = 30;
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect (BLEServer* pServer){
    deviceConnected = true;
  };
  
  void onDisconnect(BLEServer* pServer){
    deviceConnected = false;
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0){
      Serial.println("=====START=RECEIVE=====");
      Serial.print("Received Value: ");

      for (int i = 0; i < rxValue.length(); i++){
        Serial.print(rxValue[i]);
        
      }

      Serial.println();
      float temp = ::atof( rxValue.c_str() );
      set_temperature = temp;
      Serial.print("Temp set to: ");
      Serial.println(set_temperature);

      if(rxValue.find("1") != -1){
        Serial.println("Temp Set!");
      }

      Serial.println();
      Serial.println("=====END=RECEIVE=====");
    }
  }
};

//////////////////////////////////////////////////////////////////////////

#include <Wire.h>

#include <Adafruit_MLX90614.h>


Adafruit_MLX90614 mlx = Adafruit_MLX90614();

//I/O
int PWM_pin = 23;  //Pin for PWM signal to the MOSFET driver (the BJT npn with pullup)

//Variables
//float set_temperature = 30;            //Default temperature setpoint. Leave it 0 and control it with rotary encoder
float temperature_read = 0.0;
float PID_error = 0;
float previous_error = 0;
float elapsedTime, Time, timePrev;
float PID_value = 0;
int button_pressed = 0;
int menu_activated=0;
float last_set_temperature = 0;

//PID constants
//////////////////////////////////////////////////////////
int kp = 90;   int ki = 30;   int kd = 80;
//////////////////////////////////////////////////////////

int PID_p = 0;    int PID_i = 0;    int PID_d = 0;
float last_kp = 0;
float last_ki = 0;
float last_kd = 0;

int PID_values_fixed =0;

//Pins for the SPI with MAX6675
const int freq = 5000;
const int HeatChannel = 0;
const int resolution = 8;

void setup() {
  BLEDevice::init("PID");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService (SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
    );

  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
    );
  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  pServer->getAdvertising()->start();
  Serial.println("Waiting for a client connection to notify...");

//////////////////////////////////////////////////////////////////////////
  
  ledcSetup(HeatChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(PWM_pin, HeatChannel);
  
  Serial.begin(9600);
  
  mlx.begin();
  pinMode(PWM_pin,OUTPUT);
  Time = millis();
  
}

void loop() {
   if (deviceConnected) {
    txValue = temperature_read;
    
    char txString[8];
    dtostrf(txValue, 5, 2, txString);

    pCharacteristic->setValue(txString);

    pCharacteristic->notify();
    Serial.println("Send value: " + String(txString));
    delay(500);
   }
  // First we read the real value of temperature
  temperature_read = readThermocouple();
  Serial.println(mlx.readObjectTempC());
  //Next we calculate the error between the setpoint and the real value
  PID_error = set_temperature - temperature_read;
  //Calculate the P value
  PID_p = 0.01*kp * PID_error;
  //Calculate the I value in a range on +-3
  PID_i = 0.01*PID_i + (ki * PID_error);
  

  //For derivative we need real time to calculate speed change rate
  timePrev = Time;                            // the previous time is stored before the actual time read
  Time = millis();                            // actual time read
  elapsedTime = (Time - timePrev) / 1000; 
  //Now we can calculate the D calue
  PID_d = 0.01*kd*((PID_error - previous_error)/elapsedTime);
  //Final total PID value is the sum of P + I + D
  PID_value = PID_p + PID_i + PID_d;

  //We define PWM range between 0 and 255
  if(PID_value < 0)
  {    PID_value = 0;    }
  if(PID_value > 255)  
  {    PID_value = 255;  }
  //Now we can write the PWM signal to the mosfet on digital pin D3
  //Since we activate the MOSFET with a 0 to the base of the BJT, we write 255-PID value (inverted)
  //analogWrite(PWM_pin,255-PID_value);
  ledcWrite(HeatChannel, PID_value);
  previous_error = PID_error;     //Remember to store the previous error for next loop.

  delay(1000); //Refresh rate + delay of LCD print


}//Loop end

double readThermocouple() {
  return mlx.readObjectTempC();
}