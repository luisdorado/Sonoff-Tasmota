/*
  xdrv_91_rollershutter.h - Rollershutter/Blind/Cover controller support for Sonoff-Tasmota

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

// Max rollershutters controlled (only accepts values 1 or 2)
#define RS_MAX_ROLLERSHUTTERS 1

// *************************
//   Rolleshutter #1 Config
// *************************

// Relays
#define RS_UP_ACTUATOR_1 1
#define RS_DOWN_ACTUATOR_1 2
// Tiempo total en ms que tarda en hacer todo el recorrido
#ifndef RS_FULL_PATH_TIME_1
#define RS_FULL_PATH_TIME_1 19000 // ms
#endif
// Margin time to be applied when target position is 0 (top) or 100 (down)
// Needed to fix possible gaps generated when too many moves from intermediate positions has ben asked
// Assures that when target 0 or 100 position is asked it reachs the real position
#define RS_FIX_TIME_1 1000 // segs 2
// Extra time needed once 100% position is reached so that motor keeps moving until rs is completely down
// 0% position   => Rollershutter is up, at the top of the window, completely hidden
// 100% position => Rollershutter has reached the bottom of the window (still some light from outside)
// 100% position + rs_time_from_100_to_max => Rollershutter reachs max bottom position (no light from outside)
#ifndef RS_ADDITIONAL_TIME_1
#define RS_ADDITIONAL_TIME_1 4000 // 2.6 segs
#endif

// *************************
//   Rolleshutter #2 Config
// *************************

// Reles
#define RS_UP_ACTUATOR_2 3
#define RS_DOWN_ACTUATOR_2 4
// Time in ms needed to get from 0% to 100%
#ifndef RS_FULL_PATH_TIME_2
#define RS_FULL_PATH_TIME_2 17000 // segs 22
#endif

// Margin time to be applied when target position is 0 (top) or 100 (down)
// Needed to fix possible gaps generated when too many moves from intermediate positions has ben asked
// Assures that when target 0 or 100 position is asked it reachs the real position
#define RS_FIX_TIME_2 1000 // segs 2
// Extra time needed once 100% position is reached so that motor keeps moving until rs is completely down
// 0% position   => Rollershutter is up, at the top of the window, completely hidden
// 100% position => Rollershutter has reached the bottom of the window (still some light from outside)
// 100% position + rs_time_from_100_to_max => Rollershutter reachs max bottom position (no light from outside)
#ifndef RS_ADDITIONAL_TIME_2
#define RS_ADDITIONAL_TIME_2 4220 // 2.6 segs
#endif

// Time between status updates
const long RS_TIME_BETWEEN_STATUS_UPDATES=3000;
// Time between percentage updates
const long RS_TIME_BETWEEN_PERCENTAGE_UPDATES=3000;


// ***************************************
// DO NOT CHANGE ANYTHING BELOW THIS LINE
// ***************************************

// Internal representation of the rs state.
#define RS_IDLE 0
#define RS_UP 1
#define RS_DOWN 2
#define RS_NOTSEND 3

// INFO UPDATES
// If status has changed, so that next time it can be sent
bool statusChanged[2] = {false,false};
// Last status value sent
int rsLastStatusSent[2] = {RS_NOTSEND,RS_NOTSEND};
// Last percentage value sent
int rsLastPercentageSent[2] = {255,255}; // 0 ==> Persiana subida, 100 => Persiana bajada
// If status has been sent
bool rsInitialStateSent[2] = {false,false};
// Last time status or percentage values were sent
unsigned long rsLastStatusSentTime[2] = {0,0};
unsigned long rsLastPercentageSentTime[2] = {0,0};

// Config
int rs_up_actuator[2] = {RS_UP_ACTUATOR_1,RS_UP_ACTUATOR_2};
int rs_down_actuator[2] = {RS_DOWN_ACTUATOR_1,RS_DOWN_ACTUATOR_2};
// Time in ms needed to get from 0% to 100%
int rs_full_path_time[2] = {RS_FULL_PATH_TIME_1,RS_FULL_PATH_TIME_2};
// Margin time to be applied when target position is 0 (top) or 100 (down)
// Needed to fix possible gaps generated when too many moves from intermediate positions has ben asked
// Assures that when target 0 or 100 position is asked it reachs the real position
int rs_fix_time_if_target_is_end[2] = {RS_FIX_TIME_1,RS_FIX_TIME_2};
// Extra time needed once 100% position is reached so that motor keeps moving until rs is completely down
// 0% position   => Rollershutter is up, at the top of the window, completely hidden
// 100% position => Rollershutter has reached the bottom of the window (still some light from outside)
// 100% position + rs_time_from_100_to_max => Rollershutter reachs max bottom position (no light from outside)
int rs_time_from_100_to_max[2] = {RS_ADDITIONAL_TIME_1,RS_ADDITIONAL_TIME_2};

// Motor current status
int rsMotorStatus[2] = {RS_IDLE,RS_IDLE};

// MOTION VARIABLES - Used when a new motion starts
// Initial time at which a new movement has been started (needed to know current progress since movement started)
unsigned long rsInitialTime[2] = {0,0};
// Time in ms needed to complete the movement until final position is reached
// Value is stored before a new movement is started
unsigned long rsTotalEstimatedTime[2] = {0,0};
// Initial position in ms (from TOP) before a new movement is started
long rsInitialPositionTime[2] = {0,0};
// Current position is ms from 0% (0%=TOP, 100%=BOTTOM)
long rsCurrentPositionTime[2] = {0,0};

// Shows if last saved status has been load on init
// Tries to load status from last persistent MQTT message sent before
// If some command is received before value is loaded, starts with default value (Position=0%) and stops waiting for MQTT last status
bool initializedWithLastPersistentMQTTValue[2] = {false,false};

// When moving, outputs current status to Serial each x handler function calls (called through FUNC_EVERY_50_MSECOND)
int serialOutputCycles = 0;
