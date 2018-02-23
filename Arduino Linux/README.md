# Habitat Hub Arduino Linux Side Code

This is the middle ware that stores configuration data from the ARV, receives sensor data and pushes it to the IoT Shadow.

habhubconfig.json - JSON file containing configuration setting and last clean water and cage day.
habhubconfig.json.bak - Backup copy.
habhubgate.py - Python middle ware program that needs to be ran automatically when system boots.
run.sh - Used to run habhubgate from the command line.  I was having trouble getting it to work.
         I used it to copy and paste the command on the command line.
certs - Folder where certificates are stored.
    -> aws_iot_root_CA.crt - aws root CA.
    -> your_thing-certificate.pem.crt - you will get this after you have created your IoT.
    -> your_thing-private.pem.key - you will get this after you have created your IoT.

To run habhubgate.py on startup, you must install the coreutils-nohup package.

## Instructions:
```
    opkg update
    opkg install coreutils-nohup
```

Add the following line to /etc/rc.local
```
/usr/bin/nohup /your/directory/name/habhubgate.py -e yourIOTendpoint.iot.us-east-1.amazonaws.com \
             -r /your/directory/name/certs/aws_iot_root_CA.crt \
             -c /your/directory/name/certs/your_thing-certificate.pem.crt \
             -k /your/directory/name/certs/your_thing-private.pem.key \
             -n habhub_1234 \
             -id yourClientID
```

**NOTE**: Some notes to help you have an easier time.
1. yourIOTendpoint can be found on the IOT AWS service page under Settings -> Custom Endpoint.
2. yourClientID can be anything but it has to be unique.