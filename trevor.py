import paho.mqtt.client as mqttClient
import paho.mqtt.subscribe as subscribe
from sympy.ntheory.factor_ import totient 
import sympy
import random

import time

params = {
    "delta": 1,
    "alpha": 1,
    "beta":  1,
    "totient_delta": 0,
}

pn = []
pm = []
psize = 100
n = 3
m = 3

USERS_TOPIC = "dcc075/users"
N_USERS     = 0
USERS       = []

gen_params = False

def on_message(client, userdata, message):
    global N_USERS, USERS
    print(message.payload[0:500])
    if message.retain:
        USERS = [ x for x in USERS if x is not message.payload ]

    if message.topic == USERS_TOPIC:
        if message.payload not in USERS:
            USERS.append(message.payload)
            N_USERS += 1

            if N_USERS == 1:
                generate_session_parameters()
                client.publish("dcc075/params/alpha", params["alpha"])
                client.publish("dcc075/params/delta", params["delta"])
                client.publish("dcc075/params/beta", params["beta"])
                client.publish("dcc075/params/totient_delta", params["totient_delta"])
                
            
    
    print(" %s connected." % (message.payload))
    print("Users: {}".format(USERS))
    print("Parameters: {}".format(params))

def generate_session_parameters():
    delta = 1
    alpha = 1
    beta = 1
    temp_q = 1

    for i in range(m):
        p = sympy.sieve[random.randrange(psize)]
        pm.append(p)
        temp_q *= p

    for i in range(n):
        p = sympy.sieve[random.randrange(psize)]
        while p in pm:
            p = sympy.sieve[random.randrange(psize)]
        pn.append(p)
        delta *= pow(p, 3)
        alpha *= pow(p, 2)*(p-1)
        beta *= p*(p-1)*temp_q

    params["alpha"]         = alpha
    params["delta"]         = delta
    params["beta"]          = beta
    params["totient_delta"] = totient(delta)


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
 
        global Connected                #Use global variable
        Connected = True                #Signal connection 
    else:
 
        print("Connection failed")
    client.subscribe("dcc075/users", qos=0)

 
Connected = False   #global variable for the state of the connection
 
broker_address= "broker.hivemq.com"
port = 1883
 
client = mqttClient.Client("Trevor")               #create new instance
client.on_connect= on_connect                      #attach function to callback
client.on_message = on_message
client.connect(broker_address, port=port)          #connect to broker
 
client.loop_start()        #start the loop
 
while Connected != True:    #Wait for connection
    time.sleep(0.1)

client.subscribe("dcc075/users", qos=0)
 
try:
    while True:
        pass
        #value = input('Enter the message:')
        #client.publish("dcc075Enviar",value)
    
except KeyboardInterrupt:
    client.disconnect()
    client.loop_stop()
