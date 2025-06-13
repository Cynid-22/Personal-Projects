import requests
import json
import Python.Valorant.creds as creds


name = "cynid"
tagline = "3301"
region = "na"
platform = "pc"
accountLevel = 49


# Get my match history
dataURL = "https://api.henrikdev.xyz/valorant/v2/by-puuid/mmr-history/"
apiURL = dataURL #+ "/" 
apiURL += region + "/" 
apiURL += platform + "/" 
# apiURL += name + "/" 
# apiURL += tagline 
apiURL += creds.puuid
apiURL += "?api_key=" + creds.API_KEY
playerInfo = requests.get(apiURL).json()

json_object = json.dumps(playerInfo, indent=4)
with open("output1.json", "w") as outfile:
    outfile.write(json_object)