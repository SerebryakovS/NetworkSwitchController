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





sudo apt-get update
sudo apt-get install -y gpiod libgpiod-dev libmicrohttpd-dev libcjson-dev

gcc Main.c GlueHandlers.c Server.c -o out -lgpiod -lmicrohttpd
