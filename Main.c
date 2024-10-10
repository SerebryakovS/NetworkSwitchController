
#include "Controller.h"
#include <signal.h>

void HandleSigint(int Signal) {
    for (uint8_t Idx = 1; Idx <= RELAY_COUNT; Idx++) {
        ControlRelay(Idx, 0);
    };
    StopWebServer();
    CleanupGPIO();
    exit(EXIT_SUCCESS);
};

int main(int Argc, char *Argv[]){
    InitGPIO();
    signal(SIGINT, HandleSigint);
    if (Argc > 1 && strcmp(Argv[1], "--test_io") == 0) {
        int State = 0;
        for (;;){
            ControlRelay(1, State);
            ControlRelay(2, State);
            ControlRelay(3, State);
            ControlRelay(4, State);
            State = State == 1 ? 0 : 1;

            printf("set_state=%d | r1=%d r2=%d r3=%d r4=%d | i1=%d i2=%d i3=%d i4=%d |\n", State,
                GetRelayState(1),
                GetRelayState(2),
                GetRelayState(3),
                GetRelayState(4),

                GetInputState(1),
                GetInputState(2),
                GetInputState(3),
                GetInputState(4)
            );
            sleep(1);
        };
    } else {
        if (LoadConfig() != EXIT_SUCCESS) {
            fprintf(stdout, "[ERR] : Failed to load configuration\n");
            fflush(stdout);
            return -EXIT_FAILURE;
        };
        RunWebServer();
        while (1) {
            MonitorInputsAndTriggerWebhook();
            sleep(0.1);
        };
    };
};
