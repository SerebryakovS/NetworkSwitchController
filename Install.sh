#!/bin/bash

SCRIPT_PATH=$pwd/Run.sh
SERVICE_FILE="/etc/systemd/system/nsc.service"

cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Network Switch Controller
After=network.target

[Service]
User=root
Group=root
ExecStart=$SCRIPT_PATH
Restart=always

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable nsc
systemctl start nsc