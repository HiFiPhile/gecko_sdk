/***************************************************************************//**
 * @file
 * @brief Zigbee Over-the-air bootload cluster for upgrading firmware and
 * downloading specific file.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "app/framework/util/common.h"
#include "app/util/serial/sl_zigbee_command_interpreter.h"

#include "app/framework/plugin/ota-common/ota.h"
#include "app/framework/plugin/ota-common/ota-cli.h"
#include "app/framework/plugin/ota-server-policy/ota-server-policy.h"
#include "app/framework/plugin/ota-server/ota-server.h"
#include "app/framework/plugin/ota-storage-simple/ota-storage-simple-driver.h"
#include "app/framework/plugin/ota-storage-common/ota-storage.h"

/**
 * @addtogroup cli
 * @{
 */
/**
 * @brief ota-server CLI commands. Used to drive the ota-server.
 *
 *        <b>plugin ota-server notify &lt;destination&gt; &lt;payload type&gt; &lt;jitter&gt;
 *             &lt;manuf-id&gt; &lt;device-id&gt; &lt;version&gt;</b>
 *        - <i>This command sends an OTA Image Notify message to the specified
 *             destination indicating a new version of an image is available for
 *             download.</i>
 *          - <i>destination - uint16_t.</i>
 *          - <i>endpoint - uint8_t.</i>
 *          - <i>payload type - uint8_t. The payload type field is as follows:
 *            - 0: Include only jitter field
 *            - 1: Include jitter and manuf-id
 *            - 2: Include jitter, manuf-id, and device-id
 *            - 3: Include jitter, manuf-id, device-id, and version</i>
 *          - <i>jitter - uint8_t.</i>
 *          - <i>manuf-id - uint16_t.</i>
 *          - <i>device-id - uint16_t.</i>
 *          - <i>version - uint16_t.</i>
 *
 *            <i>All fields in the CLI command must be specified.
 *            However if the payload type is less than 3, those
 *            values will be ignored and not included in the message.</i>
 *
 *        <b>plugin ota-server upgrade</b>
 *        - <i></i>
 *
 *        <b>plugin ota-server policy print</b>
 *        - <i>Prints the polices used by the ota-server Policy Plugin.</I>
 *
 *        <b>plugin ota-server policy query &lt;policy value&gt;</b>
 *        - <i>Sets the policy used by the ota-server Policy Plugin
 *             when it receives a query request from the client.</i>
 *          - <i>policy value - uint8_t. The policy values are as follows:
 *            - 0: Upgrade if server has newer (default)
 *            - 1: Downgrade if server has older
 *            - 2: Reinstall if server has same
 *            - 3: No next version (no next image is available for download)</i>
 *
 *        <b>plugin ota-server policy block-request &lt;policy value&gt;</b>
 *        - <i>Sets the policy used by the ota-server Policy Plugin
 *             when it receives an image block request.
 *          - <i>policy value - uint8_t. The policy values are as follows:
 *            - 0: Send block (default)
 *            - 1: Delay download once for 2 minutes
 *            - 2: Always abort download after first block</i>
 *
 *        <b>plugin ota-server policy upgrade &lt;policy value&gt;</b>
 *        - <i>Sets the policy used by the ota-server Policy Plugin
 *             when it receives an upgrade end request.
 *          - <i>policy value - uint8_t. The policy values are as follows:
 *            - 0: Upgrade Now (default)
 *            - 1: Upgrade in 2 minutes
 *            - 2: Ask me later to upgrade
 *            - 3: Abort Upgrade (send default response)</i>
 *
 *        <b>plugin ota-server policy page-req-miss</b>
 *        - <i><i>
 *
 *        <b>plugin ota-server policy page-req-sup</b>
 *        - <i></i>
 *
 */
#define EMBER_AF_DOXYGEN_CLI__OTA_SERVER_COMMANDS
/** @} END addtogroup */

#ifdef  UC_BUILD
#include "sl_component_catalog.h"

void otaImageNotifyCommand(sl_cli_command_arg_t *arguments)
{
  EmberAfOtaImageId id;
  EmberNodeId dest = (EmberNodeId)sl_cli_get_argument_uint16(arguments, 0);
  uint8_t endpoint = sl_cli_get_argument_uint8(arguments, 1);
  uint8_t payloadType = sl_cli_get_argument_uint8(arguments, 2);
  uint8_t jitter = sl_cli_get_argument_uint8(arguments, 3);
  id.manufacturerId = sl_cli_get_argument_uint16(arguments, 4);
  id.imageTypeId = sl_cli_get_argument_uint16(arguments, 5);
  id.firmwareVersion = sl_cli_get_argument_uint32(arguments, 6);

  emberAfOtaServerSendImageNotifyCallback(dest,
                                          endpoint,
                                          payloadType,
                                          jitter,
                                          &id);
}

#if defined(SL_CATALOG_ZIGBEE_OTA_SERVER_POLICY_PRESENT)
void setPolicy(sl_cli_command_arg_t *arguments)
{
  if (memcmp(arguments->argv[arguments->arg_ofs - 1], "image-req-min-period", strlen("image-req-min-period")) == 0 ) {
    uint16_t minBlockPeriodMs = sl_cli_get_argument_uint16(arguments, 0);
    emAfOtaServerPolicySetMinBlockRequestPeriod(minBlockPeriodMs);
  } else {
    uint8_t value = sl_cli_get_argument_uint8(arguments, 0);
    if (memcmp(arguments->argv[arguments->arg_ofs - 1], "query", strlen("query")) == 0  ) {
      emAfOtaServerSetQueryPolicy(value);
    } else if (memcmp(arguments->argv[arguments->arg_ofs - 1], "block-request", strlen("block-request")) == 0  ) {
      emAfOtaServerSetBlockRequestPolicy(value);
    } else if (memcmp(arguments->argv[arguments->arg_ofs - 1], "upgrade", strlen("upgrade")) == 0  ) {
      emAfOtaServerSetUpgradePolicy(value);
    } else if (memcmp(arguments->argv[arguments->arg_ofs - 1], "page-req-miss", strlen("page-req-miss")) == 0 ) {
      emAfSetPageRequestMissedBlockModulus(value);
    } else if (memcmp(arguments->argv[arguments->arg_ofs - 1], "page-req-sup", strlen("page-req-sup")) == 0 ) {
      emAfOtaServerSetPageRequestPolicy(value);
    }
  }
}
#endif // SL_CATALOG_ZIGBEE_OTA_SERVER_POLICY_PRESENT

// This should be deadstripped out if EMBER_TEST is undefined
#ifdef EMBER_TEST
void otaServerSetClientDelayUnits(sl_cli_command_arg_t *arguments)
{
  #ifdef SL_CATALOG_ZIGBEE_OTA_SERVER_POLICY_PRESENT
  extern uint8_t testClientDelayUnit;
  testClientDelayUnit = sl_cli_get_argument_uint8(arguments, 0);
  #endif // SL_CATALOG_ZIGBEE_OTA_SERVER_POLICY_PRESENT
}
#endif

void otaSendUpgradeCommand(sl_cli_command_arg_t *arguments)
{
  EmberNodeId dest = (EmberNodeId)sl_cli_get_argument_uint16(arguments, 0);
  uint8_t endpoint = sl_cli_get_argument_uint8(arguments, 1);
  EmberAfOtaImageId id;
  id.manufacturerId = sl_cli_get_argument_uint16(arguments, 2);
  id.imageTypeId = sl_cli_get_argument_uint16(arguments, 3);
  id.firmwareVersion = sl_cli_get_argument_uint32(arguments, 4);

  // Use 0xFFFF for all three arguments if you want wildcard
  // The wild cards tell any device to upgrade, regardless of manufacturer ID,
  // image type, or version

  emberAfOtaServerSendUpgradeCommandCallback(dest, endpoint, &id);
}

#else //UC_BUILD

#ifdef EMBER_AF_PLUGIN_OTA_SERVER

//------------------------------------------------------------------------------
// Globals

void setPolicy(void);

#if defined(EMBER_AF_PLUGIN_OTA_SERVER_POLICY)

#ifndef EMBER_AF_GENERATE_CLI
static const char * queryArguments[] = {
  "Policy enumeration",
  NULL,
};

static EmberCommandEntry policyCommands[] = {
  emberCommandEntryAction("print", emAfOtaServerPolicyPrint, "",
                          "Print the OTA Server's policies"),
  emberCommandEntryActionWithDetails("query", setPolicy, "v",
                                     "Set the OTA Server's query policy",
                                     queryArguments),
  emberCommandEntryAction("block-request", setPolicy, "v", ""),
  emberCommandEntryAction("upgrade", setPolicy, "v", ""),
  emberCommandEntryAction("page-req-miss", setPolicy, "v", ""),
  emberCommandEntryAction("page-req-sup", setPolicy, "v", ""),
  emberCommandEntryAction("image-req-min-period", setPolicy, "v", ""),
  emberCommandEntryTerminator(),
};
#define POLICY_COMMANDS emberCommandEntryAction("policy", NULL, (const char *)policyCommands, ""),
#endif

#if defined(EMBER_TEST)
#define LOAD_FILE_COMMAND emberCommandEntryAction("load-file", emAfOtaLoadFileCommand, "b", ""),
#else
#define LOAD_FILE_COMMAND
#endif

#endif // EMBER_AF_GENERATE_CLI

#ifndef EMBER_AF_GENERATE_CLI
static const char * notifyArguments[] = {
  "node ID destination",
  "dest endpoint",
  "payload type",
  "jitter",
  "manuf ID",
  "image ID",
  "version",
  NULL,
};

#define OTA_SERVER_COMMANDS                                                       \
  emberCommandEntryActionWithDetails("notify",                                    \
                                     otaImageNotifyCommand,                       \
                                     "vuuuvvw",                                   \
                                     "Send a notification about a new OTA image", \
                                     notifyArguments),                            \
  emberCommandEntryAction("upgrade", otaSendUpgradeCommand, "vu", ""),            \
  LOAD_FILE_COMMAND                                                               \
  POLICY_COMMANDS                                                                 \
  emberCommandEntryTerminator(),

EmberCommandEntry emberAfPluginOtaServerCommands[] = {
  OTA_SERVER_COMMANDS

  emberCommandEntryTerminator(),
};
#endif // EMBER_AF_GENERATE_CLI

//------------------------------------------------------------------------------
// Forward Declarations

//------------------------------------------------------------------------------
// Functions

// plugin ota-server notify <destination> <dst endpoint> <payload type>
//   <queryJitter> <manufacturer-id> <image-type-id> <firmware-version>
// Payload Type:
//   0 = jitter value only
//   1 = jitter and manufacuter id
//   2 = jitter, manufacuter id, and device id
//   3 = jitter, manufacuter id, device id, and firmware version
//
// The CLI requires all parameters, but if the payload type does not
// require that parameter, simply set it to 0.
void otaImageNotifyCommand(void)
{
  EmberAfOtaImageId id;
  EmberNodeId dest = (EmberNodeId)emberUnsignedCommandArgument(0);
  uint8_t endpoint = (uint8_t)emberUnsignedCommandArgument(1);
  uint8_t payloadType = (uint8_t)emberUnsignedCommandArgument(2);
  uint8_t jitter = (uint8_t)emberUnsignedCommandArgument(3);
  id.manufacturerId = (uint16_t)emberUnsignedCommandArgument(4);
  id.imageTypeId = (uint16_t)emberUnsignedCommandArgument(5);
  id.firmwareVersion = emberUnsignedCommandArgument(6);

  emberAfOtaServerSendImageNotifyCallback(dest,
                                          endpoint,
                                          payloadType,
                                          jitter,
                                          &id);
}

#if defined(EMBER_AF_PLUGIN_OTA_SERVER_POLICY)
void setPolicy(void)
{
  if (emberCurrentCommand->name[0] == 'i') {
    uint16_t minBlockPeriodMs = (uint16_t)emberUnsignedCommandArgument(0);
    emAfOtaServerPolicySetMinBlockRequestPeriod(minBlockPeriodMs);
  } else {
    uint8_t value = (uint8_t)emberUnsignedCommandArgument(0);
    if (emberCurrentCommand->name[0] == 'q') {
      emAfOtaServerSetQueryPolicy(value);
    } else if (emberCurrentCommand->name[0] == 'b') {
      emAfOtaServerSetBlockRequestPolicy(value);
    } else if (emberCurrentCommand->name[0] == 'u') {
      emAfOtaServerSetUpgradePolicy(value);
    } else if (emberCurrentCommand->name[0] == 'p') {
      if (emberCurrentCommand->name[9] == 'm') {
        emAfSetPageRequestMissedBlockModulus(value);
      } else if (emberCurrentCommand->name[9] == 's') {
        emAfOtaServerSetPageRequestPolicy(value);
      }
    }
  }
}
#endif // EMBER_AF_PLUGIN_OTA_SERVER_POLICY

// This should be deadstripped out if EMBER_TEST is undefined
#ifdef EMBER_TEST
void otaServerSetClientDelayUnits(void)
{
  #ifdef EMBER_AF_PLUGIN_OTA_SERVER_POLICY
  extern uint8_t testClientDelayUnit;
  testClientDelayUnit = (uint8_t)emberUnsignedCommandArgument(0);
  #endif // EMBER_AF_PLUGIN_OTA_SERVER_POLICY
}
#endif

void otaSendUpgradeCommand(void)
{
  EmberNodeId dest = (EmberNodeId)emberUnsignedCommandArgument(0);
  uint8_t endpoint = (uint8_t)emberUnsignedCommandArgument(1);
  EmberAfOtaImageId id;
  id.manufacturerId = (uint16_t)emberUnsignedCommandArgument(2);
  id.imageTypeId = (uint16_t)emberUnsignedCommandArgument(3);
  id.firmwareVersion = (uint32_t)emberUnsignedCommandArgument(4);

  // Use 0xFFFF for all three arguments if you want wildcard
  // The wild cards tell any device to upgrade, regardless of manufacturer ID,
  // image type, or version

  emberAfOtaServerSendUpgradeCommandCallback(dest, endpoint, &id);
}

#endif // (EMBER_AF_PLUGIN_OTA_SERVER)
#endif//UC_BUILD
