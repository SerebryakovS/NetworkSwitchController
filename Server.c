
#include "Controller.h"
#include <microhttpd.h>

struct MHD_Daemon *Daemon;

char WebResponseBuffer[WEB_RESPONSE_SIZE] = {};

struct PostRequest {
    char* Data; size_t Size;
};
//
const char* GetLogin() {
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"auth\": true}\n");
    return WebResponseBuffer;
};
//
int ValidateCredentials(struct MHD_Connection *Connection) {
    char *Password = NULL;
    char *Username = MHD_basic_auth_get_username_password(Connection, &Password);
    if (Username == NULL) {
        return -EXIT_FAILURE; // or MHD_HTTP_UNAUTHORIZED
    };

    if (strcmp(Username, Config.Username) == 0 &&
        strcmp(Password, Config.Password) == 0) {
        free(Username);
        free(Password);
        return EXIT_SUCCESS;
    };
    free(Username);
    free(Password);
    return -EXIT_FAILURE;
};
//
static int HandleGetRequest(struct MHD_Connection *Connection, const char* Url) {
    const char* ResponseStr = NULL;
    if (strcmp(Url, URI_GET_LOGIN) == 0) {
        ResponseStr = GetLogin();
    } else
    if (strcmp(Url, URI_GET_INPUTS) == 0) {
        ResponseStr = GetInputs();
    } else
    if (strcmp(Url, URI_GET_RELAYS) == 0) {
        ResponseStr = GetRelays();
    } else {
        ResponseStr = "{\"error\":\"unknown endpoint received\"}\n";
    };
    if (ResponseStr == NULL) {
        ResponseStr = "{\"error\":\"internal server error\"}\n";
    };
    struct MHD_Response *Response = MHD_create_response_from_buffer(strlen(ResponseStr),
                                                                    (void*)ResponseStr,
                                                                    MHD_RESPMEM_MUST_COPY);
    int ReturnValue = MHD_queue_response(Connection, MHD_HTTP_OK, Response);
    MHD_destroy_response(Response);
    return ReturnValue;
};
//
static int HandlePostRequest(struct MHD_Connection *Connection, const char* Url, struct PostRequest *_PostRequest) {
    const char *ResponseStr = NULL;
    cJSON *JsonObject = cJSON_Parse(_PostRequest->Data);
    if (JsonObject == NULL) {
        return MHD_HTTP_BAD_REQUEST;
    };
    if (strcmp(Url, URI_TOGGLE_RELAY) == 0) {
        const cJSON *RelayNum = cJSON_GetObjectItem(JsonObject, "relay_num");
        if (!cJSON_IsNumber(RelayNum)) {
            cJSON_Delete(JsonObject);
            return MHD_HTTP_BAD_REQUEST;
        };
        ResponseStr = ToggleRelay(RelayNum->valueint);
    } else if (strcmp(Url, URI_RELAY_DELAY) == 0) {
        const cJSON *RelayNum = cJSON_GetObjectItem(JsonObject, "io_number");
        const cJSON *DelaySec = cJSON_GetObjectItem(JsonObject, "delay_sec");
        if (!cJSON_IsNumber(RelayNum) || !cJSON_IsNumber(DelaySec)) {
            cJSON_Delete(JsonObject);
            return MHD_HTTP_BAD_REQUEST;
        };
        ResponseStr = SetRelayDelay(RelayNum->valueint, DelaySec->valuedouble);
    } else if (strcmp(Url, URI_INPUT_DELAY) == 0) {
        const cJSON *InputNum = cJSON_GetObjectItem(JsonObject, "io_number");
        const cJSON *DelaySec = cJSON_GetObjectItem(JsonObject, "delay_sec");
        if (!cJSON_IsNumber(InputNum) || !cJSON_IsNumber(DelaySec)) {
            cJSON_Delete(JsonObject);
            return MHD_HTTP_BAD_REQUEST;
        };
        ResponseStr = SetInputDelay(InputNum->valueint, DelaySec->valuedouble);
    } else if (strcmp(Url, URI_SET_RELAY) == 0) {
        const cJSON *RelayNum = cJSON_GetObjectItem(JsonObject, "relay_num");
        if (!cJSON_IsNumber(RelayNum)) {
            cJSON_Delete(JsonObject);
            return MHD_HTTP_BAD_REQUEST;
        };
        ResponseStr = SetRelay(RelayNum->valueint);
    } else if (strcmp(Url, URI_RESET_RELAY) == 0) {
        const cJSON *RelayNum = cJSON_GetObjectItem(JsonObject, "relay_num");
        if (!cJSON_IsNumber(RelayNum)) {
            cJSON_Delete(JsonObject);
            return MHD_HTTP_BAD_REQUEST;
        };
        ResponseStr = ResetRelay(RelayNum->valueint);
    } else if (strcmp(Url, URI_SET_WEBHOOK) == 0) {
        const cJSON *WebhookEndpoint = cJSON_GetObjectItem(JsonObject, "endpoint");
        if (!cJSON_IsString(WebhookEndpoint)) {
            cJSON_Delete(JsonObject);
            return MHD_HTTP_BAD_REQUEST;
        };
        ResponseStr = SetWebhook(WebhookEndpoint->valuestring);
    } else {
        ResponseStr = "{\"error\":\"unknown endpoint received\"}\n";
    };
    if (ResponseStr == NULL) {
        ResponseStr = "{\"error\":\"internal server error\"}\n";
    };
    struct MHD_Response *Response = MHD_create_response_from_buffer(strlen(ResponseStr),
                                                                    (void *)ResponseStr, MHD_RESPMEM_MUST_COPY);
    int ReturnValue = MHD_queue_response(Connection, MHD_HTTP_OK, Response);
    MHD_destroy_response(Response);
    cJSON_Delete(JsonObject);
    return ReturnValue;
};

//
static void RequestCompleted(void *Cls, struct MHD_Connection *Connection,
                              void **ConCls, enum MHD_RequestTerminationCode Toe){
    struct PostRequest *_PostRequest = *ConCls;
    if (_PostRequest != NULL) {
        if (_PostRequest->Data != NULL){
            free(_PostRequest->Data);
        };
        free(_PostRequest);
    };
    *ConCls = NULL;
};
//
static enum MHD_Result AnswerToWebRequest(void *Cls, struct MHD_Connection *Connection,
                                          const char *Url, const char *Method,
                                          const char *Version, const char *UploadData,
                                          uint64_t *UploadDataSize, void **ConCls) {
    if (ValidateCredentials(Connection) != EXIT_SUCCESS) {
        char UnauthorizedResponse[] = "{\"auth\": false}\n";
        struct MHD_Response *Response = MHD_create_response_from_buffer(strlen(UnauthorizedResponse),
                                                                        (void*)UnauthorizedResponse, MHD_RESPMEM_MUST_COPY);
        return MHD_queue_basic_auth_fail_response(Connection, "", Response);
    };
    if (strcmp(Method, "POST") == 0) {
        if (*ConCls == NULL) {
            struct PostRequest *_PostRequest = malloc(sizeof(struct PostRequest));
            if (_PostRequest == NULL){
                return MHD_NO;
            };
            _PostRequest->Data = NULL;
            _PostRequest->Size = 0;
            *ConCls = _PostRequest;
            return MHD_YES;
        };
        struct PostRequest *_PostRequest = *ConCls;
        if (*UploadDataSize != 0) {
            _PostRequest->Data = realloc(_PostRequest->Data, _PostRequest->Size + *UploadDataSize + 1);
            if (_PostRequest->Data == NULL)
                return MHD_NO;
            memcpy(_PostRequest->Data + _PostRequest->Size, UploadData, *UploadDataSize);
            _PostRequest->Size += *UploadDataSize;
            _PostRequest->Data[_PostRequest->Size] = '\0';
            *UploadDataSize = 0;
            return MHD_YES;
        } else if (_PostRequest->Data) {
            return HandlePostRequest(Connection, Url, _PostRequest);
        };
    } else if (strcmp(Method, "GET") == 0) {
        return HandleGetRequest(Connection, Url);
    };
    return MHD_NO;
};
//
int8_t RunWebServer(void){
        Daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, Config.RestPort,
                                  NULL, NULL, &AnswerToWebRequest, NULL,
                                  MHD_OPTION_NOTIFY_COMPLETED, RequestCompleted, NULL,
                                  MHD_OPTION_END);
        if (NULL == Daemon){
            printf("[ERR]: Could not start web-server...\n");
            return -EXIT_FAILURE;
        };
        printf("[OK]: Web-server is running on port: %d\n", Config.RestPort);
        return EXIT_SUCCESS;
};
//
void StopWebServer(void){
    MHD_stop_daemon(Daemon);
    printf("[OK]: Web-server stopped\n");
};
