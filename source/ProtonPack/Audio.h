/**
 *   GPStar Proton Pack - Ghostbusters Proton Pack & Neutrona Wand.
 *   Copyright (C) 2023 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
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

#pragma once

/**
 ***** IMPORTANT *****
 * If using a WAV Trigger, please make sure they are running the custom gpstar version firmware version 1.40 or higher.
 * You can download the latest directly from the GPStar github repository.
 * https://github.com/gpstar81/haslab-proton-pack/tree/main/extras
 *
 * Information on how to update your WAV Trigger devices can be found on the GPStar github repository.
 * https://github.com/gpstar81/haslab-proton-pack/blob/main/WAVTRIGGER.md
 */
#include "GPStarAudio.h"
gpstarAudio audio;

/*
 * Audio Devices
 */
enum AUDIO_DEVICES { A_NONE, A_GPSTAR_AUDIO, A_WAV_TRIGGER };
enum AUDIO_DEVICES AUDIO_DEVICE;

/*
 * Audio Variables
 */
uint16_t i_music_count = 0;
uint16_t i_current_music_track = 0;
const int i_music_track_start = 500; // Music tracks start on file named 500_ and higher.
const int8_t i_volume_abs_min = -70; // System (absolute) minimum volume possible.
const int8_t i_volume_abs_max = 10; // System (absolute) maximum volume possible.
bool b_playing_music = false;
bool b_music_paused = false;
bool b_repeat_track = false;
uint8_t i_wand_beep_level = 10; // 10 for WAV Trigger. 40 for GPStar Audio. This lowers the volume of certain Neutrona Wand beep sounds that the Proton Pack can play.
uint8_t i_wand_idle_level = 20; // This adjusts the volume of certain Afterlife / Frozen Empire Neutrona Wand idle sounds that the Proton pack can play.
uint8_t i_gpstar_audio_volume_factor = 0; // Main volume gain factor for the GPStar Audio. This is applied to certain sound effects only.
const uint8_t i_volume_master_percentage_max = 100; // Max percentage of master volume.

/*
 * Music Control/Checking
 */
const unsigned int i_music_check_delay = 2000;
const unsigned int i_music_next_track_delay = 500;
millisDelay ms_check_music;
millisDelay ms_music_next_track;
millisDelay ms_music_status_check;

/*
 * Volume (-70 = quietest, 0 = loudest)
 */
uint8_t i_volume_master_percentage = STARTUP_VOLUME; // Master overall volume
uint8_t i_volume_effects_percentage = STARTUP_VOLUME_EFFECTS; // Sound effects
uint8_t i_volume_music_percentage = STARTUP_VOLUME_MUSIC; // Music volume

/*
 * General Volume
 */
int8_t i_volume_master = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100); // Master overall volume
int8_t i_volume_master_eeprom = i_volume_master; // Master overall volume that is saved into the eeprom menu and loaded during bootup.
int8_t i_volume_effects = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_effects_percentage / 100); // Sound effects
int8_t i_volume_music = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_music_percentage / 100); // Music volume
int8_t i_volume_revert = i_volume_master;

millisDelay ms_volume_check; // Put some timing on the master volume gain to not overload the audio device serial communication.

/*
 * Function Prototypes
 */
void playEffect(int i_track_id, bool b_track_loop = false, int8_t i_track_volume = i_volume_effects, bool b_fade_in = false, unsigned int i_fade_time = 0, bool b_lock = true);
void stopEffect(int i_track_id);
void playMusic();
void stopMusic();
void adjustGainEffect(int i_track_id, int8_t i_track_volume = i_volume_effects, bool b_fade = false, unsigned int i_fade_time = 0);

/*
 * Audio playback functions.
 */

// Play a sound effect using certain defaults.
void playEffect(int i_track_id, bool b_track_loop, int8_t i_track_volume, bool b_fade_in, unsigned int i_fade_time, bool b_lock) {
  if(AUDIO_DEVICE == A_WAV_TRIGGER) {
    if(i_track_volume < i_volume_abs_min) {
      i_track_volume = i_volume_abs_min;
    }

    if(i_track_volume > i_volume_abs_max) {
      i_track_volume = i_volume_abs_max;
    }
  }

  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      if(b_fade_in == true) {
        audio.trackGain(i_track_id, i_volume_abs_min);
        audio.trackPlayPoly(i_track_id, b_lock);
        audio.trackFade(i_track_id, i_track_volume, i_fade_time, 0);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
        audio.trackPlayPoly(i_track_id, b_lock);
      }

      if(b_track_loop == true) {
        audio.trackLoop(i_track_id, 1);
      }
      else {
        audio.trackLoop(i_track_id, 0);
      }
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

void stopEffect(int i_track_id) {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      audio.trackStop(i_track_id);
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

// Adjust the gain of a single track.
void adjustGainEffect(int i_track_id, int8_t i_track_volume, bool b_fade, unsigned int i_fade_time) {
  if(i_track_volume < i_volume_abs_min) {
    i_track_volume = i_volume_abs_min;
  }

  if(i_track_volume > i_volume_abs_max) {
    i_track_volume = i_volume_abs_max;
  }

  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      if(b_fade == true) {
        audio.trackFade(i_track_id, i_track_volume, i_fade_time, 0);
      }
      else {
        audio.trackGain(i_track_id, i_track_volume);
      }
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

void updateEffectsVolume() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      // Since adjusting only from the wand, only certain effects need to be adjusted on the fly.
      audio.trackGain(S_BEEPS, i_volume_effects);
      audio.trackGain(S_BEEPS_ALT, i_volume_effects);
      audio.trackGain(S_BEEPS_LOW, i_volume_effects);
      audio.trackGain(S_BEEPS_BARGRAPH, i_volume_effects);
      audio.trackGain(S_WAND_BOOTUP, i_volume_effects);
      audio.trackGain(S_PACK_RIBBON_ALARM_1, i_volume_effects);
      audio.trackGain(S_ALARM_LOOP, i_volume_effects);
      audio.trackGain(S_RIBBON_CABLE_START, i_volume_effects);
      audio.trackGain(S_SHUTDOWN, i_volume_effects);

      if(SYSTEM_YEAR == SYSTEM_1989) {
        audio.trackGain(S_GB2_PACK_START, i_volume_effects);
        audio.trackGain(S_GB2_PACK_LOOP, i_volume_effects);
        audio.trackGain(S_GB2_PACK_OFF, i_volume_effects);
      }

      if(SYSTEM_YEAR == SYSTEM_1984 || SYSTEM_YEAR == SYSTEM_1989) {
        audio.trackGain(S_PACK_SHUTDOWN, i_volume_effects);
      }

      if(SYSTEM_YEAR == SYSTEM_1984) {
        audio.trackGain(S_IDLE_LOOP, i_volume_effects);
        audio.trackGain(S_BOOTUP, i_volume_effects);
      }

      if(SYSTEM_YEAR == SYSTEM_FROZEN_EMPIRE || SYSTEM_YEAR == SYSTEM_AFTERLIFE) {
        audio.trackGain(S_POWERCELL, i_volume_effects);
        audio.trackGain(S_AFTERLIFE_BEEP_WAND_S1, i_volume_effects - i_wand_beep_level);
        audio.trackGain(S_AFTERLIFE_BEEP_WAND_S2, i_volume_effects - i_wand_beep_level);
        audio.trackGain(S_AFTERLIFE_BEEP_WAND_S3, i_volume_effects - i_wand_beep_level);
        audio.trackGain(S_AFTERLIFE_BEEP_WAND_S4, i_volume_effects - i_wand_beep_level);
        audio.trackGain(S_AFTERLIFE_BEEP_WAND_S5, i_volume_effects - i_wand_beep_level);
        audio.trackGain(S_AFTERLIFE_PACK_STARTUP, i_volume_effects);
        audio.trackGain(S_AFTERLIFE_PACK_IDLE_LOOP, i_volume_effects);
        audio.trackGain(S_PACK_SHUTDOWN_AFTERLIFE_ALT, i_volume_effects);
        audio.trackGain(S_AFTERLIFE_WAND_RAMP_1, i_volume_effects - i_wand_idle_level);
        audio.trackGain(S_AFTERLIFE_WAND_RAMP_2, i_volume_effects - i_wand_idle_level);
        audio.trackGain(S_AFTERLIFE_WAND_RAMP_2_FADE_IN, i_volume_effects - i_wand_idle_level);
        audio.trackGain(S_AFTERLIFE_WAND_IDLE_1, i_volume_effects - i_wand_idle_level);
        audio.trackGain(S_AFTERLIFE_WAND_IDLE_2, i_volume_effects - i_wand_idle_level);
        audio.trackGain(S_AFTERLIFE_WAND_RAMP_DOWN_2, i_volume_effects - i_wand_idle_level);
        audio.trackGain(S_AFTERLIFE_WAND_RAMP_DOWN_2_FADE_OUT, i_volume_effects - i_wand_idle_level);
        audio.trackGain(S_AFTERLIFE_WAND_RAMP_DOWN_1, i_volume_effects - i_wand_idle_level);
      }

      if(SYSTEM_YEAR == SYSTEM_FROZEN_EMPIRE && b_cyclotron_lid_on != true) {
        audio.trackGain(S_FROZEN_EMPIRE_BOOT_EFFECT, i_volume_effects + i_gpstar_audio_volume_factor);
      }

      if(FIRING_MODE == SLIME) {
        audio.trackGain(S_PACK_SLIME_TANK_LOOP, i_volume_effects);
      }

      if(FIRING_MODE == STASIS) {
        audio.trackGain(S_STASIS_IDLE_LOOP, i_volume_effects);
      }

      if(FIRING_MODE == MESON) {
        audio.trackGain(S_MESON_IDLE_LOOP, i_volume_effects);
      }
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }

  serial1SendData(A_VOLUME_SYNC); // Tell the connected device about this change.
}

// Play a music track using certain defaults.
void playMusic() {
  if(b_music_paused != true && i_music_count > 0 && i_current_music_track >= i_music_track_start) {
    b_playing_music = true;

    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
        // Loop the music track.
        if(b_repeat_track == true) {
          audio.trackLoop(i_current_music_track, 1);
        }
        else {
          audio.trackLoop(i_current_music_track, 0);
        }

        audio.trackGain(i_current_music_track, i_volume_music);
        audio.trackPlayPoly(i_current_music_track, true);
        audio.update();

        audio.resetTrackCounter(true);
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }

    // Manage track navigation.
    ms_music_status_check.start(i_music_check_delay * 10);

    // Tell connected serial device music playback has started.
    serial1Send(A_MUSIC_IS_PLAYING, i_current_music_track);
    serial1Send(A_MUSIC_IS_NOT_PAUSED);

    // Tell the Neutrona Wand which music track to change to and play it.
    packSerialSend(P_MUSIC_START, i_current_music_track);
  }
}

void stopMusic() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      if(i_music_count > 0 && i_current_music_track >= i_music_track_start) {
        audio.trackStop(i_current_music_track);
      }

      audio.update();
    break;

    case A_NONE:
    default:
      // Nothing.
    break;
  }

  b_music_paused = false;
  b_playing_music = false;

  // Tell connected serial device music playback has stopped.
  serial1Send(A_MUSIC_IS_NOT_PLAYING, i_current_music_track);
  serial1Send(A_MUSIC_IS_NOT_PAUSED);

  // Tell the Neutrona Wand to stop music playback and confirm track.
  packSerialSend(P_MUSIC_STOP);
}

void pauseMusic() {
  if(b_playing_music == true) {
    // Tell connected devices music playback is paused.
    packSerialSend(P_MUSIC_PAUSE);
    serial1Send(A_MUSIC_IS_PAUSED);

    // Pause music playback on the Proton Pack
    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
        if(i_music_count > 0 && i_current_music_track >= i_music_track_start) {
          audio.trackPause(i_current_music_track);
        }

        audio.update();
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }

    b_music_paused = true;
  }
}

void resumeMusic() {
  if(b_playing_music == true) {
    // Reset the music check timer.
    ms_music_status_check.start(i_music_check_delay * 4);

    // Resume music playback on the Proton Pack
    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
        audio.resetTrackCounter(true);

        if(i_music_count > 0 && i_current_music_track >= i_music_track_start) {
          audio.trackResume(i_current_music_track);
        }

        audio.update();
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }

    b_music_paused = false;

    // Tell connected devices music playback has resumed.
    packSerialSend(P_MUSIC_RESUME);
    serial1Send(A_MUSIC_IS_NOT_PAUSED);
  }
}

void musicNextTrack() {
  unsigned int i_temp_track = i_current_music_track; // Used for music navigation.

  // Determine the next track.
  if(i_current_music_track + 1 > i_music_track_start + i_music_count - 1) {
    // Start at the first track if already on the last.
    i_temp_track = i_music_track_start;
  }
  else {
    i_temp_track++;
  }

  // Switch to the next track.
  if(b_playing_music == true) {
    // Stops music using the current track number as the identifier.
    stopMusic();

    i_current_music_track = i_temp_track; // Change only AFTER stopping music playback.

    // Play the appropriate track on pack and wand, and notify the serial1 device.
    playMusic();
  }
  else {
    // Set the new track.
    i_current_music_track = i_temp_track;

    serial1Send(A_MUSIC_IS_NOT_PLAYING); // Updates the music track on the attenuator.
  }
}

void musicPrevTrack() {
  unsigned int i_temp_track = i_current_music_track; // Used for music navigation.

  // Determine the previous track.
  if(i_current_music_track - 1 < i_music_track_start) {
    // Start at the last track if already on the first.
    i_temp_track = i_music_track_start + (i_music_count - 1);
  }
  else {
    i_temp_track--;
  }

  // Switch to the previous track.
  if(b_playing_music == true) {
    // Stops music using the current track number as the identifier.
    stopMusic();

    i_current_music_track = i_temp_track; // Change only AFTER stopping music playback.

    // Play the appropriate track on pack and wand, and notify the serial1 device.
    playMusic();
  }
  else {
    // Set the new track.
    i_current_music_track = i_temp_track;

    serial1Send(A_MUSIC_IS_NOT_PLAYING); // Updates the music track on the attenuator.
  }
}

void increaseVolumeEffects() {
  if(i_volume_effects_percentage + VOLUME_EFFECTS_MULTIPLIER > 100) {
    i_volume_effects_percentage = 100;

    // Provide feedback at maximum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT);
  }
  else {
    i_volume_effects_percentage = i_volume_effects_percentage + VOLUME_EFFECTS_MULTIPLIER;
  }

  i_volume_effects = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_effects_percentage / 100);

  updateEffectsVolume();
}

void decreaseVolumeEffects() {
  if(i_volume_effects_percentage - VOLUME_EFFECTS_MULTIPLIER < 0) {
    i_volume_effects_percentage = 0;

    // Provide feedback at minimum volume.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, i_volume_master - i_wand_beep_level);
  }
  else {
    i_volume_effects_percentage = i_volume_effects_percentage - VOLUME_EFFECTS_MULTIPLIER;
  }

  i_volume_effects = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_effects_percentage / 100);

  updateEffectsVolume();
}

void updateMusicVolume() {
  if(i_music_count > 0) {
    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
        audio.trackGain(i_current_music_track, i_volume_music);
      break;

      case A_NONE:
      default:
        // Nothing.
      break;
    }
  }
}

void increaseVolumeEEPROM() {
  if(i_volume_master_eeprom == i_volume_abs_min && MINIMUM_VOLUME > i_volume_master_eeprom) {
    i_volume_master_eeprom = MINIMUM_VOLUME;
  }

  if(i_volume_master_percentage + VOLUME_MULTIPLIER > 100) {
    i_volume_master_percentage = 100;
  }
  else {
    i_volume_master_percentage = i_volume_master_percentage + VOLUME_MULTIPLIER;
  }

  i_volume_master_eeprom = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100);
  i_volume_revert = i_volume_master_eeprom;

  if(b_pack_on != true && b_pack_shutting_down != true) {
    // Provide feedback when the pack is not running.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, i_volume_master_eeprom);
  }

  i_volume_master = i_volume_master_eeprom;

  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      audio.masterGain(i_volume_master_eeprom);
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }
}

void decreaseVolumeEEPROM() {
  if(i_volume_master_eeprom == i_volume_abs_min) {
    // Cannot go any lower.
  }
  else {
    if(i_volume_master_percentage - VOLUME_MULTIPLIER < 0) {
      i_volume_master_percentage = 0;
    }
    else {
      i_volume_master_percentage = i_volume_master_percentage - VOLUME_MULTIPLIER;
    }

    i_volume_master_eeprom = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100);
    i_volume_revert = i_volume_master_eeprom;

    i_volume_master = i_volume_master_eeprom;

    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
        audio.masterGain(i_volume_master_eeprom);
      break;

      case A_NONE:
      default:
        // No audio device connected.
      break;
    }
  }

  if(b_pack_on != true && b_pack_shutting_down != true) {
    // Provide feedback when the pack is not running.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, i_volume_master_eeprom);
  }
}

void increaseVolume() {
  if(i_volume_master == i_volume_abs_min && MINIMUM_VOLUME > i_volume_master) {
    i_volume_master = MINIMUM_VOLUME;
  }

  if(i_volume_master_percentage + VOLUME_MULTIPLIER > 100) {
    i_volume_master_percentage = i_volume_master_percentage_max;
  }
  else {
    i_volume_master_percentage = i_volume_master_percentage + VOLUME_MULTIPLIER;
  }

  i_volume_master = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100);
  i_volume_revert = i_volume_master;

  if(b_pack_on != true && b_pack_shutting_down != true) {
    // Provide feedback when the pack is not running.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, i_volume_master);
  }

  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      audio.masterGain(i_volume_master);
    break;

    case A_NONE:
    default:
      // No audio device connected.
    break;
  }

  serial1SendData(A_VOLUME_SYNC);
}

void decreaseVolume() {
  if(i_volume_master == i_volume_abs_min) {
    // Cannot go any lower.
  }
  else {
    if(i_volume_master_percentage - VOLUME_MULTIPLIER < 0) {
      i_volume_master_percentage = 0;
    }
    else {
      i_volume_master_percentage = i_volume_master_percentage - VOLUME_MULTIPLIER;
    }

    i_volume_master = MINIMUM_VOLUME - (MINIMUM_VOLUME * i_volume_master_percentage / 100);
    i_volume_revert = i_volume_master;

    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
        audio.masterGain(i_volume_master);
      break;

      case A_NONE:
      default:
        // No audio device connected.
      break;
    }
  }

  if(b_pack_on != true && b_pack_shutting_down != true) {
    // Provide feedback when the pack is not running.
    stopEffect(S_BEEPS_ALT);
    playEffect(S_BEEPS_ALT, false, i_volume_master);
  }

  serial1SendData(A_VOLUME_SYNC);
}

void buildMusicCount(uint16_t i_num_tracks) {
  // Build the music track count.
  i_music_count = i_num_tracks - i_last_effects_track;

  if(i_music_count > 0 && i_music_count < 5000) {
    i_current_music_track = i_music_track_start; // Set the first track of music as file 500_
  }
  else {
    i_music_count = 0; // If the music count is corrupt, make it 0
    debugln(F("Warning: Calculated music count exceeds 5000; SD card corruption likely!"));
  }
}

bool musicGetTrackCounter() {
    switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      return audio.trackCounterReset();
    break;

    case A_NONE:
    default:
      return false;
    break;
  }
}

void musicTrackPlayingStatus() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      audio.trackPlayingStatus(i_current_music_track);
    break;

    case A_NONE:
    default:
      // Do nothing.
    break;
  }
}

bool musicTrackStatus() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      return audio.currentMusicTrackStatus(i_current_music_track);
    break;

    case A_NONE:
    default:
      return false;
    break;
  }
}

void checkMusic() {
  if(ms_check_music.justFinished() && ms_music_next_track.isRunning() != true) {
    switch(AUDIO_DEVICE) {
      case A_WAV_TRIGGER:
      case A_GPSTAR_AUDIO:
        ms_check_music.start(i_music_check_delay);

        musicTrackPlayingStatus();

        // Loop through all the tracks if the music is not set to repeat a track.
        if(b_playing_music == true && b_repeat_track == false && b_music_paused != true) {
          if(musicTrackStatus() != true && ms_music_status_check.justFinished() && musicGetTrackCounter() != true) {
            ms_check_music.stop();
            ms_music_status_check.stop();

            stopMusic();

            // Switch to the next track.
            if(i_current_music_track + 1 > i_music_track_start + i_music_count - 1) {
              i_current_music_track = i_music_track_start;
            }
            else {
              i_current_music_track++;
            }

            // Start timer to prepare to play music again.
            ms_music_next_track.start(i_music_next_track_delay);
          }
          else {
            if(ms_music_status_check.justFinished()) {
              ms_music_status_check.start(i_music_check_delay * 4);
            }
          }
        }
      break;

      case A_NONE:
      default:
        // None
      break;
    }
  }

  // Start playing music again.
  if(ms_music_next_track.justFinished()) {
    ms_music_next_track.stop();
    ms_check_music.start(i_music_check_delay);

    // Play the appropriate track on the pack and wand, and notify the serial1 device.
    playMusic();
  }
}

void toggleMusicLoop() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      // Loop the music track.
      if(b_repeat_track == false) {
        b_repeat_track = true;

        if(i_music_count > 0) {
          audio.trackLoop(i_current_music_track, 1);
        }
      }
      else {
        b_repeat_track = false;

        if(i_music_count > 0) {
          audio.trackLoop(i_current_music_track, 0);
        }
      }
    break;

    case A_NONE:
    default:
      if(b_repeat_track == false) {
        b_repeat_track = true;
      }
      else {
        b_repeat_track = false;
      }
    break;
  }
}

/*
 * Audio Setup Routines
 * Used to detect, update, and reset the available audio devices.
 */
bool setupAudioDevice() {
  // Short delay to allow the audio boards to boot up.
  delay(1000);

  char gVersion[VERSION_STRING_LEN];

  audio.start();

  delay(10);

  // Stop all tracks.
  audio.stopAllTracks();

  // Reset the sample rate offset. Only for the WAV Trigger.
  audio.samplerateOffset(0);

  audio.masterGain(-70); // Reset the master gain db. Range is -70 to 0. Bootup the system at the lowest volume, then we reset it after the system is loaded.

  // Onboard amplifier on or off. Only for the WAV Trigger.
  audio.setAmpPwr(b_onboard_amp_enabled);

  // Enable track reporting. Only for the WAV Trigger.
  audio.setReporting(true);

  // Allow time for hello command and other data to return back.
  delay(350);

  if(audio.getVersion(gVersion)) {
    // We found a WAV Trigger. Build the music track count.
    if(audio.wasSysInfoRcvd()) {
      // Only attempt to build a music track count if the WAV Trigger responded with RSP_SYSTEM_INFO.
      buildMusicCount((uint16_t) audio.getNumTracks());
    }
    else {
      debugln(F("Warning: RSP_SYSTEM_INFO not received!"));
    }

    AUDIO_DEVICE = A_WAV_TRIGGER;

    debugln(F("Using WAV Trigger"));

    return true;
  }

  audio.hello();

  delay(350);

  if(audio.gpstarAudioHello()) {
    AUDIO_DEVICE = A_GPSTAR_AUDIO;

    i_wand_beep_level = 40; // Special setting to adjust certain wand sounds on the pack side as they can be too loud.
    i_gpstar_audio_volume_factor = 30; // Special setting to amplify certain pack sounds.

    debugln(F("Using GPStar Audio"));

    buildMusicCount((uint16_t) audio.getNumTracks());

    return true;
  }
  else {
    // No audio devices connected.
    AUDIO_DEVICE = A_NONE;

    debugln(F("No Audio Device"));

    return false;
  }
}

void resetMasterVolume() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      audio.masterGain(i_volume_master);
    break;

    case A_NONE:
    default:
      // Nothing.
    break;
  }
}

void updateAudio() {
  switch(AUDIO_DEVICE) {
    case A_WAV_TRIGGER:
    case A_GPSTAR_AUDIO:
      audio.update();
    break;

    case A_NONE:
    default:
      // Nothing.
    break;
  }
}
