
#include "Controller.h"
#include <gpiod.h>
#include <pthread.h>
#include <curl/curl.h>

struct gpiod_chip *RelayChips[RELAY_COUNT];
struct gpiod_line *RelayLines[RELAY_COUNT];
struct gpiod_chip *InputChips[INPUT_COUNT];
struct gpiod_line *InputLines[INPUT_COUNT];

typedef struct {
    uint8_t    CurrentState;
    uint32_t LastChangeTime;
} InputCache;

InputCache InputStates[INPUT_COUNT];

uint32_t GetTimeMs() {
    struct timeval Time;
    gettimeofday(&Time, NULL);
    return (Time.tv_sec * 1000) + (Time.tv_usec / 1000);
};

////////////////////////////
// GPIO RELATED FUNCTIONS //
////////////////////////////
uint8_t GetGpioChipAndLine(uint8_t Gpio, struct gpiod_chip **Chip, struct gpiod_line **Line, const char *Label) {
    uint8_t GpioBank = Gpio / 32;
    uint8_t BankPin = Gpio % 32;
    char GpioChipName[20];
    snprintf(GpioChipName, sizeof(GpioChipName), "%s%d", GPIO_CHIP_BASE_PATH, GpioBank);
    *Chip = gpiod_chip_open(GpioChipName);
    if (!*Chip) {
        fprintf(stderr, "[ERR]: Failed to open GPIO chip: %s\n", GpioChipName);
        return -EXIT_FAILURE;
    };
    *Line = gpiod_chip_get_line(*Chip, BankPin);
    if (!*Line) {
        fprintf(stderr, "[ERR]: Failed to get GPIO line %d on chip %s\n", BankPin, GpioChipName);
        gpiod_chip_close(*Chip);
        return -EXIT_FAILURE;
    };
    if (Label && gpiod_line_request_output(*Line, Label, 0) < 0) {
        fprintf(stderr, "[ERR]: Failed to request output for GPIO %d\n", Gpio);
        gpiod_chip_close(*Chip);
        return -EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
};
int8_t InitGPIO() {
    uint8_t relayPins[] = {RELAY_1_PIN, RELAY_2_PIN, RELAY_3_PIN, RELAY_4_PIN};
    uint8_t inputPins[] = {INPUT_1_PIN, INPUT_2_PIN, INPUT_3_PIN, INPUT_4_PIN};
    for (uint8_t Idx = 0; Idx < RELAY_COUNT; Idx++) {
        if (GetGpioChipAndLine(relayPins[Idx], &RelayChips[Idx], &RelayLines[Idx], "relay") != 0) {
            return -EXIT_FAILURE;
        };
    };
    for (uint8_t Idx = 0; Idx < INPUT_COUNT; Idx++) {
        if (GetGpioChipAndLine(inputPins[Idx], &InputChips[Idx], &InputLines[Idx], NULL) != 0) {
            return -EXIT_FAILURE;
        };
        if (gpiod_line_request_input(InputLines[Idx], "input") < 0) {
            fprintf(stderr, "[ERR]: Failed to request input for GPIO %d\n", inputPins[Idx]);
            gpiod_chip_close(InputChips[Idx]);
            return -EXIT_FAILURE;
        };
        InputStates[Idx].CurrentState = GetInputState(Idx + 1);
        InputStates[Idx].LastChangeTime = time(NULL);
    };
    return EXIT_SUCCESS;
};
void CleanupGPIO() {
    for (uint8_t Idx = 0; Idx < RELAY_COUNT; Idx++) {
        if (RelayChips[Idx]) {
            gpiod_chip_close(RelayChips[Idx]);
            RelayChips[Idx] = NULL;
        };
    };
    for (uint8_t Idx = 0; Idx < INPUT_COUNT; Idx++) {
        if (InputChips[Idx]) {
            gpiod_chip_close(InputChips[Idx]);
            InputChips[Idx] = NULL;
        };
    };
};
uint8_t ControlRelay(uint8_t RelayNum, uint8_t State) {
    if (RelayNum < 1 || RelayNum > RELAY_COUNT) {
        return -EXIT_FAILURE;
    };
    struct gpiod_line *Line = RelayLines[RelayNum - 1];
    if (!Line) {
        return -EXIT_FAILURE;
    };
    if (gpiod_line_set_value(Line, State) < 0) {
        fprintf(stderr, "[ERR]: Failed to set relay GPIO\n");
        return -EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
};
uint8_t GetRelayState(uint8_t RelayNum) {
    if (RelayNum < 1 || RelayNum > RELAY_COUNT) {
        return -EXIT_FAILURE;
    };
    struct gpiod_line *Line = RelayLines[RelayNum - 1];
    if (!Line) {
        return -EXIT_FAILURE;
    };
    return gpiod_line_get_value(Line);
};
uint8_t GetInputState(uint8_t InputNum) {
    if (InputNum < 1 || InputNum > INPUT_COUNT) {
        return -EXIT_FAILURE;
    };
    struct gpiod_line *Line = InputLines[InputNum - 1];
    if (!Line) {
        return -EXIT_FAILURE;
    };
    return gpiod_line_get_value(Line);
};
uint8_t GetDebouncedInputState(uint8_t InputNum) {
    uint8_t NewState = GetInputState(InputNum);
    uint32_t CurrentTime = GetTimeMs();
    InputCache *Input = &InputStates[InputNum - 1];
    if (NewState != Input->CurrentState) {
        if (CurrentTime - Input->LastChangeTime >= Config.InputDelay[InputNum - 1]) {
            Input->LastChangeTime = CurrentTime;
            Input->CurrentState = NewState;
            return true;
        };
    };
    return false;
};

////////////////////////////
// REST RELATED FUNCTIONS //
////////////////////////////

const char* GetInputs() {
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"inputs\": [");
    for (uint8_t Idx = 0; Idx < INPUT_COUNT; Idx++) {
        uint8_t PinState = GetInputState(Idx + 1);
        snprintf(WebResponseBuffer + strlen(WebResponseBuffer), WEB_RESPONSE_SIZE - strlen(WebResponseBuffer),
                 "{\"input_num\": %d, \"state\": %s}%s", Idx + 1, PinState ? "false" : "true", Idx < INPUT_COUNT - 1 ? ", " : "");
    };
    snprintf(WebResponseBuffer + strlen(WebResponseBuffer), WEB_RESPONSE_SIZE - strlen(WebResponseBuffer), "]}\n");
    return WebResponseBuffer;
};
const char* GetRelays() {
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"relays\": [");
    for (uint8_t Idx = 0; Idx < RELAY_COUNT; Idx++) {
        uint8_t RelayState = GetRelayState(Idx + 1);
        snprintf(WebResponseBuffer + strlen(WebResponseBuffer), WEB_RESPONSE_SIZE - strlen(WebResponseBuffer),
                 "{\"relay_num\": %d, \"state\": %s}%s", Idx + 1, RelayState ? "true" : "false", Idx < RELAY_COUNT - 1 ? ", " : "");
    };
    snprintf(WebResponseBuffer + strlen(WebResponseBuffer), WEB_RESPONSE_SIZE - strlen(WebResponseBuffer), "]}\n");
    return WebResponseBuffer;
};
//
const char* SetRelay(uint8_t RelayNum) {
    if (ControlRelay(RelayNum, 1) < 0) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Set operation failed\"}\n");
    } else {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"success\": true}\n");
    };
    return WebResponseBuffer;
};
const char* ResetRelay(uint8_t RelayNum) {
    if (ControlRelay(RelayNum, 0) < 0) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Reset operation failed\"}\n");
    } else {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"success\": true}\n");
    };
    return WebResponseBuffer;
};

void* ToggleRelayThread(void* _RelayNum) {
    uint8_t RelayNum = *((uint8_t *)_RelayNum);
    free(_RelayNum);
    if (ControlRelay(RelayNum, 1) < 0) {
        fprintf(stderr, "[ERR]: Failed to set relay %d to HIGH\n", RelayNum);
        return NULL;
    };
    usleep((uint32_t)Config.RelayDelay[RelayNum-1]*1000);
    if (ControlRelay(RelayNum, 0) < 0) {
        fprintf(stderr, "[ERR]: Failed to set relay %d to LOW\n", RelayNum);
    };
    return NULL;
};
const char* ToggleRelay(uint8_t RelayNum) {
    if (RelayNum < 1 || RelayNum > RELAY_COUNT) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Invalid relay number\"}\n");
        return WebResponseBuffer;
    };
    int* RelayNumPtr = (int*)malloc(sizeof(int));
    if (!RelayNumPtr) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Memory allocation failed\"}\n");
        return WebResponseBuffer;
    };
    *RelayNumPtr = RelayNum;
    pthread_t ToggleThread;
    if (pthread_create(&ToggleThread, NULL, ToggleRelayThread, (void*)RelayNumPtr) != 0) {
        free(RelayNumPtr);
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to create thread\"}\n");
        return WebResponseBuffer;
    };
    pthread_detach(ToggleThread);
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"success\": true}\n");
    return WebResponseBuffer;
};
//
const char* SetRelayDelay(uint8_t RelayNum, float DelaySec) {
    if (RelayNum < 1 || RelayNum > RELAY_COUNT) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Invalid relay number\"}\n");
        return WebResponseBuffer;
    };
    if (DelaySec < (float)DEFAULT_RELAY_DELAY/1000.0) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Invalid delay value\"}\n");
        return WebResponseBuffer;
    };
    Config.RelayDelay[RelayNum - 1] = (uint16_t)(DelaySec * 1000);
    if (SaveConfig() != EXIT_SUCCESS) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to save configuration\"}\n");
        return WebResponseBuffer;
    };
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"success\": true}\n");
    return WebResponseBuffer;
};

const char* SetInputDelay(uint8_t InputNum, float DelaySec) {
    if (InputNum < 1 || InputNum > INPUT_COUNT) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Invalid input number\"}\n");
        return WebResponseBuffer;
    };
    if (DelaySec < (float)DEFAULT_INPUT_DELAY/1000.0) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Invalid delay value\"}\n");
        return WebResponseBuffer;
    };
    Config.InputDelay[InputNum - 1] = (uint16_t)(DelaySec * 1000);
    if (SaveConfig() != EXIT_SUCCESS) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to save configuration\"}\n");
        return WebResponseBuffer;
    };
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"success\": true}\n");
    return WebResponseBuffer;
};
//
const char* SetWebhook(const char *WebhookUrl) {
    if (WebhookUrl == NULL || strlen(WebhookUrl) >= sizeof(Config.WebhookUrl)) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Invalid webhook URL\"}\n");
        return WebResponseBuffer;
    };
    strcpy(Config.WebhookUrl, WebhookUrl);
    if (SaveConfig() != EXIT_SUCCESS) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to save configuration\"}\n");
        return WebResponseBuffer;
    };
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"success\": true}\n");
    return WebResponseBuffer;
};
void MonitorInputsAndTriggerWebhook() {
    uint8_t StateChanged = 0;
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"inputs\": [");
    for (uint8_t Idx = 0; Idx < INPUT_COUNT; Idx++) {


        if (GetDebouncedInputState(Idx + 1)) {
        // uint8_t PinState = GetDebouncedInputState(Idx + 1);
        // if (PinState != InputStates[Idx].CurrentState) {
            StateChanged = 1;
            // InputStates[Idx].CurrentState = PinState;
            snprintf(WebResponseBuffer + strlen(WebResponseBuffer),
                     WEB_RESPONSE_SIZE - strlen(WebResponseBuffer), "{\"input_num\": %d, \"state\": %s}%s",
                     Idx + 1, InputStates[Idx].CurrentState ? "true" : "false", Idx < INPUT_COUNT - 1 ? ", " : "");
        };
    };
    if (StateChanged && strlen(Config.WebhookUrl) > 0) {
        if (strlen(Config.WebhookUrl) > 0) {
            fprintf(stdout,"[OK]: Input value has been changed. Sending webhook to: %s\n",Config.WebhookUrl);
            fflush(stdout);
            CURL *CurlObject = curl_easy_init();
            if (CurlObject) {
                curl_easy_setopt(CurlObject, CURLOPT_URL, Config.WebhookUrl);
                curl_easy_setopt(CurlObject, CURLOPT_POST, 1L);
                curl_easy_setopt(CurlObject, CURLOPT_POSTFIELDS, WebResponseBuffer);
                curl_easy_setopt(CurlObject, CURLOPT_POSTFIELDSIZE, (long)strlen(WebResponseBuffer));
                CURLcode res = curl_easy_perform(CurlObject);
                if (res != CURLE_OK) {
                    fprintf(stderr, "[ERR]: Failed to send webhook: %s\n", curl_easy_strerror(res));
                };
                curl_easy_cleanup(CurlObject);
            };
        };
    };
};
//
const char* SetPassword(const char* NewPassword) {
    if (NewPassword == NULL || strlen(NewPassword) >= sizeof(Config.Password)) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Invalid password length\"}\n");
        return WebResponseBuffer;
    };
    strcpy(Config.Password, NewPassword);
    if (SaveConfig() != EXIT_SUCCESS) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to save configuration\"}\n");
        return WebResponseBuffer;
    };
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"success\": true}\n");
    return WebResponseBuffer;
};

