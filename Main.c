
#include "Controller.h"

int main(void){
    InitGPIO();
    int State = 0;
    for (;;){
        ControlRelay(1, State);
        ControlRelay(2, State);
        ControlRelay(3, State);
        ControlRelay(4, State);
        State = State == 1 ? 0 : 1;

        printf("%d | %d %d %d %d | %d %d %d %d |\n", State,
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
    CleanupGPIO();
};
