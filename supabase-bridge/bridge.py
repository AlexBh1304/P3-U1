#libraries import
import os
from paho.mqtt import client as mqtt_client
from supabase import create_client, Client 

#supabase configuration
SUPABASE_URL = ""
SUPABASE_KEY = ""
supabase: Client = create_client(SUPABASE_URL,SUPABASE_KEY)

#Broker credentials

broker = ""
port = 8883
topic = ""
client_id = ""
username = ""
password = ""

#connection events

#This function manages the connection with the MQTT Broker

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connetion succeessfully to mqtt")
        client.subscribe(topic)
    else:
        print(f"Error connecting to Broker {rc}")

# This function manages the reception from the MQTT
#broker and Supabase communication

def on_message (client, userdata, msg):

    #Data received from MQTT Broker
    payload = msg.payload.decode()
    print(f"Data received -> Tópic {msg.topic} | valor: {payload}")

    try:
        temperatura_valor = float(payload)

        #Data sending to Supabase
        data = {"valor": temperatura_valor}
        response = supabase.table("temperaturas").insert(data).execute()
        print("Data has been sent successfully")

    except Exception as e:
        print(f"Error while inserting to Supabase: {e}")

#Loop function

def run():

    client = mqtt_client.Client(
        client_id=client_id,
        callback_api_version=mqtt_client.CallbackAPIVersion.VERSION1 
    )

    client.username_pw_set(username, password)

    #Enable tls for encrypted conecction
    client.tls_set()

    #Event management
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(broker, port)

    print("Listening to MQTT bridge...")
    client.loop_forever()

if __name__ == "__main__":
    run()
