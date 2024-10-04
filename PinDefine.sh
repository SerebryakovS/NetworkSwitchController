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
RELAY_1_PIN=5
RELAY_2_PIN=7
RELAY_3_PIN=3
RELAY_4_PIN=4
#-------------
INPUT_1_PIN=6
INPUT_2_PIN=9
INPUT_3_PIN=10
INPUT_4_PIN=13
##############
TOGGLE_TIME=5

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

ReadAllInputsJson() {
    echo $(
        for PinNum in 1 2 3 4; do
            InputPin="INPUT_${PinNum}_PIN"
            InputPinState=$(ReadInputPin "${!InputPin}")
            echo "{\"input_num\": $PinNum, \"state\": $InputPinState}"
        done | jq -s '.'
    );
};
