
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <cjson/cJSON.h>

#define WEB_RESPONSE_SIZE   256
#define DEFAULT_REST_PORT  8112

#define DEFAULT_USERNAME   "admin"
#define DEFAULT_PASSWORD   "admin"


extern char WebResponseBuffer[WEB_RESPONSE_SIZE];

#define URI_GET_LOGIN    "/api/get_login"       // DONE
#define URI_GET_INPUTS   "/api/get_inputs"      // DONE
#define URI_GET_RELAYS   "/api/get_relays"      // DONE
#define URI_SET_RELAY    "/api/set_relay"       // DONE
#define URI_RESET_RELAY  "/api/reset_relay"     // DONE
#define URI_TOGGLE_RELAY "/api/toggle_relay"    // DONE
#define URI_RELAY_DELAY  "/api/relay_delay"     // DONE
#define URI_INPUT_DELAY  "/api/input_delay"     // DONE
#define URI_SET_WEBHOOK  "/api/inputs_webhook"  // DONE
#define URI_SET_PASSWORD "/api/set_password"    // DONE

#define GPIO_CHIP_BASE_PATH "/dev/gpiochip"

#define RELAY_COUNT   4
#define RELAY_1_PIN 138
#define RELAY_2_PIN 139
#define RELAY_3_PIN 131
#define RELAY_4_PIN 132
#define DEFAULT_RELAY_DELAY 200
extern struct gpiod_chip *RelayChips[RELAY_COUNT];
extern struct gpiod_line *RelayLines[RELAY_COUNT];

#define INPUT_COUNT   4
#define INPUT_1_PIN  29
#define INPUT_2_PIN  59
#define INPUT_3_PIN  58
#define INPUT_4_PIN  92
#define DEFAULT_INPUT_DELAY 200
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
const char* SetRelay(uint8_t RelayNum);
const char* ResetRelay(uint8_t RelayNum);
const char* ToggleRelay(uint8_t RelayNum);

const char* SetRelayDelay(uint8_t RelayNum, float DelaySec);
const char* SetInputDelay(uint8_t InputNum, float DelaySec);
const char* SetWebhook(const char *WebhookUrl);

void MonitorInputsAndTriggerWebhook();
///////////////////////////////////

typedef struct {
    uint16_t RelayDelay[RELAY_COUNT];
    uint16_t InputDelay[INPUT_COUNT];
    char WebhookUrl[256];
    char Username[64];
    char Password[64];
    uint32_t RestPort;
} ControllerConfig;

extern ControllerConfig Config;

int8_t LoadConfig(void);
int8_t SaveConfig(void);

#endif // CONTROLLER_H
