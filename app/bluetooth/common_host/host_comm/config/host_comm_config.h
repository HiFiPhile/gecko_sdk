/***************************************************************************//**
 * @file
 * @brief Host communication application module config.
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
#ifndef HOST_COMM_CONFIG_H
#define HOST_COMM_CONFIG_H

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Receive Thread Configuration

// <q RECV_FUNC_US_SLEEP> <0-10000>
// <i> Microsec delay used in receive thread.
// <i> Set to 0 if e.g. ioctl is not supported or high data flowrate.
// <i> Setting it to 0 might mean higher CPU usage.
#ifndef RECV_FUNC_US_SLEEP
#define RECV_FUNC_US_SLEEP      0
#endif

// </h> End Receive Thread Configuration

// <<< end of configuration section >>>

#endif // HOST_COMM_CONFIG_H
