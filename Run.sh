#!/bin/bash

source $(dirname "$0")/PinDefine.sh
SCRIPT_PATH=$(realpath "${BASH_SOURCE[0]}")
REST_PORT=13222
TOGGLE_TIME=5
WS_PORT=13223

RunWebSocketServer() {
    WebSocketServer="websocat --text -s 0.0.0.0:$WS_PORT"
    PreviousState=""
    PoolingRate=1
    $WebSocketServer < <(while true; do
        CurrentState=$(ReadAllInputsJson)
        if [ "$CurrentState" != "$PreviousState" ]; then
            echo $CurrentState
            PreviousState=$CurrentState
        fi;
        sleep $PoolingRate
    done);
}

RunHTTPServer() {
    socat TCP-LISTEN:$REST_PORT,fork,reuseaddr SYSTEM:"$SCRIPT_PATH APIRequestsHandler"
}

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
            InputPinsStatus=$(ReadAllInputsJson)
            echo -e "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"inputs\": $InputPinsStatus}"
            ;;
        *)
            echo -e "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\": \"Not found\"}"
            ;;
    esac;
}


Main(){
    trap 'kill $(jobs -p); exit' EXIT
    SetupGPIO
    RunWebSocketServer &
    RunHTTPServer &
    wait -n
    kill $(jobs -p);
};

if [ $# -eq 0 ]; then
    echo "Running services on ports: $REST_PORT; $WS_PORT"
    Main;
elif [ $1 = "APIRequestsHandler" ];then
    APIRequestsHandler $2;
fi;