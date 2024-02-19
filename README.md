# NetworkSwitchController
Controller currenly deployed as systemd service for operation on SBC like OrangePI and also as external hardware solution based on STM32.

### Software installation (systemd option)
```
# apt-get install socat jq
# sudo wget -qO /usr/local/bin/websocat https://github.com/vi/websocat/releases/latest/download/websocat.aarch64-unknown-linux-musl
# sudo chmod a+x /usr/local/bin/websocat
# websocat --version
# git clone https://github.com/SerebryakovS/NetworkSwitchController.git
# cd NetworkSwitchController
# bash Install.sh
```

### Troubleshooting
```
# /sbin/ifconfig
# systemctl status nsc
# systemctl restart nsc
```
### General API calls
Following API calls works for both controller implementations

#### POST /api/toggle_relay
request body:
```
{
    "relay_num" : ... // 1,2,3 or 4
}
```
#### POST /api/open_delay
request body:
```
{
    "delay_sec" : ... // from 0.5 to 5.0 
}
```
#### POST /api/set_relay
request body:
```
{
    "relay_num" : ... // 1,2,3 or 4
}
```
#### POST /api/reset_relay
request body:
```
{
    "relay_num" : ... // 1,2,3 or 4
}
```
#### GET /api/get_inputs
```
{
    "inputs" : [
        {
            "input_num" : 1,
            "state" : true // opened
        },
        ...
    ]
}
```
response body for all above requests:
```
{
    "success" : true | false,
}
```
### Systemd Controller specific feature
In order to prevent a huge amount of client requests for just checking input values, additional Web-socket server works on port 13223. It's used for read-only.
Each time one of input pins changes it's value client would be informed over ws connection.
```
$ websocat ws://IP:WS_PORT --ping-interval=1 -v
    INFO  websocat::lints Auto-inserting the line mode
    INFO  websocat::stdio_threaded_peer get_stdio_peer (threaded)
    INFO  websocat::ws_client_peer get_ws_client_peer
    INFO  websocat::ws_client_peer Connected to ws
    INFO  websocat::ws_peer Sending WebSocket ping
    INFO  websocat::ws_peer Received a pong from websocket; RTT = 780.778µs
    INFO  websocat::ws_peer Sending WebSocket ping
    INFO  websocat::ws_peer Received a pong from websocket; RTT = 834.153µs
    INFO  websocat::ws_peer Sending WebSocket ping
    INFO  websocat::ws_peer Received a pong from websocket; RTT = 801.779µs
    INFO  websocat::ws_peer Sending WebSocket ping
    INFO  websocat::ws_peer Received a pong from websocket; RTT = 779.32µs
```
### STM32 Controller specific requests

#### POST /api/upload_firmware