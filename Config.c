
#include "Controller.h"

ControllerConfig Config;

int8_t LoadConfig(void) {
    FILE *ConfigFile = fopen("./config.json", "r");
    if (!ConfigFile) {
        for (int Idx = 0; Idx < RELAY_COUNT; Idx++) {
            Config.RelayDelay[Idx] = DEFAULT_RELAY_DELAY;
        };
        for (int Idx = 0; Idx < INPUT_COUNT; Idx++) {
            Config.InputDelay[Idx] = DEFAULT_INPUT_DELAY;
        };
        strcpy(Config.WebhookUrl, "");
        strcpy(Config.Username, DEFAULT_USERNAME);
        strcpy(Config.Password, DEFAULT_PASSWORD);
        Config.RestPort = DEFAULT_REST_PORT;
        SaveConfig();
        return EXIT_SUCCESS;
    };
    char ConfigBuffer[1024];
    fread(ConfigBuffer, 1, sizeof(ConfigBuffer), ConfigFile);
    fclose(ConfigFile);
    cJSON *JsonObject = cJSON_Parse(ConfigBuffer);
    if (!JsonObject) {
        fprintf(stderr, "[ERR]: Failed to parse config JSON\n");
        return -EXIT_FAILURE;
    };
    cJSON *relayDelays = cJSON_GetObjectItem(JsonObject, "RelayDelay");
    for (int Idx = 0; Idx < RELAY_COUNT; Idx++) {
        Config.RelayDelay[Idx] = cJSON_GetArrayItem(relayDelays, Idx)->valueint;
    };
    cJSON *inputDelays = cJSON_GetObjectItem(JsonObject, "InputDelay");
    for (int Idx = 0; Idx < INPUT_COUNT; Idx++) {
        Config.InputDelay[Idx] = cJSON_GetArrayItem(inputDelays, Idx)->valueint;
    };
    cJSON *WebhookUrl = cJSON_GetObjectItem(JsonObject, "WebhookUrl");
    strcpy(Config.WebhookUrl, WebhookUrl->valuestring);
    cJSON *Username = cJSON_GetObjectItem(JsonObject, "Username");
    strcpy(Config.Username, Username->valuestring);
    cJSON *Password = cJSON_GetObjectItem(JsonObject, "Password");
    strcpy(Config.Password, Password->valuestring);
    cJSON *RestPort = cJSON_GetObjectItem(JsonObject, "RestPort");
    Config.RestPort = RestPort->valueint;
    cJSON_Delete(JsonObject);
    return EXIT_SUCCESS;
};

int8_t SaveConfig(void) {
    cJSON *JsonObject = cJSON_CreateObject();
    int RelayDelaysInt[RELAY_COUNT];
    int InputDelaysInt[INPUT_COUNT];
    for (int Idx = 0; Idx < RELAY_COUNT; Idx++) {
        RelayDelaysInt[Idx] = (int)Config.RelayDelay[Idx];
    };
    for (int Idx = 0; Idx < INPUT_COUNT; Idx++) {
        InputDelaysInt[Idx] = (int)Config.InputDelay[Idx];
    };
    cJSON *RelayDelays = cJSON_CreateIntArray(RelayDelaysInt, RELAY_COUNT);
    cJSON_AddItemToObject(JsonObject, "RelayDelay", RelayDelays);
    cJSON *InputDelays = cJSON_CreateIntArray(InputDelaysInt, INPUT_COUNT);
    cJSON_AddItemToObject(JsonObject, "InputDelay", InputDelays);
    cJSON_AddStringToObject(JsonObject, "WebhookUrl", Config.WebhookUrl);
    cJSON_AddStringToObject(JsonObject, "Username", Config.Username);
    cJSON_AddStringToObject(JsonObject, "Password", Config.Password);
    cJSON_AddNumberToObject(JsonObject, "RestPort", Config.RestPort);
    FILE *ConfigFile = fopen("./config.json", "w");
    if (!ConfigFile) {
        fprintf(stderr, "[ERR]: Could not write to config file\n");
        cJSON_Delete(JsonObject);
        return -EXIT_FAILURE;
    };
    char *JsonString = cJSON_Print(JsonObject);
    fwrite(JsonString, 1, strlen(JsonString), ConfigFile);
    fclose(ConfigFile);
    free(JsonString);
    cJSON_Delete(JsonObject);
    return EXIT_SUCCESS;
}
