import requests
import json
import cred

playerTag = "#GLGQPJCPU"
clanTag = "#2RYGPO2R2"

dataURL = "https://api.clashofclans.com/v1/players"
apiURL = dataURL + "/" + playerTag + "?api_key=" + cred.API_KEY

playerInfo = requests.get(apiURL).json()

print(playerInfo)