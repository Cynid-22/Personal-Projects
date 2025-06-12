import requests
import json
import Python.Valorant.creds as creds


name = "cynid"
tagline = "3301"
region = "na"


# Get my match history
dataURL = "https://api.henrikdev.xyz/valorant/v3/matches"
apiURL = dataURL + "/" + region + "/" + name + "/" + tagline + "?api_key=" + creds.API_KEY
playerInfo = requests.get(apiURL).json()

# Create a list of players in the current match
playerList = [[0 for i in range(3)]for j in range(12)]
for i in range(len(playerInfo['data'][0]['players']['all_players'])):
    playerList[i][0] = (playerInfo['data'][0]['players']['all_players'][i]['name'])
    playerList[i][1] = (playerInfo['data'][0]['players']['all_players'][i]['tag'])
    playerList[i][2] = (playerInfo['data'][0]['players']['all_players'][i]['puuid'])

# Create a list of the stats of all the players in the matc

otherPlayerInfoList = {}

for i in range(len(playerList)):
    eachPlayerInfo = {}
    dataURL = "https://api.henrikdev.xyz/valorant/v3/matches"
    if (playerList[i][0] != 0):
        apiURL = dataURL + "/" + region + "/" + playerList[i][0] + "/" + playerList[i][1] + "?api_key=" + creds.API_KEY
        otherPlayerInfo = requests.get(apiURL).json()
        for j in range(5):
            try:
                tempDict = {}
                for h in range(12):
                    dir = otherPlayerInfo['data'][j]['players']['all_players'][h]
                    if (dir['puuid'] == playerList[i][2]):
                        tempDict['level'] = dir['level']
                        tempDict['character'] = dir['character']
                        tempDict['currenttier_patched'] = dir['currenttier_patched']
                        tempDict['mode'] = otherPlayerInfo['data'][j]['metadata']['mode']
                        tempDict['rounds_played'] = otherPlayerInfo['data'][j]['metadata']['rounds_played']
                        for index in range(7):
                            k = list(dir['stats'].keys())[index]
                            tempDict[k] = dir['stats'][k]
                            index += 1
                        eachPlayerInfo["match_" + str(j)] = tempDict
                        otherPlayerInfoList[str(playerList[i][0] + "#" + playerList[i][1])] = eachPlayerInfo
                        break
            except:
                pass


finalOtherPlayerInfoList = {}
for i in otherPlayerInfoList:
    tempDict = {}
    char = []; rank = []
    maxLevel = 0; combatScore = 0; roundsPlayed = 0; DMRoundsCount = 0; kills = 0; deaths = 0; assists = 0; headshots = 0; bodyshots = 0; legshots = 0
    for j in range(5):
        dir = otherPlayerInfoList[i]['match_' + str(j)]
        maxLevel = max(maxLevel, dir['level'])
        char.append(dir['character'])
        rank.append(dir['currenttier_patched'])
        headshots += dir['headshots']
        bodyshots += dir['bodyshots']
        legshots += dir['legshots']
        if (dir['mode'] != "Deathmatch"):
            combatScore += dir['score']
            roundsPlayed += dir['rounds_played']
            kills += dir['kills']
            deaths += dir['deaths']
            assists += dir['assists']
        else:
            DMRoundsCount += 1
    tempDict['level'] = maxLevel
    tempDict['agent'] = char
    tempDict['rank'] = rank
    tempDict['No. of DM'] = str(DMRoundsCount) + " of 5 rounds"
    if (DMRoundsCount < 5):
        tempDict['avg. cs'] = round(combatScore/roundsPlayed,1)
        tempDict['K/D/A'] = str(round(kills/deaths, 3)) + " | A: " + str(assists)
    else:
        tempDict['avg. cs'] = "N/A"
        tempDict['K/D/A'] = "N/A"
    if (headshots + bodyshots + legshots != 0):
        tempDict['HS accuracy'] = str(round((headshots / (headshots + bodyshots + legshots)),5)*100) + "%"
    else:
        tempDict['HS accuracy'] = "N/A"
    finalOtherPlayerInfoList[i] = tempDict


json_object = json.dumps(finalOtherPlayerInfoList, indent=4)
with open("output.json", "w") as outfile:
    outfile.write(json_object)



