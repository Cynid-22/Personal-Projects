import requests
import json
import Python.COC.creds as creds

dataURL = "https://api.clashofclans.com/v1/players"
apiURL = dataURL + "/" + creds.playerTag + "?api_key=" + creds.API_KEY

playerInfo = requests.get(apiURL).json()

print(playerInfo)