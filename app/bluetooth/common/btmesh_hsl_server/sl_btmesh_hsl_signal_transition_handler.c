/***************************************************************************//**
 * @file  sl_btmesh_hsl_signal_transition_handler.c
 * @brief HSL Transition Handler Module
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/* C Standard Library headers */
#include <stdlib.h>
#include <stdio.h>

#include "app_assert.h"
#include "em_common.h"
#include "sl_status.h"

#include "sl_simple_timer.h"

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif // SL_COMPONENT_CATALOG_PRESENT

#include "sl_btmesh_hsl_server.h"
#include "sl_btmesh_hsl_signal_transition_handler.h"
#include "sl_btmesh_hsl_server_config.h"

/***************************************************************************//**
 * @addtogroup HSL Transition Handler
 * @{
 ******************************************************************************/

#define NO_FLAGS              0       ///< No flags used for message
#define IMMEDIATE             0       ///< Immediate transition time is 0 seconds
#define NO_CALLBACK_DATA      (void *)NULL   // Callback has not parameters
#define HIGH_PRIORITY         0              // High Priority

/// current hue level
static uint16_t current_hue = HSL_SERVER_DEFAULT_HUE;
/// starting level of hue transition
static uint16_t start_hue;
/// target level of hue transition
static uint16_t target_hue;

/// current saturation level
static uint16_t current_saturation = HSL_SERVER_DEFAULT_SATURATION;
/// starting level of saturation transition
static uint16_t start_saturation;
/// target level of saturation transition
static uint16_t target_saturation;

/// hue transition time in timer ticks
static uint32_t hue_transtime_ticks;
/// time elapsed from hue transition start
static uint32_t hue_transtime_elapsed;
/// non-zero if hue transition is active
static uint8_t hue_transitioning;

/// saturation transition time in timer ticks
static uint32_t saturation_transtime_ticks;
/// time elapsed from saturation transition start
static uint32_t saturation_transtime_elapsed;
/// non-zero if saturation transition is active
static uint8_t saturation_transitioning;

static sl_simple_timer_t hue_transition_timer;
static sl_simple_timer_t saturation_transition_timer;

// Timer callbacks
static void hue_transition_timer_cb(sl_simple_timer_t *timer, void *data);
static void saturation_transition_timer_cb(sl_simple_timer_t *timer, void *data);

////////////////////////////////////////////////////////////////////////////////
//    HSL Callbacks                                                           //
////////////////////////////////////////////////////////////////////////////////

SL_WEAK void sl_btmesh_hsl_hue_cb(uint16_t hue)
{
  (void)hue;
}

SL_WEAK void sl_btmesh_hsl_saturation_cb(uint16_t saturation)
{
  (void)saturation;
}

SL_WEAK void sl_btmesh_hsl_hue_on_ui_update(uint16_t hue)
{
  (void)hue;
}

SL_WEAK void sl_btmesh_hsl_saturation_on_ui_update(uint16_t saturation)
{
  (void)saturation;
}

/***************************************************************************//**
 * Handler for Hue Transition Timer, which manages LEDs transitions.
 ******************************************************************************/
static void hue_transition_timer_cb(sl_simple_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  // Initialize the variable to UI update period in order to trigger a UI update
  // at the beginning of the transition.
  static uint16_t time_elapsed_since_ui_update = HSL_SERVER_HUE_UI_UPDATE_PERIOD;

  if (!hue_transitioning) {
    sl_status_t sc = sl_simple_timer_stop(&hue_transition_timer);
    app_assert_status_f(sc, "Failed to stop Periodic Hue Transition Timer\n");
    return;
  } else {
    hue_transtime_elapsed++;

    if (hue_transtime_elapsed >= hue_transtime_ticks) {
      // transition complete
      hue_transitioning = 0;
      current_hue = target_hue;

      // Set the variable to UI update period in order to trigger a UI update
      // at the beginning of the next transition.
      time_elapsed_since_ui_update = HSL_SERVER_HUE_UI_UPDATE_PERIOD;

      // Trigger a UI update in order to provide the target values at the end
      // of the current transition
      sl_btmesh_hsl_hue_on_ui_update(current_hue);
    } else {
      // calculate current hue based on elapsed transition time
      if (target_hue >= start_hue) {
        current_hue = start_hue
                      + (target_hue - start_hue)
                      * (uint64_t)hue_transtime_elapsed
                      / hue_transtime_ticks;
      } else {
        current_hue = start_hue
                      - (start_hue - target_hue)
                      * (uint64_t)hue_transtime_elapsed
                      / hue_transtime_ticks;
      }

      // When transition is ongoing generate an event to application once every
      // HSL_SERVER_HUE_UI_UPDATE_PERIOD ms because the event is used to update
      // display status and therefore the rate should not be too high
      time_elapsed_since_ui_update += HSL_SERVER_HUE_UI_UPDATE_PERIOD;

      if (HSL_SERVER_HUE_UI_UPDATE_PERIOD <= time_elapsed_since_ui_update) {
        time_elapsed_since_ui_update -= HSL_SERVER_HUE_UI_UPDATE_PERIOD;
        sl_btmesh_hsl_hue_on_ui_update(current_hue);
      }
    }
  }

  sl_btmesh_hsl_hue_cb(current_hue);
}

/***************************************************************************//**
 * Handler for Saturation Transition Timer, which manages LEDs transitions.
 ******************************************************************************/
static void saturation_transition_timer_cb(sl_simple_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  // Initialize the variable to UI update period in order to trigger a UI update
  // at the beginning of the transition.
  static uint16_t time_elapsed_since_ui_update = HSL_SERVER_SATURATION_UI_UPDATE_PERIOD;

  if (!saturation_transitioning) {
    sl_status_t sc = sl_simple_timer_stop(&saturation_transition_timer);
    app_assert_status_f(sc, "Failed to stop Periodic Saturation Transition Timer\n");
    return;
  } else {
    saturation_transtime_elapsed++;

    if (saturation_transtime_elapsed >= saturation_transtime_ticks) {
      // transition complete
      saturation_transitioning = 0;
      current_saturation = target_saturation;

      // Set the variable to UI update period in order to trigger a UI update
      // at the beginning of the next transition.
      time_elapsed_since_ui_update = HSL_SERVER_SATURATION_UI_UPDATE_PERIOD;

      // Trigger a UI update in order to provide the target values at the end
      // of the current transition
      sl_btmesh_hsl_saturation_on_ui_update(current_saturation);
    } else {
      // calculate current hue based on elapsed transition time
      if (target_saturation >= start_saturation) {
        current_saturation = start_saturation
                             + (target_saturation - start_saturation)
                             * (uint64_t)saturation_transtime_elapsed
                             / saturation_transtime_ticks;
      } else {
        current_saturation = start_saturation
                             - (target_saturation - start_saturation)
                             * (uint64_t)saturation_transtime_elapsed
                             / saturation_transtime_ticks;
      }

      // When transition is ongoing generate an event to application once every
      // HSL_SERVER_SATURATION_UI_UPDATE_PERIOD ms because the event is used to update
      // display status and therefore the rate should not be too high
      time_elapsed_since_ui_update += HSL_SERVER_SATURATION_UI_UPDATE_PERIOD;

      if (HSL_SERVER_SATURATION_UI_UPDATE_PERIOD <= time_elapsed_since_ui_update) {
        time_elapsed_since_ui_update -= HSL_SERVER_SATURATION_UI_UPDATE_PERIOD;
        sl_btmesh_hsl_saturation_on_ui_update(current_saturation);
      }
    }
  }

  sl_btmesh_hsl_saturation_cb(current_saturation);
}

/*******************************************************************************
 * Set LED hue in given transition time.
 *
 * @param[in] hue            Hue level.
 * @param[in] transition_ms  Transition time in milliseconds.
 ******************************************************************************/
void sl_btmesh_hsl_set_hue_level(uint16_t hue, uint32_t transition_ms)
{
#if HSL_SERVER_MINIMUM_HUE != (0)
  if (hue < HSL_SERVER_MINIMUM_HUE) {
    hue = HSL_SERVER_MINIMUM_HUE;
  }
#endif

#if HSL_SERVER_MAXIMUM_HUE != (65535)
  if (hue > HSL_SERVER_MAXIMUM_HUE) {
    hue = HSL_SERVER_MAXIMUM_HUE;
  }
#endif

  if (transition_ms == 0) {
    current_hue = hue;

    sl_btmesh_hsl_hue_cb(current_hue);

    /* if a transition was in progress, cancel it */
    if (hue_transitioning) {
      hue_transitioning = 0;
      sl_status_t sc = sl_simple_timer_stop(&hue_transition_timer);
      app_assert_status_f(sc, "Failed to stop periodic Hue Transition Timer\n");
    }
    sl_btmesh_hsl_hue_on_ui_update(current_hue);
    return;
  }

  hue_transtime_ticks = transition_ms;

  start_hue = current_hue;
  target_hue = hue;

  hue_transtime_elapsed = 0;
  hue_transitioning = 1;

  // enabling timer IRQ -> the temperature is adjusted in timer interrupt
  // gradually until target temperature is reached.
  sl_status_t sc = sl_simple_timer_start(&hue_transition_timer,
                                         HSL_SERVER_HUE_UPDATE_PERIOD,
                                         hue_transition_timer_cb,
                                         NO_CALLBACK_DATA,
                                         true);
  app_assert_status_f(sc, "Failed to start periodic Hue Transition Timer\n");

  return;
}

/*******************************************************************************
 * Set LED saturation in given transition time.
 *
 * @param[in] saturation     Saturation level.
 * @param[in] transition_ms  Transition time in milliseconds.
 ******************************************************************************/
void sl_btmesh_hsl_set_saturation_level(uint16_t saturation, uint32_t transition_ms)
{
#if HSL_SERVER_MINIMUM_SATURATION != (0)
  if (saturation < HSL_SERVER_MINIMUM_SATURATION) {
    saturation = HSL_SERVER_MINIMUM_SATURATION;
  }
#endif

#if HSL_SERVER_MAXIMUM_SATURATION != (65535)
  if (saturation > HSL_SERVER_MAXIMUM_SATURATION) {
    saturation = HSL_SERVER_MAXIMUM_SATURATION;
  }
#endif

  if (transition_ms == 0) {
    current_saturation = saturation;

    sl_btmesh_hsl_saturation_cb(current_saturation);

    /* if a transition was in progress, cancel it */
    if (saturation_transitioning) {
      saturation_transitioning = 0;
      sl_status_t sc = sl_simple_timer_stop(&saturation_transition_timer);
      app_assert_status_f(sc, "Failed to stop periodic Saturation Transition Timer\n");
    }
    sl_btmesh_hsl_saturation_on_ui_update(current_saturation);
    return;
  }

  saturation_transtime_ticks = transition_ms;

  start_saturation = current_saturation;
  target_saturation = saturation;

  saturation_transtime_elapsed = 0;
  saturation_transitioning = 1;

  // enabling timer IRQ -> the temperature is adjusted in timer interrupt
  // gradually until target temperature is reached.
  sl_status_t sc = sl_simple_timer_start(&saturation_transition_timer,
                                         HSL_SERVER_SATURATION_UPDATE_PERIOD,
                                         saturation_transition_timer_cb,
                                         NO_CALLBACK_DATA,
                                         true);
  app_assert_status_f(sc, "Failed to start periodic Saturation Transition Timer\n");

  return;
}

/*******************************************************************************
 * Function for retrieving current hue.
 *
 * @return Current hue level.
 ******************************************************************************/
uint16_t sl_btmesh_get_hue(void)
{
  return(current_hue);
}

/*******************************************************************************
 * Function for retrieving current saturation.
 *
 * @return Current saturation level.
 ******************************************************************************/
uint16_t sl_btmesh_get_saturation(void)
{
  return(current_saturation);
}

/** @} (end addtogroup HSL Transition Handler) */
