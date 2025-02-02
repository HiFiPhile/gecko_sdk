/***************************************************************************/ /**
 * @file
 * @brief BT Mesh Configurator Component - Default Configuration
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

#ifndef BTMESH_CONF_CONFIG_H
#define BTMESH_CONF_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

// <<< Use Configuration Wizard in Context Menu >>>

// <o BTMESH_CONF_MAX_LOG_MSG_FRAGMENT_SIZE> Log message fragment buffer size
// <i> Default: 256
// <8-1024:1>
// <i> Log message fragment buffers are allocated on stack and used to format parts of log messages.
#define BTMESH_CONF_MAX_LOG_MSG_FRAGMENT_SIZE         256

// <o BTMESH_CONF_EXECUTOR_COUNT> Configuration executor count
// <i> Default: 1
// <1-256:1>
// <i> Specifies the maximum number of parallel active configuration jobs.
// <i> The executor count shall be less than SL_BTMESH_CONFIG_MAX_SEND_SEGS and SL_BTMESH_CONFIG_APP_TXQ_SIZE in sl_btmesh_config.h of NCP embedded code.
#define BTMESH_CONF_EXECUTOR_COUNT                      1

// <o BTMESH_CONF_REQUEST_TIMEOUT_MS> Configuration client request timeout (ms)
// <i> Default: 5000
// <1-4294967295:1>
// <i> Default timeout in milliseconds of configuration client requests.
// <i> The BT Mesh Stack emits a configuration client event with SL_STATUS_TIMEOUT result if no reponse is received from configuration server.
#define BTMESH_CONF_REQUEST_TIMEOUT_MS                  5000

// <o BTMESH_CONF_LPN_REQUEST_TIMEOUT_MS> Configuration client LPN request timeout (ms)
// <i> Default: 15000
// <1-4294967295:1>
// <i> Default timeout in milliseconds of configuration client requests when communicating with an LPN node.
// <i> The BT Mesh Stack emits a configuration client event with SL_STATUS_TIMEOUT result if no reponse is received from configuration server.
#define BTMESH_CONF_LPN_REQUEST_TIMEOUT_MS              15000

// <o BTMESH_CONF_EVENT_WAIT_TIMEOUT_MS> Wait for event timeout (ms)
// <i> Default: 30000
// <1-4294967295:1>
// <i> Timeout in milliseconds to wait for configuration client events.
// <i> It shall be greater than BTMESH_CONF_REQUEST_TIMEOUT_MS and BTMESH_CONF_LPN_REQUEST_TIMEOUT_MS.
// <i> The BT Mesh Stack triggers configuration events if the configuration client request timeout occurs.
// <i> Wait for event timeout handles the extremely rare case of missing events. (e.g. too many events through NCP protocol)
#define BTMESH_CONF_EVENT_WAIT_TIMEOUT_MS               30000

// <o BTMESH_CONF_COMMUNICATION_RETRY_MAX> Maximum number of request retry due to communication issues
// <i> Default: 3
// <0-1000:1>
// <i> The maximum number of configuration request retry due to communication issues (BT Mesh Stack configuration request timeout).
#define BTMESH_CONF_COMMUNICATION_RETRY_MAX             3

// <o BTMESH_CONF_REQUEST_BUSY_RETRY_INTERVAL_MS> Retry interval in case of busy configuration request (ms)
// <i> Default: 1000
// <1-4294967295:1>
// <i> Retry interval in milliseconds in case of busy configuration client requests. Config requests can be busy due to unavailable resources in BT Mesh Stack.
#define BTMESH_CONF_REQUEST_BUSY_RETRY_INTERVAL_MS      1000

// <o BTMESH_CONF_REQUEST_BUSY_RETRY_MAX> Maximum number of configuration request retry due to busy BT Mesh Stack
// <i> Default: 10
// <0-10000:1>
// <i> The maximum number of configuration request retry because of rejected configuration request due to busy BT Mesh Stack.
// <i> The BT Mesh Stack might reject configuration client requests temporarily due to lack of resources.
// <i> For example maximum number of parallel segmented message transmissions is reached.
#define BTMESH_CONF_REQUEST_BUSY_RETRY_MAX              10

// <e BTMESH_CONF_JOB_AUTO_DESTROY> Configuration Job Auto Destroy
// <i> Default: 1
// <i> The default configuration job is deallocated automatically after the job status notification.
// <i> If auto destroy feature is turned off (0) then the job deallocation shall be performed manually (btmesh_conf_job_destroy) after job execution ends.
#define BTMESH_CONF_JOB_AUTO_DESTROY                    1

// <q BTMESH_CONF_JOB_AUTO_DESTROY_ON_SUBMIT_FAILURE> Configuration Job Auto Destroy on Submit failure
// <i> Default: 1
// <i> The default configuration job is deallocated automatically if the submit job operation is failed.
// <i> If auto destroy feature is turned off (0) then the job deallocation shall be performed manually (btmesh_conf_job_destroy) after submit operation fails.
#define BTMESH_CONF_JOB_AUTO_DESTROY_ON_SUBMIT_FAILURE  1

// </e>

// <<< end of configuration section >>>

#if (0 == BTMESH_CONF_JOB_AUTO_DESTROY) \
  && (0 != BTMESH_CONF_JOB_AUTO_DESTROY_ON_SUBMIT_FAILURE)
#error "If the auto destroy feature is turned off then the auto destroy on " \
  "submit failure option shall be disabled as well."
#endif

#if (BTMESH_CONF_EVENT_WAIT_TIMEOUT_MS <= BTMESH_CONF_REQUEST_TIMEOUT_MS) \
  || (BTMESH_CONF_EVENT_WAIT_TIMEOUT_MS <= BTMESH_CONF_LPN_REQUEST_TIMEOUT_MS)
#error "Wait for event timeout shall be greater than configuration client " \
  "request timeout in the BT Mesh Stack."
#endif

#ifdef __cplusplus
};
#endif

#endif /* BTMESH_CONF_CONFIG_H */
