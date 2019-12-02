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
    "y": 0
}

pn = []
pm = []
psize = 20
n = 3
m = 3

CONNECT_USER        = "dcc075/users/connect"
DISCONNECT_USER     = "dcc075/users/disconnect"
NUMBER_USERS        = "dcc075/users/number_users"
COMMAND_USER        = "dcc075/users/command"
PARAM_ALPHA         = "dcc075/params/alpha"
PARAM_BETA          = "dcc075/params/beta"
PARAM_Y             = "dcc075/params/y"
PARAM_DELTA         = "dcc075/params/delta"
PARAM_TOTIENTDELTA  = "dcc075/params/totient_delta"
N_USERS     = 0
USERS       = []

def broadcast_to_users(command):
    print()
    for user in USERS:
        msg = user.decode("utf-8") + "_{}".format(command)
        client.publish(COMMAND_USER, msg)
        print("- Command \'{0}\' sent to \'{1}\'.".format(command, user))

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
    
    y = random.randrange(delta)
    while sympy.igcd(y, delta) != 1:
        y = random.randrange(delta)

    params["alpha"]         = alpha
    params["delta"]         = delta
    params["beta"]          = beta
    params["totient_delta"] = totient(delta)
    params["y"]             = y

def on_message(client, userdata, message):
    global N_USERS, USERS
    if message.topic == CONNECT_USER:
        if message.payload not in USERS:
            USERS.append(message.payload)
            N_USERS += 1
            client.publish(NUMBER_USERS, N_USERS)
            print("\n %s connected." % (message.payload))
            print("Users: {}".format(USERS))

            if N_USERS == 1:
                print("\nGenerating session parameters...")
                generate_session_parameters()
                client.publish(PARAM_ALPHA, params["alpha"])
                client.publish(PARAM_DELTA, params["delta"])
                client.publish(PARAM_BETA, params["beta"])
                client.publish(PARAM_TOTIENTDELTA, params["totient_delta"])
                print("Parameters: {}".format(params))
            
            client.publish(PARAM_Y, params["y"])
        else:
            print("{} is already connected.".format(message.payload))

    if message.topic == DISCONNECT_USER:
        if message.payload in USERS:
            USERS = [user for user in USERS if message.payload != user]
            N_USERS -= 1
            print("\n %s disconnected." % (message.payload))
            print("Users: {}".format(USERS))

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
 
        global Connected                #Use global variable
        Connected = True                #Signal connection 
    else:
 
        print("Connection failed")
    
    client.subscribe(CONNECT_USER, qos=0)
    client.subscribe(DISCONNECT_USER, qos=0)

 
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

try:
    while True:
        pass
       
except KeyboardInterrupt:
    broadcast_to_users("disconnect")
    while len(USERS) > 0:
        pass
    client.disconnect()
    client.loop_stop()

