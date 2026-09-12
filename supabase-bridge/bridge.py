#libraries import
import os
from paho.mqtt import client as mqtt_client
from supabase import create_client, Client 

#supabase configuration
SUPABASE_URL = "https://apnttjwirbaknvzrocul.supabase.co"
SUPABASE_KEY = "sb_publishable_doPGjQFXfbP6cNfrthIX7g_OhiHDUTH"
supabase: Client = create_client(SUPABASE_URL,SUPABASE_KEY)

#broker configuration

broker = "bdd53fed217747629eb941f979434002.s1.eu.hivemq.cloud"
port = 8883
topic = "sensor/temperatura"
client_id = "python-supabase-local-bridge"
username = "esp32_s3"
password = "passwordjeje"

#connection events

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connetion succeessfully to mqtt")
        client.subscribe(topic)
    else:
        print(f"Error al conectar al broker con codigo {rc}")

def on_message (client, userdata, msg):
    payload = msg.payload.decode()
    print(f"Dato recibido -> Tópic {msg.topic} | valor: {payload}")

    try:
        temperatura_valor = float(payload)


        #sending to supabase
        data = {"valor": temperatura_valor}
        response = supabase.table("temperaturas").insert(data).execute()
        print("Data has been sent successfully")
    except Exception as e:
        print(f"Error while inserting to Supabase: {e}")
def run():
    client = mqtt_client.Client(


        client_id=client_id,
        callback_api_version=mqtt_client.CallbackAPIVersion.VERSION1
        
    )
    client.username_pw_set(username, password)


    #habilitate tls for encrypted conecction
    client.tls_set()
    
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(broker, port)

    print("Listening to mqtt bridge...")
    client.loop_forever()

if __name__ == "__main__":
    run()
