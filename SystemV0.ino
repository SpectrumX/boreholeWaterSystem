/*
 * Tank 1 --> Contains the limestone and has a high pH
 * Tank 2 -- Contains water at correct pH for feeding into house
 * RESERVE pins A4 and A5 for the LCD display & RTC module
 * Spare analog pins can be used a digital pins, refering to them as A0 - A5
 * Stabalize analog reference voltage using a cap
 * Instead of using the shift registor we could instead have the outputs to each solenoid be the indicator... plus then there is no room for extra error in coding part!
 * ^ Hoever then there is no startup test sequence to see that all the leds are working....
 * 
 *  TODO:
 *    Add safety mechanisim for overflow prevention....
 *    Add time limit for length of runtime allowed for filling so that any issues will not prevail for more than x amount of time *    
 */

//Solenoid variables, set high to open the path described and set low to close the path
#define sourceToTank1 2; //Digital pin 2 for "the borehole source into limestone tank"
#define sourceToTank2 3; //Digtal pin 3 for "the borehole source into drinking tank"
#define tank1ToPump 4; //Digtal pin 4 for "The limestone tank into the pump input"
#define tank2ToPump 5; //Digtal pin 5 for "The drinking tank into the pump input"
#define pumpToTank2 6; //Digtal pin 6 for "The pump output into drinking tank"
#define pumpToHouse 7; //Digital pin 7 for "The pump output into house mains"

//Pins for the US distance sensors
#define USTank1Trig 8;
#define USTank1Echo 9;
#define USTank2Trig 10;
#define USTank2Echo 11;

//Pin for borehole pump on or off
#define boreholePump 12;

/*Code for if we use seperated outputs for state leds
  //Pins for the shiftregistor which indicates which solenoids are active/deactive
  #define shiftDataIn ; //This pin is set to the logic level desired for the next clock tick (Shared with onboard LED - check for issues...)
  #define shiftDataClk ; //This pin clocks the data into the storage
  #define shiftStorageClk ; //This pin clocks the data to the outputs of the shift registor
*/

//Pin for the pH sensor
#define pHSensor A0; //Analog 0 pin for pH sensor

//Pin for button input
#define button1 A1; //Use an analog as a digital pin

//Booleans to keep track of solenoid states, true means open.
bool stateSourceToTank1 2;
bool stateSourceToTank2 3;
bool stateTank1ToPump 4;
bool stateTank2ToPump 5;
bool statePumpToTank2 6;
bool statePumpToHouse 7;



void setup() {
  //Setup pinModes
  pinMode(SourceToTank1, OUTPUT);
  pinMode(SourceToTank2, OUTPUT);
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


  //Setup display


  //Check / Setup RTC

}

void loop() {
  // put your main code here, to run repeatedly:

}

//Methods













