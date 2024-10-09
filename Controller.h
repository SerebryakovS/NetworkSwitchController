
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#define REST_PORT         8112
#define WEB_RESPONSE_SIZE  256

extern char WebResponseBuffer[WEB_RESPONSE_SIZE];

#define URI_GET_LOGIN    "/api/get_login"       // DONE
#define URI_GET_INPUTS   "/api/get_inputs"      // DONE
#define URI_GET_RELAYS   "/api/get_relays"
#define URI_TOGGLE_RELAY "/api/toggle_relay"
#define URI_RELAY_DELAY  "/api/relay_delay"
#define URI_INPUT_DELAY  "/api/input_delay"
#define URI_SET_RELAY    "/api/set_relay"
#define URI_RESET_RELAY  "/api/reset_relay"
#define URI_SET_WEBHOOK  "/api/inputs_webhook"

#define GPIO_CHIP_BASE_PATH "/dev/gpiochip"

#define RELAY_COUNT   4
#define RELAY_1_PIN 138
#define RELAY_2_PIN 139
#define RELAY_3_PIN 131
#define RELAY_4_PIN 132
extern struct gpiod_chip *RelayChips[RELAY_COUNT];
extern struct gpiod_line *RelayLines[RELAY_COUNT];

#define INPUT_COUNT   4
#define INPUT_1_PIN  29
#define INPUT_2_PIN  59
#define INPUT_3_PIN  58
#define INPUT_4_PIN  92
extern struct gpiod_chip *InputChips[INPUT_COUNT];
extern struct gpiod_line *InputLines[INPUT_COUNT];

int8_t InitGPIO();
void CleanupGPIO();
uint8_t ControlRelay(uint8_t RelayNum, uint8_t State);
uint8_t GetRelayState(uint8_t RelayNum);
uint8_t GetInputState(uint8_t InputNum);
uint8_t GetGpioChipAndLine(uint8_t Gpio, struct gpiod_chip **Chip, struct gpiod_line **Line, const char *Label);
///////////////////////////////////
int8_t RunWebServer(void);
void StopWebServer(void);

const char* GetInputs(void);
const char* GetRelays(void);
const char* ToggleRelay(uint8_t RelayNum);

#endif // CONTROLLER_H
