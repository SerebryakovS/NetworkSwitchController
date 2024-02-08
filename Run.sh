#!/bin/bash
# +------+-----+----------+--------+---+   OPI5   +---+--------+----------+-----+------+
# | GPIO | wPi |   Name   |  Mode  | V | Physical | V |  Mode  | Name     | wPi | GPIO |
# +------+-----+----------+--------+---+----++----+---+--------+----------+-----+------+
# |      |     |     3.3V |        |   |  1 || 2  |   |        | 5V       |     |      |
# |   47 |   0 |    SDA.5 |     IN | 1 |  3 || 4  |   |        | 5V       |     |      |
# |   46 |   1 |    SCL.5 |     IN | 1 |  5 || 6  |   |        | GND      |     |      |
# |   54 |   2 |    PWM15 |     IN | 1 |  7 || 8  | 0 | IN     | RXD.0    | 3   | 131  |
# |      |     |      GND |        |   |  9 || 10 | 0 | IN     | TXD.0    | 4   | 132  |
# |  138 |   5 |  CAN1_RX |     IN | 1 | 11 || 12 | 1 | IN     | CAN2_TX  | 6   | 29   |
# |  139 |   7 |  CAN1_TX |     IN | 1 | 13 || 14 |   |        | GND      |     |      |
# |   28 |   8 |  CAN2_RX |     IN | 1 | 15 || 16 | 1 | IN     | SDA.1    | 9   | 59   |
# |      |     |     3.3V |        |   | 17 || 18 | 1 | IN     | SCL.1    | 10  | 58   |
# |   49 |  11 | SPI4_TXD |     IN | 1 | 19 || 20 |   |        | GND      |     |      |
# |   48 |  12 | SPI4_RXD |     IN | 1 | 21 || 22 | 1 | IN     | GPIO2_D4 | 13  | 92   |
# |   50 |  14 | SPI4_CLK |     IN | 1 | 23 || 24 | 1 | IN     | SPI4_CS1 | 15  | 52   |
# |      |     |      GND |        |   | 25 || 26 | 1 | IN     | PWM1     | 16  | 35   |
# +------+-----+----------+--------+---+----++----+---+--------+----------+-----+------+
# | GPIO | wPi |   Name   |  Mode  | V | Physical | V |  Mode  | Name     | wPi | GPIO |
# +------+-----+----------+--------+---+   OPI5   +---+--------+----------+-----+------+

##############
RELAY_1_PIN=0
RELAY_2_PIN=1
RELAY_3_PIN=2
RELAY_4_PIN=5
#-------------
INPUT_1_PIN=7
INPUT_2_PIN=8
INPUT_3_PIN=11
INPUT_4_PIN=12
##############
TOGGLE_TIME=5
REST_PORT=13222
SCRIPT_PATH=$(realpath "${BASH_SOURCE[0]}")

SetupGPIO() {
    gpio mode $RELAY_1_PIN out
    gpio mode $RELAY_2_PIN out
    gpio mode $RELAY_3_PIN out
    gpio mode $RELAY_4_PIN out
    gpio mode $INPUT_1_PIN in
    gpio mode $INPUT_2_PIN in
    gpio mode $INPUT_3_PIN in
    gpio mode $INPUT_4_PIN in
};

SetOutputPin() {
    gpio write $1 1
};

ResetOutputPin() {
    gpio write $1 0
};

ToggleOutputPin(){
    OutputPin=$1
    gpio write $OutputPin 1
    sleep $2
    gpio write $OutputPin 0
};

ReadInputPin() { # gpio is pulled-up, so logic is inverted
    CurrentInputPinValue=$(gpio read $1);
    if [ "$CurrentInputPinValue" -eq "1" ]; then
        echo 0;
    else
        echo 1;
    fi;
};

ValidateRelayNum() {
    if [ $1 -ge 1 ] && [ $1 -le 4 ]; then
        echo "true"
    else
        echo "false"
    fi;
};

APIRequestsHandler() {
    read -r RequestMethod RequestPath RequestProtocol
    ContentLength=0
    while IFS= read -r Header; do
        [[ $Header == $'\r' ]] && break
        if [[ $Header =~ ^Content-Length: ]]; then
            ContentLength="${Header#*: }"
            ContentLength="${ContentLength//[^0-9]/}"
        fi
    done
    if [[ $ContentLength -gt 0 ]]; then
        IFS= read -r -n "$ContentLength" Body
    fi
    case "$RequestPath" in
        '/api/toggle_relay')
            SelectedRelay=$(echo "$Body" | jq -r '.relay_num')
            RelayPin="RELAY_${SelectedRelay}_PIN"
            if [[ $(ValidateRelayNum $SelectedRelay) == "true" ]]; then
                ToggleOutputPin "${!RelayPin}" "$TOGGLE_TIME"
                echo -e "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\": true}"
            else
                echo -e "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\": false, \"error\": \"Invalid relay number\"}"
            fi
            ;;
        '/api/open_delay')
            Delay=$(echo "$Body" | jq -r '.delay_sec')
            if (( $(echo "$Delay >= 0.5 && $Delay <= 5.0" | bc -l) )); then
                sed -i "s/^TOGGLE_TIME=.*/TOGGLE_TIME=$Delay/" "$SCRIPT_PATH"
                echo -e "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\": true}"
            else
                echo -e "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\": false, \"error\": \"Invalid delay\"}"
            fi
            ;;
        '/api/set_relay')
            SelectedRelay=$(echo "$Body" | jq -r '.relay_num')
            RelayPin="RELAY_${SelectedRelay}_PIN"
            if [[ $(ValidateRelayNum $SelectedRelay) == "true" ]]; then
                SetOutputPin "${!RelayPin}"
                echo -e "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\": true}"
            else
                echo -e "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\": false, \"error\": \"Invalid relay number\"}"
            fi
            ;;
        '/api/reset_relay')
            SelectedRelay=$(echo "$Body" | jq -r '.relay_num')
            RelayPin="RELAY_${SelectedRelay}_PIN"
            if [[ $(ValidateRelayNum $SelectedRelay) == "true" ]]; then
                ResetOutputPin "${!RelayPin}"
                echo -e "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\": true}"
            else
                echo -e "HTTP/1.1 400 OK\r\nContent-Type: application/json\r\n\r\n{\"success\": false, \"error\": \"Invalid relay number\"}"
            fi
            ;;
        '/api/get_inputs')
            InputPinsStatus=$(
                for PinNum in 1 2 3 4; do
                    InputPin="INPUT_${PinNum}_PIN"
                    InputPinState=$(ReadInputPin "${!InputPin}")
                    echo "{\"input_num\": $PinNum, \"state\": $InputPinState}"
                done | jq -s '.'
            )
            echo -e "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"inputs\": $InputPinsStatus}"
            ;;
        *)
            echo -e "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\": \"Not found\"}"
            ;;
    esac;
}


Main(){
    SetupGPIO
    socat TCP-LISTEN:$REST_PORT,fork,reuseaddr SYSTEM:"$SCRIPT_PATH APIRequestsHandler"
};

if [ $# -eq 0 ]; then
    echo "Running web server on port: $REST_PORT"
    Main;
elif [ $1 = "APIRequestsHandler" ];then
    APIRequestsHandler $2;
fi;