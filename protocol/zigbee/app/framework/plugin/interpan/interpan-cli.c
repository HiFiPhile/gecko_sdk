/***************************************************************************//**
 * @file
 * @brief CLI for the Interpan plugin.
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

#include "app/framework/include/af.h"

#include "interpan.h"
#include "app/util/serial/sl_zigbee_command_interpreter.h"

#ifndef EMBER_AF_GENERATE_CLI
  #error The Interpan plugin is not compatible with the legacy CLI.
#endif

#ifdef UC_BUILD
// Prototypes

void emAfInterpanEnableCommand(sl_cli_command_arg_t *args);
void emAfInterpanDisableCommand(sl_cli_command_arg_t *args);
void emAfInterpanFragmentTestCommand(sl_cli_command_arg_t *args);
void emAfInterpanSetMessageTimeoutCommand(sl_cli_command_arg_t *args);

// Global variables

// This is large. It may go away or be refactored for future releases.
uint8_t testMessage[EMBER_AF_PLUGIN_INTERPAN_FRAGMENTATION_MAX_PAYLOAD_SIZE];

// Functions

// Globally enable inter-PAN messages
void emAfInterpanEnableCommand(sl_cli_command_arg_t *args)
{
  emAfPluginInterpanSetEnableState(true);
}

// Globally disable inter-PAN messages
void emAfInterpanDisableCommand(sl_cli_command_arg_t *args)
{
  emAfPluginInterpanSetEnableState(false);
}

// Test an inter-PAN fragment transmission with a randomly-filled payload
void emAfInterpanFragmentTestCommand(sl_cli_command_arg_t *args)
{
  EmberPanId panId = sl_cli_get_argument_uint16(args, 0);
  EmberEUI64 eui64;
  sl_zigbee_copy_eui64_arg(args, 1, eui64, true);
  uint16_t clusterId = sl_cli_get_argument_uint16(args, 2);
  uint16_t messageLen = sl_cli_get_argument_uint16(args, 3);

  messageLen = (messageLen > sizeof(testMessage))
               ? sizeof(testMessage) : messageLen;

  uint16_t i;
  for (i = 0; i < messageLen; i++) {
    testMessage[i] = (emberGetPseudoRandomNumber() & 0xFF);
  }

  emberAfCorePrint("Sending inter-PAN message of len %d to ", messageLen);
  //emberAfCoreDebugExec(emberAfPrintBigEndianEui64(eui64));
  emberAfCorePrint(" with random values: ");

  EmberStatus status = emberAfSendInterPan(panId,
                                           eui64,
                                           EMBER_NULL_NODE_ID,
                                           0,             // mcast id - unused
                                           clusterId,
                                           SE_PROFILE_ID, // GBCS only
                                           messageLen,
                                           testMessage);

  emberAfCorePrintln("%s (0x%X)",
                     (EMBER_SUCCESS == status) ? "success" : "failure",
                     status);
  emberAfCoreFlush();
}

// Set the timeout for inter-PAN fragment transmissions
void emAfInterpanSetMessageTimeoutCommand(sl_cli_command_arg_t *args)
{
#if defined(ALLOW_FRAGMENTATION)
  uint16_t timeout = sl_cli_get_argument_uint16(args, 0);
  if (0 == timeout) {
    emberAfCorePrintln("ERR: inter-PAN message timeout of 0 not allowed");
    emberAfCoreFlush();
    return;
  }
  interpanPluginSetFragmentMessageTimeout(timeout);
#else
  emberAfCorePrintln("ERR: inter-PAN fragmentation feature missing");
#endif //ALLOW_FRAGMENTATION
}

#else // UC_BUILD
// Prototypes

void emAfInterpanEnableCommand(void);
void emAfInterpanDisableCommand(void);
void emAfInterpanFragmentTestCommand(void);
void emAfInterpanSetMessageTimeoutCommand(void);

// Global variables

// This is large. It may go away or be refactored for future releases.
uint8_t testMessage[EMBER_AF_PLUGIN_INTERPAN_FRAGMENTATION_MAX_PAYLOAD_SIZE];

// Functions

// Globally enable inter-PAN messages
void emAfInterpanEnableCommand(void)
{
  emAfPluginInterpanSetEnableState(true);
}

// Globally disable inter-PAN messages
void emAfInterpanDisableCommand(void)
{
  emAfPluginInterpanSetEnableState(false);
}

// Test an inter-PAN fragment transmission with a randomly-filled payload
void emAfInterpanFragmentTestCommand(void)
{
  EmberPanId panId = (EmberPanId)emberUnsignedCommandArgument(0);
  EmberEUI64 eui64;
  emberCopyBigEndianEui64Argument(1, eui64);
  uint16_t clusterId = (uint16_t)emberUnsignedCommandArgument(2);
  uint16_t messageLen = (uint16_t)emberUnsignedCommandArgument(3);

  messageLen = (messageLen > sizeof(testMessage))
               ? sizeof(testMessage) : messageLen;

  uint16_t i;
  for (i = 0; i < messageLen; i++) {
    testMessage[i] = (emberGetPseudoRandomNumber() & 0xFF);
  }

  emberAfCorePrint("Sending inter-PAN message of len %d to ", messageLen);
  emberAfCoreDebugExec(emberAfPrintBigEndianEui64(eui64));
  emberAfCorePrint(" with random values: ");

  EmberStatus status = emberAfSendInterPan(panId,
                                           eui64,
                                           EMBER_NULL_NODE_ID,
                                           0,             // mcast id - unused
                                           clusterId,
                                           SE_PROFILE_ID, // GBCS only
                                           messageLen,
                                           testMessage);

  emberAfCorePrintln("%s (0x%X)",
                     (EMBER_SUCCESS == status) ? "success" : "failure",
                     status);
  emberAfCoreFlush();
}

// Set the timeout for inter-PAN fragment transmissions
void emAfInterpanSetMessageTimeoutCommand(void)
{
#if defined(ALLOW_FRAGMENTATION)
  uint16_t timeout = (uint16_t)emberUnsignedCommandArgument(0);
  if (0 == timeout) {
    emberAfCorePrintln("ERR: inter-PAN message timeout of 0 not allowed");
    emberAfCoreFlush();
    return;
  }
  interpanPluginSetFragmentMessageTimeout(timeout);
#else
  emberAfCorePrintln("ERR: inter-PAN fragmentation feature missing");
#endif //ALLOW_FRAGMENTATION
}
#endif
