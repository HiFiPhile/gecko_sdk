/***************************************************************************//**
 * @file
 * @brief Air pressure sensor
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#include <stddef.h>
#include "sl_board_control.h"
#include "sl_bmp280.h"
#include "sl_sensor_select.h"
#include "app_assert.h"
#include "sl_sensor_pressure.h"

// -----------------------------------------------------------------------------
// Public function definitions

void sl_sensor_pressure_init(void)
{
  sl_status_t sc;
  sl_i2cspm_t *pressure_sensor = sl_sensor_select(SL_BOARD_SENSOR_PRESSURE);
  sc = sl_board_enable_sensor(SL_BOARD_SENSOR_RHT);
  app_assert((SL_STATUS_OK == sc) && (NULL != pressure_sensor),
             "[E: %#04x] Pressure sensor not available\n",
             sc);
  sc = sl_bmp280_init(pressure_sensor);
  app_assert_status(sc);
}

void sl_pressure_deinit(void)
{
  (void)sl_board_disable_sensor(SL_BOARD_SENSOR_PRESSURE);
}

sl_status_t sl_pressure_get(float *pressure)
{
  sl_status_t sc;
  sl_i2cspm_t *pressure_sensor = sl_sensor_select(SL_BOARD_SENSOR_PRESSURE);
  sc = sl_bmp280_measure_pressure(pressure_sensor, pressure);
  return sc;
}
