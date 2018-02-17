//Imports
#include <Wire.h>
#include <DS1302.h>
#include <Metro.h>
#include <LiquidCrystal_I2C.h>

/*
 * Tank 1 --> Contains the limestone and has a high pH
 * Tank 2 -- Contains water at correct pH for feeding into house
 * RESERVE pins A4 and A5 for the LCD display (SDA - A4; SCL - A5)
 * Spare analog pins can be used a digital pins, refering to them as A0 - A5
 * Stabalize analog reference voltage using a cap
 * Instead of using the shift registor we could instead have the outputs to each solenoid be the indicator... plus then there is no room for extra error in coding part!
 * ^ Hoever then there is no startup test sequence to see that all the leds are working....
 * 
 *  TODO:
 *    Add safety mechanisim for overflow prevention....
 *    Add time limit for length of runtime allowed for filling so that any issues will not prevail for more than x amount of time *    
 */


//CONSTANTS --> Require setting:
#define US1Level 77 //Height above ground/base of the sensor
#define US2Level 77 //Height above ground/base of the sensor
#define maxLevel 70 //Maximum hight (cm) water may get to / %Full lvl calculated by this
#define tank1FullPercent 100 //Target Full percentage
#define tank2FullPercent 100 //Target Full percentage
#define emptyThreshT1 30 //The minimum level before air gets sucked
#define emptyThreshT2 30 // The minimum level before air gets sucked
#define threshold 10 //This is the amount (in percent) that the tanks current level must be below the desired full level in order to commence filling
#define periodBegin 0 //The hour in which the system may begin to fill tanks
#define periodEnd 4 //The hour in which the system must stop filling tanks
#define litresCM 13 //Number of litres per CM water height

//Global Variables / Objects
LiquidCrystal_I2C lcd(0x3F, 16, 2);     //Create object   MARTIN
DS1302 rtc(A0, A1, A2); // rst, IO, clk                      MARTIN

int displayNumber = 0;
Metro displayInterval = new Metro(1000); //Interval to update clock/diplay
Metro checkInterval = new Metro(300000);
int hoursSince = 0; //Set to 48 to insta run...
int lastHour;
String daysOTW[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
int error = 0;
bool alternator = true;
bool ft1wbs = false; //These two booleans are here to decide when to update previous day.. only when both are true on the day of filling the tanks
bool ft2w1s = false;

//Solenoid variables, set high to open the path described and set low to close the path
#define sourceToTank1 2 //Digital pin 2 for "the borehole source into limestone tank"
#define sourceToTank2 A3 //Pin A3 for "the borehole source into drinking tank" (UNUSED)
#define tank1ToPump 4 //Digtal pin 4 for "The limestone tank into the pump input"
#define tank2ToPump 5 //Digtal pin 5 for "The drinking tank into the pump input"
#define pumpToTank2 6 //Digtal pin 6 for "The pump output into drinking tank"
#define pumpToHouse 7 //Digital pin 7 for "The pump output into house mains"

//Pins for the US distance sensors
#define USTank1Trig 8
#define USTank1Echo 9
#define USTank2Trig 10
#define USTank2Echo 11

//Pin for borehole pump on or off
#define boreholePump 12

/*Code for if we use seperated outputs for state leds
  //Pins for the shiftregistor which indicates which solenoids are active/deactive
  #define shiftDataIn ; //This pin is set to the logic level desired for the next clock tick (Shared with onboard LED - check for issues...)
  #define shiftDataClk ; //This pin clocks the data into the storage
  #define shiftStorageClk ; //This pin clocks the data to the outputs of the shift registor
*/

//Pin for the pH sensor
#define pHSensor A0 //Analog 0 pin for pH sensor

//Pin for button input
#define button1 3 //Use an analog as a digital pin

//Booleans to keep track of solenoid states, true means open.
bool stateSourceToTank1;
bool stateSourceToTank2;
bool stateTank1ToPump;
bool stateTank2ToPump;
bool statePumpToTank2;
bool statePumpToHouse;



void setup() {

  Serial.begin(9600);
  
  //Setup pinModes
  pinMode(sourceToTank1, OUTPUT);
  pinMode(sourceToTank2, OUTPUT);
  pinMode(tank1ToPump, OUTPUT);
  pinMode(tank2ToPump, OUTPUT);
  pinMode(pumpToTank2, OUTPUT);
  pinMode(pumpToHouse, OUTPUT);

  pinMode(USTank1Trig, OUTPUT);
  pinMode(USTank2Trig, OUTPUT);
  pinMode(USTank1Echo, INPUT);
  pinMode(USTank2Echo, INPUT);

  pinMode(boreholePump, OUTPUT);

  pinMode(button1, INPUT);

  //Set outputs to 'default' modes  
  digitalWrite(boreholePump, LOW);
  defaultSolenoids();

  //Setup display                  MARTIN
  lcd.init();                   //Initiating        
  lcd.backlight();              
  lcd.setCursor(0, 0);
  lcd.print("WATER CONTROL");
  lcd.setCursor(0, 1);
  lcd.print("SYSTEM V1");
  delay(1000);
  lcd.clear();


  //Check / Setup RTC
  rtc.halt(false);
  rtc.writeProtect(false);
  //Comment out later:
  rtc.setDOW(MONDAY);
  rtc.setTime(23, 59, 55); // 24hr format
  rtc.setDate(29, 1, 2018);
  rtc.poke(0, 7);

  //Interrupts Setup
  attachInterrupt(digitalPinToInterrupt(button1), buttonPressed, RISING);

  //Final tasks
  displayInterval.reset();
  checkInterval.reset();
}

void loop() {

  updates();
  
  if (checkInterval.check()){
    if (itIsTheDay() && itIsTheHour()){
      
      if(fullTank2W1(90)){
        Serial.println("Fill tank 2 with tank 1 SUCCESS");
        ft2w1s = true;
      }else{
        Serial.println("Fill tank 2 with tank 1 FAIL!");
        ft2w1s = false;
      }
      if(fullTank1WB(70)){
        Serial.println("Fill tank 1 with borehole SUCCESS");
        ft1wbs = true; 
      }else{
        Serial.println("Fill tank 1 with borehole FAIL!");
        ft1wbs = false; 
      }

      if(ft2w1s && ft1wbs){
        updatePreviousDay();
        ft2w1s = false;
        ft1wbs = false;
      }
    }
  }
 
  
}

void buttonPressed(){
  Serial.println("HI");
  if (displayNumber == 1){
      displayNumber = 0;
    }else{
      displayNumber++;
    }
    updateDisplay();
}

void updates(){
  if(displayInterval.check()){
    updateDisplay();
  }

  //if (digitalRead(button1)){
  //  if (displayNumber == 1){
  //    displayNumber = 0;
  //  }else{
  //    displayNumber++;
  //  }
  //  delay(500);
 //}
}

//Functions
void updatePreviousDay(){
  int currentDay; //Sunday is day 0
  String currentDayStr = rtc.getDOWStr();

  for (int i = 0; i < 7; i++){
    if (currentDayStr.equals(daysOTW[i])){
      currentDay = i;
    }
  }
  rtc.poke(0, currentDay);
}
boolean itIsTheDay(){ //Returns true when 26 hours has passed and the hour is 0h00, else returns false, must be run regularly
  
  int previousDay = rtc.peek(0); //IN SETUP MAKE SURE TO SET THE INITIAL DAY!!! //Also update once the auto full has activated
  int currentDay; //Sunday is day 0
  String currentDayStr = rtc.getDOWStr();

  for (int i = 0; i < 7; i++){
    if (currentDayStr.equals(daysOTW[i])){
      currentDay = i;
    }
  }
  
  switch (currentDay){
    case 0:
      if (previousDay != 6 && previousDay != 0){
        return true;
      }else{
        return false;
      }

      break;

    case 1:
     if (previousDay != 0 && previousDay != 1){
        return true;
      }else{
        return false;
      }

      break;

    case 2:
    if (previousDay != 1 && previousDay != 2){
        return true;
      }else{
        return false;
      }

      break;

    case 3:
    if (previousDay != 2 && previousDay != 3){
        return true;
      }else{
        return false;
      }

      break;

    case 4:
    if (previousDay != 3 && previousDay != 4){
        return true;
      }else{
        return false;
      }

      break;

    case 5:
    if (previousDay != 4 && previousDay != 5){
        return true;
      }else{
        return false;
      }

      break;

    case 6:
    if (previousDay != 5 && previousDay != 6){
        return true;
      }else{
        return false;
      }

      break;
    
    default:
      return true;
      break;
  }

  
}

boolean itIsTheHour(){
  String currentTime = rtc.getTimeStr();
  int currentHour = (currentTime.substring(0,2)).toInt();
  
  if (currentHour >= 0 && currentHour <= 4){
    return true;
  }else{
    return false;
    }
  
}

void updateLastDay(){
  String currentDayStr = rtc.getDOWStr();
  for (int i = 0; i < 7; i++){
    if (currentDayStr.equals(daysOTW[i])){
      rtc.poke(0, i);
    }
  }
}

void updateSolenoids(){
  digitalWrite(sourceToTank1, stateSourceToTank1);
  digitalWrite(sourceToTank2, stateSourceToTank2);
  digitalWrite(tank1ToPump, stateTank1ToPump);
  digitalWrite(tank2ToPump, stateTank2ToPump);
  digitalWrite(pumpToTank2, statePumpToTank2);
  digitalWrite(pumpToHouse, statePumpToHouse);
}

void defaultSolenoids(){
  stateSourceToTank1 = false;
  stateTank1ToPump  = false;
  stateTank2ToPump  = true;
  statePumpToTank2  = false;
  statePumpToHouse  = true;

  updateSolenoids();
}

void clearSolenoids(){
  stateSourceToTank1 = false;
  stateTank1ToPump  = false;
  stateTank2ToPump  = false;
  statePumpToTank2  = false;
  statePumpToHouse  = false;

  updateSolenoids();
}


boolean fullTank2W1(int percent){ //Argument --> Enter percentage full that tank 2 must be filled too.


  if (waterLevel(2) < maxLevel && waterLevel(1) > emptyThreshT1 && percentageFull(2) < (percent - threshold)){
    clearSolenoids();
    stateTank1ToPump = true;
    statePumpToTank2 = true;
    updateSolenoids(); 
  
  
    while (waterLevel(2) < maxLevel && waterLevel(1) > emptyThreshT1 && percentageFull(2) < percent){
    
      updates(); //Display and ----button continue to work----
  
    }
    
    defaultSolenoids();
    
    if (percentageFull(2) >= (percent - threshold)){
      return true;
    }else{
      return false;
    }

  }
  return (waterLevel(1) > emptyThreshT1); //To get here either 'this' is the problem or the tank is already full
}

boolean fullTank1WB(int percent){ //Argument --> Enter percentage full that tank 1 must be filled too.
  double Level = percentageFull(1);
  

  if (waterLevel(1) < maxLevel && Level < (percent - threshold)){
    Serial.println((percent - threshold));
    Serial.println(Level);
    defaultSolenoids();
    stateSourceToTank1 = true;
    updateSolenoids();
    delay(100);
    digitalWrite(boreholePump, HIGH);
      
  
    while (waterLevel(1) < maxLevel && percentageFull(1) < percent){
      
      updates();   
    
    }
    
    digitalWrite(boreholePump, LOW);
    delay(100);
    defaultSolenoids();
    
    if (percentageFull(1) >= (percent - threshold)){
      return true;
    }else{
        return false;
    } 
  }
  return true;
}

//MARTINS Methods

double getDistance(int tank){
  double distance;
  int duration;
  if (tank == 1){
    digitalWrite(USTank1Trig, LOW);         //Clear the trigPin
    delayMicroseconds(2);
    digitalWrite(USTank1Trig, HIGH);        //Sending out signal
    delayMicroseconds(10);
    digitalWrite(USTank1Trig, LOW);          
    duration = pulseIn(USTank1Echo, HIGH);  // Receiving signal
    return(duration * 0.034 / 2);        // Calculating distance from water surface to sensor    
    
  }else{
    digitalWrite(USTank2Trig, LOW);         //Clear the trigPin
    delayMicroseconds(2);
    digitalWrite(USTank2Trig, HIGH);        //Sending out signal
    delayMicroseconds(10);
    digitalWrite(USTank2Trig, LOW);
    duration = pulseIn(USTank2Echo, HIGH);  // Receiving signal
    return(duration * 0.034 / 2);        // Calculating distance from water surface to sensor
    
  }
  
}

double waterLevel(int tank){ //TODO: Time out for too many reading attempts
  double distances[10];
  int variance = 5; //CM of varience allowed per reading
  bool success = false;
  double average;
  int count = 0;

  while(!success){
    for (int i = 0; i < 10; i++){
      distances[i] = getDistance(tank);
      delay(20);
      average += distances[i];
    }
    average = average/10.0;
    for (int i = 0; i < 10; i++){
      if(distances[i] > (average - variance) && distances[i] < (average + variance)){
        count++;
      }
    }
    if (count > 7){
      success = true;
    }else{
      count = 0;
      average = 0;
    }
  }
  if(tank == 1){
    return US1Level - average;             // Calculating height of water
  }else{
    return US2Level - average;             // Calculating height of water
  }

  
}

int volumeOfWater(int tank){
  return litresCM * waterLevel(tank);       // Calculating volume of water
}

int percentageFull(int tank){
  return ((waterLevel(tank) / maxLevel) * 100); // Calculating percentage full
  
}

void updateDisplay(){
  if (displayNumber == 0){
    lcd.setCursor(0, 0);
    lcd.print("T1: ");                  // Printing the percentage full of tank 1
    lcd.print(percentageFull(1));
    lcd.print("%         ");
    lcd.setCursor(9, 0);
    lcd.print("Error");
    if(alternator){
      lcd.print(":");
    }else{
      lcd.print(" ");
    }
    alternator = !alternator;
    lcd.print(error);
    
    lcd.setCursor(0, 1);
    lcd.print("T2: ");                  // Printing the percentage full of tank 2
    lcd.print(percentageFull(2));
    lcd.print("%    ");
    lcd.setCursor(9, 1);
    lcd.print("pH: ");                  // Printing the pH of tank 2
    lcd.print("x.x");
  }else if (displayNumber == 1 ){
    lcd.setCursor(0, 0);
    lcd.print("    ");
    lcd.print(rtc.getTimeStr());      // Printing the time, date and day of the week to the LCD to check everything is running smoothly.
    lcd.print("    ");
    lcd.setCursor(0, 1);
    lcd.print(rtc.getDateStr());
    lcd.print("   ");
    lcd.print(rtc.getDOWStr());
  }
}










