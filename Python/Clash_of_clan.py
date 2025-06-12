import requests
import json

API_KEY = "37zsm9gd"
playerTag = "#GLGQPJCPU"
clanTag = "#2RYGPO2R2"

dataURL = "https://api.clashofclans.com/v1/players"
apiURL = dataURL + "/" + playerTag + "?api_key=" + API_KEY

playerInfo = requests.get(apiURL).json()

print(playerInfo)