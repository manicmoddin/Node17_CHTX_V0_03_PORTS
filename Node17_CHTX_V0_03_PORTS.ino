/*
 * Firmware for the Central Heating Transceiver V 0.02
 * This is derived from the work from JCW and his relay Plugs
 *
 * Changelog
 * Version  Notes
 * 0.01     Initial Write
 * 0.02     Added the override funtion with static LED, Activity LED
 * 0.03     Added the logic to stop it constantly writing to the relays, 
 just on change
 
 *
 */

#include <JeeLib.h>
int debug = 1;

const int activityLed = 9;

int mainOverride = 0;
int previousOverride = 0;
int boiler = 0;
int pump   = 0;
int oldBoiler = 0;
int oldPump =0;
int boilerTracker = 0;
int pumpTracker = 0;


Port relays (2);
Port overridePort(1);
Port leds(4);

typedef struct {
  int pump;		                                      
  int boiler;	
  int override;	                                      
} 
Payload;
Payload centralHeating;

long previousMillis = 0;        // will store last time LED was updated
long interval = 10000;          // interval at which to blink (milliseconds)
long pwmInterval = 30;

void setup () {
  rf12_initialize(17, RF12_433MHZ, 210);
  pinMode(activityLed, OUTPUT);

  leds.mode(OUTPUT);
  leds.mode2(OUTPUT);
  leds.digiWrite(0);
  leds.digiWrite2(0);

  overridePort.mode(INPUT);
  overridePort.mode2(OUTPUT);

  relays.mode(OUTPUT);
  relays.mode2(OUTPUT);
  relays.digiWrite(0);
  relays.digiWrite2(0);

  Serial.begin(9600);
}

void loop () {
  if (rf12_recvDone() && rf12_crc == 0 && rf12_len == 2) {
    centralHeating.boiler = rf12_data[0];
    centralHeating.pump = rf12_data[1];
    if (mainOverride == 1 ) {
      //data has come in while override is on
      if(debug == 1)
        Serial.println("Data come in while override is on");
      //change old values
      oldBoiler = centralHeating.boiler;
      oldPump = centralHeating.pump;
    }
    else {
      //safe to change the normal ones
      boiler = centralHeating.boiler;
      pump = centralHeating.pump;
    }
  }
  centralHeating.override = mainOverride;
  doRelays();



  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis > interval) {
    // save the last time you sent the data 
    previousMillis = currentMillis;
    send_rf_data();   
  }
}

void send_rf_data()
{
  digitalWrite(activityLed, HIGH);
  rf12_sleep(RF12_WAKEUP);
  // if ready to send + exit loop if it gets stuck as it seems too
  int i = 0; 
  while (!rf12_canSend() && i<10) {
    rf12_recvDone(); 
    i++;
  }
  rf12_sendStart(0, &centralHeating, sizeof centralHeating);
  // set the sync mode to 2 if the fuses are still the Arduino default
  // mode 3 (full powerdown) can only be used with 258 CK startup fuses
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);
  digitalWrite(activityLed, LOW);
}

void doRelays() {
  int override = overridePort.digiRead();
  if(override != previousOverride) {
    previousOverride = override;
    overridePort.digiWrite2(override);
    if(debug == 1) {
      Serial.print("Changing Override to ");
      Serial.println(override);
    }
    mainOverride = override;
    if(override == 1) {
      //copy the values of the pump and boiler to oldPump and oldBoiler
      oldPump = pump;
      oldBoiler = boiler;
      //now set the pump and boilr to 1
      pump = 1;
      boiler = 1;
    }
    else {
      //revert back to what it was
      pump = oldPump;
      boiler = oldBoiler;
    }
  }
  //make no changes

  if (boilerTracker != boiler) {
    boilerTracker = boiler;
    centralHeating.boiler = boiler;
    relays.digiWrite(boiler);
    leds.digiWrite(boiler);
    if(debug == 1) {
      Serial.print("Boiler Tracker Set to ");
      Serial.println(boilerTracker);
    }
  }
  if (pumpTracker != pump) {
    pumpTracker = pump;
    centralHeating.pump = pump;
    relays.digiWrite2(pump);
    leds.digiWrite2(pump);
    if(debug == 1) {
      Serial.print("Pump Tracker Set to ");
      Serial.println(pumpTracker);
    }
  }

}



