import context  # Ensures paho is in PYTHONPATH
import paho
import paho.mqtt.client as mqtt
import time

def on_connect(mqttc, obj, flags, rc):
    if rc == 0:
        print("Conectado al broker")
        global Connected
        Connected = True
    else:
        print("Ha fallado la conexion")

def on_message(mqttc, obj, msg):
    fields = []
    fields = (str(msg.payload)).split('_')
    dicc[fields[0]] = 1
    
Connected = False

def on_publish(mqttc, obj, mid):
    print("mid: " + str(mid))


def on_subscribe(mqttc, obj, mid, granted_qos):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))


def on_log(mqttc, obj, level, string):
    print(string)

# Create empty dictionary
global dicc
dicc = {}
# Create an empty list
nodelist = []

mqttc = mqtt.Client()
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_publish = on_publish
mqttc.on_subscribe = on_subscribe
# Uncomment to enable debug messages
# mqttc.on_log = on_log
mqttc.connect("localhost", 1883, 60) # Blocking function according to doc
mqttc.subscribe("public_topic", 0)
mqttc.loop_start()

while True:
    nodelist = []
    print('Diccionario sin actualizar')
    print(dicc)
    # Calculate inactive nodes and remove them from dicc
    for node in dicc:
        if dicc[node] == 0:
            nodelist.append(node)
        else:
            dicc[node] = 0
    for item in nodelist:
        dicc.pop(item, None)
    # Dictionary update
    print('')
    print('Diccionario actualizado')
    print(dicc)
    # 10 sec sleep
    time.sleep(10)