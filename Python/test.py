import requests
import json


API_KEY = "HDEV-b51471a5-4257-4302-bb76-38b0ecf1c45d"
name = "cynid"
tagline = "3301"
region = "na"
platform = "pc"
puuid = 'd9a23e00-8dc0-50d6-9c7e-f757464852c5'
accountLevel = 49


# Get my match history
dataURL = "https://api.henrikdev.xyz/valorant/v2/by-puuid/mmr-history/"
apiURL = dataURL #+ "/" 
apiURL += region + "/" 
apiURL += platform + "/" 
# apiURL += name + "/" 
# apiURL += tagline 
apiURL += puuid
apiURL += "?api_key=" + API_KEY
playerInfo = requests.get(apiURL).json()

json_object = json.dumps(playerInfo, indent=4)
with open("output1.json", "w") as outfile:
    outfile.write(json_object)