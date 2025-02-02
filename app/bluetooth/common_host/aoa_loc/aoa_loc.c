/***************************************************************************//**
 * @file
 * @brief Location calculation engine.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "aoa_loc.h"
#include "aoa_util.h"

// -----------------------------------------------------------------------------
// Defines

#ifndef SL_WEAK
#define SL_WEAK __attribute__ ((weak))
#endif

#define CHECK_ERROR(x)             \
  if ((x) != SL_RTL_ERROR_SUCCESS) \
    return SL_STATUS_FAIL

#define ROUND_DIV(num, den) (((num) + ((den) / 2)) / (den))

// -----------------------------------------------------------------------------
// Default values of configuration parameters.

// Location estimation mode.
#define AOA_LOC_ESTIMATION_MODE         SL_RTL_LOC_ESTIMATION_MODE_THREE_DIM_HIGH_ACCURACY

// Estimation interval in seconds.
// This value should approximate the time interval between two consecutive CTEs.
#define AOA_LOC_ESTIMATION_INTERVAL_SEC 0.02f

// Filter weight applied on the location coordinates. Ranges from 0 to 1.
#define AOA_LOC_FILTERING_AMOUNT        0.1f

// Measurement validation mode for bad angle feedback.
#define AOA_LOC_VALIDATION_MODE         SL_RTL_LOC_MEASUREMENT_VALIDATION_MEDIUM

// -----------------------------------------------------------------------------
// Type definitions.

// Forward declarations
typedef struct aoa_locator_node aoa_locator_node_t;
typedef struct aoa_asset_tag_node aoa_asset_tag_node_t;

// -----------------------------------------------------------------------------
// Private function declarations.
static sl_status_t aoa_loc_init_asset_tag(aoa_asset_tag_t *tag);
static sl_status_t aoa_loc_deinit_asset_tag(aoa_asset_tag_t *tag);
static sl_status_t aoa_loc_run_estimation(aoa_asset_tag_t *tag,
                                          aoa_corr_angles_t *correlated_angles);
static void aoa_loc_destroy_tags(void);
static void aoa_loc_destroy_locators(void);

// -----------------------------------------------------------------------------
// Module variables

// Locator node
struct aoa_locator_node{
  aoa_locator_t locator;
  aoa_locator_node_t *next;
};

// Asset tag node
struct aoa_asset_tag_node{
  aoa_asset_tag_t tag;
  aoa_asset_tag_node_t *next;
};

// Linked list heads
static aoa_asset_tag_node_t *head_tag = NULL;
static aoa_locator_node_t *head_locator = NULL;

// Configuration parameters
aoa_loc_config_t aoa_loc_config = {
  0,
  AOA_LOC_ESTIMATION_MODE,
  AOA_LOC_VALIDATION_MODE,
  AOA_LOC_ESTIMATION_INTERVAL_SEC,
  true,
  AOA_LOC_FILTERING_AMOUNT,
  true
};

// The expected number of angles depends on the slot index. A slot with smaller
// index (i.e. with more recent sequence number) expects more angles, while a
// slot with higher index (i.e. with older sequence number) requires less
// angles.
// This module implements a linear connection, thus the first slot expects
// an angle from all locators, while the last slot needs only 2 locators.
static uint32_t *expected_angles_count;

// -----------------------------------------------------------------------------
// Public function definitions.

/**************************************************************************//**
 * Initializes the expected angle count for the module.
 *****************************************************************************/
void aoa_loc_init_expected_angle_count(void)
{
  uint32_t coeff;

  // Init the expected angles
  expected_angles_count = (uint32_t *)malloc(aoa_corr_angles_config.sequence_id_slots * sizeof(uint32_t));
  coeff = (aoa_loc_config.locator_count < 2) ? 0 : (aoa_loc_config.locator_count - 2);
  for (uint32_t i = 0; i < aoa_corr_angles_config.sequence_id_slots; i++) {
    expected_angles_count[i] = aoa_loc_config.locator_count  - ROUND_DIV(i * coeff, aoa_corr_angles_config.sequence_id_slots - 1);
  }
}

/**************************************************************************//**
 * Add a locator to the list.
 *****************************************************************************/
sl_status_t aoa_loc_add_locator(aoa_id_t locator_id,
                                struct sl_rtl_loc_locator_item item,
                                aoa_locator_t **locator)
{
  aoa_locator_node_t *new = (aoa_locator_node_t *)malloc(sizeof(aoa_locator_node_t));
  if (NULL == new) {
    return SL_STATUS_ALLOCATION_FAILED;
  }
  new->next = head_locator;
  head_locator = new;

  aoa_id_copy(new->locator.id, locator_id);
  new->locator.item.coordinate_x = item.coordinate_x;
  new->locator.item.coordinate_y = item.coordinate_y;
  new->locator.item.coordinate_z = item.coordinate_z;

  new->locator.item.orientation_x_axis_degrees = item.orientation_x_axis_degrees;
  new->locator.item.orientation_y_axis_degrees = item.orientation_y_axis_degrees;
  new->locator.item.orientation_z_axis_degrees = item.orientation_z_axis_degrees;

  *locator = &(new->locator);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Returns a locator from the locator list by its id.
 *****************************************************************************/
sl_status_t aoa_loc_get_locator_by_id(aoa_id_t locator_id,
                                      uint32_t *locator_idx,
                                      aoa_locator_t **locator)
{
  aoa_locator_node_t *current = head_locator;
  uint32_t i = 0;

  if (NULL == head_locator) {
    return SL_STATUS_NOT_FOUND;
  }

  while (NULL != current) {
    if (0 == aoa_id_compare(current->locator.id, locator_id)) {
      *locator = &(current->locator);
      *locator_idx = i;
      return SL_STATUS_OK;
    }
    i++;
    current = current->next;
  }

  return SL_STATUS_NOT_FOUND;
}

/**************************************************************************//**
 * Returns a locator from the locator list by its index.
 *****************************************************************************/
sl_status_t aoa_loc_get_locator_by_index(uint32_t locator_idx,
                                         aoa_locator_t **locator)
{
  uint32_t i = 0;
  aoa_locator_node_t *current = head_locator;

  if (NULL == head_locator) {
    return SL_STATUS_NOT_FOUND;
  }

  while (NULL != current) {
    if (i == locator_idx) {
      *locator = &(current->locator);
      return SL_STATUS_OK;
    }
    i++;
    current = current->next;
  }

  return SL_STATUS_NOT_FOUND;
}

/**************************************************************************//**
 * Returns the number of locators on the list.
 *****************************************************************************/
size_t aoa_loc_get_number_of_locators(void)
{
  size_t i = 0;
  aoa_locator_node_t *current = head_locator;

  while (NULL != current) {
    i++;
    current = current->next;
  }

  return i;
}

/**************************************************************************//**
 * Add asset tag to the list.
 *****************************************************************************/
sl_status_t aoa_loc_add_asset_tag(aoa_id_t tag_id,
                                  aoa_asset_tag_t **tag)
{
  aoa_asset_tag_node_t *new = (aoa_asset_tag_node_t *)malloc(sizeof(aoa_asset_tag_node_t));
  if (NULL == new) {
    return SL_STATUS_ALLOCATION_FAILED;
  }
  aoa_id_copy(new->tag.id, tag_id);
  new->next = head_tag;
  head_tag = new;
  *tag = &(new->tag);

  if (SL_STATUS_OK != aoa_loc_init_asset_tag(*tag)) {
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Returns a tag from the tag list by its id.
 *****************************************************************************/
sl_status_t aoa_loc_get_tag_by_id(aoa_id_t id,
                                  aoa_asset_tag_t **tag)
{
  aoa_asset_tag_node_t *current = head_tag;

  if (NULL == head_tag) {
    return SL_STATUS_NOT_FOUND;
  }

  while (NULL != current) {
    if (0 == aoa_id_compare(current->tag.id, id)) {
      *tag = &(current->tag);
      return SL_STATUS_OK;
    }
    current = current->next;
  }

  return SL_STATUS_NOT_FOUND;
}

/**************************************************************************//**
 * Returns a tag from the tag list by its index.
 *****************************************************************************/
sl_status_t aoa_loc_get_tag_by_index(uint32_t index,
                                     aoa_asset_tag_t **tag)
{
  aoa_asset_tag_node_t *current = head_tag;
  uint32_t i = 0;

  if (NULL == head_tag) {
    return SL_STATUS_NOT_FOUND;
  }

  while (NULL != current) {
    if (i == index) {
      *tag = &(current->tag);
      return SL_STATUS_OK;
    }
    i++;
    current = current->next;
  }

  return SL_STATUS_NOT_FOUND;
}

/**************************************************************************//**
 * Calculates the asset tag position and notify the app.
 *****************************************************************************/
sl_status_t aoa_loc_calc_position(aoa_asset_tag_t *tag,
                                  int32_t start_slot,
                                  aoa_corr_angles_t *corr_angles_array,
                                  uint32_t *last_updated_slot)
{
  sl_status_t sc;
  aoa_locator_t *locator;

  if (start_slot < 0) {
    return SL_STATUS_FAIL;
  }
  *last_updated_slot = aoa_corr_angles_config.sequence_id_slots;
  // Check start from the oldest slots to keep in order and complete as many
  // slots as possible.
  for (int32_t i = aoa_corr_angles_config.sequence_id_slots - 1; i >= start_slot; i--) {
    if (corr_angles_array[i].num_angles >= expected_angles_count[i]) {
      sc = aoa_loc_run_estimation(tag, &corr_angles_array[i]);
      if (SL_STATUS_OK != sc) {
        return sc;
      }

      // Notify the application
      aoa_loc_on_position_ready(tag);
      *last_updated_slot = i;

      // Check if some of the locators require correction.
      if (aoa_loc_config.is_feedback_enabled && sl_rtl_loc_get_number_disabled(&tag->loc) > 0) {
        aoa_correction_t correction;

        for (uint32_t j = 0; j < aoa_loc_config.locator_count; j++) {
          if (corr_angles_array[i].has_angle[j]) {
            sc = sl_rtl_loc_get_expected_direction(&tag->loc,
                                                   tag->loc_id[j],
                                                   &correction.direction.azimuth,
                                                   &correction.direction.elevation,
                                                   &correction.direction.distance);
            if (sc == SL_RTL_ERROR_INCORRECT_MEASUREMENT) {
              sl_rtl_loc_get_expected_deviation(&tag->loc,
                                                tag->loc_id[j],
                                                &correction.deviation.azimuth,
                                                &correction.deviation.elevation,
                                                &correction.deviation.distance);
              correction.sequence = tag->position.sequence;
              aoa_loc_get_locator_by_index(j, &locator);
              // The first angle in the correlated angle list is the most recent one.
              aoa_loc_on_correction_ready(tag, corr_angles_array[0].sequence, locator->id, j, &correction);
            }
          }
        }
      }
    }
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Destroys the module database
 *****************************************************************************/
void aoa_loc_destroy(void)
{
  free(expected_angles_count);
  aoa_loc_destroy_tags();
  aoa_loc_destroy_locators();
}

/**************************************************************************//**
 * Position ready callback. Weak, implement it in the application.
 *****************************************************************************/
SL_WEAK void aoa_loc_on_position_ready(aoa_asset_tag_t *tag)
{
  // Implement in the application.
}

/**************************************************************************//**
 * Correction ready callback. Weak, implement it in the application.
 *****************************************************************************/
SL_WEAK void aoa_loc_on_correction_ready(aoa_asset_tag_t *tag,
                                         int32_t sequence,
                                         aoa_id_t locator_id,
                                         uint32_t loc_idx,
                                         aoa_correction_t *correction)
{
  // Implement in the application.
}

/**************************************************************************//**
 * Angle init callback. Used when angle calculation is needed from IQ report.
 * Weak, implement it in the application.
 *****************************************************************************/
SL_WEAK void aoa_loc_angle_init(aoa_asset_tag_t *tag,
                                uint32_t locator_count)
{
  // Implement in the application.
}

/**************************************************************************//**
 * Angle deinit callback. Weak, implement it in the application.
 *****************************************************************************/
SL_WEAK void aoa_loc_angle_deinit(aoa_asset_tag_t *tag,
                                  uint32_t locator_count)
{
  // Implement in the application.
}

// -----------------------------------------------------------------------------
// Private function definitions.

/**************************************************************************//**
 * Initializes an asset tag.
 *****************************************************************************/
static sl_status_t aoa_loc_init_asset_tag(aoa_asset_tag_t *tag)
{
  enum sl_rtl_error_code ec;
  aoa_locator_node_t *current_locator = head_locator;

  // Invalid sequence
  tag->position.sequence = -1;

  tag->aoa_state = NULL;

  // Initialize locator ids for the tag
  tag->loc_id = (uint32_t *)malloc(aoa_loc_config.locator_count * sizeof(uint32_t));

  // Initialize RTL library
  ec = sl_rtl_loc_init(&tag->loc);
  CHECK_ERROR(ec);

  // Select estimation mode.
  ec = sl_rtl_loc_set_mode(&tag->loc, aoa_loc_config.estimation_mode);
  CHECK_ERROR(ec);

  // Provide locator configurations to the position estimator.
  for (size_t i = 0; i < aoa_loc_config.locator_count; i++) {
    ec = sl_rtl_loc_add_locator(&tag->loc, &current_locator->locator.item, &tag->loc_id[i]);
    current_locator = current_locator->next;
    CHECK_ERROR(ec);
  }

  // Initialize the angle calculation.
  aoa_loc_angle_init(tag, aoa_loc_config.locator_count);

  // Create position estimator.
  ec = sl_rtl_loc_create_position_estimator(&tag->loc);
  CHECK_ERROR(ec);

  if (aoa_loc_config.is_feedback_enabled) {
    // Turn on the bad angle detection mechanism.
    ec = sl_rtl_loc_set_measurement_validation(&tag->loc, aoa_loc_config.validation_method);
    CHECK_ERROR(ec);
  }

  if (aoa_loc_config.filtering_enabled == true) {
    // Initialize util functions.
    for (enum axis_list i = 0; i < AXIS_COUNT; i++) {
      ec = sl_rtl_util_init(&tag->filter[i]);
      CHECK_ERROR(ec);
      // Set position filtering parameter for every axis.
      ec = sl_rtl_util_set_parameter(&tag->filter[i],
                                     SL_RTL_UTIL_PARAMETER_AMOUNT_OF_FILTERING,
                                     aoa_loc_config.filtering_amount);
      CHECK_ERROR(ec);
    }
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Deinitializes an asset tag.
 *****************************************************************************/
static sl_status_t aoa_loc_deinit_asset_tag(aoa_asset_tag_t *tag)
{
  enum sl_rtl_error_code ec;

  aoa_loc_angle_deinit(tag, aoa_loc_config.locator_count);
  ec = sl_rtl_loc_deinit(&tag->loc);
  CHECK_ERROR(ec);

  if (aoa_loc_config.filtering_enabled == true) {
    for (enum axis_list i = 0; i < AXIS_COUNT; i++) {
      ec = sl_rtl_util_deinit(&tag->filter[i]);
      CHECK_ERROR(ec);
    }
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Run position estimation algorithm for a given asset tag.
 *****************************************************************************/
static sl_status_t aoa_loc_run_estimation(aoa_asset_tag_t *tag,
                                          aoa_corr_angles_t *correlated_angles)
{
  enum sl_rtl_error_code ec;
  sl_status_t sc;
  float time_step = aoa_loc_config.estimation_interval_sec;
  int32_t seq_diff;
  // Feed measurement values into RTL lib.
  for (uint32_t i = 0; i < aoa_loc_config.locator_count; i++) {
    if (correlated_angles->has_angle[i]) {
      ec = sl_rtl_loc_set_locator_measurement(&tag->loc,
                                              tag->loc_id[i],
                                              SL_RTL_LOC_LOCATOR_MEASUREMENT_AZIMUTH,
                                              correlated_angles->angles[i].azimuth);

      CHECK_ERROR(ec);

      ec = sl_rtl_loc_set_locator_measurement(&tag->loc,
                                              tag->loc_id[i],
                                              SL_RTL_LOC_LOCATOR_MEASUREMENT_ELEVATION,
                                              correlated_angles->angles[i].elevation);
      CHECK_ERROR(ec);

      // Feeding RSSI distance measurement to the RTL library improves location
      // accuracy when the measured distance is reasonably correct.
      // If the received signal strength of the incoming signal is altered for any
      // other reason than the distance between the TX and RX itself, it will lead
      // to incorrect measurement and it will lead to incorrect position estimates.
      // For this reason the RSSI distance usage is disabled by default in the
      // multilocator case.
      // Single locator mode however always requires the distance measurement in
      // addition to the angle, please note the if-condition below.
      // In case the distance estimation should be used in the  multilocator case,
      // you can enable it by commenting out the condition.
      if (aoa_loc_config.locator_count == 1) {
        ec = sl_rtl_loc_set_locator_measurement(&tag->loc,
                                                tag->loc_id[i],
                                                SL_RTL_LOC_LOCATOR_MEASUREMENT_DISTANCE,
                                                correlated_angles->angles[i].distance);
        CHECK_ERROR(ec);
      }
    }
  }

  // Estimate the time step based on the sequence number.
  seq_diff = aoa_sequence_compare(correlated_angles->sequence,
                                  tag->position.sequence);
  if (seq_diff <= aoa_corr_angles_config.max_sequence_diff) {
    time_step *= (float)seq_diff;
  }

  // Process new measurements, time step given in seconds.
  sc = sl_rtl_loc_process(&tag->loc, time_step);
  tag->position.sequence = correlated_angles->sequence;

  CHECK_ERROR(sc);

  // Get results from the estimator.
  sc = sl_rtl_loc_get_result(&tag->loc,
                             SL_RTL_LOC_RESULT_POSITION_X,
                             &tag->position.x);
  CHECK_ERROR(sc);
  sc = sl_rtl_loc_get_result(&tag->loc,
                             SL_RTL_LOC_RESULT_POSITION_Y,
                             &tag->position.y);
  CHECK_ERROR(sc);
  sc = sl_rtl_loc_get_result(&tag->loc,
                             SL_RTL_LOC_RESULT_POSITION_Z,
                             &tag->position.z);
  CHECK_ERROR(sc);

  if (aoa_loc_config.filtering_enabled == true) {
    // Apply filter on the result.
    sc = sl_rtl_util_filter(&tag->filter[AXIS_X],
                            tag->position.x,
                            &tag->position.x);
    CHECK_ERROR(sc);
    sc = sl_rtl_util_filter(&tag->filter[AXIS_Y],
                            tag->position.y,
                            &tag->position.y);
    CHECK_ERROR(sc);
    sc = sl_rtl_util_filter(&tag->filter[AXIS_Z],
                            tag->position.z,
                            &tag->position.z);
    CHECK_ERROR(sc);
  }

  // Clear measurements.
  ec = sl_rtl_loc_clear_measurements(&tag->loc);
  CHECK_ERROR(ec);

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Destroys the tags database
 *****************************************************************************/
static void aoa_loc_destroy_tags(void)
{
  aoa_asset_tag_node_t *current;
  aoa_asset_tag_node_t *next;

  for (current = head_tag; current != NULL; current = next) {
    next = current->next;
    aoa_loc_deinit_asset_tag(&current->tag);
    free(current->tag.loc_id);
    free(current);
  }

  head_tag = NULL;
}

/**************************************************************************//**
 * Destroys the locator database
 *****************************************************************************/
static void aoa_loc_destroy_locators(void)
{
  aoa_locator_node_t *current;
  aoa_locator_node_t *next;

  for (current = head_locator; current != NULL; current = next) {
    next = current->next;
    free(current);
  }

  head_locator = NULL;
}
