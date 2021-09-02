import paho.mqtt.client as mqtt
import json


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print('Connected with result code ' + str(rc))
    client.subscribe('+/devices/+/up')


# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    ergebnis = json.loads(msg.payload)
    values = ergebnis['payload_fields']
    to = values['to']
    m = values['message']



client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.tls_set()
client.username_pw_set('lora-mobile-messenger',
                       password='NNSXS.HVJUZGVK6KJY2Z5URNW4F7BZMRWBEBV6KXHLYRA.26MXTX2NZS2AHPJKKHO3AALVGNPCHTRWDGENZQKFJCRBGAMSRH3Q')

client.connect('eu.thethings.network', 8883, 60)

client.loop_forever()
