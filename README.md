# NetworkSwitchController
Controller currenly deployed as systemd service for operation on SBC like OrangePI and also as external hardware solution based on GD32F103.
### Installation
```
sudo apt-get update
sudo apt-get install -y gpiod libgpiod-dev libmicrohttpd-dev libcjson-dev
sudo apt-get install libcurl4-openssl-dev
```
OR, use provided docker-compose.yml(with modifying included .env file):
```
docker-compose up --build
```
In case of missing envs values predefined defaults will be set:
```
#define DEFAULT_REST_PORT  8112
#define DEFAULT_USERNAME   "admin"
#define DEFAULT_PASSWORD   "admin"
```
### Testing requests
```
curl -u admin:admin http://10.66.100.188:8112/api/get_login
curl -X POST -u admin:admin -d "{\"relay_num\":1}" http://10.66.100.188:8112/api/toggle_relay
curl -X POST -u admin:admin -d "{\"relay_num\":1}" http://10.66.100.188:8112/api/set_relay
curl -X POST -u admin:admin -d "{\"relay_num\":1}" http://10.66.100.188:8112/api/reset_relay
curl -X POST -u admin:admin -d "{\"io_number\": 1, \"delay_sec\": 1.5}" http://10.66.100.188:8112/api/relay_delay
curl -X POST -u admin:admin -d "{\"io_number\": 1, \"delay_sec\": 1.5}" http://10.66.100.188:8112/api/input_delay
curl -X POST -u admin:admin -d "{\"webhook_url\": \"http://localhost/webhook\"}" http://10.66.100.188:8112/api/inputs_webhook
curl -X POST -u admin:admin -d "{\"new_pass\": \"new_password\"}" http://10.66.100.188:8112/api/set_password
```





