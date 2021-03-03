/*
  loratx.ino - Main file for the project

  Copyright (C) 2021  Marius Bezuidenhout

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// include the library
#include <RadioLib.h>

#define RADIO_FREQ_MHZ 868

enum RadioAction { RADIO_OFF, RADIO_TX, RADIO_RX };
enum TimeAwareActions { FUNC_EVERY_SECOND, FUNC_EVERY_100_MSEC };

void ICACHE_RAM_ATTR setFlag(void);

// RF95 has the following connections:
// CS pin:    15
// DIO0 pin:  5
// RESET pin: 16
// DIO1 pin:  3
RFM95 radio = new Module(15, 5, 16);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//RFM95 radio = RadioShield.ModuleA;

// save transmission state between loops
int transmissionState = ERR_NONE;
uint16_t radioAction = RADIO_OFF;

void setup() {
  Serial.begin(115200);

  // initialize RFM95 with default settings
  Serial.print(F("[RFM95] Initializing ... "));
  int state = radio.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  radio.setFrequency(RADIO_FREQ_MHZ);

  // NOTE: some RFM95 modules use high power output,
  //       those are usually marked RFM95(C/CW).
  //       To configure RadioLib for these modules,
  //       you must call setOutputPower() with
  //       second argument set to true.
  /*
    Serial.print(F("[RFM95] Setting high power module ... "));
    state = radio.setOutputPower(20, true);
    if (state == ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true);
    }
  */

  // start transmitting the first packet
  Serial.print(F("[RFM95] Sending first packet ... "));

  // you can transmit C-string or Arduino string up to
  // 64 characters long

  // set the function that will be called
  // when packet transmission is finished
  radio.setDio0Action(setFlag);
  
  transmissionState = radio.startTransmit("Hello Pet Projects!");
  radioAction = RADIO_TX;

  // you can also transmit byte array up to 64 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    state = radio.startTransmit(byteArr, 8);
  */
}

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we got a packet, set the flag
  if(radioAction == RADIO_TX) {
    transmittedFlag = true;
  } else if(radioAction == RADIO_RX) {
    receivedFlag = true;    
  }
  radioAction = RADIO_OFF;
}

void sendLoraKeepAlive(void)
{
  // check if the previous transmission finished
  if(transmittedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;
  
    // reset flag
    transmittedFlag = false;
  
    if (transmissionState == ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));
  
      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()
  
    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);

    }
  
    // send another one
    Serial.print(F("[RFM95] Sending another packet ... "));
  
    // you can transmit C-string or Arduino string up to
    // 256 characters long
    radioAction = RADIO_TX;
    transmissionState = radio.startTransmit("Hello World!");
  
    // you can also transmit byte array up to 64 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
      int state = radio.startTransmit(byteArr, 8);
    */

    // we're ready to send more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }
}

void checkLoraSend(void)
{
  if(transmittedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;
  
    // reset flag
    transmittedFlag = false;
  
    if (transmissionState == ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));
  
      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()
  
    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);
    }
  }
}

void checkLoraRecv(void)
{
  
}

void timeLoop(uint8_t action)
{
  switch action {
    case FUNC_EVERY_SECOND:
      sendLoraKeepAlive();
      break;
    case FUNC_EVENT_100_MSEC:
      checkLoraSend();
      checkLoraRecv();
      break;
  }
}

void loop() {
  static uint32_t state_second = 0;                // State second timer
  static uint32_t state_100_msec = 0;              // State 100 msec timer
  if(false == enableInterrupt && radioAction == RADIO_OFF) { // We are not waiting for anything so lets listen for packets
    radioAction = RADIO_RX;
    radio.startReceive();
  }
  if (TimeReached(state_second)) {
    SetNextTimeInterval(state_second, 1000);
    timeLoop(FUNC_EVERY_SECOND);
  }
  if (TimeReaced(state_100_msec)) {
    SetNextTimeInterval(state_100_msec, 100);
    timeLoop(FUNC_EVERY_100_MSEC);
  }
}
