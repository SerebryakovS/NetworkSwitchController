
#include "Controller.h"
#include <gpiod.h>

struct gpiod_chip *RelayChips[RELAY_COUNT];
struct gpiod_line *RelayLines[RELAY_COUNT];
struct gpiod_chip *InputChips[INPUT_COUNT];
struct gpiod_line *InputLines[INPUT_COUNT];
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
        fprintf(stderr, "Failed to open GPIO chip: %s\n", GpioChipName);
        return -EXIT_FAILURE;
    };
    *Line = gpiod_chip_get_line(*Chip, BankPin);
    if (!*Line) {
        fprintf(stderr, "Failed to get GPIO line %d on chip %s\n", BankPin, GpioChipName);
        gpiod_chip_close(*Chip);
        return -EXIT_FAILURE;
    };
    if (Label && gpiod_line_request_output(*Line, Label, 0) < 0) {
        fprintf(stderr, "Failed to request output for GPIO %d\n", Gpio);
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
            fprintf(stderr, "Failed to request input for GPIO %d\n", inputPins[Idx]);
            gpiod_chip_close(InputChips[Idx]);
            return -EXIT_FAILURE;
        };
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
        fprintf(stderr, "Failed to set relay GPIO\n");
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
////////////////////////////
// REST RELATED FUNCTIONS //
////////////////////////////



// const char* GetInputs(void) {
//     snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "[");
//     for (int i = 0; i < 4; i++) {
//         int input_state = GetInputState(i + 1);
//         if (input_state < 0) {
//             snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to read input state for input %d\"}\n", i + 1);
//             return WebResponseBuffer;
//         }
//
//         snprintf(WebResponseBuffer + strlen(WebResponseBuffer), WEB_RESPONSE_SIZE - strlen(WebResponseBuffer),
//                  "{\"input_num\": %d, \"state\": %d}%s", i + 1, input_state, i < 3 ? ", " : "");
//     }
//     snprintf(WebResponseBuffer + strlen(WebResponseBuffer), WEB_RESPONSE_SIZE - strlen(WebResponseBuffer), "]");
//     return WebResponseBuffer;
// }
//
// // Получение состояния реле
// const char* GetRelays(void) {
//     snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "[");
//     for (int i = 0; i < 4; i++) {
//         int relay_state = gpiod_line_get_value(RelayLines[i]);
//         if (relay_state < 0) {
//             snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to read relay state for relay %d\"}\n", i + 1);
//             return WebResponseBuffer;
//         }
//
//         snprintf(WebResponseBuffer + strlen(WebResponseBuffer), WEB_RESPONSE_SIZE - strlen(WebResponseBuffer),
//                  "{\"relay_num\": %d, \"state\": %d}%s", i + 1, relay_state, i < 3 ? ", " : "");
//     }
//     snprintf(WebResponseBuffer + strlen(WebResponseBuffer), WEB_RESPONSE_SIZE - strlen(WebResponseBuffer), "]");
//     return WebResponseBuffer;
// }
//
// // Переключение реле
// const char* ToggleRelay(int RelayNum) {
//     if (RelayNum < 1 || RelayNum > 4) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Invalid relay number\"}\n");
//         return WebResponseBuffer;
//     }
//
//     int current_state = gpiod_line_get_value(RelayLines[RelayNum - 1]);
//     if (current_state < 0) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to read relay state\"}\n");
//         return WebResponseBuffer;
//     }
//
//     int new_state = !current_state;
//     if (ControlRelay(RelayNum, new_state) < 0) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to toggle relay\"}\n");
//         return WebResponseBuffer;
//     }
//
//     snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"relay_num\": %d, \"new_state\": %d}\n", RelayNum, new_state);
//     return WebResponseBuffer;
// }
//
// // Установка задержки на реле
// const char* SetRelayDelay(int RelayNum, double DelaySec) {
//     if (ControlRelay(RelayNum, 1) < 0) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to set relay state\"}\n");
//         return WebResponseBuffer;
//     }
//
//     sleep(DelaySec);
//
//     if (ControlRelay(RelayNum, 0) < 0) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to reset relay state\"}\n");
//         return WebResponseBuffer;
//     }
//
//     snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"relay_num\": %d, \"delay_sec\": %.2f, \"new_state\": 0}\n", RelayNum, DelaySec);
//     return WebResponseBuffer;
// }
//
// // Установка задержки на вход
// const char* SetInputDelay(int InputNum, double DelaySec) {
//     sleep(DelaySec);
//     int input_state = GetInputState(InputNum);
//     if (input_state < 0) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to read input state after delay\"}\n");
//         return WebResponseBuffer;
//     }
//
//     snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"input_num\": %d, \"delay_sec\": %.2f, \"state\": %d}\n", InputNum, DelaySec, input_state);
//     return WebResponseBuffer;
// }
//
// // Установка состояния реле
// const char* SetRelay(int RelayNum) {
//     if (ControlRelay(RelayNum, 1) < 0) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to set relay state\"}\n");
//         return WebResponseBuffer;
//     }
//
//     snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"relay_num\": %d, \"state\": 1}\n", RelayNum);
//     return WebResponseBuffer;
// }
//
// // Сброс состояния реле
// const char* ResetRelay(int RelayNum) {
//     if (ControlRelay(RelayNum, 0) < 0) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"Failed to reset relay state\"}\n");
//         return WebResponseBuffer;
//     }
//
//     snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"relay_num\": %d, \"state\": 0}\n", RelayNum);
//     return WebResponseBuffer;
// }
