
//Include the library files
  #include <LiquidCrystal_I2C.h>
  #include <ESP8266WiFi.h>
  #include <BlynkSimpleEsp8266.h>
  #include <SPI.h>
  #include <Wire.h> 
  #include <DHT.h>
  #include <Servo.h>
  // Define the component pins
  #define BLYNK_PRINT Serial
  #define DHTPIN 2          // Mention the digital pin connected 
  #define DHTTYPE DHT11     // DHT 11 
  #define trig D7
  #define echo D8
  #define SENSOR  D3
  int gatePin = D5;
  int Buzzer = D6;

  int openPosition = 90; // servo position for gate open
  int closePosition = 0; 

  DHT dht(DHTPIN, DHTTYPE);
  Servo servo;
  BlynkTimer timer;
  //Initialize the LCD display
  LiquidCrystal_I2C lcd(0x27, 16, 2);

  char auth[] = "zmYmupCMFOnhdTi7imSNEB7Jb09tZtbI";//Enter your Auth token
  char ssid[] = "Dialog 4G 737";//Enter your WIFI name
  char pass[] = "bF0253f3";//Enter your WIFI password

  long currentMillis = 0;
  long previousMillis = 0;
  int interval = 1000;
  bool ledState = LOW;
  float calibrationFactor = 4.5;
  volatile byte pulseCount;
  float pulse1Sec = 0;
  float flowRate;
  unsigned long flowMilliLitres;
  unsigned int totalMilliLitres;
  float flowLitres;
  float totalLitres;

  //tank max value(CM)
  int MaxLevel = 20;

  int Level1 = (MaxLevel * 75) / 100;
  int Level2 = (MaxLevel * 65) / 100;
  int Level3 = (MaxLevel * 55) / 100;
  int Level4 = (MaxLevel * 45) / 100;
  int Level5 = (MaxLevel * 15) / 100;

    void IRAM_ATTR pulseCounter()
    {
      pulseCount++;
    }

    BLYNK_WRITE(V10) 
    {
        servo.write(param.asInt());
        digitalWrite(Buzzer, HIGH);
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("WARNING !");
        lcd.setCursor(1, 1);
        lcd.print("Dam Gate Open");
        lcd.clear();
    }

     void ultrasonic() 
    {
      digitalWrite(trig, LOW);
      delayMicroseconds(4);
      digitalWrite(trig, HIGH);
      delayMicroseconds(10);
      digitalWrite(trig, LOW);
 
      long t = pulseIn(echo, HIGH);
      int distance = t / 29 / 2;

      int blynkDistance = (distance - MaxLevel) * -1;
  
      if (distance <= MaxLevel) 
      {
        Blynk.virtualWrite(V0, blynkDistance);
        
    
      } 
        else 
        {
          Blynk.virtualWrite(V0, 0);
          
        }
        lcd.setCursor(0, 0);
        lcd.print("Le:");


        if (Level1 <= distance) 
        {
          lcd.setCursor(3, 0);
          lcd.print("L");
          digitalWrite(Buzzer, LOW);
        } 
          else if (Level3 <= distance && Level2 > distance) 
          {
            lcd.setCursor(3, 0);
            lcd.print("M");
            digitalWrite(Buzzer, LOW);
          } 
            else if (Level4 <= distance && Level3 > distance) 
            {
              lcd.setCursor(3, 0);
              lcd.print("H");
              digitalWrite(Buzzer, LOW);
            } 
              else if (Level5 >= distance) 
              {
                digitalWrite(Buzzer, HIGH);
                servo.write(openPosition);
                lcd.clear();

                lcd.setCursor(5, 0);
                lcd.print("WARNING !");
                lcd.setCursor(1, 1);
                lcd.print("Dam Gate Open");
                
                Blynk.virtualWrite(V11, openPosition);
                delay(1000);
                servo.write(closePosition);
                Blynk.virtualWrite(V11, closePosition);
                lcd.clear();
                
                
  
              }
                

              
    }

    void sendSensor()
    {
      int h = dht.readHumidity();
      float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
  
      if (isnan(h) || isnan(t))
      {
        Serial.println("Failed to read from DHT sensor!");
        return;
      }

      Serial.println(t);
      

      Serial.print("Temperature : ");
      Serial.print(t);

      Serial.print("Humidity : ");
      Serial.println(h);

      lcd.setCursor(10, 1);
      lcd.print("T:");
      lcd.print(t);

      Blynk.virtualWrite(V5, h);  //V5 is for Humidity
      Blynk.virtualWrite(V6, t);  //V6 is for Temperature
    }


    void setup() 
    {
      Serial.begin(9600);
      lcd.begin();
      lcd.backlight();

      pinMode(trig, OUTPUT);
      pinMode(echo, INPUT);

      pinMode(Buzzer, OUTPUT);

      Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

      dht.begin();
      timer.setInterval(2500L, sendSensor);

      servo.attach(gatePin);
      servo.write(closePosition);

      lcd.setCursor(0, 0);
      lcd.print("Dam Management ");
  
      lcd.setCursor(4, 1);
      lcd.print("System");
      delay(4000);
      lcd.clear();

      //Call the functions
      timer.setInterval(100L, ultrasonic);
      pinMode(SENSOR, INPUT_PULLUP);
 
      pulseCount = 0;
      flowRate = 0.0;
      flowMilliLitres = 0;
      totalMilliLitres = 0;
      previousMillis = 0;
 
      attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
    }

  
   



    void loop() 
    {
  
      currentMillis = millis();

      if (currentMillis - previousMillis > interval) 
      {
        pulse1Sec = pulseCount;
        pulseCount = 0;
 
        // Because this loop may not complete in exactly 1 second intervals we calculate
        // the number of milliseconds that have passed since the last execution and use
        // that to scale the output. We also apply the calibrationFactor to scale the output
        // based on the number of pulses per second per units of measure (litres/minute in
        // this case) coming from the sensor.
        flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
        previousMillis = millis();
 
        // Divide the flow rate in litres/minute by 60 to determine how many litres have
        // passed through the sensor in this 1 second interval, then multiply by 1000 to
        // convert to millilitres.
        flowMilliLitres = (flowRate / 60) * 1000;
        flowLitres = (flowRate / 60);
 
        // Add the millilitres passed in this second to the cumulative total
        totalMilliLitres += flowMilliLitres;
        totalLitres += flowLitres;
    
        // Print the flow rate for this second in litres / minute
        Serial.print("Flow rate: ");
        Serial.print(float(flowRate));  // Print the integer part of the variable
        Serial.print("L/min");
        Serial.print("\t");       // Print tab space
 
        lcd.setCursor(7, 0);
        lcd.print("FR:");
        lcd.print(float(flowRate));
        lcd.setCursor(14,0);  //oled display
        lcd.print("LM");
 
        // Print the cumulative total of litres flowed since starting
        Serial.print("Output Liquid Quantity: ");
        Serial.print(totalMilliLitres);
        Serial.print("mL / ");
        Serial.print(totalLitres);
        Serial.println("L");

 
        lcd.setCursor(0, 1);
        lcd.print("OLQ:");
        lcd.print(totalLitres);
        lcd.setCursor(8, 1);
        lcd.print("L");


        Blynk.virtualWrite(V7, float(flowRate));
        Blynk.virtualWrite(V8, totalLitres);
        
    
      }
      
      Blynk.run();//Run the Blynk library
      timer.run();//Run the Blynk timer
    }