/*
  xdrv_91_rollershutter.ino - Rollershutter/Blind/Cover controller support for Sonoff-Tasmota

  Copyright (C) 2019  Luis Dorado

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

#ifdef USE_ROLLERSHUTTER

#warning **** DEF XDRV_91 ****

#define XDRV_91             91

#include <xdrv_91_rollershutter.h>

enum RollerShutterCommands { CMND_RS_AC, CMND_RS_PC };
const char kRollerShutterCommands[] PROGMEM = D_CMND_RS_AC "|" D_CMND_RS_PC;

// Init function, called on FUNC_INIT
void RollerShutterInit(void){
  snprintf_P(log_data, sizeof(log_data), "RollerShutter Init");
  AddLog(LOG_LEVEL_INFO);

  ExecuteCommandPower(1, POWER_OFF_NO_STATE, SRC_BUTTON); // SRC_BUTTON SO THAT IT WONT SEND A MQTT UPDATE MESSAGE
  ExecuteCommandPower(2, POWER_OFF_NO_STATE, SRC_BUTTON);
  ExecuteCommandPower(3, POWER_OFF_NO_STATE, SRC_BUTTON);
  ExecuteCommandPower(4, POWER_OFF_NO_STATE, SRC_BUTTON);
}

// Called on FUNC_MQTT_SUBSCRIBE
// Subscribe to percentage initial states, so that mqtt inits my position at start with last persistent mqtt value stored
void RollerShutterMqttSubscribe(void) {
  char stopic[TOPSZ];

  if(RS_MAX_ROLLERSHUTTERS > 0){
    GetTopic_P(stopic, STAT, Settings.mqtt_topic, "RollerShutterPercent1"); // Subscribed to last persistent Status value in MQTT for RS #1
    MqttSubscribe(stopic);

    snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Subscribed to last persistent Status value in MQTT => %s"), stopic);
    AddLog(LOG_LEVEL_INFO);
  }
  if(RS_MAX_ROLLERSHUTTERS > 1){
    GetTopic_P(stopic, STAT, Settings.mqtt_topic, "RollerShutterPercent2"); // Subscribed to last persistent Status value in MQTT for RS #2
    MqttSubscribe(stopic);

    snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Subscribed to last persistent Status value in MQTT => %s"), stopic);
    AddLog(LOG_LEVEL_INFO);
  }
}

// MQTT last persistent value received
void rsInitMqttValue(int rs_index, byte value) {
  if(!initializedWithLastPersistentMQTTValue[rs_index-1]){
    if(value >= 100){
      // Current position = ms needed for the full path + additional time
      rsCurrentPositionTime[rs_index-1] = rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1];
    }else{
      // Current position = ms needed to move from 0 position to current percentage value
      rsCurrentPositionTime[rs_index-1] = convertPercentageToTime(rs_index, value);
    }
    // Set as initialized
    initializedWithLastPersistentMQTTValue[rs_index-1] = true;

    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: received OK last position stored in MQTT, unsubscribing: %d"), rs_index, value);
    AddLog(LOG_LEVEL_INFO);
  }
  // Remove subscription
  char stopic[TOPSZ];
  GetTopic_P(stopic, STAT, Settings.mqtt_topic, (rs_index == 1)? "RollerShutterPercent1" : "RollerShutterPercent2");
  MqttClient.unsubscribe(stopic);
}

// Assure that some initial value is set
void initValues(int rs_index) {
    if(!initializedWithLastPersistentMQTTValue[rs_index-1]){
      // Position still not initalized
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Initial position forced initial value to 0"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);
      // Still not received last MQTT stored value, but we need to start moving, so we it manual to 0
      rsCurrentPositionTime[rs_index-1] = 0;
      initializedWithLastPersistentMQTTValue[rs_index-1] = true;
      // Unsubscribe MQTT last stored value, we dont need it anymore
      char stopic[TOPSZ];
      GetTopic_P(stopic, STAT, Settings.mqtt_topic, (rs_index == 1)? "RollerShutterPercent1" : "RollerShutterPercent2");
      MqttClient.unsubscribe(stopic);
    }
}

// Triggers when physical Sonoff buttun is pressed
// We use it to start actuators manually
boolean RS_Button(void){
  if (XdrvMailbox.index > 0 && (PRESSED == XdrvMailbox.payload) && (NOT_PRESSED == lastbutton[XdrvMailbox.index])) {
    snprintf_P(log_data, sizeof(log_data), PSTR("RS: Button %d pressed"), XdrvMailbox.index);
    AddLog(LOG_LEVEL_DEBUG);

    int btn_index = XdrvMailbox.index;
    int hold_btn;

    // Choose the right Rollershutter to be moved depending on the pressed button number
    switch(btn_index){
      case 1:
        // RS#1, UP
        if(rsMotorStatus[0] == RS_IDLE){
          snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Button pressed: UP. Going up"));
          AddLog(LOG_LEVEL_INFO);
          RSUp(1);
        }else{
          snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Button pressed: UP. Stop"));
          AddLog(LOG_LEVEL_INFO);
          RSStop(1);
        }
        break;
      case 2:
        // RS#1, DOWN
        if(rsMotorStatus[0] == RS_IDLE){
          snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Button pressed: DOWN. Going down"));
          AddLog(LOG_LEVEL_INFO);
          RSDown(1);
        }else{
          snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Button pressed: DOWN. Stop"));
          AddLog(LOG_LEVEL_INFO);
          RSStop(1);
        }
        break;
      case 3:
        // RS#2, UP
        if(rsMotorStatus[1] == RS_IDLE){
          snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Button pressed: UP. Going up"));
          AddLog(LOG_LEVEL_INFO);
          RSUp(2);
        }else{
          snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Button pressed: UP. Stop"));
          AddLog(LOG_LEVEL_INFO);
          RSStop(2);
        }
        break;
      case 4:
        // RS#2, DOWN
        if(rsMotorStatus[1] == RS_IDLE){
          snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Button pressed: DOWN. Going down"));
          AddLog(LOG_LEVEL_INFO);
          RSDown(2);
        }else{
          snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Button pressed: DOWN. Stop"));
          AddLog(LOG_LEVEL_INFO);
          RSStop(2);
        }
        break;
    }

    return true;  // Button press has been served here
  }
  return false;   // Don't serve other buttons
}

// Handler, called by FUNC_EVERY_50_MSECOND througn interface
void RollerShutterHandler(void){
  if(RS_MAX_ROLLERSHUTTERS>0){
    RollerShutterHandler(1);
  }
  if(RS_MAX_ROLLERSHUTTERS>1){
    RollerShutterHandler(2);
  }
}

void RollerShutterHandler(int rs_index){
  unsigned long now = millis();

  // Check if moving
  if(rsMotorStatus[rs_index-1] != RS_IDLE){
    // Check if target position is reached
    if((now - rsInitialTime[rs_index-1]) >= rsTotalEstimatedTime[rs_index-1]){
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Target reached tiempoMoviendo:%d tiempoAMover:%d"), rs_index, (now - rsInitialTime[rs_index-1]), rsTotalEstimatedTime[rs_index-1]);
      AddLog(LOG_LEVEL_DEBUG);

      // Update current position before setting status to IDLE
      updateCurrentPositionTime(rs_index);

      // Update motor status
      rsMotorStatus[rs_index-1] = RS_IDLE;
      // Set statusChanged variable to be managed in next iteration
      statusChanged[rs_index-1] = true;
    }else{
      // Still target not reached
      // Every 50 cycles show output to Serial
      if(serialOutputCycles == 50){
        updateCurrentPositionTime(rs_index);
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: elapsedTime:%d totalEstimatedTime:%d currentPositionTime:%d"), rs_index, (now - rsInitialTime[rs_index-1]), rsTotalEstimatedTime[rs_index-1], rsCurrentPositionTime[rs_index-1]);
        AddLog(LOG_LEVEL_DEBUG);
        serialOutputCycles = 0;
      }else{
        serialOutputCycles++;
      }
    }
  }

  // Status has been changed
  if(statusChanged[rs_index-1]){
    if(rsMotorStatus[rs_index-1] == RS_IDLE){
      // Motor idle: target has been reached or stop command has been received

      // Switch off both relays
      ExecuteCommandPower(rs_down_actuator[rs_index-1], POWER_OFF_NO_STATE, SRC_BUTTON);
      ExecuteCommandPower(rs_up_actuator[rs_index-1], POWER_OFF_NO_STATE, SRC_BUTTON);

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Motor status changed to IDLE. Current position: %d Current percentage: %d. Sending MQTT retained message to store value"), rs_index, rsCurrentPositionTime[rs_index-1], convertTimeToPercentage(rs_index, rsCurrentPositionTime[rs_index-1]));
      AddLog(LOG_LEVEL_DEBUG);

      // Send MQTT position value as retained message
      rsSendPercentage(rs_index, true);
      // Send MQTT motor status message
      rsSendStatus(rs_index);
    }else if(rsMotorStatus[rs_index-1] == RS_UP){
      // Going up

      // Change relays to start moving
      ExecuteCommandPower(rs_down_actuator[rs_index-1], POWER_OFF_NO_STATE, SRC_BUTTON);
      ExecuteCommandPower(rs_up_actuator[rs_index-1], POWER_ON_NO_STATE, SRC_BUTTON);

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Motor Status changed to UP"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);

      // Send MQTT motor status message
      rsSendStatus(rs_index);
    }else if(rsMotorStatus[rs_index-1] == RS_DOWN){
      // Going down

      // Change relays to start moving
      ExecuteCommandPower(rs_up_actuator[rs_index-1], POWER_OFF_NO_STATE, SRC_BUTTON);
      ExecuteCommandPower(rs_down_actuator[rs_index-1], POWER_ON_NO_STATE, SRC_BUTTON);

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Motor Status changed to DOWN"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);

      // Send MQTT motor status message
      rsSendStatus(rs_index);
    }
    // set statusChanged again to false
    statusChanged[rs_index-1] = false;
  }
}

void RSStop(int rs_index){
  initValues(rs_index);
  unsigned long now = millis();

  if(rsMotorStatus[rs_index-1] != RS_IDLE){
    // STOP command received and motor not IDLE

    // Set target to current position so that will stopped in handler function
    rsTotalEstimatedTime[rs_index-1] = now - rsInitialTime[rs_index-1];

    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Stop command received"), rs_index);
    AddLog(LOG_LEVEL_INFO);

    // Call RollerShutterHandler right now instead of wait for next iteration
    RollerShutterHandler(rs_index);
  }else{
    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Stop command received but already stopped. Doing nothing. Current status resent"), rs_index);
    AddLog(LOG_LEVEL_INFO);

    // Status and position are sent again as it seems to have wrong values stored
    rsSendCurrentValues(rs_index);
  }
}

void RSUp(int rs_index){
  initValues(rs_index);
  unsigned long now = millis();

  // Update current position (if it was idle it wont be updated. If it was moving, position will be updated)
  updateCurrentPositionTime(rs_index);

  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Up command received. currentPos:%d currentStatus:%d"), rs_index, rsCurrentPositionTime[rs_index-1], rsMotorStatus[rs_index-1]);
  AddLog(LOG_LEVEL_INFO);

  // Check if: (already at position 0 (TOP)) AND (motor is not going up or is going up from intermediate position)
  if((rsCurrentPositionTime[rs_index-1] > 0) && (rsMotorStatus[rs_index-1] != RS_UP || ((rsInitialPositionTime[rs_index-1] - rsTotalEstimatedTime[rs_index-1]) > 0))){
    // Set current time as initial time
    rsInitialTime[rs_index-1] = now;
    // Current position in ms as initial position
    rsInitialPositionTime[rs_index-1] = rsCurrentPositionTime[rs_index-1];

    // Estimated time = current position in ms
    rsTotalEstimatedTime[rs_index-1] = rsCurrentPositionTime[rs_index-1];
    // Check if starting from intermediate position
    if(rsCurrentPositionTime[rs_index-1] < (rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1])){
      // intermediate position
      rsTotalEstimatedTime[rs_index-1] = rsTotalEstimatedTime[rs_index-1] + rs_fix_time_if_target_is_end[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Starting from intermediate position, adding rs_fix_time_if_target_is_end"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);
    }else{
      // Starting from max bottom position
      rsTotalEstimatedTime[rs_index-1] = rsTotalEstimatedTime[rs_index-1] +rs_time_from_100_to_max[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Starting from bottom, adding rs_time_from_100_to_max"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);
    }

    // Going up
    if(rsMotorStatus[rs_index-1] != RS_UP){
      rsMotorStatus[rs_index-1] = RS_UP;
      statusChanged[rs_index-1] = true;
    }

    // Call RollerShutterHandler right now instead of wait for next iteration
    RollerShutterHandler(rs_index);
  }else{
    #ifdef SERIAL_OUTPUT
      if(rsCurrentPositionTime[rs_index-1] == 0){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Already at TOP, do nothing"), rs_index);
        AddLog(LOG_LEVEL_DEBUG);
      }
      if((rsInitialPositionTime[rs_index-1] - rsTotalEstimatedTime[rs_index-1]) <= 0){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Same target position than before, doing nothing"), rs_index);
        AddLog(LOG_LEVEL_DEBUG);
      }
    #endif
    // Status and position are sent again as it seems to have wrong values stored
    rsSendCurrentValues(rs_index);
  }
}

void RSDown(int rs_index){
  initValues(rs_index);
  unsigned long now = millis();

  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Down command. Current pos:%d Current st:%d"), rs_index, rsCurrentPositionTime[rs_index-1], rsMotorStatus[rs_index-1]);
  AddLog(LOG_LEVEL_INFO);

  // Update current position (only will be updated if it was moving)
  updateCurrentPositionTime(rs_index);

  if(
      (rsCurrentPositionTime[rs_index-1] < (rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1])) && // No esta ya abajo
      (rsMotorStatus[rs_index-1] != RS_DOWN || ((rsInitialPositionTime[rs_index-1] + rsTotalEstimatedTime[rs_index-1]) < (rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1]))) // El objetivo no es abajo del todo
    ){
    // Current position is not bottom AND (not going down OR going down to intermediate position)

    // Current time as initial time
    rsInitialTime[rs_index-1] = now;
    // Current position in ms as init position
    rsInitialPositionTime[rs_index-1] = rsCurrentPositionTime[rs_index-1];

    // Estimated time = rs_full_path_time - current position
    rsTotalEstimatedTime[rs_index-1] = (rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1])  - rsCurrentPositionTime[rs_index-1];
    if(rsCurrentPositionTime[rs_index-1] > 0){
      // Start from intermediate position, we have to add fix time
      rsTotalEstimatedTime[rs_index-1] = rsTotalEstimatedTime[rs_index-1] + rs_fix_time_if_target_is_end[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Start from intermediate pos., add fix time"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);
    }

    // Motor going down
    if(rsMotorStatus[rs_index-1] != RS_DOWN){
      rsMotorStatus[rs_index-1] = RS_DOWN;
      statusChanged[rs_index-1] = true;
    }

    // Call handler right now instead of waiting for next iteration
    RollerShutterHandler(rs_index);
  }else{
    #ifdef SERIAL_OUTPUT
      if(rsCurrentPositionTime[rs_index-1] >= (rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1])){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Already at the bottom, doing nothing. MQTT status resent"), rs_index);
        AddLog(LOG_LEVEL_DEBUG);
      }
      if((rsInitialPositionTime[rs_index-1] + rsTotalEstimatedTime[rs_index-1]) >= (rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1])){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Same target than before, doing nothing. MQTT status resent"), rs_index);
        AddLog(LOG_LEVEL_DEBUG);
      }
    #endif
    if(rsCurrentPositionTime[rs_index-1] >= (rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1])){
      // Status and position are sent again as it seems to have wrong values stored
      rsSendCurrentValues(rs_index);
    }
  }
}

void RSUpdateInfo(int rs_index){
  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Current status request. Sending MQTT status"), rs_index);
  AddLog(LOG_LEVEL_INFO);
  rsSendCurrentValues(rs_index);
}

void RSPercent(int rs_index, byte value) {
  initValues(rs_index);
  unsigned long now = millis();

  // Update current position if moving
  updateCurrentPositionTime(rs_index);
  int currentPercentage = convertTimeToPercentage(rs_index, rsCurrentPositionTime[rs_index-1]);
  int requestedPercentage = value;
  unsigned long requestedPositionTime = convertPercentageToTime(rs_index, requestedPercentage);

  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Requested percentage:%d current:%d requested pos.:%d current pos.:%d"), rs_index, requestedPercentage, currentPercentage, requestedPositionTime, rsCurrentPositionTime[rs_index-1]);
  AddLog(LOG_LEVEL_DEBUG);

  // Check if already at requested percentage
  if(currentPercentage != requestedPercentage){
    // Set initial time
    rsInitialTime[rs_index-1] = now;
    // Set initial position
    rsInitialPositionTime[rs_index-1] = rsCurrentPositionTime[rs_index-1];
    // Calculate moving direction
    if(requestedPercentage < currentPercentage){
      // Have to go UP
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Going up"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);

      // Set estimated time
      // If currently at the bottom, current position includes additional_time
      rsTotalEstimatedTime[rs_index-1] = rsCurrentPositionTime[rs_index-1] - requestedPositionTime;

      if(requestedPercentage == 0 && currentPercentage < 100){
        // Intermediate position, adding fix time
        rsTotalEstimatedTime[rs_index-1] = rsTotalEstimatedTime[rs_index-1] + rs_fix_time_if_target_is_end[rs_index-1];
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Requested TOP position from intermediate position. Adding fix time"), rs_index);
        AddLog(LOG_LEVEL_DEBUG);
      }

      // Set status
      if(rsMotorStatus[rs_index-1] != RS_UP){
        rsMotorStatus[rs_index-1] = RS_UP;
        statusChanged[rs_index-1] = true;
      }
    }else{
      // Have to go down
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Going down"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);

      // Set estimated time
      rsTotalEstimatedTime[rs_index-1] = requestedPositionTime - rsCurrentPositionTime[rs_index-1];
      if(requestedPercentage == 100){
        // Have to add additional time
        rsTotalEstimatedTime[rs_index-1] = rsTotalEstimatedTime[rs_index-1] + rs_time_from_100_to_max[rs_index-1];
        // Check if currently at intermediate position
        if(rsCurrentPositionTime[rs_index-1] > 0){
          // Adding fix time
          rsTotalEstimatedTime[rs_index-1] = rsTotalEstimatedTime[rs_index-1] + rs_fix_time_if_target_is_end[rs_index-1];

          snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Requested bottom from intermediate position. Adding fix time"), rs_index);
          AddLog(LOG_LEVEL_DEBUG);
        }
      }

      // Set status
      if(rsMotorStatus[rs_index-1] != RS_DOWN){
        rsMotorStatus[rs_index-1] = RS_DOWN;
        statusChanged[rs_index-1] = true;
      }
    }
    // Call handler right now instead of waiting for next iteration
    RollerShutterHandler(rs_index);
  }else{ // Already at target requested
    // Stop
    if(rsMotorStatus[rs_index-1] != RS_IDLE){ // Check if already IDLE
      // Set time needed to make it stop
      rsTotalEstimatedTime[rs_index-1] = now - rsInitialTime[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Requested same percentage we are currently at. Stopping"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);
      // Call handler right now instead of waiting for next iteration
      RollerShutterHandler(rs_index);
    }
  }
}

// Send current status and percentage
void rsSendCurrentValues(int rs_index){
	// Status
  rsSendStatus(rs_index);
	// Percentage
  rsSendPercentage(rs_index);
}

// Send current status
void rsSendStatus(int rs_index){
  unsigned long currentMillis = millis();
  // Motor status
  if(!rsInitialStateSent[rs_index-1] || ((rsMotorStatus[rs_index-1] != rsLastStatusSent[rs_index-1]) && (currentMillis - rsLastStatusSentTime[rs_index-1] > RS_TIME_BETWEEN_STATUS_UPDATES))){
    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Sending status to MQTT: %d"), rs_index, rsMotorStatus[rs_index-1]);
    AddLog(LOG_LEVEL_DEBUG);

    // Update status in MQTT
    char stopic[TOPSZ];
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%d"), rsMotorStatus[rs_index-1]); // Setting value
    GetTopic_P(stopic, STAT, Settings.mqtt_topic, (rs_index == 1)? "RollerShutterAction1" : "RollerShutterAction2"); // Publish
    MqttPublish(stopic);

    rsLastStatusSentTime[rs_index-1] = currentMillis;
    rsLastStatusSent[rs_index-1] = rsMotorStatus[rs_index-1];
  }
}

// Send current percentage
void rsSendPercentage(int rs_index){
  rsSendPercentage(rs_index, false);
}

void rsSendPercentage(int rs_index, boolean retained){
  unsigned long currentMillis = millis();
  int currentPct = convertTimeToPercentage(rs_index, rsCurrentPositionTime[rs_index-1]);
  // Check if: value is retained or not initialized or (has changed and time between updates reached)
  if(!rsInitialStateSent[rs_index-1] || retained || ((rsLastPercentageSent[rs_index-1] != currentPct) && (currentMillis - rsLastPercentageSentTime[rs_index-1] > RS_TIME_BETWEEN_PERCENTAGE_UPDATES))){
    #ifdef SERIAL_OUTPUT
      if(retained){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Send percentage to MQTT (retained): %d"), rs_index, currentPct);
        AddLog(LOG_LEVEL_DEBUG);
      }else{
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Send percentage to MQTT: %d"), rs_index, currentPct);
        AddLog(LOG_LEVEL_DEBUG);
      }
    #endif
    // Update status in MQTT
    char stopic[TOPSZ];
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%d"), currentPct);
    GetTopic_P(stopic, STAT, Settings.mqtt_topic, (rs_index == 1)? "RollerShutterPercent1" : "RollerShutterPercent2"); // Publish
    MqttPublish(stopic, retained);

    rsLastPercentageSentTime[rs_index-1] = currentMillis;
    rsLastPercentageSent[rs_index-1] = currentPct;
  }
}

uint8_t convertTimeToPercentage(int rs_index, unsigned long tiempo){
  uint8_t percent = (uint8_t)(tiempo*100/rs_full_path_time[rs_index-1]);
  if(percent > 100) percent = 100;
  if(percent < 0) percent = 0;
  return percent;
}

// Return conversion from percentage to time in ms. If percentage greater than 100 includes additional time
unsigned long convertPercentageToTime(int rs_index, uint8_t percentage){
  unsigned long value;
  if(percentage < 0){
    value = 0;
  }else if(value >= 100){
    value = (unsigned long) rs_full_path_time[rs_index-1];
  }else{
    value = (unsigned long) (rs_full_path_time[rs_index-1]*percentage/100);
  }
  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Percentage to time: percent:%d time:%d totaltime:%d"), rs_index, percentage, value, rs_full_path_time[rs_index-1]);
  AddLog(LOG_LEVEL_DEBUG_MORE);
  return value;
}

// Updates rsCurrentPositionTime if not idle.
// Call this function just before stopping to update its value
void updateCurrentPositionTime(int rs_index){
  unsigned long now = millis();
  if(rsMotorStatus[rs_index-1] == RS_UP){
    // it was going up
    // current pos = initial pos minus elapsed time since started
    rsCurrentPositionTime[rs_index-1] = rsInitialPositionTime[rs_index-1] - (now - rsInitialTime[rs_index-1]);

    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Current pos updated going up:%d"), rs_index, rsCurrentPositionTime[rs_index-1]);
    AddLog(LOG_LEVEL_DEBUG_MORE);
    // Fix it
    if(rsCurrentPositionTime[rs_index-1] < 0){
      rsCurrentPositionTime[rs_index-1] = 0;

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Fixed to zero:%d"), rs_index, rsCurrentPositionTime[rs_index-1]);
      AddLog(LOG_LEVEL_DEBUG_MORE);
    }
  }else if(rsMotorStatus[rs_index-1] == RS_DOWN){
    // it was going down
    // current por = initial pos plus time elapsed since started
    rsCurrentPositionTime[rs_index-1] = rsInitialPositionTime[rs_index-1] + (now - rsInitialTime[rs_index-1]);

    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Current pos updated going down: %d"), rs_index, rsCurrentPositionTime[rs_index-1]);
    AddLog(LOG_LEVEL_DEBUG);

    // Fix it
    if(rsCurrentPositionTime[rs_index-1] > (rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1])){
      rsCurrentPositionTime[rs_index-1] = rs_full_path_time[rs_index-1] + rs_time_from_100_to_max[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Fixed max value:%d"), rs_index, rsCurrentPositionTime[rs_index-1]);
      AddLog(LOG_LEVEL_DEBUG_MORE);
    }
  }
}

boolean RollershutterMqttData(void){
  char command [CMDSZ];
  char *type = NULL;
  uint16_t index;
  uint16_t index2;
  int16_t found = 0;
  uint16_t i = 0;

  // Extract command and index to be applied
  type = strrchr(XdrvMailbox.topic, '/');

  index = 1;
  if (type != NULL) {
    type++;
    for (i = 0; i < strlen(type); i++){
      // type[i] = toupper(type[i]);
      type[i] = type[i];
    }
    while (isdigit(type[i-1])){
      i--;
    }
    if (i < strlen(type)){
      index = atoi(type +i);
    }
    type[i] = '\0';
  }

  snprintf_P(log_data, sizeof(log_data), PSTR("RS MQTT CMD: " D_INDEX " %d, " D_COMMAND " %s, " D_DATA " %s"),
    index, type, XdrvMailbox.data);
  AddLog(LOG_LEVEL_DEBUG);

  if (type != NULL) {
    int command_code = GetCommandCode(command, sizeof(command), type, kRollerShutterCommands);

    if ((CMND_RS_AC == command_code) && (index > 0) && (index <= RS_MAX_ROLLERSHUTTERS)) {
      // Its a rollershutter command for action
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: ======= MQTT Command ======="), index);
      AddLog(LOG_LEVEL_DEBUG);

      // 0 Stop, 1 UP, 2 DOWN, 3 UPDATE_VALUES
      int value = atoi(XdrvMailbox.data);
      switch(value){
        case 0: // Stop
          RSStop(index);
          break;
        case 1: // Up
          RSUp(index);
          break;
        case 2: // Down
          RSDown(index);
          break;
        case 3: // Update info
          RSUpdateInfo(index);
          break;
      }
      // Set mqtt_data to zero so that no additional MQTT messages are sent
      mqtt_data[0] = '\0';
      found = 1;
    }else if ((CMND_RS_PC == command_code) && (index > 0) && (index <= RS_MAX_ROLLERSHUTTERS)) {
      // Its a rollershutter command for percentage
      int value = atoi(XdrvMailbox.data);
      if ((value >= 0) && (value <= 100)) {
        // Value is between 0 and 100, ok
        // Check if its a request or a previous value retained to be set as initial value
        if(strstr(XdrvMailbox.topic, D_STAT) != NULL){
          // Is a retained value
          snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: ======= Retained value received: %d"), index, value);
          AddLog(LOG_LEVEL_DEBUG);

          rsInitMqttValue(index, value);
        }else{
          snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: ======= Requested a percentage: %d"), index, value);
          AddLog(LOG_LEVEL_DEBUG);

          RSPercent(index, value);
        }
        found = 1;
      }
      // Set mqtt_data to zero so that no additional MQTT messages are sent
      mqtt_data[0] = '\0';
    }
  }
  return found;
}

/* struct XDRVMAILBOX { */
/*   uint16_t      valid; */
/*   uint16_t      index; */
/*   uint16_t      data_len; */
/*   int16_t       payload; */
/*   char         *topic; */
/*   char         *data; */
/* } XdrvMailbox; */
boolean RollerShutterCommand(void){
  char command [CMDSZ];
  boolean serviced = true;
  uint8_t ua_prefix_len = strlen(D_CMND_RS); // Length of the prefix RollerShutter

  snprintf_P(log_data, sizeof(log_data), "Command Action called: "
    "index: %d data_len: %d payload: %d topic: %s data: %s\n",
    XdrvMailbox.index,
    XdrvMailbox.data_len,
    XdrvMailbox.payload,
    (XdrvMailbox.payload >= 0 ? XdrvMailbox.topic : ""),
    (XdrvMailbox.data_len >= 0 ? XdrvMailbox.data : ""));

    AddLog(LOG_LEVEL_DEBUG);

  if(0 == strncasecmp_P(XdrvMailbox.topic, PSTR(D_CMND_RS), ua_prefix_len)){
    // command starts with RollerShutter string
    int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic + ua_prefix_len, kRollerShutterCommands);
    if(CMND_RS_AC == command_code){
      snprintf_P(log_data, sizeof(log_data), "Command Action called: "
        "index: %d data_len: %d payload: %d topic: %s data: %s\n",
	      XdrvMailbox.index,
	      XdrvMailbox.data_len,
	      XdrvMailbox.payload,
	      (XdrvMailbox.payload >= 0 ? XdrvMailbox.topic : ""),
	      (XdrvMailbox.data_len >= 0 ? XdrvMailbox.data : ""));

        AddLog(LOG_LEVEL_DEBUG);
    }else if(CMND_RS_PC == command_code){
      snprintf_P(log_data, sizeof(log_data), "Command Percent called: "
        "index: %d data_len: %d payload: %d topic: %s data: %s\n",
	      XdrvMailbox.index,
	      XdrvMailbox.data_len,
	      XdrvMailbox.payload,
	      (XdrvMailbox.payload >= 0 ? XdrvMailbox.topic : ""),
	      (XdrvMailbox.data_len >= 0 ? XdrvMailbox.data : ""));

        AddLog(LOG_LEVEL_DEBUG);
    }else{
      serviced = false;
    }
  }else{
    serviced = false;
  }
  return serviced;
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

boolean Xdrv91(byte function){
  boolean result = false;

  switch (function) {
    case FUNC_INIT:
      RollerShutterInit();
      break;
    case FUNC_EVERY_50_MSECOND:
      RollerShutterHandler();
      break;
    case FUNC_EVERY_SECOND:
      // Mandar porcentaje actualizado
      break;
    case FUNC_COMMAND:
      snprintf_P(log_data, sizeof(log_data), "RS: FUNC_COMMAND");
      AddLog(LOG_LEVEL_DEBUG);
      //result = RollerShutterCommand();
      break;
    case FUNC_SHOW_SENSOR:
      snprintf_P(log_data, sizeof(log_data), "RS: FUNC_SHOW_SENSOR");
      AddLog(LOG_LEVEL_DEBUG);
      break;
    case FUNC_BUTTON_PRESSED:
      // snprintf_P(log_data, sizeof(log_data), "RS: FUNC_BUTTON_PRESSED");
      // AddLog(LOG_LEVEL_DEBUG);
      result = RS_Button();
      break;
    case FUNC_SET_POWER:
      snprintf_P(log_data, sizeof(log_data), "RS: FUNC_SET_POWER");
      AddLog(LOG_LEVEL_DEBUG);
      break;
    case FUNC_MQTT_SUBSCRIBE:
      // 2 Called just after connection with MQTT is established
      RollerShutterMqttSubscribe();
      break;
    case FUNC_MQTT_DATA:
      result = RollershutterMqttData();
      break;
    case FUNC_MQTT_INIT:
      // 3 Called just after connection with MQTT established, after FUNC_MQTT_SUBSCRIBE and only one triggered one time
      snprintf_P(log_data, sizeof(log_data), "RS: FUNC_MQTT_INIT");
      AddLog(LOG_LEVEL_DEBUG);
      break;
  }
  return result;
}

#endif
