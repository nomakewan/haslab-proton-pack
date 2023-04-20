/**
 *   Neutrona Wand - Arduino Powered Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023 Michael Rajotte <michael.rajotte@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

/*
IMPORTANT: Do not forget to unplug the TX1/RX1 cables from Serial1 while you are uploading code to your Nano.
*/

/*
 * You need to edit wavTrigger.h and make sure you comment out the proper serial port. (Near the top of the wavTrigger.h file).
 * We are going to use Pins 8 and 9 on the Nano. __WT_USE_ALTSOFTSERIAL__
 */
#include <wavTrigger.h>
/* 
 *  AltSoftSerial uses: Pin 9 = TX & Pin 8 = RX. 
 *  So Pin 9 goes to the RX of the WavTrigger and Pin 8 goes to the TX of the WavTrigger. 
 */
#include <AltSoftSerial.h>
#include <millisDelay.h> 
#include <FastLED.h>
#include <ezButton.h>
#include <ht16k33.h>

/*
 * -------------****** CUSTOM USER CONFIGURABLE SETTINGS ******-------------
 * Change the variables below to alter the behaviour of your Neutrona wand.
 */
 
/*
 * You can set the default master startup volume for your wand here.
 * This gets overridden if you connect your wand to the pack.
 * Values are in % of the volume.
 * 0 = quietest
 * 100 = loudest
 */
const int STARTUP_VOLUME = 100;

/*
 * You can set the default music volume for your wand here.
 * This gets overridden if you connect your wand to the pack.
 * Values are in % of the volume.
 * 0 = quietest
 * 100 = loudest
 */
const int STARTUP_VOLUME_MUSIC = 100;

/*
 * You can set the default sound effects volume for your wand here.
 * This gets overridden if you connect your wand to the pack.
 * Values are in % of the volume.
 * 0 = quietest
 * 100 = loudest
 */
const int STARTUP_VOLUME_EFFECTS = 100;

/*
 * Minimum volume that the Neutrona Wand can achieve. 
 * Values must be from 0 to -70. 0 = the loudest and -70 = the quietest.
 * Volume changes are based on percentages. 
 * If your pack is overpowering the wand at lower volumes, you can either increase the minimum value in the wand,
 * or decrease the minimum value for the pack.
*/
const int MINIMUM_VOLUME = -35;

/*
 * Percentage increments of main volume change.
*/
const int VOLUME_MULTIPLIER = 2;

/*
 * Percentage increments of the music volume change..
*/
const int VOLUME_MUSIC_MULTIPLIER = 5;

/*
 * Percentage increments of the sound effects volume change.
*/
const int VOLUME_EFFECTS_MULTIPLIER = 5;

/*
 * Set to false to disable the onboard amplifer on the wav trigger. 
 * Turning off the onboard amp draws less power. 
 * If using the AUX cable jack, the amp can be disabled to save power.
 * If you use the output pins directly on the wav trigger board to your speakers, you will need to enable the onboard amp.
 * NOTE: The On-board mono audio amplifier and speaker connector specifications: 2W into 4 Ohms, 1.25W into 8 Ohms
 */
const bool b_onboard_amp_enabled = true;

/*
 * When set to true, the mode switch button to change firing modes changes to a alternate firing button.
 * Pressing this button together at the same time as the Intensify button does a cross the streams firing.
 * The video game firing modes will be disabled when you enable this. 
 * Access to the wand system menu settings to control volume and music will only be able to be reached when the wand is powered down.
*/
const bool b_cross_the_streams = false;

/*
 * Set to true if you are replacing the stock Hasbro bargraph with a Barmeter 28 segment bargraph.
 * Set to false if you are using the sock Hasbro bargraph.
 * Part #: BL28Z-3005SA04Y
*/
const bool b_bargraph_alt = false;

/*
 * Enable or disable vibration control for the Neutrona wand.
 * Vibration is toggled on and off by a toggle switch in the Proton Pack.
 * When set to false, there will be no vibration enabled for the Neutrona wand, and it will ignore the toggle switch settings from the Proton Pack.
*/
const bool b_vibration_enabled = true;

/*
 * When set to true, when vibration is enabled from the Proton Pack side, the Neutrona wand will only vibrate during firing.
 * Setting b_vibration_enabled to false will override this.
*/
const bool b_vibration_firing = false;

/*
 * Set this to true to be able to use your wand without a Proton Pack connected.
 * Otherwise set to false and the wand will wait until it is connected to a Proton Pack before it can activate.
 */
const bool b_no_pack = false;

/*
 * Which power modes do you want to be able to overheat.
 * Set to true to allow the wand and pack to overheat in that mode.
 * Set to false to disable overheating in that power mode. You will be able to continously fire instead.
 */
const bool b_overheat_mode_1 = false;
const bool b_overheat_mode_2 = false;
const bool b_overheat_mode_3 = false;
const bool b_overheat_mode_4 = false;
const bool b_overheat_mode_5 = true;

/*
 * Time in milliseconds for when overheating will initiate if enabled for that power mode.
 * Overheat only happens if enabled for that power mode (see above).
 * Example: 12000 = (12 seconds)
 */
const unsigned long int i_ms_overheat_initiate_mode_1 = 60000;
const unsigned long int i_ms_overheat_initiate_mode_2 = 30000;
const unsigned long int i_ms_overheat_initiate_mode_3 = 20000;
const unsigned long int i_ms_overheat_initiate_mode_4 = 15000;
const unsigned long int i_ms_overheat_initiate_mode_5 = 12000;

/*
 * Debug testing
 * Set to true to debug some switch readings.
 * Keep your wand unplugged from the pack while this is set to true.
 * It uses the USB port and tx/rx need to be free so serial information can be sent back to the Arduino IDE.
 * The wand will respond a bit slower as it is streaming serial data back. For debugging the analog switch readings only.
 */
const bool b_debug = false;

/*
 * -------------****** DO NOT CHANGE ANYTHING BELOW THIS LINE ******-------------
 */

 /* 
 *  SD Card sound files in order. If you have no sound, your SD card might be too slow. 
 *  Try a faster one. File naming 000_ is important as well. For music, it is 100_ and higher.
 *  Also note if you add more sounds to this list, you need to update the i_last_effects_track variable.
 *  The wav trigger uses this to determine how many music tracks there are if any.
 */
enum sound_fx {
  S_EMPTY, 
  S_BOOTUP,
  S_SHUTDOWN,
  S_IDLE_LOOP,
  S_IDLE_LOOP_GUN,
  S_FIRE_START,
  S_FIRE_START_SPARK,
  S_FIRE_LOOP,
  S_FIRE_LOOP_GUN,
  S_FIRE_LOOP_IMPACT,
  S_FIRING_END,
  S_FIRING_END_GUN,
  S_AFTERLIFE_BEEP_WAND,
  S_AFTERLIFE_GUN_RAMP_LOW,
  S_AFTERLIFE_GUN_RAMP_HIGH,
  S_AFTERLIFE_PACK_STARTUP,
  S_AFTERLIFE_PACK_IDLE_LOOP,
  S_WAND_SHUTDOWN,
  S_AFTERLIFE_BEEP_WAND_S1,
  S_AFTERLIFE_BEEP_WAND_S2,
  S_AFTERLIFE_BEEP_WAND_S3,
  S_AFTERLIFE_BEEP_WAND_S4,
  S_AFTERLIFE_BEEP_WAND_S5,
  S_AFTERLIFE_GUN_RAMP_1,
  S_AFTERLIFE_GUN_LOOP_1,
  S_AFTERLIFE_GUN_RAMP_2,
  S_AFTERLIFE_GUN_LOOP_2,
  S_AFTERLIFE_GUN_RAMP_DOWN_2,
  S_AFTERLIFE_GUN_RAMP_DOWN_1,
  S_IDLE_LOOP_GUN_2,
  S_IDLE_LOOP_GUN_3,
  S_IDLE_LOOP_GUN_4,
  S_IDLE_LOOP_GUN_5,
  S_IDLE_LOOP_GUN_1,
  S_PACK_BEEPING,
  S_PACK_SHUTDOWN,
  S_PACK_SHUTDOWN_AFTERLIFE,
  S_GB2_PACK_START,
  S_GB2_PACK_LOOP,
  S_GB2_PACK_OFF,
  S_CLICK,
  S_VENT,
  S_VENT_SLOW,
  S_VENT_FAST,
  S_VENT_DRY,
  S_VENT_BEEP,
  S_VENT_BEEP_3,
  S_VENT_BEEP_7,
  S_PACK_SLIME_OPEN,
  S_PACK_SLIME_CLOSE,
  S_PACK_SLIME_TANK_LOOP,
  S_SLIME_START,
  S_SLIME_LOOP,
  S_SLIME_END,
  S_STASIS_START,
  S_STASIS_LOOP,
  S_STASIS_END,
  S_MESON_START,
  S_MESON_LOOP,
  S_MESON_END,
  S_BEEP_8,
  S_VENT_SMOKE,
  S_MODE_SWITCH,
  S_BEEPS,
  S_BEEPS_ALT,
  S_SPARKS_LONG,
  S_SPARKS_LOOP,
  S_BEEPS_LOW,
  S_BEEPS_BARGRAPH,
  S_MESON_OPEN,
  S_STASIS_OPEN,
  S_FIRING_END_MID,
  S_FIRING_LOOP_GB1,
  S_CROSS_STREAMS_END,
  S_CROSS_STREAMS_START,
  S_PACK_RIBBON_ALARM_1,
  S_PACK_RIBBON_ALARM_2
};

/*
 * Need to keep track which is the last sound effect, so we can iterate over the effects to adjust volume gain on them.
 */
const int i_last_effects_track = S_PACK_RIBBON_ALARM_2;

/* 
 * Wand state. 
 */
enum WAND_STATE { MODE_OFF, MODE_ON };
enum WAND_STATE WAND_STATUS;

/*
 * Various wand action states.
 */
enum WAND_ACTION_STATE { ACTION_IDLE, ACTION_OFF, ACTION_ACTIVATE, ACTION_FIRING, ACTION_OVERHEATING, ACTION_SETTINGS };
enum WAND_ACTION_STATE WAND_ACTION_STATUS;

/* 
 *  Barrel LEDs. There are 5 LEDs. 0 = Base, 4 = tip. These are addressable with a single pin and are RGB.
 */
#define BARREL_LED_PIN 10
#define BARREL_NUM_LEDS 5
CRGB barrel_leds[BARREL_NUM_LEDS];

/*
 * Delay for fastled to update the addressable LEDs. 
 * We have up to 5 addressable LEDs in the wand barrel.
 * 0.03 ms to update 1 LED. So 0.15 ms should be ok? Lets bump it up to 3 just in case.
 */
const int i_fast_led_delay = 3;
millisDelay ms_fast_led;

/* 
 *  Wav trigger
 */
wavTrigger w_trig;
uint8_t i_music_count = 0;
uint8_t i_current_music_track = 0;
const uint8_t i_music_track_start = 100; // Music tracks start on file named 100_ and higher.

/*
 *  Volume (0 = loudest, -70 = quietest)
*/
int i_volume_percentage = STARTUP_VOLUME_EFFECTS; // Sound effects
int i_volume_master_percentage = STARTUP_VOLUME; // Master overall volume
int i_volume_music_percentage = STARTUP_VOLUME_MUSIC; // Music volume
int i_volume = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_percentage / 100); // Sound effects
int i_volume_master = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100); // Master overall volume
int i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100); // Music volume

/* 
 * Rotary encoder on the top of the wand. Changes the wand power level and controls the wand settings menu.
 * Also controls independent music volume while the pack/wand is off and if music is playing.
 */
#define r_encoderA 6
#define r_encoderB 7
static uint8_t prev_next_code = 0;
static uint16_t store = 0;

/* 
 *  Vibration
 */
const uint8_t vibration = 11;
int i_vibration_level = 55;
int i_vibration_level_prev = 0;
bool b_vibration_on = false;

/* 
 *  Various Switches on the wand.
 */
ezButton switch_wand(A0); // Contols the beeping. Top right switch on the wand.
ezButton switch_intensify(2);
ezButton switch_activate(3);
ezButton switch_vent(4); // Turns on the vent light.
const int switch_mode = A6; // Changes firing modes or to reach the settings menu.
const int switch_barrel = A7; // Barrel extension/open switch.

/*
 * Some switch settings.
 */
const int i_switch_mode_value = 200;
const int i_switch_barrel_value = 100;
const uint8_t switch_debounce_time = 50;
const uint8_t a_switch_debounce_time = 250;
millisDelay ms_switch_mode_debounce;
millisDelay ms_intensify_timer;
const int i_intensify_delay = 400;

/*
 * Wand lights
 */
const uint8_t led_slo_blo = 5; // There are 2 LED's attached to this pin. The slo-blo and the light on the front of the wand body. You can drive up to 2 led's from 1 pin on a arduino.
const uint8_t led_white = 12; // Blinking white light beside the vent on top of the wand.
const uint8_t led_vent = 13; // Vent light
const uint8_t led_bargraph_1 = A1;
const uint8_t led_bargraph_2 = A2;
const uint8_t led_bargraph_3 = A3;
const uint8_t led_bargraph_4 = A4;
const uint8_t led_bargraph_5 = A5;

/* 
 *  Idling timers
 */
millisDelay ms_gun_loop_1;
millisDelay ms_gun_loop_2;
millisDelay ms_white_light;
const int d_white_light_interval = 150;

/* 
 *  Overheat timers
 */
millisDelay ms_overheat_initiate;
millisDelay ms_overheating;
const int i_ms_overheating = 6500; // Overheating for 6.5 seconds.
const bool b_overheat_mode[5] = { b_overheat_mode_1, b_overheat_mode_2, b_overheat_mode_3, b_overheat_mode_4, b_overheat_mode_5 };
const long int i_ms_overheat_initiate[5] = { i_ms_overheat_initiate_mode_1, i_ms_overheat_initiate_mode_2, i_ms_overheat_initiate_mode_3, i_ms_overheat_initiate_mode_4, i_ms_overheat_initiate_mode_5 };

/* 
 *  Stock Hasbro Bargraph timers
 */
millisDelay ms_bargraph;
millisDelay ms_bargraph_firing;
const int d_bargraph_ramp_interval = 120;
unsigned int i_bargraph_status = 0;

/*
 * (Optional) Barmeter 28 segment bargraph configuration and timers.
 * Part #: BL28Z-3005SA04Y
*/
HT16K33 ht_bargraph;
const int i_bargraph_interval = 4;
const int i_bargraph_wait = 180;
millisDelay ms_bargraph_alt;
bool b_bargraph_up = false;
unsigned int i_bargraph_status_alt = 0;
const int d_bargraph_ramp_interval_alt = 40;
const int i_bargraph_multiplier_ramp_1984 = 3;
const int i_bargraph_multiplier_ramp_2021 = 10;
int i_bargraph_multiplier_current = i_bargraph_multiplier_ramp_2021;

/*
 * (Optional) Barmeter 28 segment bargraph mapping.
 * Part #: BL28Z-3005SA04Y
*/
const uint8_t i_bargraph[28] = {0, 16, 32, 48, 1, 17, 33, 49, 2, 18, 34, 50, 3, 19, 35, 51, 4, 20, 36, 52, 5, 21, 37, 53, 6, 22, 38, 54};

/* 
 *  A timer for controlling the wand beep. in 2021 mode.
 */
millisDelay ms_reset_sound_beep;
const int i_sound_timer = 50;

/* 
 *  Wand tip heatup timers (when changing firing modes).
 */
millisDelay ms_wand_heatup_fade;
const int i_delay_heatup = 10;
int i_heatup_counter = 0;
int i_heatdown_counter = 100;

/* 
 *  Firing timers.
 */
millisDelay ms_firing_lights;
millisDelay ms_firing_lights_end;
millisDelay ms_firing_stream_blue;
millisDelay ms_firing_stream_orange;
millisDelay ms_impact; // Mix some impact sounds while firing.
millisDelay ms_firing_start_sound_delay;
millisDelay ms_firing_stop_sound_delay;
const int d_firing_lights = 20; // 20 milliseconds. Timer for adjusting the firing stream colours.
const int d_firing_stream = 100; // 100 milliseconds. Used by the firing timers to adjust stream colours.
int i_barrel_light = 0; // using this to keep track which LED in the barrel is currently lighting up.
const int i_fire_start_sound_delay = 50; // Delay for starting firing sounds.
const int i_fire_stop_sound_delay = 100; // Delay for stopping fire sounds.

/* 
 *  Wand power mode. Controlled by the rotary encoder on the top of the wand.
 *  You can enable or disable overheating for each mode individually in the user adjustable values at the top of this file.
 */
const int i_power_mode_max = 5;
const int i_power_mode_min = 1;
int i_power_mode = 1;
int i_power_mode_prev = 1;

/* 
 *  Wand / Pack communication
 */
int rx_byte = 0;

/*
 * Some pack flags which get transmitted to the wand depending on the pack status.
 */
bool b_pack_on = false;
bool b_pack_alarm = false;
bool b_wait_for_pack = true;
bool b_volume_sync_wait = false;
int i_cyclotron_speed_up = 1; // For telling the pack to speed up or slow down the cyclotron lights.

/*
 * Volume sync status with the pack.
 */
enum VOLUME_SYNC { EFFECTS, MASTER, MUSIC };
enum VOLUME_SYNC VOLUME_SYNC_WAIT;

/* 
 *  Wand menu & music
 */
int i_wand_menu = 5;
const int i_settings_blinking_delay = 350;
bool b_playing_music = false;
bool b_repeat_track = false;
const int i_music_check_delay = 2000;
const int i_music_next_track_delay = 2000;
millisDelay ms_settings_blinking;
millisDelay ms_check_music;
millisDelay ms_music_next_track;

/* 
 *  Wand firing modes + settings
 *  Proton, Slime, Stasis, Meson, Settings
 */
enum FIRING_MODES { PROTON, SLIME, STASIS, MESON, SETTINGS };
enum FIRING_MODES FIRING_MODE;
enum FIRING_MODES PREV_FIRING_MODE;

/*
 * Misc wand settings and flags.
 */
int year_mode = 2021;
bool b_firing = false;
bool b_firing_intensify = false;
bool b_firing_alt = false;
bool b_firing_cross_streams = false;
bool b_sound_firing_intensify_trigger = false;
bool b_sound_firing_alt_trigger = false;
bool b_sound_firing_cross_the_streams = false;

bool b_sound_idle = false;
bool b_beeping = false;

void setup() {
  Serial.begin(9600);

  // Change PWM frequency of pin 3 and 11 for the vibration motor, we do not want it high pitched.
  TCCR2B = (TCCR2B & B11111000) | (B00000110); // for PWM frequency of 122.55 Hz
  
  setupWavTrigger();
  
  // Barrel LEDs
  FastLED.addLeds<NEOPIXEL, BARREL_LED_PIN>(barrel_leds, BARREL_NUM_LEDS);

  switch_wand.setDebounceTime(switch_debounce_time);
  switch_intensify.setDebounceTime(switch_debounce_time);
  switch_activate.setDebounceTime(switch_debounce_time);
  switch_vent.setDebounceTime(switch_debounce_time);

  // Rotary encoder on the top of the wand.
  pinMode(r_encoderA, INPUT_PULLUP);
  pinMode(r_encoderB, INPUT_PULLUP);
  
  bargraphYearModeUpdate();

  // Setup the bargraph.
  if(b_bargraph_alt == true) {
    // 28 Segment optional bargraph.
    ht_bargraph.begin(0x00);
  }
  else {
    // Original Hasbro bargraph.
    pinMode(led_bargraph_1, OUTPUT);
    pinMode(led_bargraph_2, OUTPUT);
    pinMode(led_bargraph_3, OUTPUT);
    pinMode(led_bargraph_4, OUTPUT);
    pinMode(led_bargraph_5, OUTPUT);
  }
  
  pinMode(led_slo_blo, OUTPUT);
  pinMode(led_vent, OUTPUT);
  pinMode(led_white, OUTPUT);

  pinMode(vibration, OUTPUT);
  
  // Make sure lights are off.
  wandLightsOff();
  
  // Wand status.
  WAND_STATUS = MODE_OFF;
  WAND_ACTION_STATUS = ACTION_IDLE;

  ms_reset_sound_beep.start(i_sound_timer);

  // Setup the mode switch debounce.
  ms_switch_mode_debounce.start(1);
  i_wand_menu = 5;

  // We bootup the wand in the classic proton mode.
  FIRING_MODE = PROTON;
  PREV_FIRING_MODE = SETTINGS;
  
  // Check music timer.
  ms_check_music.start(i_music_check_delay);

  if(b_no_pack == true || b_debug == true) {
    b_wait_for_pack = false;
    b_pack_on = true;
  }
}

void loop() { 
  if(b_wait_for_pack == true) {
    if(b_volume_sync_wait != true) {
      // Handshake with the pack. Telling the pack that we are here.
      Serial.write(14);
    }

    // Synchronise some settings with the pack.
    checkPack();
    
    delay(200);
  }
  else {   
    mainLoop();
  } 
}

void mainLoop() {
  w_trig.update();
  
  checkMusic();
  checkPack();
  switchLoops();
  checkRotary();  
  checkSwitches();

  if(ms_firing_stop_sound_delay.justFinished()) {
    modeFireStopSounds();
  }
      
  switch(WAND_ACTION_STATUS) {
    case ACTION_IDLE:
      // Do nothing.
    break;

    case ACTION_OFF:
      wandOff();
    break;
    
    case ACTION_FIRING:
      if(ms_firing_start_sound_delay.justFinished()) {
        modeFireStartSounds();
      }
    
      if(b_pack_on == true && b_pack_alarm == false) {
        if(b_firing == false) {
          b_firing = true;
          modeFireStart();
        }

        // Overheating.
        if(ms_overheat_initiate.justFinished() && b_overheat_mode[i_power_mode - 1] == true) {
          ms_overheat_initiate.stop();
          modeFireStop();

          delay(100);
          
          WAND_ACTION_STATUS = ACTION_OVERHEATING;

          // Play overheating sounds.
          ms_overheating.start(i_ms_overheating);
          ms_settings_blinking.start(i_settings_blinking_delay);
          
          w_trig.trackPlayPoly(S_VENT_DRY);
          w_trig.trackPlayPoly(S_CLICK);

          // Tell the pack we are overheating.
          Serial.write(10);
        }
        else {
          modeFiring();
          
          // Stop firing if any of the main switches are turned off or the barrel is retracted.
          if(switch_vent.getState() == HIGH || switch_wand.getState() == HIGH || analogRead(switch_barrel) > i_switch_barrel_value) {
            modeFireStop();
          }
        }
      }
      else if(b_pack_alarm == true && b_firing == true) {
        modeFireStop();
      }
    break;
    
    case ACTION_OVERHEATING:
      settingsBlinkingLights();
      
      if(ms_overheating.justFinished()) {
        ms_overheating.stop();
        ms_settings_blinking.stop();
        
        WAND_ACTION_STATUS = ACTION_IDLE;

        w_trig.trackStop(S_CLICK);
        w_trig.trackStop(S_VENT_DRY);
        
        // Tell the pack that we finished overheating.
        Serial.write(11);
      }
    break;
    
    case ACTION_ACTIVATE:
      modeActivate();
    break;

    case ACTION_SETTINGS:
      settingsBlinkingLights();
      
      switch(i_wand_menu) {
        case 5:
        // Track loop setting.
        if(switch_intensify.isPressed() && ms_intensify_timer.isRunning() != true) {
          ms_intensify_timer.start(i_intensify_delay / 2);
          
          if(b_repeat_track == false) {
            // Loop the track.
            b_repeat_track = true;
            w_trig.trackLoop(i_current_music_track, 1);
          }
          else {
            b_repeat_track = false;
            w_trig.trackLoop(i_current_music_track, 0);
          }
          
          // Tell pack to loop the music track.
          Serial.write(93);
        }
        break;

        // Pack / Wand sound effects volume.
        case 4:
          if(switch_intensify.isPressed() && ms_intensify_timer.isRunning() != true) {
            ms_intensify_timer.start(i_intensify_delay);
            
            increaseVolumeEffects();
            
            // Tell pack to increase the sound effects volume.
            Serial.write(92);
          }

          if(analogRead(switch_mode) > i_switch_mode_value && ms_switch_mode_debounce.justFinished()) {
            decreaseVolumeEffects();
            
            // Tell pack to lower the sound effects volume.
            Serial.write(91);
            
            ms_switch_mode_debounce.start(a_switch_debounce_time * 2);
          }
        
        break;

        // Pack / Wand music volume.
        case 3:
          if(b_playing_music == true) {
            if(switch_intensify.isPressed() && ms_intensify_timer.isRunning() != true) {
              ms_intensify_timer.start(i_intensify_delay);
              
              if(i_volume_music_percentage + VOLUME_MUSIC_MULTIPLIER > 100) {
                i_volume_music_percentage = 100;
              }
              else {
                i_volume_music_percentage = i_volume_music_percentage + VOLUME_MUSIC_MULTIPLIER;
              }

              i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100);

              w_trig.trackGain(i_current_music_track, i_volume_music);
              
              // Tell pack to increase music volume.
              Serial.write(90);
            }
  
            if(analogRead(switch_mode) > i_switch_mode_value && ms_switch_mode_debounce.justFinished()) {              
              if(i_volume_music_percentage - VOLUME_MUSIC_MULTIPLIER < 0) {
                i_volume_music_percentage = 0;
              }
              else {
                i_volume_music_percentage = i_volume_music_percentage - VOLUME_MUSIC_MULTIPLIER;
              }

              i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100);

              w_trig.trackGain(i_current_music_track, i_volume_music);
              
              // Tell pack to lower music volume.
              Serial.write(89);
              
              ms_switch_mode_debounce.start(a_switch_debounce_time * 2);
            }  
          }
        break;

        // Change music tracks.
        case 2:          
          if(switch_intensify.isPressed() && ms_intensify_timer.isRunning() != true) {
            ms_intensify_timer.start(i_intensify_delay);
            
            if(i_current_music_track + 1 > i_music_track_start + i_music_count - 1) {
              if(b_playing_music == true) {               
                // Go to the first track to play it.
                stopMusic();
                i_current_music_track = i_music_track_start;
                playMusic();
              }
              else {
                i_current_music_track = i_music_track_start;
              }
            }
            else {
              // Stop the old track and play the new track if music is currently playing.
              if(b_playing_music == true) {
                stopMusic();
                i_current_music_track++;                
                playMusic();
              }
              else {
                i_current_music_track++;
              }
            }
          
            // Tell the pack which music track to change to.
            Serial.write(i_current_music_track);
          }

          if(analogRead(switch_mode) > i_switch_mode_value && ms_switch_mode_debounce.justFinished()) {            
            if(i_current_music_track - 1 < i_music_track_start) {
              if(b_playing_music == true) {
                // Go to the last track to play it.
                stopMusic();
                i_current_music_track = i_music_track_start + (i_music_count -1);
                playMusic();
              }
              else {
                i_current_music_track = i_music_track_start + (i_music_count -1);
              }
            }
            else {
              // Stop the old track and play the new track if music is currently playing.
              if(b_playing_music == true) {
                stopMusic();
                i_current_music_track--;
                playMusic();
              }
              else {
                i_current_music_track--;
              }
            }
          
            // Tell the pack which music track to change to.
            Serial.write(i_current_music_track);
            
            ms_switch_mode_debounce.start(a_switch_debounce_time * 2);
          }          
        break;

        // Play music or stop music.
        case 1:          
          if(switch_intensify.isPressed() && ms_intensify_timer.isRunning() != true) {
            ms_intensify_timer.start(i_intensify_delay);

            if(b_playing_music == true) {
              // Stop music
              b_playing_music = false;

              // Tell the pack to stop music.
              Serial.write(98);
              
              stopMusic();             
            }
            else {
              if(i_music_count > 0 && i_current_music_track > 99) {
                // Start music.
                b_playing_music = true;

                // Tell the pack to play music.
                Serial.write(99);
                
                playMusic();
              }
            }
          }
        break;
      }
    break;
  }
  
  switch(WAND_STATUS) {
    case MODE_OFF:      
      if(analogRead(switch_mode) > i_switch_mode_value && ms_switch_mode_debounce.justFinished()) {        
        w_trig.trackPlayPoly(S_CLICK);
          
        if(FIRING_MODE != SETTINGS) {
          PREV_FIRING_MODE = FIRING_MODE;
          FIRING_MODE = SETTINGS;
          
          WAND_ACTION_STATUS = ACTION_SETTINGS;
          i_wand_menu = 5;
          ms_settings_blinking.start(i_settings_blinking_delay);
    
          // Tell the pack we are in settings mode.
          Serial.write(9);
        }
        else {
          // Only exit the settings menu when on menu #5.
          if(i_wand_menu == 5) {
            FIRING_MODE = PREV_FIRING_MODE;
  
            switch(PREV_FIRING_MODE) {
              case MESON:
                // Tell the pack we are in meson mode.
                Serial.write(8);
              break;
  
              case STASIS:
                // Tell the pack we are in stasis mode.
                Serial.write(7);
              break;
  
              case SLIME:  
                // Tell the pack we are in slime mode.
                Serial.write(6);
              break;
  
              case PROTON: 
                // Tell the pack we are in proton mode.
                Serial.write(5);
              break;
  
              default:
                // Tell the pack we are in proton mode.
                Serial.write(5);
              break;
            }
            
            WAND_ACTION_STATUS = ACTION_IDLE;
  
            wandLightsOff();
          }
        }
      
        ms_switch_mode_debounce.start(a_switch_debounce_time);
      }
    break;

    case MODE_ON:
      if(b_vibration_on == true && WAND_ACTION_STATUS != ACTION_SETTINGS) {
        vibrationSetting();
      }
  
      // Top white light.
      if(ms_white_light.justFinished()) {
        ms_white_light.start(d_white_light_interval);
        if(digitalRead(led_white) == LOW) {
          digitalWrite(led_white, HIGH);
        }
        else {
          digitalWrite(led_white, LOW);
        }
      }

      // Ramp the bargraph up ramp down back to the default power level setting on a fresh start.
      if(ms_bargraph.justFinished()) {
        bargraphRampUp();
      }
      else if(ms_bargraph.isRunning() == false && WAND_ACTION_STATUS != ACTION_FIRING && WAND_ACTION_STATUS != ACTION_SETTINGS && WAND_ACTION_STATUS != ACTION_OVERHEATING) {
        // Bargraph idling loop.
        bargraphPowerCheck();
      }

      if(year_mode == 2021) {
        if(ms_gun_loop_1.justFinished()) {
          w_trig.trackPlayPoly(S_AFTERLIFE_GUN_LOOP_1, true);
          w_trig.trackLoop(S_AFTERLIFE_GUN_LOOP_1, 1);
          ms_gun_loop_1.stop();
        }
      }

      wandBarrelHeatUp();
  
    break;
  } 

  if(b_firing == true && WAND_ACTION_STATUS != ACTION_FIRING) {
    modeFireStop();
  }

  if(ms_firing_lights_end.justFinished()) {
    fireStreamEnd(0,0,0);
  }

  // Update the barrel LEDs.
  if(ms_fast_led.justFinished()) {
    FastLED.show();
    ms_fast_led.stop();
  }
}

void checkMusic() { 
  if(ms_check_music.justFinished() && ms_music_next_track.isRunning() != true) {
    ms_check_music.start(i_music_check_delay);
        
    // Loop through all the tracks if the music is not set to repeat a track.
    if(b_playing_music == true && b_repeat_track == false) {      
      if(!w_trig.isTrackPlaying(i_current_music_track)) {
        ms_check_music.stop();
                
        stopMusic();

        // Tell the pack to stop playing music.
        Serial.write(98);
            
        if(i_current_music_track + 1 > i_music_track_start + i_music_count - 1) {
          i_current_music_track = i_music_track_start;
        }
        else {
          i_current_music_track++;          
        }

        // Tell the pack which music track to change to.
        Serial.write(i_current_music_track);
    
        ms_music_next_track.start(i_music_next_track_delay);
      }
    }
  }

  if(ms_music_next_track.justFinished()) {
    ms_music_next_track.stop();
        
    playMusic(); 

    // Tell the pack to play music.
    Serial.write(99);
    
    ms_check_music.start(i_music_check_delay); 
  }
} 

void stopMusic() {
  w_trig.trackStop(i_current_music_track);
  w_trig.update();
}

void playMusic() {  
  w_trig.trackGain(i_current_music_track, i_volume_music);
  w_trig.trackPlayPoly(i_current_music_track, true);
  
  if(b_repeat_track == true) {
    w_trig.trackLoop(i_current_music_track, 1);
  }
  else {
    w_trig.trackLoop(i_current_music_track, 0);
  }

  w_trig.update();
}

void settingsBlinkingLights() {  
  if(ms_settings_blinking.justFinished()) {
     ms_settings_blinking.start(i_settings_blinking_delay);
  }

  if(ms_settings_blinking.remaining() < i_settings_blinking_delay / 2) {
    if(b_bargraph_alt == true) {
      for(int i = 0; i < 24; i++) {
        ht_bargraph.clearLedNow(i_bargraph[i]);
      }
    }
    else {
      digitalWrite(led_bargraph_1, HIGH);
      digitalWrite(led_bargraph_2, HIGH);
      digitalWrite(led_bargraph_3, HIGH);
      digitalWrite(led_bargraph_4, HIGH);
    }

    // Indicator for looping track setting.
    if(b_repeat_track == true && i_wand_menu == 5 && WAND_ACTION_STATUS != ACTION_OVERHEATING) {
      if(b_bargraph_alt == true) {
        for(int i = 24; i < 28; i++) {
          ht_bargraph.clearLedNow(i_bargraph[i]);
        }
      }
      else {
        digitalWrite(led_bargraph_5, LOW);
      }
    }
    else {
      if(b_bargraph_alt == true) {
        for(int i = 24; i < 28; i++) {
          ht_bargraph.setLedNow(i_bargraph[i]);
        }
      }
      else {
        digitalWrite(led_bargraph_5, HIGH);
      }
    }
  }
  else {
    switch(i_wand_menu) {
      case 5:
        if(b_bargraph_alt == true) {
          for(int i = 0; i < 28; i++) {
            ht_bargraph.setLedNow(i_bargraph[i]);
          }
        }
        else {
          digitalWrite(led_bargraph_1, LOW);
          digitalWrite(led_bargraph_2, LOW);
          digitalWrite(led_bargraph_3, LOW);
          digitalWrite(led_bargraph_4, LOW);
          digitalWrite(led_bargraph_5, LOW);
        }
      break;

      case 4:
        if(b_bargraph_alt == true) {
          for(int i = 0; i < 28; i++) {
            if(i < 24) {
              ht_bargraph.setLedNow(i_bargraph[i]);
            }
            else {
              ht_bargraph.clearLedNow(i_bargraph[i]);
            }
          }
        }
        else {
          digitalWrite(led_bargraph_1, LOW);
          digitalWrite(led_bargraph_2, LOW);
          digitalWrite(led_bargraph_3, LOW);
          digitalWrite(led_bargraph_4, LOW);
          digitalWrite(led_bargraph_5, HIGH);
        }
      break;

      case 3:
        if(b_bargraph_alt == true) {
          for(int i = 0; i < 28; i++) {
            if(i < 18) {
              ht_bargraph.setLedNow(i_bargraph[i]);
            }
            else {
              ht_bargraph.clearLedNow(i_bargraph[i]);
            }
          }
        }
        else {      
          digitalWrite(led_bargraph_1, LOW);
          digitalWrite(led_bargraph_2, LOW);
          digitalWrite(led_bargraph_3, LOW);
          digitalWrite(led_bargraph_4, HIGH);
          digitalWrite(led_bargraph_5, HIGH);
        }
      break;

      case 2:
        if(b_bargraph_alt == true) {
          for(int i = 0; i < 28; i++) {
            if(i < 12) {
              ht_bargraph.setLedNow(i_bargraph[i]);
            }
            else {
              ht_bargraph.clearLedNow(i_bargraph[i]);
            }
          }
        }
        else {   
          digitalWrite(led_bargraph_1, LOW);
          digitalWrite(led_bargraph_2, LOW);
          digitalWrite(led_bargraph_3, HIGH);
          digitalWrite(led_bargraph_4, HIGH);
          digitalWrite(led_bargraph_5, HIGH);
        }
      break;

      case 1:
        if(b_bargraph_alt == true) {
          for(int i = 0; i < 28; i++) {
            if(i < 6) {
              ht_bargraph.setLedNow(i_bargraph[i]);
            }
            else {
              ht_bargraph.clearLedNow(i_bargraph[i]);
            }
          }
        }
        else {  
          digitalWrite(led_bargraph_1, LOW);
          digitalWrite(led_bargraph_2, HIGH);
          digitalWrite(led_bargraph_3, HIGH);
          digitalWrite(led_bargraph_4, HIGH);
          digitalWrite(led_bargraph_5, HIGH);
        }
      break;
    }
  }
}

// Change the WAND_STATE here based on switches changing or pressed.
void checkSwitches() {
  if(b_debug == true) {
    Serial.print(F("A6 -> "));
    Serial.println(analogRead(switch_mode));  

    Serial.print(F("\n"));
    
    Serial.print(F("A7 -> "));
    Serial.println(analogRead(switch_barrel));
  }
  
  if(ms_intensify_timer.justFinished()) {
    ms_intensify_timer.stop();
  }
        
  switch(WAND_STATUS) {
    case MODE_OFF:
     if(switch_activate.isPressed() && WAND_ACTION_STATUS == ACTION_IDLE) {
        // Turn wand and pack on.
        WAND_ACTION_STATUS = ACTION_ACTIVATE;
      }
      
      soundBeepLoopStop();
    break;

    case MODE_ON:
      // This is for when the mode switch is enabled for video game mode. b_cross_the_streams must not be true.
      if(WAND_ACTION_STATUS != ACTION_FIRING && WAND_ACTION_STATUS != ACTION_OFF && WAND_ACTION_STATUS != ACTION_OVERHEATING && b_cross_the_streams != true) {
        if(analogRead(switch_mode) > i_switch_mode_value && ms_switch_mode_debounce.justFinished()) {     
          // Only exit the settings menu when on menu #5 and or cycle through modes when the settings menu is on menu #5
          if(i_wand_menu == 5) {
            // Cycle through the firing modes and setting menu.
            if(FIRING_MODE == PROTON) {
              FIRING_MODE = SLIME;
            }
            else if(FIRING_MODE == SLIME) {
              FIRING_MODE = STASIS;
            }
            else if(FIRING_MODE == STASIS) {
              FIRING_MODE = MESON;
            }
            else if(FIRING_MODE == MESON) {
              FIRING_MODE = SETTINGS;
            }
            else {
              FIRING_MODE = PROTON;
            }
   
            w_trig.trackPlayPoly(S_CLICK);
  
            switch(FIRING_MODE) {
              case SETTINGS:
                WAND_ACTION_STATUS = ACTION_SETTINGS;
                i_wand_menu = 5;
                ms_settings_blinking.start(i_settings_blinking_delay);
  
                // Tell the pack we are in settings mode.
                Serial.write(9);
              break;
  
              case MESON:
                WAND_ACTION_STATUS = ACTION_IDLE;
                wandHeatUp();
  
                // Tell the pack we are in meson mode.
                Serial.write(8);
              break;
  
              case STASIS:
                WAND_ACTION_STATUS = ACTION_IDLE;
                wandHeatUp();
  
                // Tell the pack we are in stasis mode.
                Serial.write(7);
              break;
  
              case SLIME:
                WAND_ACTION_STATUS = ACTION_IDLE;
                wandHeatUp();
  
                // Tell the pack we are in slime mode.
                Serial.write(6);
              break;
  
              case PROTON:
                WAND_ACTION_STATUS = ACTION_IDLE;
                wandHeatUp();
  
                // Tell the pack we are in proton mode.
                Serial.write(5);
              break;
            }
          }
          
          ms_switch_mode_debounce.start(a_switch_debounce_time);
        }
      }

      // Vent light and first stage of the safety system.
      if(switch_vent.getState() == LOW) {
        // Vent light and top white light on.
        digitalWrite(led_vent, LOW);

        soundIdleStart();

        if(switch_wand.getState() == LOW) {
          if(b_beeping != true) {
            // Beep loop.
            soundBeepLoop();
          }  
        }
        else {
          soundBeepLoopStop();
        }
      }
      else if(switch_vent.getState() == HIGH) {        
        // Vent light and top white light off.
        digitalWrite(led_vent, HIGH);

        soundBeepLoopStop();
        soundIdleStop();
      }
      
      if(WAND_ACTION_STATUS != ACTION_SETTINGS && WAND_ACTION_STATUS != ACTION_OVERHEATING) {
        if(switch_intensify.getState() == LOW && ms_intensify_timer.isRunning() != true && switch_wand.getState() == LOW && switch_vent.getState() == LOW && switch_activate.getState() == LOW && b_pack_on == true && analogRead(switch_barrel) < i_switch_barrel_value) {          
          if(WAND_ACTION_STATUS != ACTION_FIRING) {
            WAND_ACTION_STATUS = ACTION_FIRING;
          }
        
          b_firing_intensify = true;
        }

        // When the mode switch is changed to a alternate firing button. Video game modes are disabled and the wand menu settings can only be accessed when the Neutrona wand is powered down.
        if(b_cross_the_streams == true) {
          if(analogRead(switch_mode) > i_switch_mode_value && ms_switch_mode_debounce.justFinished() && switch_wand.getState() == LOW && switch_vent.getState() == LOW && switch_activate.getState() == LOW && b_pack_on == true && analogRead(switch_barrel) < i_switch_barrel_value) {
            if(WAND_ACTION_STATUS != ACTION_FIRING) {
              WAND_ACTION_STATUS = ACTION_FIRING;
            }

            b_firing_alt = true;

            ms_switch_mode_debounce.start(a_switch_debounce_time);
          }
          else if(analogRead(switch_mode) < i_switch_mode_value && ms_switch_mode_debounce.justFinished()) {
            if(b_firing_intensify != true && WAND_ACTION_STATUS == ACTION_FIRING) {
              WAND_ACTION_STATUS = ACTION_IDLE;
            }

            b_firing_alt = false;

            ms_switch_mode_debounce.start(a_switch_debounce_time);
          }
        }

        if(switch_intensify.getState() == HIGH && b_firing == true && b_firing_intensify == true) {
          if(b_firing_alt != true) {
            WAND_ACTION_STATUS = ACTION_IDLE;
          }
          
          b_firing_intensify = false;
        }
      
        if(switch_activate.getState() == HIGH) {
          WAND_ACTION_STATUS = ACTION_OFF;
        }
      }
      else if(WAND_ACTION_STATUS == ACTION_OVERHEATING) {
        if(switch_activate.getState() == HIGH) {
          WAND_ACTION_STATUS = ACTION_OFF;
        }
      }
    break;
  }
}

void wandOff() {
  // Tell the pack the wand is turned off.
  Serial.write(2);

  if(FIRING_MODE == SETTINGS) {
    // If the wand is shut down while we are in settings mode (can happen if the pack is manually turned off), switch the wand and pack to proton mode.
    Serial.write(5);
    FIRING_MODE = PROTON;
  }
  
  WAND_STATUS = MODE_OFF;
  WAND_ACTION_STATUS = ACTION_IDLE;

  soundBeepLoopStop();
  soundIdleStop();
  soundIdleLoopStop();
  
  vibrationOff();
  
  // Stop firing if the wand is turned off.
  if(b_firing == true) {
    modeFireStop();
  }

  w_trig.trackStop(S_AFTERLIFE_GUN_LOOP_1);
  w_trig.trackStop(S_AFTERLIFE_GUN_LOOP_2);
  
  w_trig.trackStop(S_AFTERLIFE_GUN_RAMP_1);
  w_trig.trackStop(S_AFTERLIFE_GUN_RAMP_2);
  w_trig.trackStop(S_AFTERLIFE_GUN_RAMP_DOWN_1);
  w_trig.trackStop(S_AFTERLIFE_GUN_RAMP_DOWN_2);
  w_trig.trackStop(S_BOOTUP);

  // Turn off any overheating sounds.
  w_trig.trackStop(S_CLICK);
  w_trig.trackStop(S_VENT_DRY);

  w_trig.trackStop(S_FIRE_START_SPARK);
  w_trig.trackStop(S_PACK_SLIME_OPEN);
  w_trig.trackStop(S_STASIS_START);
  w_trig.trackStop(S_MESON_START);

  w_trig.trackPlayPoly(S_WAND_SHUTDOWN);
  w_trig.trackPlayPoly(S_AFTERLIFE_GUN_RAMP_DOWN_1, true);
  
  // Turn off some timers.
  ms_bargraph.stop();
  ms_bargraph_alt.stop();
  ms_bargraph_firing.stop();
  ms_overheat_initiate.stop();
  ms_overheating.stop();
  ms_settings_blinking.stop();
    
  // Turn off remaining lights.
  wandLightsOff();
  barrelLightsOff();

  switch(year_mode) {
    case 2021:
      i_bargraph_multiplier_current  = i_bargraph_multiplier_ramp_2021;
    break;

    case 1984:
      i_bargraph_multiplier_current  = i_bargraph_multiplier_ramp_1984;
    break;
  }
}

void modeActivate() {
  // Tell the pack the wand is turned on.
  Serial.write(1);
  
  WAND_STATUS = MODE_ON;
  WAND_ACTION_STATUS = ACTION_IDLE;
  
  // Ramp up the bargraph.
  switch(year_mode) {
    case 2021:
      i_bargraph_multiplier_current  = i_bargraph_multiplier_ramp_2021;
    break;

    case 1984:
      i_bargraph_multiplier_current  = i_bargraph_multiplier_ramp_1984;
    break;
  }

  bargraphRampUp();
  
  // Turn on slo-blo light.
  analogWrite(led_slo_blo, 255);

  // Top white light.
  ms_white_light.start(d_white_light_interval);
  digitalWrite(led_white, LOW);

  switch(year_mode) {
    case 1984:
      w_trig.trackPlayPoly(S_CLICK, true);
    break;

    default:
      soundIdleLoop(true);

      w_trig.trackPlayPoly(S_AFTERLIFE_GUN_RAMP_1, true); // Start track
      
      ms_gun_loop_1.start(620);
    break;
  }
}

void soundIdleLoop(bool fade) {      
  switch(i_power_mode) {
    case 1:
      if(fade == true) {
        w_trig.trackGain(S_IDLE_LOOP_GUN_1, i_volume - 20);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_1, true);
        w_trig.trackFade(S_IDLE_LOOP_GUN_1, i_volume, 1000, 0);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_1, 1);
      }
      else {
        w_trig.trackGain(S_IDLE_LOOP_GUN_1, i_volume);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_1, true);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_1, 1);
      }
     break;

     case 2:
      if(fade == true) {
        w_trig.trackGain(S_IDLE_LOOP_GUN_1, i_volume - 20);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_1, true);
        w_trig.trackFade(S_IDLE_LOOP_GUN_1, i_volume, 1000, 0);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_1, 1);
      }
      else {
        w_trig.trackGain(S_IDLE_LOOP_GUN_1, i_volume);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_1, true);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_1, 1);
      }
     break;

     case 3:
      if(fade == true) {
        w_trig.trackGain(S_IDLE_LOOP_GUN_2, i_volume - 20);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_2, true);
        w_trig.trackFade(S_IDLE_LOOP_GUN_2, i_volume, 1000, 0);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_2, 1);
      }
      else {
        w_trig.trackGain(S_IDLE_LOOP_GUN_2, i_volume);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_2, true);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_2, 1);
      }
     break;

     case 4:
      if(fade == true) {
        w_trig.trackGain(S_IDLE_LOOP_GUN_2, i_volume - 20);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_2, true);
        w_trig.trackFade(S_IDLE_LOOP_GUN_2, i_volume, 1000, 0);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_2, 1);
      }
      else {
        w_trig.trackGain(S_IDLE_LOOP_GUN_2, i_volume);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_2, true);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_2, 1);
      }
     break;

     case 5:
      if(fade == true) {
        w_trig.trackGain(S_IDLE_LOOP_GUN_5, i_volume - 20);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_5, true);
        w_trig.trackFade(S_IDLE_LOOP_GUN_5, i_volume, 1000, 0);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_5, 1);
      }
      else {
        w_trig.trackGain(S_IDLE_LOOP_GUN_5, i_volume);
        w_trig.trackPlayPoly(S_IDLE_LOOP_GUN_5, true);
        w_trig.trackLoop(S_IDLE_LOOP_GUN_5, 1);
      }
     break;
    }
}

void soundIdleLoopStop() {
  w_trig.trackStop(S_IDLE_LOOP_GUN);
  w_trig.trackStop(S_IDLE_LOOP_GUN_1);
  w_trig.trackStop(S_IDLE_LOOP_GUN_2);
  w_trig.trackStop(S_IDLE_LOOP_GUN_3);
  w_trig.trackStop(S_IDLE_LOOP_GUN_4);
  w_trig.trackStop(S_IDLE_LOOP_GUN_5);
}

void soundIdleStart() {
  if(b_sound_idle == false) {  
    switch(year_mode) {
      case 1984:
          w_trig.trackPlayPoly(S_BOOTUP, true);

          soundIdleLoop(true);        
      break;
  
      default:
          w_trig.trackStop(S_AFTERLIFE_GUN_RAMP_1);
          w_trig.trackStop(S_AFTERLIFE_GUN_RAMP_DOWN_1);
          w_trig.trackStop(S_AFTERLIFE_GUN_RAMP_DOWN_2);
          w_trig.trackStop(S_AFTERLIFE_GUN_LOOP_1);
          
          w_trig.trackPlayPoly(S_AFTERLIFE_GUN_RAMP_2, true); // Start track

          ms_gun_loop_1.stop();
          ms_gun_loop_2.start(700);
      break;
    }
  }

  if(b_sound_idle == false) {
    b_sound_idle = true;
  }

  if(year_mode == 2021) {
    if(ms_gun_loop_2.justFinished()) {
      w_trig.trackPlayPoly(S_AFTERLIFE_GUN_LOOP_2, true);
      w_trig.trackLoop(S_AFTERLIFE_GUN_LOOP_2, 1);
      
      ms_gun_loop_2.stop();
    }
  }
}

void soundIdleStop() {
  if(b_sound_idle == true) {
    switch(year_mode) {
      case 1984:
        w_trig.trackPlayPoly(S_WAND_SHUTDOWN, true);
      break;

      default:
        w_trig.trackPlayPoly(S_AFTERLIFE_GUN_RAMP_DOWN_2, true);
        ms_gun_loop_1.start(1400);
        ms_gun_loop_2.stop();
      break;
    }
  }
  
  b_sound_idle = false;

  switch(year_mode) {
    case 1984:
      w_trig.trackStop(S_BOOTUP);
      soundIdleLoopStop();
    break;

    case 2021:
      w_trig.trackStop(S_AFTERLIFE_GUN_RAMP_2);
      w_trig.trackStop(S_AFTERLIFE_GUN_LOOP_2);
    break;
  }
}

void soundBeepLoopStop() {
  if(b_beeping == true) {
    b_beeping = false;
    
    w_trig.trackStop(S_AFTERLIFE_BEEP_WAND);
    w_trig.trackStop(S_AFTERLIFE_BEEP_WAND_S1);
    w_trig.trackStop(S_AFTERLIFE_BEEP_WAND_S2);
    w_trig.trackStop(S_AFTERLIFE_BEEP_WAND_S3);
    w_trig.trackStop(S_AFTERLIFE_BEEP_WAND_S4);
    w_trig.trackStop(S_AFTERLIFE_BEEP_WAND_S5);
    
    ms_reset_sound_beep.stop();
    ms_reset_sound_beep.start(i_sound_timer);
  }
}
void soundBeepLoop() {  
  if(ms_reset_sound_beep.justFinished()) {
    if(b_beeping == false) {
      switch(i_power_mode) {
        case 1:
          w_trig.trackPlayPoly(S_AFTERLIFE_BEEP_WAND_S1, true);
          
          if(year_mode != 1984) {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S1, 1);
          }
          else {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S1, 0);
          }
         break;
  
         case 2:
          w_trig.trackPlayPoly(S_AFTERLIFE_BEEP_WAND_S2, true);
          
          if(year_mode != 1984) {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S2, 1);
          }
          else {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S2, 0);
          }
         break;
  
         case 3:
          w_trig.trackPlayPoly(S_AFTERLIFE_BEEP_WAND_S3, true);
          
          if(year_mode != 1984) {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S3, 1);
          }
          else {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S3, 0);
          }
         break;
  
         case 4:
          w_trig.trackPlayPoly(S_AFTERLIFE_BEEP_WAND_S4, true);
          
          if(year_mode != 1984) {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S4, 1);
          }
          else {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S4, 0);
          }
         break;
  
         case 5:
          w_trig.trackPlayPoly(S_AFTERLIFE_BEEP_WAND_S5, true);
          
          if(year_mode != 1984) {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S5, 1);
          }
          else {
            w_trig.trackLoop(S_AFTERLIFE_BEEP_WAND_S5, 0);
          }
         break;
      }
      
      b_beeping = true;
      
      ms_reset_sound_beep.stop();
    }
  }
}

void modeFireStartSounds() {
  ms_firing_start_sound_delay.stop();

  // Some sparks for firing start.
  w_trig.trackPlayPoly(S_FIRE_START_SPARK);
  
  switch(FIRING_MODE) {
    case PROTON:
        if(b_firing_intensify == true) {
          // Reset some sound triggers.
          b_sound_firing_intensify_trigger = true;
          w_trig.trackPlayPoly(S_FIRE_LOOP_GUN, true);
          w_trig.trackLoop(S_FIRE_LOOP_GUN, 1);
        }
        else {
          b_sound_firing_intensify_trigger = false;
        }

        if(b_firing_alt == true) {
          // Reset some sound triggers.
          b_sound_firing_alt_trigger = true;

          w_trig.trackPlayPoly(S_FIRING_LOOP_GB1, true);
          w_trig.trackLoop(S_FIRING_LOOP_GB1, 1);          
        }
        else {
          b_sound_firing_alt_trigger = false;
        }

        w_trig.trackPlayPoly(S_FIRE_START);        
    break;

    case SLIME:
      w_trig.trackPlayPoly(S_SLIME_START);
      
      w_trig.trackPlayPoly(S_SLIME_LOOP, true);
      w_trig.trackLoop(S_SLIME_LOOP, 1);
    break;

    case STASIS:
      w_trig.trackPlayPoly(S_STASIS_START);
      
      w_trig.trackPlayPoly(S_STASIS_LOOP, true);
      w_trig.trackLoop(S_STASIS_LOOP, 1);
    break;

    case MESON:
      w_trig.trackPlayPoly(S_MESON_START);
      
      w_trig.trackPlayPoly(S_MESON_LOOP, true);
      w_trig.trackLoop(S_MESON_LOOP, 1);
    break;

    case SETTINGS:
      // Nothing.
    break;
  }  
}

void modeFireStart() {
  // Reset some sound triggers.
  b_sound_firing_intensify_trigger = true;
  b_sound_firing_alt_trigger = true;
  b_sound_firing_cross_the_streams = false;
  b_firing_cross_streams = false;

  if(ms_intensify_timer.isRunning() != true) {
    ms_intensify_timer.start(i_intensify_delay);
  }
  
  // Tell the Proton Pack that the Neutrona wand is firing in Intensify mode.
  if(b_firing_intensify == true) {
    Serial.write(21);
  }

  // Tell the Proton Pack that the Neutrona wand is firing in Alt mode.
  if(b_firing_alt == true) {
    Serial.write(23);
  }

  // Stop all firing sounds first.
  switch(FIRING_MODE) {
    case PROTON:
      w_trig.trackStop(S_FIRE_LOOP);
      w_trig.trackStop(S_FIRE_LOOP_GUN);
      w_trig.trackStop(S_FIRING_LOOP_GB1);

      w_trig.trackStop(S_FIRE_START);
      w_trig.trackStop(S_FIRE_START_SPARK);
      w_trig.trackStop(S_FIRING_END_GUN);
      w_trig.trackStop(S_FIRE_LOOP_IMPACT);
    break;

    case SLIME:
      w_trig.trackStop(S_SLIME_START);
      w_trig.trackStop(S_SLIME_LOOP);
      w_trig.trackStop(S_SLIME_END);
    break;

    case STASIS:
      w_trig.trackStop(S_STASIS_START);
      w_trig.trackStop(S_STASIS_LOOP);
      w_trig.trackStop(S_STASIS_END);
    break;

    case MESON:
      w_trig.trackStop(S_MESON_START);
      w_trig.trackStop(S_MESON_LOOP);
      w_trig.trackStop(S_MESON_END);
    break;

    case SETTINGS:
      // Nothing.
    break;
  }

  ms_firing_start_sound_delay.start(i_fire_stop_sound_delay);

  // Tell the pack the wand is firing.
  Serial.write(3);

  ms_overheat_initiate.stop();

  bool b_overheat_flag = true;

  if(b_cross_the_streams == true && b_firing_alt != true) {
    b_overheat_flag = false;
  }

  if(b_overheat_flag == true) {
    // If in high power mode on the wand, start a overheat timer.
    if(b_overheat_mode[i_power_mode - 1] == true) {
      ms_overheat_initiate.start(i_ms_overheat_initiate[i_power_mode - 1]);
    }
    else if(b_cross_the_streams == true) {
      if(b_firing_alt == true) {
        ms_overheat_initiate.start(i_ms_overheat_initiate[i_power_mode - 1]);
      }
    }
  }
  
  barrelLightsOff();
  
  ms_firing_lights.start(10);
  i_barrel_light = 0;

  // Stop any bargraph ramps.
  ms_bargraph.stop();
  ms_bargraph_alt.stop();
  b_bargraph_up = false;
  i_bargraph_status = 1;
  i_bargraph_status_alt = 0;
  bargraphRampFiring();

  ms_impact.start(random(10,15) * 1000);
}

void modeFireStopSounds() {
  // Reset some sound triggers.
  b_sound_firing_intensify_trigger = false;
  b_sound_firing_alt_trigger = false;
  b_sound_firing_cross_the_streams = false;

  ms_firing_stop_sound_delay.stop();

 switch(FIRING_MODE) {
    case PROTON:
      w_trig.trackPlayPoly(S_FIRING_END_GUN, true);
    break;

    case SLIME:
      w_trig.trackPlayPoly(S_SLIME_END, true);
    break;

    case STASIS:
      w_trig.trackPlayPoly(S_STASIS_END, true);
    break;

    case MESON:
      w_trig.trackPlayPoly(S_MESON_END, true);
    break;

    case SETTINGS:
      // Nothing.
    break;
  }

  if(b_firing_cross_streams == true) {
    w_trig.trackPlayPoly(S_CROSS_STREAMS_END, true);

    b_firing_cross_streams = false;
  }
}

void modeFireStop() {
  ms_overheat_initiate.stop();
  
  // Tell the pack the wand stopped firing.
  Serial.write(4);
  
  WAND_ACTION_STATUS = ACTION_IDLE;
  
  b_firing = false;
  b_firing_intensify = false;
  b_firing_alt = false;

  ms_bargraph_firing.stop();
  ms_bargraph_alt.stop(); // Stop the 1984 24 segment optional bargraph timer just in case.
  b_bargraph_up = false;

  i_bargraph_status = i_power_mode - 1;
  i_bargraph_status_alt = 0;
  bargraphClearAlt();

  switch(year_mode) {
    case 2021:
      i_bargraph_multiplier_current  = i_bargraph_multiplier_ramp_2021 / 3;
    break;

    case 1984:
      i_bargraph_multiplier_current  = i_bargraph_multiplier_ramp_1984 / 2;
    break;
  }
  bargraphRampUp();
  
  ms_firing_stream_blue.stop();
  ms_firing_lights.stop();
  ms_impact.stop();

  i_barrel_light = 0;
  ms_firing_lights_end.start(10);

  // Stop all other firing sounds.
  switch(FIRING_MODE) {
    case PROTON:
      w_trig.trackStop(S_FIRE_LOOP);
      w_trig.trackStop(S_FIRE_LOOP_GUN);
      w_trig.trackStop(S_FIRING_LOOP_GB1);
      w_trig.trackStop(S_FIRING_END_GUN);
      w_trig.trackStop(S_FIRE_START);
      w_trig.trackStop(S_FIRE_START_SPARK);
      w_trig.trackStop(S_FIRE_LOOP_IMPACT);
    break;

    case SLIME:
      w_trig.trackStop(S_SLIME_START);
      w_trig.trackStop(S_SLIME_LOOP);
      w_trig.trackStop(S_SLIME_END);
    break;

    case STASIS:
      w_trig.trackStop(S_STASIS_START);
      w_trig.trackStop(S_STASIS_LOOP);
      w_trig.trackStop(S_STASIS_END);
    break;

    case MESON:
      w_trig.trackStop(S_MESON_START);
      w_trig.trackStop(S_MESON_LOOP);
      w_trig.trackStop(S_MESON_END);
    break;

    case SETTINGS:
      // Nothing.
    break;
  }

  // A tiny ramp down delay, helps with the sounds.
  ms_firing_stop_sound_delay.start(i_fire_stop_sound_delay);
}

void modeFiring() {
  // Sound trigger flags.
  if(b_firing_intensify == true && b_sound_firing_intensify_trigger != true) {
    // Tell the Proton Pack that the Neutrona wand is firing in Intensify mode.
    Serial.write(21);

    b_sound_firing_intensify_trigger = true;
    w_trig.trackPlayPoly(S_FIRE_LOOP_GUN, true);
    w_trig.trackLoop(S_FIRE_LOOP_GUN, 1);
  }

  if(b_firing_intensify != true && b_sound_firing_intensify_trigger == true) {
    // Tell the Proton Pack that the Neutrona wand is no longer firing in Intensify mode.
    Serial.write(22);

    b_sound_firing_intensify_trigger = false;
    w_trig.trackStop(S_FIRE_LOOP_GUN);
  }

  if(b_firing_alt == true && b_sound_firing_alt_trigger != true) {
    // Tell the Proton Pack that the Neutrona wand is firing in Alt mode.
    Serial.write(23);

    b_sound_firing_alt_trigger = true;
    w_trig.trackPlayPoly(S_FIRING_LOOP_GB1, true);
    w_trig.trackLoop(S_FIRING_LOOP_GB1, 1);
  }

  if(b_firing_alt != true && b_sound_firing_alt_trigger == true) {
    // Tell the Proton Pack that the Neutrona wand is firing in Alt mode.
    Serial.write(24);

    b_sound_firing_alt_trigger = false;
    w_trig.trackStop(S_FIRING_LOOP_GB1);
  }

  if(b_firing_alt == true && b_firing_intensify == true && b_sound_firing_cross_the_streams != true && b_firing_cross_streams != true) {
    // Tell the Proton Pack that the Neutrona wand is crossing the streams.
    Serial.write(25);

    b_firing_cross_streams = true;
    b_sound_firing_cross_the_streams = true;
    w_trig.trackPlayPoly(S_CROSS_STREAMS_START, true);
    w_trig.trackPlayPoly(S_FIRE_START_SPARK);
    w_trig.trackPlayPoly(S_FIRE_LOOP, true);
    w_trig.trackLoop(S_FIRE_LOOP, 1);
  }

  if((b_firing_alt != true || b_firing_intensify != true) && b_firing_cross_streams == true) {
    // Tell the Proton Pack that the Neutrona wand is no longer crossing the streams.
    Serial.write(26);

    b_firing_cross_streams = false;
    b_sound_firing_cross_the_streams = false;
    w_trig.trackPlayPoly(S_CROSS_STREAMS_END, true);
    w_trig.trackStop(S_FIRE_LOOP);
  }

  // Overheat timers.
  bool b_overheat_flag = true;
  
  if(b_cross_the_streams == true && b_firing_alt != true) {
    b_overheat_flag = false;
  }

  if(b_overheat_flag == true) {
    // If the user changes the wand power output while firing, turn off the overheat timer.
    if(b_overheat_mode[i_power_mode - 1] != true && ms_overheat_initiate.isRunning()) {
      ms_overheat_initiate.stop();
      
      // Tell the pack to revert back to regular cyclotron speeds.
      Serial.write(12);
    }
    else if(b_overheat_mode[i_power_mode - 1] == true && ms_overheat_initiate.remaining() == 0) {
      // If the user changes back to power mode that overheats while firing, start up a timer.
      ms_overheat_initiate.start(i_ms_overheat_initiate[i_power_mode - 1]);
    }
  }
  else {
    if(ms_overheat_initiate.isRunning()) {
      ms_overheat_initiate.stop();
      
      // Tell the pack to revert back to regular cyclotron speeds.
      Serial.write(12);
    }
  }

  /*
   * CRGB 
   * R = green
   * G = red
   * B = blue
   * 
   * yellow = 255, 255, 0
   * mid-yellow = 150,255,0
   * orange = 40, 255, 0
   * dark orange = 20, 255, 0
   */
  switch(FIRING_MODE) {     
    case PROTON:
      // Make the stream more slightly more red on higher power modes.
      switch(i_power_mode) {
        case 1:
          fireStreamStart(255, 20, 0);
        break;
      
        case 2:
          fireStreamStart(255, 30, 0);
        break;
      
        case 3:
          fireStreamStart(255, 40, 0);
        break;
      
        case 4:
          fireStreamStart(255, 60, 0);
        break;
      
        case 5:
          fireStreamStart(255, 70, 0);
        break;
      
        default:
          fireStreamStart(255, 20, 0);
        break;
      }
          
      fireStream(0, 0, 255);     
    break;
      
    case SLIME:
       fireStreamStart(0, 255, 45);
       fireStream(20, 200, 45);  
    break;

    case STASIS:
       fireStreamStart(0, 45, 100);
       fireStream(0, 100, 255);  
    break;

    case MESON:
       fireStreamStart(200, 200, 20);
       fireStream(190, 20, 70);
    break;

    case SETTINGS:
      // Nothing.
    break;
  }

  // Bargraph loop / scroll.
  if(ms_bargraph_firing.justFinished()) {
    bargraphRampFiring();
  }

  // Mix some impact sound every 10-15 seconds while firing.
  if(ms_impact.justFinished()) {
    w_trig.trackPlayPoly(S_FIRE_LOOP_IMPACT);
    ms_impact.start(15000);
  }
}

void wandHeatUp() {
  w_trig.trackStop(S_FIRE_START_SPARK);
  w_trig.trackStop(S_PACK_SLIME_OPEN);
  w_trig.trackStop(S_STASIS_OPEN);
  w_trig.trackStop(S_MESON_OPEN);

  switch(FIRING_MODE) {
    case PROTON:
      w_trig.trackPlayPoly(S_FIRE_START_SPARK);
    break;

    case SLIME:
      w_trig.trackPlayPoly(S_PACK_SLIME_OPEN);
    break;

    case STASIS:
      w_trig.trackPlayPoly(S_STASIS_OPEN);
    break;

    case MESON:
      w_trig.trackPlayPoly(S_MESON_OPEN);
    break;

    case SETTINGS:
      // Nothing.
    break;
  }

  wandBarrelPreHeatUp();
}

void wandBarrelPreHeatUp() {
  i_heatup_counter = 0;
  i_heatdown_counter = 100;
  ms_wand_heatup_fade.start(i_delay_heatup);
}

void wandBarrelHeatUp() {
  if(i_heatup_counter > 100) {
    wandBarrelHeatDown();
  }
  else if(ms_wand_heatup_fade.justFinished() && i_heatup_counter <= 100) {        
    switch(FIRING_MODE) {
      case PROTON:
        barrel_leds[BARREL_NUM_LEDS - 1] = CRGB(i_heatup_counter, i_heatup_counter, i_heatup_counter);
        ms_fast_led.start(i_fast_led_delay);
      break;
  
      case SLIME:
        barrel_leds[BARREL_NUM_LEDS - 1] = CRGB(i_heatup_counter, 0, 0);
        ms_fast_led.start(i_fast_led_delay);
      break;
  
      case STASIS:
        barrel_leds[BARREL_NUM_LEDS - 1] = CRGB(0, 0, i_heatup_counter);
        ms_fast_led.start(i_fast_led_delay);
      break;
  
      case MESON:
        barrel_leds[BARREL_NUM_LEDS - 1] = CRGB(i_heatup_counter, i_heatup_counter, 0);
        ms_fast_led.start(i_fast_led_delay);
      break;

      case SETTINGS:
        // nothing
      break;
    } 

    i_heatup_counter++;
    ms_wand_heatup_fade.start(i_delay_heatup);
  }
}

void wandBarrelHeatDown() {
  if(ms_wand_heatup_fade.justFinished() && i_heatdown_counter > 0) {
    switch(FIRING_MODE) {
      case PROTON:
        barrel_leds[BARREL_NUM_LEDS - 1] = CRGB(i_heatdown_counter, i_heatdown_counter, i_heatdown_counter);
        ms_fast_led.start(i_fast_led_delay);
      break;
  
      case SLIME:
        barrel_leds[BARREL_NUM_LEDS - 1] = CRGB(i_heatdown_counter, 0, 0);
        ms_fast_led.start(i_fast_led_delay);
      break;
  
      case STASIS:
        barrel_leds[BARREL_NUM_LEDS - 1] = CRGB(0, 0, i_heatdown_counter);
        ms_fast_led.start(i_fast_led_delay);
      break;
  
      case MESON:
        barrel_leds[BARREL_NUM_LEDS - 1] = CRGB(i_heatdown_counter, i_heatdown_counter, 0);
        ms_fast_led.start(i_fast_led_delay);
      break;

      case SETTINGS:
        // Nothing.
      break;
    }

    i_heatdown_counter--;
    ms_wand_heatup_fade.start(i_delay_heatup);
  }
  
  if(i_heatdown_counter == 0) {
    barrelLightsOff();
  }
}

void fireStream(int r, int g, int b) {
  if(ms_firing_stream_blue.justFinished()) {
    if(i_barrel_light - 1 > -1 && i_barrel_light - 1 < BARREL_NUM_LEDS) {      
      switch(FIRING_MODE) {
        case PROTON:
          barrel_leds[i_barrel_light - 1] = CRGB(10, 255, 0);

          // Make the stream more slightly more red on higher power modes.
          switch(i_power_mode) {
            case 1:
              barrel_leds[i_barrel_light - 1] = CRGB(10, 255, 0);
            break;

            case 2:
              barrel_leds[i_barrel_light - 1] = CRGB(20, 255, 0);
            break;

            case 3:
              barrel_leds[i_barrel_light - 1] = CRGB(30, 255, 0);
            break;

            case 4:
              barrel_leds[i_barrel_light - 1] = CRGB(40, 255, 0);
            break;

            case 5:
              barrel_leds[i_barrel_light - 1] = CRGB(50, 255, 0);
            break;

            default:
              barrel_leds[i_barrel_light - 1] = CRGB(10, 255, 0);
            break;
          }
        break;

        case SLIME:
          barrel_leds[i_barrel_light - 1] = CRGB(120, 20, 45);
        break;

        case STASIS:
          barrel_leds[i_barrel_light - 1] = CRGB(15, 50, 155);
        break;

        case MESON:
          barrel_leds[i_barrel_light - 1] = CRGB(200, 200, 15);
        break;

        case SETTINGS:
          // Nothing.
        break;
      }
      
      ms_fast_led.start(i_fast_led_delay);
    }

    if(i_barrel_light == BARREL_NUM_LEDS) {
      i_barrel_light = 0;
            
      ms_firing_stream_blue.start(d_firing_stream / 2);
    }
    else if(i_barrel_light < BARREL_NUM_LEDS) {
      barrel_leds[i_barrel_light] = CRGB(g,r,b);

      ms_fast_led.start(i_fast_led_delay);
            
      ms_firing_stream_blue.start(d_firing_lights);
  
      i_barrel_light++;
    }
  }
}

void barrelLightsOff() {
  ms_wand_heatup_fade.stop();
  i_heatup_counter = 0;
  i_heatdown_counter = 100;
  
  for(int i = 0; i < BARREL_NUM_LEDS; i++) {
    barrel_leds[i] = CRGB(0,0,0);
  }

  ms_fast_led.start(i_fast_led_delay);
}

void fireStreamStart(int r, int g, int b) {
  if(ms_firing_lights.justFinished() && i_barrel_light < BARREL_NUM_LEDS) {
    barrel_leds[i_barrel_light] = CRGB(g,r,b);
    ms_fast_led.start(i_fast_led_delay);
          
    ms_firing_lights.start(d_firing_lights);

    i_barrel_light++;

    if(i_barrel_light == BARREL_NUM_LEDS) {
      i_barrel_light = 0;
            
      ms_firing_lights.stop();
      ms_firing_stream_blue.start(d_firing_stream);
    }
  }
}

void fireStreamEnd(int r, int g, int b) {
  if(i_barrel_light < BARREL_NUM_LEDS) {
    barrel_leds[i_barrel_light] = CRGB(g,r,b);
    ms_fast_led.start(i_fast_led_delay);
          
    ms_firing_lights_end.start(d_firing_lights);

    i_barrel_light++;

    if(i_barrel_light == BARREL_NUM_LEDS) {
      i_barrel_light = 0;
      
      ms_firing_lights_end.stop();
    }
  }
}

void vibrationWand(int i_level) {
  if(b_vibration_on == true && b_vibration_enabled == true) {
    // Only vibrate the wand during firing only when enabled. (When enabled by the pack)
    if(b_vibration_firing == true) {
      if(WAND_ACTION_STATUS == ACTION_FIRING) {
        if(i_level != i_vibration_level_prev) {
          i_vibration_level_prev = i_level;
          analogWrite(vibration, i_level);
        }
      }
      else {
        analogWrite(vibration, 0);
      }
    }
    else {
      // Wand vibrates, even when idling, etc. (When enabled by the pack)
      if(i_level != i_vibration_level_prev) {
        i_vibration_level_prev = i_level;
        analogWrite(vibration, i_level);
      }
    }
  }
  else {
    analogWrite(vibration, 0);
  }
}

void bargraphRampFiring() {
  // (Optional) 28 Segment barmeter bargraph.
  if(b_bargraph_alt == true) {    
    // Start ramping up and down from the middle to the top/bottom and back to the middle again.
    switch(i_bargraph_status_alt) {
      case 0:
        vibrationWand(i_vibration_level + 110);

        ht_bargraph.setLedNow(i_bargraph[13]);
        ht_bargraph.setLedNow(i_bargraph[14]);

        i_bargraph_status_alt++;

        if(b_bargraph_up == false) {
          ht_bargraph.clearLedNow(i_bargraph[12]);
          ht_bargraph.clearLedNow(i_bargraph[15]);
        }

        b_bargraph_up = true;
      break;

      case 1:
        vibrationWand(i_vibration_level + 110);

        ht_bargraph.setLedNow(i_bargraph[12]);
        ht_bargraph.setLedNow(i_bargraph[15]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[13]);
          ht_bargraph.clearLedNow(i_bargraph[14]);

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[11]);
          ht_bargraph.clearLedNow(i_bargraph[16]);

          i_bargraph_status_alt--;
        }
      break;

      case 2:
        vibrationWand(i_vibration_level + 110);

        ht_bargraph.setLedNow(i_bargraph[11]);
        ht_bargraph.setLedNow(i_bargraph[16]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[12]);
          ht_bargraph.clearLedNow(i_bargraph[15]);

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[10]);
          ht_bargraph.clearLedNow(i_bargraph[17]);

          i_bargraph_status_alt--;
        }
      break;

      case 3:
        vibrationWand(i_vibration_level + 110);

        ht_bargraph.setLedNow(i_bargraph[10]);
        ht_bargraph.setLedNow(i_bargraph[17]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[11]);
          ht_bargraph.clearLedNow(i_bargraph[16]);

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[9]);
          ht_bargraph.clearLedNow(i_bargraph[18]);

          i_bargraph_status_alt--;
        }
      break;

      case 4:
        vibrationWand(i_vibration_level + 110);

        ht_bargraph.setLedNow(i_bargraph[9]);
        ht_bargraph.setLedNow(i_bargraph[18]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[10]);
          ht_bargraph.clearLedNow(i_bargraph[17]);

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[8]);
          ht_bargraph.clearLedNow(i_bargraph[19]);
          
          i_bargraph_status_alt--;
        }
      break;

      case 5:
        vibrationWand(i_vibration_level + 110);

        ht_bargraph.setLedNow(i_bargraph[8]);
        ht_bargraph.setLedNow(i_bargraph[19]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[9]);
          ht_bargraph.clearLedNow(i_bargraph[18]);

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[7]);
          ht_bargraph.clearLedNow(i_bargraph[20]);

          i_bargraph_status_alt--;
        }
      break;

      case 6:
        vibrationWand(i_vibration_level + 112);

        ht_bargraph.setLedNow(i_bargraph[7]);
        ht_bargraph.setLedNow(i_bargraph[20]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[8]);
          ht_bargraph.clearLedNow(i_bargraph[19]);

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[6]);
          ht_bargraph.clearLedNow(i_bargraph[21]);

          i_bargraph_status_alt--;
        }
      break;

      case 7:
        vibrationWand(i_vibration_level + 112);

        ht_bargraph.setLedNow(i_bargraph[6]);
        ht_bargraph.setLedNow(i_bargraph[21]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[7]);
          ht_bargraph.clearLedNow(i_bargraph[20]);

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[5]);
          ht_bargraph.clearLedNow(i_bargraph[22]);

          i_bargraph_status_alt--;
        }
      break;

      case 8:
        vibrationWand(i_vibration_level + 112);

        ht_bargraph.setLedNow(i_bargraph[5]);
        ht_bargraph.setLedNow(i_bargraph[22]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[6]);
          ht_bargraph.clearLedNow(i_bargraph[21]); 

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[4]);
          ht_bargraph.clearLedNow(i_bargraph[23]);

          i_bargraph_status_alt--;
        }
      break;

      case 9:
        vibrationWand(i_vibration_level + 112);

        ht_bargraph.setLedNow(i_bargraph[4]);
        ht_bargraph.setLedNow(i_bargraph[23]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[5]);
          ht_bargraph.clearLedNow(i_bargraph[22]); 

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[3]);
          ht_bargraph.clearLedNow(i_bargraph[24]);

          i_bargraph_status_alt--;
        }
      break;            

      case 10:
        vibrationWand(i_vibration_level + 112);

        ht_bargraph.setLedNow(i_bargraph[3]);
        ht_bargraph.setLedNow(i_bargraph[24]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[4]);
          ht_bargraph.clearLedNow(i_bargraph[23]); 

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[2]);
          ht_bargraph.clearLedNow(i_bargraph[25]);

          i_bargraph_status_alt--;
        }
      break;

      case 11:
        vibrationWand(i_vibration_level + 115);

        ht_bargraph.setLedNow(i_bargraph[2]);
        ht_bargraph.setLedNow(i_bargraph[25]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[3]);
          ht_bargraph.clearLedNow(i_bargraph[24]);

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[1]);
          ht_bargraph.clearLedNow(i_bargraph[26]);

          i_bargraph_status_alt--;
        }
      break;      

      case 12:
        vibrationWand(i_vibration_level + 115);

        ht_bargraph.setLedNow(i_bargraph[1]);
        ht_bargraph.setLedNow(i_bargraph[26]);

        if(b_bargraph_up == true) {
          ht_bargraph.clearLedNow(i_bargraph[2]);
          ht_bargraph.clearLedNow(i_bargraph[25]);

          i_bargraph_status_alt++;
        }
        else {
          ht_bargraph.clearLedNow(i_bargraph[0]);
          ht_bargraph.clearLedNow(i_bargraph[27]);

          i_bargraph_status_alt--;
        }
      break;

      case 13:
        vibrationWand(i_vibration_level + 115);

        ht_bargraph.setLedNow(i_bargraph[0]);
        ht_bargraph.setLedNow(i_bargraph[27]);

        ht_bargraph.clearLedNow(i_bargraph[1]);
        ht_bargraph.clearLedNow(i_bargraph[26]);

        i_bargraph_status_alt--;

        b_bargraph_up = false;
      break;    
    }
  }
  else {
    // Hasbro bargraph.
    switch(i_bargraph_status) {
      case 1:
        vibrationWand(i_vibration_level + 110);
              
        digitalWrite(led_bargraph_1, LOW);
        digitalWrite(led_bargraph_2, HIGH);
        digitalWrite(led_bargraph_3, HIGH);
        digitalWrite(led_bargraph_4, HIGH);
        digitalWrite(led_bargraph_5, LOW);
        i_bargraph_status++;
      break;

      case 2:
        vibrationWand(i_vibration_level + 112);  
        
        digitalWrite(led_bargraph_1, HIGH);
        digitalWrite(led_bargraph_2, LOW);
        digitalWrite(led_bargraph_3, HIGH);
        digitalWrite(led_bargraph_4, LOW);
        digitalWrite(led_bargraph_5, HIGH);
        i_bargraph_status++;
      break;

      case 3:
        vibrationWand(i_vibration_level + 115);
        
        digitalWrite(led_bargraph_1, HIGH);
        digitalWrite(led_bargraph_2, HIGH);
        digitalWrite(led_bargraph_3, LOW);
        digitalWrite(led_bargraph_4, HIGH);
        digitalWrite(led_bargraph_5, HIGH);
        i_bargraph_status++;
      break;

      case 4:
        vibrationWand(i_vibration_level + 112);

        digitalWrite(led_bargraph_1, HIGH);
        digitalWrite(led_bargraph_2, LOW);
        digitalWrite(led_bargraph_3, HIGH);
        digitalWrite(led_bargraph_4, LOW);
        digitalWrite(led_bargraph_5, HIGH);
        i_bargraph_status++;
      break;

      case 5:
        vibrationWand(i_vibration_level + 110);
        
        digitalWrite(led_bargraph_1, LOW);
        digitalWrite(led_bargraph_2, HIGH);
        digitalWrite(led_bargraph_3, HIGH);
        digitalWrite(led_bargraph_4, HIGH);
        digitalWrite(led_bargraph_5, LOW);
        i_bargraph_status = 1;
      break;
    }
  }

  int i_ramp_interval = d_bargraph_ramp_interval;

  if(b_bargraph_alt == true) {
    // Switch to a different ramp speed if using the (Optional) 28 segment barmeter bargraph.
    i_ramp_interval = d_bargraph_ramp_interval_alt;
  }

  // If in power mode on the wand that can overheat, change the speed of the bargraph ramp during firing based on time remaining before we overheat.
  if(b_overheat_mode[i_power_mode - 1] == true && ms_overheat_initiate.isRunning()) {
    if(ms_overheat_initiate.remaining() < i_ms_overheat_initiate[i_power_mode - 1] / 6) {
      if(b_bargraph_alt == true) {
        ms_bargraph_firing.start(i_ramp_interval / i_ramp_interval);
      }
      else {
        ms_bargraph_firing.start(i_ramp_interval / 5);
      }
      
      cyclotronSpeedUp(6);
    }
    else if(ms_overheat_initiate.remaining() < i_ms_overheat_initiate[i_power_mode - 1] / 5) {
      if(b_bargraph_alt == true) {
        ms_bargraph_firing.start(i_ramp_interval / 9);
      }
      else {
        ms_bargraph_firing.start(i_ramp_interval / 4);
      }

      cyclotronSpeedUp(5);
    }
    else if(ms_overheat_initiate.remaining() < i_ms_overheat_initiate[i_power_mode - 1] / 4) {
      if(b_bargraph_alt == true) {
        ms_bargraph_firing.start(i_ramp_interval / 7);
      }
      else {
        ms_bargraph_firing.start(i_ramp_interval / 3.5);
      }

      cyclotronSpeedUp(4);    
    }
    else if(ms_overheat_initiate.remaining() < i_ms_overheat_initiate[i_power_mode - 1] / 3) {
      if(b_bargraph_alt == true) {
        ms_bargraph_firing.start(i_ramp_interval / 5);
      }
      else {      
        ms_bargraph_firing.start(i_ramp_interval / 3);
      }

      cyclotronSpeedUp(3);
    }
    else if(ms_overheat_initiate.remaining() < i_ms_overheat_initiate[i_power_mode - 1] / 2) {
      if(b_bargraph_alt == true) {
        ms_bargraph_firing.start(i_ramp_interval / 3);
      }
      else {      
        ms_bargraph_firing.start(i_ramp_interval / 2.5);
      }

      cyclotronSpeedUp(2);
    }
    else {
      ms_bargraph_firing.start(i_ramp_interval / 2);
      i_cyclotron_speed_up = 1;
    }
  }
  else {  
    ms_bargraph_firing.start(i_ramp_interval / 2);
  }
}

void cyclotronSpeedUp(int i_switch) {
  if(i_switch != i_cyclotron_speed_up) {
    if(i_switch == 4) {
      // Tell pack to start beeping before we overheat it.
      Serial.write(15);

      // Beep the wand 8 times.
      w_trig.trackPlayPoly(S_BEEP_8);
    }
    
    i_cyclotron_speed_up++;
    
    // Tell the pack to speed up the cyclotron.
    Serial.write(13);
  }  
}

/*
 * 2021 mode for optional 28 segment bargraph. Checks if we ramp up or down when changing power levels.
*/
void bargraphPowerCheck2021Alt() {
  if(WAND_ACTION_STATUS != ACTION_FIRING && WAND_ACTION_STATUS != ACTION_SETTINGS && WAND_ACTION_STATUS != ACTION_OVERHEATING) {
    if(i_power_mode != i_power_mode_prev) {
      if(i_power_mode > i_power_mode_prev) {
        b_bargraph_up = true;
      }
      else {
        b_bargraph_up = false;
      }

      switch(i_power_mode) {
        case 5:
          ms_bargraph_alt.start(i_bargraph_wait / 3);
        break;

        case 4:
          ms_bargraph_alt.start(i_bargraph_wait / 4);
        break;

        case 3:
          ms_bargraph_alt.start(i_bargraph_wait / 5);
        break;

        case 2:
          ms_bargraph_alt.start(i_bargraph_wait / 6);
        break;

        case 1:
          ms_bargraph_alt.start(i_bargraph_wait / 7);
        break;
      }
    }
  }
}

void bargraphClearAlt() {
  if(b_bargraph_alt == true) {
    for(int i = 0; i < 28; i++) {
      ht_bargraph.clearLedNow(i_bargraph[i]);
    }

    i_bargraph_status_alt = 0;
  }
}

void bargraphPowerCheck() {
  // Control for the 28 segment barmeter bargraph.
  if(b_bargraph_alt == true) {
    if(ms_bargraph_alt.justFinished()) {
      int i_bargraph_multiplier[5] = { 7, 6, 5, 4, 3 };
      
      if(year_mode == 2021) {
        for(int i = 0; i <= sizeof(i_bargraph_multiplier); i++) {
          i_bargraph_multiplier[i] = 10;
        }
      }

      if(b_bargraph_up == true) {     
        ht_bargraph.setLedNow(i_bargraph[i_bargraph_status_alt]);

        switch(i_power_mode) {
          case 5:
            if(i_bargraph_status_alt > 26) {
              b_bargraph_up = false;

              i_bargraph_status_alt = 27;

              if(year_mode == 2021) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
               // A little pause when we reach the top.
                ms_bargraph_alt.start(i_bargraph_wait / 2);
              }
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_mode - 1]);
            }
          break;

          case 4:
            if(i_bargraph_status_alt > 21) {
              b_bargraph_up = false;

              if(year_mode == 2021) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
                // A little pause when we reach the top.
                ms_bargraph_alt.start(i_bargraph_wait / 2);
              }
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_mode - 1]);
            }
          break;

          case 3:
            if(i_bargraph_status_alt > 16) {
              b_bargraph_up = false;
              if(year_mode == 2021) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
                // A little pause when we reach the top.
                ms_bargraph_alt.start(i_bargraph_wait / 2);
              }
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_mode - 1]);
            }
          break;

          case 2:
            if(i_bargraph_status_alt > 10) {
              b_bargraph_up = false;
              if(year_mode == 2021) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
                // A little pause when we reach the top.
                ms_bargraph_alt.start(i_bargraph_wait / 2);
              }
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_mode - 1]);
            }
          break;

          case 1:
            if(i_bargraph_status_alt > 4) {
              b_bargraph_up = false;
              if(year_mode == 2021) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
                // A little pause when we reach the top.
                ms_bargraph_alt.start(i_bargraph_wait / 2);
              }
            }
            else {
              ms_bargraph_alt.start(i_bargraph_interval * i_bargraph_multiplier[i_power_mode - 1]);
            }
          break;
        }

        if(b_bargraph_up == true) {
          i_bargraph_status_alt++;
        }
      }
      else {
        ht_bargraph.clearLedNow(i_bargraph[i_bargraph_status_alt]);

        if(i_bargraph_status_alt == 0) {
          i_bargraph_status_alt = 0;
          b_bargraph_up = true;
          // A little pause when we reach the bottom.
          ms_bargraph_alt.start(i_bargraph_wait / 2);
        }
        else {
          i_bargraph_status_alt--;

          switch(i_power_mode) {
            case 5:
              if(year_mode == 2021 && i_bargraph_status_alt < 27) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
                ms_bargraph_alt.start(i_bargraph_interval * 3);
              }
            break;

            case 4:
              if(year_mode == 2021 && i_bargraph_status_alt < 22) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
                ms_bargraph_alt.start(i_bargraph_interval * 4);
              }
            break;

            case 3:
              if(year_mode == 2021 && i_bargraph_status_alt < 17) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
                ms_bargraph_alt.start(i_bargraph_interval * 5);
              }
            break; 

            case 2:
              if(year_mode == 2021 && i_bargraph_status_alt < 11) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
                ms_bargraph_alt.start(i_bargraph_interval * 6);
              }
            break;

            case 1:
              if(year_mode == 2021 && i_bargraph_status_alt < 5) {
                // In 2021 mode, we stop when we reach our target.
                ms_bargraph_alt.stop();
              }
              else {
                ms_bargraph_alt.start(i_bargraph_interval * 7);
              }
            break;
          }
        }
      }
    }
  }
  else {
    // Stock haslab bargraph control.
    switch(i_power_mode) {
      case 1:
        digitalWrite(led_bargraph_1, LOW);
        digitalWrite(led_bargraph_2, HIGH);
        digitalWrite(led_bargraph_3, HIGH);
        digitalWrite(led_bargraph_4, HIGH);
        digitalWrite(led_bargraph_5, HIGH);
      break;

      case 2:
        digitalWrite(led_bargraph_1, LOW);
        digitalWrite(led_bargraph_2, LOW);
        digitalWrite(led_bargraph_3, HIGH);
        digitalWrite(led_bargraph_4, HIGH);
        digitalWrite(led_bargraph_5, HIGH);
      break;

      case 3:
        digitalWrite(led_bargraph_1, LOW);
        digitalWrite(led_bargraph_2, LOW);
        digitalWrite(led_bargraph_3, LOW);
        digitalWrite(led_bargraph_4, HIGH);
        digitalWrite(led_bargraph_5, HIGH);
      break;

      case 4:
        digitalWrite(led_bargraph_1, LOW);
        digitalWrite(led_bargraph_2, LOW);
        digitalWrite(led_bargraph_3, LOW);
        digitalWrite(led_bargraph_4, LOW);
        digitalWrite(led_bargraph_5, HIGH);
      break;

      case 5:
        digitalWrite(led_bargraph_1, LOW);
        digitalWrite(led_bargraph_2, LOW);
        digitalWrite(led_bargraph_3, LOW);
        digitalWrite(led_bargraph_4, LOW);
        digitalWrite(led_bargraph_5, LOW);
      break;
    }
  }
}

void bargraphRampUp() { 
  if(b_bargraph_alt == true) {
    switch(i_bargraph_status_alt) {
      case 0 ... 27:
        ht_bargraph.setLedNow(i_bargraph[i_bargraph_status_alt]);

        if(i_bargraph_status > 22) {
          vibrationWand(i_vibration_level + 80);
        }
        else if(i_bargraph_status > 16) {
          vibrationWand(i_vibration_level + 40);
        }
        else if(i_bargraph_status > 10) {
          vibrationWand(i_vibration_level + 30);
        }
        else if(i_bargraph_status > 4) {
          vibrationWand(i_vibration_level + 20);
        }
        else if(i_bargraph_status > 0) {
          vibrationWand(i_vibration_level + 10);
        }

        i_bargraph_status_alt++;

        if(i_bargraph_status_alt == 28) {
          // A little pause when we reach the top.
          ms_bargraph.start(i_bargraph_wait / 2);
        }
        else {
          ms_bargraph.start(i_bargraph_interval * i_bargraph_multiplier_current);
        }
      break;

      case 28 ... 56:
        int i_tmp = i_bargraph_status_alt - 27;
        i_tmp = 28 - i_tmp;

        if((i_power_mode < 5 && year_mode == 2021) || year_mode == 1984) {
          ht_bargraph.clearLedNow(i_bargraph[i_tmp]);
        }

        switch(year_mode) {
          case 1984:
            // Bargraph has ramped up and down. In 1984 mode we want to start the ramping.
            if(i_bargraph_status_alt == 54) {
              ms_bargraph_alt.start(i_bargraph_interval); // Start the alternate bargraph to ramp up and down continiuously.
              ms_bargraph.stop();
              b_bargraph_up = true;
              i_bargraph_status_alt = 0;
              bargraphYearModeUpdate();
              
              vibrationWand(i_vibration_level);
            }
            else {
              ms_bargraph.start(i_bargraph_interval * i_bargraph_multiplier_current);
              i_bargraph_status_alt++;
            }
            
          break;

          case 2021:
            switch(i_power_mode) {
              case 5:
                ms_bargraph.stop();
                b_bargraph_up = false;
                i_bargraph_status_alt = 27;
                bargraphYearModeUpdate();

                vibrationWand(i_vibration_level + 25);
              break;

              case 4:
                if(i_bargraph_status_alt == 31) {
                  ms_bargraph.stop();
                  b_bargraph_up = false;
                  i_bargraph_status_alt = 23;
                  bargraphYearModeUpdate();

                  vibrationWand(i_vibration_level + 30);
                }
                else {
                  ms_bargraph.start(i_bargraph_interval * i_bargraph_multiplier_current);
                  i_bargraph_status_alt++;

                  vibrationWand(i_vibration_level + 12);
                }
              break;

              case 3:
                if(i_bargraph_status_alt == 37) {
                  ms_bargraph.stop();
                  b_bargraph_up = false;
                  i_bargraph_status_alt = 17;
                  bargraphYearModeUpdate();

                  vibrationWand(i_vibration_level + 10);
                }
                else {
                  ms_bargraph.start(i_bargraph_interval * i_bargraph_multiplier_current);
                  i_bargraph_status_alt++;

                  vibrationWand(i_vibration_level + 20);
                }
              break;

              case 2:
                if(i_bargraph_status_alt == 43) {
                  ms_bargraph.stop();
                  b_bargraph_up = false;
                  i_bargraph_status_alt = 11;
                  bargraphYearModeUpdate();
                  
                  vibrationWand(i_vibration_level + 5);
                }
                else {
                  ms_bargraph.start(i_bargraph_interval * i_bargraph_multiplier_current);
                  i_bargraph_status_alt++;

                  vibrationWand(i_vibration_level + 10);
                }
              break;

              case 1:
                vibrationWand(i_vibration_level);

                if(i_bargraph_status_alt == 49) {
                  ms_bargraph.stop();
                  b_bargraph_up = false;
                  i_bargraph_status_alt = 5;

                  bargraphYearModeUpdate();
                }
                else {
                  ms_bargraph.start(i_bargraph_interval * i_bargraph_multiplier_current);
                  i_bargraph_status_alt++;
                }
              break;
            }
          break;
        }              

      break;
    }
  }
  else {
    switch(i_bargraph_status) {
      case 0:
        vibrationWand(i_vibration_level + 10);
                
        digitalWrite(led_bargraph_1, LOW);
        digitalWrite(led_bargraph_2, HIGH);
        digitalWrite(led_bargraph_3, HIGH);
        digitalWrite(led_bargraph_4, HIGH);
        digitalWrite(led_bargraph_5, HIGH);
        ms_bargraph.start(d_bargraph_ramp_interval);
        i_bargraph_status++;
      break;

      case 1:
        vibrationWand(i_vibration_level + 20);
        
        digitalWrite(led_bargraph_2, LOW);
        digitalWrite(led_bargraph_3, HIGH);
        digitalWrite(led_bargraph_4, HIGH);
        digitalWrite(led_bargraph_5, HIGH);
        ms_bargraph.start(d_bargraph_ramp_interval);
        i_bargraph_status++;
      break;

      case 2:
        vibrationWand(i_vibration_level + 30);
            
        digitalWrite(led_bargraph_3, LOW);
        digitalWrite(led_bargraph_4, HIGH);
        digitalWrite(led_bargraph_5, HIGH);
        ms_bargraph.start(d_bargraph_ramp_interval);
        i_bargraph_status++;
      break;

      case 3:
        vibrationWand(i_vibration_level + 40);
        
        digitalWrite(led_bargraph_4, LOW);
        digitalWrite(led_bargraph_5, HIGH);
        ms_bargraph.start(d_bargraph_ramp_interval);
        i_bargraph_status++;
      break;

      case 4:
        vibrationWand(i_vibration_level + 80);
        
        digitalWrite(led_bargraph_5, LOW);

        if(i_bargraph_status + 1 == i_power_mode) {
          ms_bargraph.stop();
          i_bargraph_status = 0;
        }
        else {
          i_bargraph_status++;
          ms_bargraph.start(d_bargraph_ramp_interval);
        }
      break;
      
      case 5:
        vibrationWand(i_vibration_level + 40);
        
        digitalWrite(led_bargraph_5, HIGH);
        
        if(i_bargraph_status - 1 == i_power_mode) {
          ms_bargraph.stop();
          i_bargraph_status = 0;
        }
        else {
          i_bargraph_status++;
          ms_bargraph.start(d_bargraph_ramp_interval);
        }
      break;
      
      case 6:
        vibrationWand(i_vibration_level + 30);
            
        digitalWrite(led_bargraph_4, HIGH);
        
        if(i_bargraph_status - 3 == i_power_mode) {
          ms_bargraph.stop();
          i_bargraph_status = 0;
        }
        else {
          i_bargraph_status++;
          ms_bargraph.start(d_bargraph_ramp_interval);
        }
      break;
      
      case 7:
        vibrationWand(i_vibration_level + 20);
            
        digitalWrite(led_bargraph_3, HIGH);
        
        if(i_bargraph_status - 5 == i_power_mode) {
          ms_bargraph.stop();
          i_bargraph_status = 0;
        }
        else {
          i_bargraph_status++;
          ms_bargraph.start(d_bargraph_ramp_interval);
        }
      break;

      case 8:
        vibrationWand(i_vibration_level + 10);
            
        digitalWrite(led_bargraph_4, HIGH);
        
        if(i_bargraph_status - 7 == i_power_mode) {
          ms_bargraph.stop();
          i_bargraph_status = 0;
        }
        else {
          ms_bargraph.start(d_bargraph_ramp_interval);
          i_bargraph_status = 1;
        }
    }
  }
}

void bargraphYearModeUpdate() {
  switch(year_mode) {
    case 2021:
      i_bargraph_multiplier_current = i_bargraph_multiplier_ramp_2021;
    break;

    case 1984:
      i_bargraph_multiplier_current = i_bargraph_multiplier_ramp_1984;
    break;
  }
}

void wandLightsOff() {
  if(b_bargraph_alt == true) {
    bargraphClearAlt();
  }
  else {
    digitalWrite(led_bargraph_1, HIGH);
    digitalWrite(led_bargraph_2, HIGH);
    digitalWrite(led_bargraph_3, HIGH);
    digitalWrite(led_bargraph_4, HIGH);
    digitalWrite(led_bargraph_5, HIGH);
  }

  analogWrite(led_slo_blo, 0);
  digitalWrite(led_vent, HIGH);
  digitalWrite(led_white, HIGH);

  i_bargraph_status = 0;
  i_bargraph_status_alt = 0;
}

void vibrationOff() {
  analogWrite(vibration, 0);
}

void adjustVolumeEffectsGain() {
  /*
   * Reset the gain on all sound effect tracks.
   */
  for(int i=0; i <= i_last_effects_track; i++) {
    w_trig.trackGain(i, i_volume);
  }
}

void increaseVolumeEffects() {
  if(i_volume_percentage + VOLUME_EFFECTS_MULTIPLIER > 100) {
    i_volume_percentage = 100;
  }
  else {
    i_volume_percentage = i_volume_percentage + VOLUME_EFFECTS_MULTIPLIER;
  }

  i_volume = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_percentage / 100);

  adjustVolumeEffectsGain();
}

void decreaseVolumeEffects() {
  if(i_volume_percentage - VOLUME_EFFECTS_MULTIPLIER < 0) {
    i_volume_percentage = 0;
  }
  else {
    i_volume_percentage = i_volume_percentage - VOLUME_EFFECTS_MULTIPLIER;
  }

  adjustVolumeEffectsGain();
}

void increaseVolume() {
  if(i_volume_master_percentage + VOLUME_MULTIPLIER > 100) {
    i_volume_master_percentage = 100;
  }
  else {
    i_volume_master_percentage = i_volume_master_percentage + VOLUME_MULTIPLIER;
  }

  i_volume_master = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100);
  
  w_trig.masterGain(i_volume_master);
}

void decreaseVolume() {
  if(i_volume_master_percentage - VOLUME_MULTIPLIER < 0) {
    i_volume_master_percentage = 0;
  }
  else {
    i_volume_master_percentage = i_volume_master_percentage - VOLUME_MULTIPLIER;
  }

  i_volume_master = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100);
  
  w_trig.masterGain(i_volume_master);
}

/*
 * Top rotary dial on the wand.
 */
void checkRotary() {
  static int8_t c,val;

  if((val = readRotary())) {
    c += val;

    switch(WAND_ACTION_STATUS) {
      case ACTION_SETTINGS:
        // Counter clockwise.
        if(prev_next_code == 0x0b) {
          if(i_wand_menu - 1 < 1) {
            i_wand_menu = 1;
          }
          else {
            i_wand_menu--;
          }
        }

        // Clockwise.
        if(prev_next_code == 0x07) {
          if(i_wand_menu + 1 > 5) {
            i_wand_menu = 5;
          }
          else {
            i_wand_menu++;
          }
        }
      break;

      default:
        // Counter clockwise.
        if(prev_next_code == 0x0b) {
          if(i_power_mode - 1 >= i_power_mode_min && WAND_STATUS == MODE_ON) {
            i_power_mode_prev = i_power_mode;
            i_power_mode--;

            if(year_mode == 2021 && b_bargraph_alt == true) {
              bargraphPowerCheck2021Alt();
            }

            soundBeepLoopStop();
    
            switch(year_mode) {
              case 1984:
                if(switch_vent.getState() == LOW) {
                  soundIdleLoopStop();
                  soundIdleLoop(false);
                }
              break;
      
              default:
                  soundIdleLoopStop();
                  soundIdleLoop(false);
              break;
            }

            updatePackPowerLevel();
          }

          // Decrease the music volume if the wand/pack is off. A quick easy way to adjust the music volume on the go.
          if(WAND_STATUS == MODE_OFF && FIRING_MODE != SETTINGS && b_playing_music == true) {
            if(i_volume_music_percentage - VOLUME_MUSIC_MULTIPLIER < 0) {
              i_volume_music_percentage = 0;
            }
            else {
              i_volume_music_percentage = i_volume_music_percentage - VOLUME_MUSIC_MULTIPLIER;
            }

            i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100);

            w_trig.trackGain(i_current_music_track, i_volume_music);
            
            // Tell pack to lower music volume.
            Serial.write(96);
          }
        }
        
        if(prev_next_code == 0x07) {
          if(i_power_mode + 1 <= i_power_mode_max && WAND_STATUS == MODE_ON) {
            i_power_mode_prev = i_power_mode;
            i_power_mode++;
            
            if(year_mode == 2021 && b_bargraph_alt == true) {
              bargraphPowerCheck2021Alt();
            }

            soundBeepLoopStop();
    
            switch(year_mode) {
              case 1984:
                if(switch_vent.getState() == LOW) {
                  soundIdleLoopStop();
                  soundIdleLoop(false);
                }
              break;
      
              default:
                  soundIdleLoopStop();
                  soundIdleLoop(false);
              break;
            }
           
            updatePackPowerLevel();     
          }

          // Increase the music volume if the wand/pack is off. A quick easy way to adjust the music volume on the go.
          if(WAND_STATUS == MODE_OFF && FIRING_MODE != SETTINGS && b_playing_music == true) {
            if(i_volume_music_percentage + VOLUME_MUSIC_MULTIPLIER > 100) {
              i_volume_music_percentage = 100;
            }
            else {
              i_volume_music_percentage = i_volume_music_percentage + VOLUME_MUSIC_MULTIPLIER;
            }

            i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100);

            w_trig.trackGain(i_current_music_track, i_volume_music);
            
            // Tell pack to increase music volume.
            Serial.write(97);
          }
        }
      break;
    }    
  }
}

/*
 * Tell the pack which power level the wand is at.
 */
void updatePackPowerLevel() {
  switch(i_power_mode) {
    case 5:
      // Level 5
      Serial.write(20);
    break;

    case 4:
      // Level 4
      Serial.write(19);
    break;

    case 3:
      // Level 3
      Serial.write(18);
    break;

    case 2:
      // Level 2
      Serial.write(17);
    break;

    default:
      // Level 1
      Serial.write(16);
    break;
  }
}

int8_t readRotary() {
  static int8_t rot_enc_table[] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0};

  prev_next_code <<= 2;
  
  if(digitalRead(r_encoderB)) { 
    prev_next_code |= 0x02;
  }
  
  if(digitalRead(r_encoderA)) {
    prev_next_code |= 0x01;
  }
  
  prev_next_code &= 0x0f;

   // If valid then store as 16 bit data.
   if(rot_enc_table[prev_next_code]) {
      store <<= 4;
      store |= prev_next_code;

      if((store&0xff) == 0x2b) {
        return -1;
      }
      
      if((store&0xff) == 0x17) {
        return 1;
      }
   }
   
   return 0;
}

void vibrationSetting() {
  if(b_vibration_on == true) {
    if(ms_bargraph.isRunning() == false && WAND_ACTION_STATUS != ACTION_FIRING) {
      switch(i_power_mode) {
        case 1:
          vibrationWand(i_vibration_level);
        break;
    
        case 2:
          vibrationWand(i_vibration_level + 5);
        break;
    
        case 3:
          vibrationWand(i_vibration_level + 10);
        break;
    
        case 4:
          vibrationWand(i_vibration_level + 12);
        break;
    
        case 5:
          vibrationWand(i_vibration_level + 25);
        break;
      }
    }
  }
  else {
    vibrationOff();
  }
}

void switchLoops() {
  switch_wand.loop();
  switch_intensify.loop();
  switch_activate.loop();
  switch_vent.loop();
}

void wandBarrelLightsOff() {
  for(int i = 0; i < BARREL_NUM_LEDS; i++) {
    barrel_leds[i] = CRGB(0,0,0);
  }
  
  ms_fast_led.start(i_fast_led_delay);
}

/*
 * Pack commuication to the wand.
 */
void checkPack() {
  if(Serial.available() > 0) {
    rx_byte = Serial.read();

    if(b_volume_sync_wait == true) {
        switch(VOLUME_SYNC_WAIT) {
          case EFFECTS:
            i_volume_percentage = rx_byte;
            i_volume = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_percentage / 100);

            adjustVolumeEffectsGain();
            VOLUME_SYNC_WAIT = MASTER;
          break;

          case MASTER:
            i_volume_master_percentage = rx_byte;
            i_volume_master = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100);

            w_trig.masterGain(i_volume_master);
            VOLUME_SYNC_WAIT = MUSIC;
          break;

          case MUSIC:
            i_volume_music_percentage = rx_byte;
            i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100);

            b_volume_sync_wait = false;
            b_wait_for_pack = false;
            VOLUME_SYNC_WAIT = EFFECTS;
          break;
        }      
    }
    else {
      switch(rx_byte) {      
        case 1:
          // Pack is on.
          b_pack_on = true;
        break;
  
        case 2:
          if(b_pack_on == true) {
            // Turn wand off.
            if(WAND_STATUS != MODE_OFF) {
              WAND_ACTION_STATUS = ACTION_OFF;
            }
          }
          
          // Pack is off.
          b_pack_on = false;
        break;
  
        case 3:
          // Alarm is on.
          b_pack_alarm = true;
        break;
  
        case 4:
          // Alarm is off.
          b_pack_alarm = false;
        break;
  
        case 5:
          // Vibration on.
          b_vibration_on = true;
        break;
  
        case 6:
          // Vibration off.
          b_vibration_on = false;
          vibrationOff();
        break;
  
        case 7:
          // 1984 mode.
          year_mode = 1984;
          bargraphYearModeUpdate();
        break;
  
        case 8:
          // 2021 mode.
          year_mode = 2021;
          bargraphYearModeUpdate();
        break;
  
        case 9:
          // Increase overall volume.
          increaseVolume();
        break;
  
        case 10:
          // Decrease overall volume.
          decreaseVolume();
        break;
  
        case 11:
          // The pack is asking us if we are still here. Respond back.
          Serial.write(14);
        break;
  
        case 12:
          // Repeat music track.
          b_repeat_track = true;
        break;
  
        case 13:
          // Repeat music track.
          b_repeat_track = false;
        break;

        /*
         * Not used.
         */
        case 14:

        break;
  
        case 15:
          // Put the wand into volume sync mode.
          b_volume_sync_wait = true;
          VOLUME_SYNC_WAIT = EFFECTS;
        break;
        
        case 99:
          // Stop music
          stopMusic();
        break;
        
        default:
          // Music track number to be played.
          if(rx_byte > 99) {
            if(b_playing_music == true) {
              stopMusic();
              i_current_music_track = rx_byte;
              playMusic();
            }
            else {
              i_current_music_track = rx_byte;
            }
          }
        break;
      }
    }
  }
}

void setupWavTrigger() {
  // If the controller is powering the WAV Trigger, we should wait for the WAV trigger to finish reset before trying to send commands.
  delay(1000);
  
  // WAV Trigger's startup at 57600
  w_trig.start();
  
  delay(10);

  // Send a stop-all command and reset the sample-rate offset, in case we have
  //  reset while the WAV Trigger was already playing.
  w_trig.stopAllTracks();
  w_trig.samplerateOffset(0); // Reset our sample rate offset        
  w_trig.masterGain(i_volume_master); // Reset the master gain db. 0db is default. Range is -70 to 0.
  w_trig.setAmpPwr(b_onboard_amp_enabled); // Turn on the onboard amp.
  
  // Enable track reporting from the WAV Trigger
  w_trig.setReporting(true);

  // Allow time for the WAV Trigger to respond with the version string and number of tracks.
  delay(350);
  
  char w_trig_version[VERSION_STRING_LEN]; // Firmware version.
  int w_num_tracks = w_trig.getNumTracks();
  w_trig.getVersion(w_trig_version, VERSION_STRING_LEN);
  
  // Build the music track count.
  i_music_count = w_num_tracks - i_last_effects_track;
  if(i_music_count > 0) {
    i_current_music_track = i_music_track_start; // Set the first track of music as file 100_
  }
}
