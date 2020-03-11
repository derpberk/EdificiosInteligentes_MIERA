#import context  # Ensures paho is in PYTHONPATH
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

# When we receive a publishing
def on_message(mqttc, obj, msg):
    fields = []
    fields = (str(msg.payload)).split('/')
    # Field 0 -> ID
    # Field 1 -> Node Type
    # Field 2,3 ... -> Topics
    node_status = 1
    dicc_frame = fields[1:]
    dicc_frame.append(node_status) 
    dicc[fields[0]] = dicc_frame
    
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
    #print(dicc)
    # Calculate inactive nodes and remove them from dicc
    for node in dicc:

        dicc_frame = dicc[node] # Catch the current frame of current Key

        if dicc_frame[-1] == 0: # If Active field is disable
            dicc.pop(node,None) # EXTERMINATE!! (from the dicc :D)

    # Dictionary update
    print('')
    print('Diccionario actualizado')

    # Conformation of the stdout msg #
    header = "<p><br></p>\n"
    body = ""
    content = ""
    out_msg = ""

    for node in dicc:
        dicc_frame = dicc[node]
        body = "<p>Nodo " + str(node) + " - " + str(dicc_frame[0]) + ": </p>\n"
        content = '<p>&nbsp;' + "Topics: " + "</p>\n"

        for i in range(1,len(dicc_frame)-1): 
            content += "&nbsp;&nbsp;" + str(node) + "/" + str(dicc_frame[0]) + "/" +  str(dicc_frame[i]) + "</p>\n" 
        content += "\n"

        out_msg += header + body + content

    # Open the file where we will write our dicc
    print(out_msg)
    f = open("active_node_list.txt", "w")
    f.write(out_msg)
    f.close()
    print("printed!")
    # 10 sec sleep
    time.sleep(10)