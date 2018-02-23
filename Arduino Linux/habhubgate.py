#!/usr/bin/python
from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTShadowClient
import json
# import paho.mqtt.client as paho
import os
import argparse
# import time
import sys
sys.path.insert(0, '/usr/lib/python2.7/bridge/')
from bridgeclient import BridgeClient as bridgeclient


awsShadowClient = None
deviceShadowHandler = None
client = bridgeclient()
mypid = os.getpid()
client_uniq = "arduino_pub_" + str(mypid)
# mqttc = paho.Client(client_uniq)

values = {'hottemp': 0, 'cooltemp': 0, 'ambtemp': 0, 'waterlevel': 0,
          'humidity': 0, 'lastwaterclean': 0, 'lastcageclean': 0}

# Read in command-line parameters
parser = argparse.ArgumentParser()
parser.add_argument("-e", "--endpoint", action="store", required=True,
                    dest="host", help="Your AWS Iot custom endpoint")
parser.add_argument("-r", "--rootCA", action="store", required=True,
                    dest="rootCAPath", help="Root CA file path")
parser.add_argument("-c", "--cert", action="store", dest="certificatePath",
                    help="Certificate file path")
parser.add_argument("-k", "--key", action="store", dest="privateKeyPath",
                    help="Private key file path")
parser.add_argument("-n", "--thingName", action="store", dest="thingName",
                    default="habhub_1234", help="Targetd thing name")
parser.add_argument("-id", "--clientId", action="store", dest="clientId",
                    default="habhubUpdater", help="ClientID")

# Set the args to variables
args = parser.parse_args()
host = args.host
rootCAPath = args.rootCAPath
certificatePath = args.certificatePath
privateKeyPath = args.privateKeyPath
thingName = args.thingName
clientId = args.clientId

# Exit if no certificatePath and no privateKeyPath
if not args.certificatePath or not args.privateKeyPath:
        parser.error("Missing credientials for authenication.")
        exit(2)

#  ShadowCallback Handler for Update
def customShadowCallback_Update(payload, responseStatus, token):
    # payload is a JSON string ready to be parsed using json.loads(...)
    if responseStatus == "timeout":
        print("Update request " + token + " time out!")
    if responseStatus == "accepted":
        payloadDict = json.loads(payload)
        print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
        print("Update request with token: " + token + " accepted!")
        for property, value in payloadDict["state"]["desired"].items():
            print('{}'.format(property), ": ", '{}'.format(value))
        print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
    if responseStatus == "rejected":
        print("Update request " + token + " rejected!")

#  ShadowCallback Handler for Delete
def customShadowCallback_Delete(payload, responseStatus, token):
    if responseStatus == "timeout":
        print("Delete request " + token + " time out!")
    if responseStatus == "accepted":
        print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
        print("Delete request with token: " + token + " accepted!")
        print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n")
    if responseStatus == "rejected":
        print("Delete request " + token + " rejected!")

#  Util function to compare json data
def compare_json_data(source_data_a, source_data_b):

    def compare(data_a, data_b):
        # type: list
        if (type(data_a) is list):
            # is [data_b] a list and of same length as [data_a]?
            if (
                (type(data_b) != list) or
                (len(data_a) != len(data_b))
            ):
                return False

            # iterate over list items
            for list_index, list_item in enumerate(data_a):
                # compare [data_a] list item against [data_b] at index
                if (not compare(list_item, data_b[list_index])):
                    return False

            # list identical
            return True

        # type: dictionary
        if (type(data_a) is dict):
            # is [data_b] a dictionary?
            if (type(data_b) != dict):
                return False

            # iterate over dictionary keys
            for dict_key, dict_value in data_a.items():
                # key exists in [data_b] dictionary, and same value?
                if (
                    (dict_key not in data_b) or
                    (not compare(dict_value, data_b[dict_key]))
                ):
                    return False

            # dictionary identical
            return True

        # simple value - compare both value and type for equality
        return (
            (data_a == data_b) and
            (type(data_a) is type(data_b))
        )

    # compare a to b, then b to a
    return (
        compare(source_data_a, source_data_b) and
        compare(source_data_b, source_data_a)
    )

#  Reads the configuration from the json file
def readConfigFile():
    # print("Reading configuration.")
    try:
        dir_path = os.path.dirname(os.path.realpath(__file__))
        config_file = os.path.join(dir_path, "habhubconfig.json")
        with open(config_file) as json_config:
            config = json.load(json_config)
        return config
    except OSError as ex:
        print("Error while reading file. Error: " % ex)
    return None

# Saves the json object to a json file
def saveConfigFile(config):
    # print("Saving configuration.")
    try:
        dir_path = os.path.dirname(os.path.realpath(__file__))
        config_file = os.path.join(dir_path, "habhubconfig.json")
        with open(config_file, 'w') as json_config:
            config = json.dump(config, json_config)
    except OSError as ex:
        print("Error while saving config file. Error: " % ex)

# Send configuration to Arduino AVR via Brige Client
def sendConfigData(config, lastConfig):
    sentData = 0
    # print("Sending configuration.")
    for i, item in enumerate(config['settings']):
        if (lastConfig is not None and
           lastConfig['settings'][i]['value'] != item['value']):
            client.put(item['hubname'], item['value'])
            sentData += 1
    if lastConfig is None:
        client.put('lastwaterday', config['lastwaterclean'])
        client.put('lastcageday', config['lastcageclean'])
        sentData = 2
    # print("Sent {} data items.".format(sentData))

# Receives configuration changes from Arduino AVR via Brige Client
def getConfigData(config):
    tempValue = client.get('scfg')
    if tempValue == '1':
        print("Saving Config File After Unit Modified.")
        for i, item in enumerate(config['settings']):
            tempValue = client.get(item['hubname'])
            item['value'] = tempValue
        saveConfigFile(config)
    client.put('scfg', '0')

# Retreives
def getSensorData(config, deviceShadowHandler):
    # print("Getting SensorData.")
    try:
        tempValue = client.get('hot')
        # print("Hot Temp: Old={} New= {}".format(values['hottemp'],tempValue))
        if (tempValue is not None and
           values['hottemp'] != tempValue):
            values['hottemp'] = tempValue
            JSONPayload = '{"state":{"desired":{"hottemp":' + str(tempValue) +\
                          '}}}'
            deviceShadowHandler.shadowUpdate(JSONPayload,
                                             customShadowCallback_Update, 5)

        tempValue = client.get('cot')
        # print("Cool Temp: {}".format(tempValue))
        if (tempValue is not None and
           values['cooltemp'] != tempValue):
            values['cooltemp'] = tempValue
            JSONPayload = '{"state":{"desired":{"cooltemp":' +\
                          str(tempValue) +\
                          '}}}'
            deviceShadowHandler.shadowUpdate(JSONPayload,
                                             customShadowCallback_Update, 5)

        tempValue = client.get('amt')
        # print("Ambient Temp: {}".format(tempValue))
        if (tempValue is not None and
           values['ambtemp'] != tempValue):
            values['ambtemp'] = tempValue
            JSONPayload = '{"state":{"desired":{"ambtemp":' + str(tempValue) +\
                          '}}}'
            deviceShadowHandler.shadowUpdate(JSONPayload,
                                             customShadowCallback_Update, 5)

        tempValue = client.get('wal')
        print("Waterlevel: {}".format(tempValue))
        if (tempValue is not None and
           values['waterlevel'] != tempValue):
            values['waterlevel'] = tempValue
            if tempValue is not None:
                JSONPayload = '{"state":{"desired":{"waterlevel":' +\
                    str(tempValue) + '}}}'
                deviceShadowHandler.shadowUpdate(
                    JSONPayload,
                    customShadowCallback_Update, 5)

        tempValue = client.get('hum')
        # print("Humidity: {}".format(tempValue))
        if (tempValue is not None and
           values['humidity'] != tempValue):
            values['humidity'] = tempValue
            JSONPayload = '{"state":{"desired":{"humidity":' +\
                str(tempValue) +\
                '}}}'
            deviceShadowHandler.shadowUpdate(JSONPayload,
                                             customShadowCallback_Update, 5)

        tempValue = client.get('lstwd')
        if (tempValue is not None and
           values['lastwaterclean'] != tempValue):
            values['lastwaterclean'] = tempValue
            config['lastwaterclean'] = tempValue
            saveConfigFile(config)
            print("Last Water Clean: {}".format(tempValue))
            if tempValue is not None:
                JSONPayload = '{"state":{"desired":{"lastwaterclean":' +\
                              str(tempValue) + '}}}'
                deviceShadowHandler.shadowUpdate(
                    JSONPayload,
                    customShadowCallback_Update, 5)

        tempValue = client.get('lstcd')
        if (tempValue is not None and
           values['lastcageclean'] != tempValue):
            values['lastcageclean'] = tempValue
            config['lastcageclean'] = tempValue
            print("Last Cage Clean: {}".format(tempValue))
            saveConfigFile(config)
            if tempValue is not None:
                JSONPayload = '{"state":{"desired":{"lastcageclean":' +\
                              str(tempValue) + '}}}'
                deviceShadowHandler.shadowUpdate(
                    JSONPayload,
                    customShadowCallback_Update, 5)

    except Exception as err:
        print("Erro in getSensorData")
        print(err)

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("Habhub/habhub_1234/#")
    print("Subscribed completed.")


# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.payload))

# Cleanup function
def cleanup(config):
    try:
        saveConfigFile(config)
        client.close()
    finally:
        print('Closed connections.')

# Our loop
def loop(config, deviceShadowHandler):
    lastConfig = config
    config = readConfigFile()
    if config is not None:
        sendConfigData(config, lastConfig)
    getSensorData(config, deviceShadowHandler)
    getConfigData(config)
    # mqttc.loop()
    return config

# Main function for config and run loop
def main():
    try:
        config = readConfigFile()

        awsShadowClient = AWSIoTMQTTShadowClient(clientId)
        awsShadowClient.configureEndpoint(host, 8883)
        awsShadowClient.configureCredentials(rootCAPath, privateKeyPath,
                                             certificatePath)

        awsShadowClient.configureAutoReconnectBackoffTime(1, 32, 20)
        awsShadowClient.configureConnectDisconnectTimeout(10)
        awsShadowClient.configureMQTTOperationTimeout(5)

        awsShadowClient.connect()
        deviceShadowHandler = awsShadowClient.createShadowHandlerWithName(
            thingName, True)
        deviceShadowHandler.shadowDelete(customShadowCallback_Delete, 5)

        client.begin()
        if config is not None:
            sendConfigData(config, None)
        while True:
            config = loop(config, deviceShadowHandler)
    except (KeyboardInterrupt):
        print('Interrupt received')
    except (RuntimeError):
        print('Snap! Bye Bye')
    finally:
        cleanup(config)


if __name__ == '__main__':
    main()
