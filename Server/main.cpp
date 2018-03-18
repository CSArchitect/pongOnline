#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include "websocket.h"
#include "pong50926235.h"
#include <queue>
#define INTERVAL_MS 33

using namespace std;

chrono::system_clock::duration artificialLatency(chrono::system_clock::duration timestamp, int type, int min, int max);
string return_current_time_and_date();
void sendToClients(string send);
bool addArtLatency = false;

/*********************************************

				HELPER CLASSES 

**********************************************/
struct player {
	int clientID;
	string username;
};

class input {
public:
	int playerNum;
	string inputChar;
	chrono::system_clock::duration time;
	chrono::system_clock::duration latencyAdded;
	input(int p, string inputChar, chrono::system_clock::duration time, chrono::system_clock::duration latencyAdded) {
		this->playerNum = p;
		this->inputChar = inputChar;
		this->time = time;
		this->latencyAdded = latencyAdded;
	}
};


class Compare
{
public:
	bool operator() (input input1, input input2)
	{
		return input1.time > input2.time;
	}
};

/**************************************************************

						Globals

***************************************************************/
webSocket server;
map<int, string> clientID_username_map;
Pong pong(500, 500);
int playerCount = 0;
priority_queue<input, vector<input>, Compare> inputTimeQueue;

void parseStringUpdatePacket(int clientID, string message);
vector<string> split(string toSplit);

player * players;

/* called when a client connects */
void openHandler(int clientID){
	int player_num = -1;
	if (playerCount == 4)
		return;
	else {
		for (int i = 0; i < 4; ++i) {
			if (players[i].clientID == -1) {
				players[i].clientID = clientID;
				player_num = i;
				break;
			}
		}
		++playerCount;
		if (playerCount == 4) {
			pong.init();
			cout << "4 players have joined";
		}
	}
	server.wsSend(clientID, string("PlayerNum") + '|' + to_string(player_num));
}


/* called when a client disconnects */
void closeHandler(int clientID){
	pong.pause();    
	playerCount--;

    vector<int> clientIDs = server.getClientIDs();
    for (int i = 0; i < clientIDs.size(); i++){
        if (clientIDs[i] != clientID )
            server.wsSend(clientIDs[i], "pause");
    }
}

/* called when a client sends a message to the server */
void messageHandler(int clientID, string message){	
    parseStringUpdatePacket(clientID, message);
}

/* called once per select() loop */
int interval_clocks = CLOCKS_PER_SEC * INTERVAL_MS / 1000;

//Run entire input log in order of timestamp
void runInputQueue() {
	int size = inputTimeQueue.size();
	for (int i = 0; i < size; i++) {
		input in = inputTimeQueue.top();
		if (in.time.count() <= chrono::system_clock::now().time_since_epoch().count()) {
			pong.updateInputs(static_cast<Pong::PLAYER>(in.playerNum), in.inputChar);
			inputTimeQueue.pop();
		}
		else
			break;
	}
}

void periodicHandler(){
    static clock_t next = clock() + interval_clocks;
    clock_t current = clock();
    if (current >= next && playerCount == 4){
		runInputQueue();
        sendToClients(return_current_time_and_date() + '|' + pong.getGameState());
        next = clock() + 10;
    }
}

void sendToClients(string send) {
	vector<int> clientIDs = server.getClientIDs();
	for (int i = 0; i < clientIDs.size(); i++) {
		int playerNum = -1;
		for (int j = 0; j < 4; j++) {
			if (players[j].clientID == clientIDs[i])
				playerNum = j;
		}
		if(playerNum != -1)
			server.wsSend(clientIDs[i], to_string(playerNum) + '|' + send);
	}
}

string return_current_time_and_date()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::stringstream ss;
	ss << std::put_time(localtime(&in_time_t), "%Y-%m-%d %X");
	ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
	//cout << ss.str() << endl;
	return ss.str();
}

/***********************************************************************
 *  Parses strings in the format (without spaces):
 *  " username | ballPosX | ballPosY | ballDirX | ballDirY | paddleTop | paddleLeft | INPUT_KEYS_STRING"
 *  the INPUT_KEYS_STRING will be a string of key characters (i.e. w or W or s or S) since last update packet
 *
 *  will send input data to pong object for simulation through methods:
 *  updateBall(double, double, double, double)
 *  updatePaddle(double)
 *  updateInputs(string)
 *
 ***********************************************************************/
void parseStringUpdatePacket(int clientID, string message){
    vector<string> tokens = split(message);

	int player_num = -1;
	for (int i = 0; i < 4; ++i) {
		if (clientID == players[i].clientID) {
			players[i].username = tokens[0];
			player_num = i;
		}
	}
	Pong::PLAYER player = static_cast<Pong::PLAYER>(player_num);

	// Add inputs to the Priority Queue such that the top of the queue has the lowest timestamp
	if (tokens.size() >= 3) {
		chrono::system_clock::duration now = chrono::system_clock::now().time_since_epoch();
		chrono::system_clock::duration timestamp;
		if (addArtLatency)
			timestamp = artificialLatency(now, 1, 5, 200); //0 =fixed, 1=random, 2=incremental (min, max) for incremental
		else
			timestamp = now;
		inputTimeQueue.push(input(player_num, tokens[1], timestamp, timestamp - now));
	}
}

/***********************************************************************
 * Split string into tokenized vector<string>
 ***********************************************************************/
vector<string> split(string toSplit){
    stringstream ss(toSplit);
    string item;
    vector<string> tokens;
    while(getline(ss, item, '|')){
        tokens.push_back(item);
    }
    return tokens;
}


/************************************************************************

	Artificial latency 
	types:
	0 = fixed		(does not use min and max)
	1 = random		(uses min and max)
	2 = incremental (uses min and max)

	return the timestamp with added latency determined by the type
*************************************************************************/
int FIXED_LATENCY = 200;
int incrementalLatStep = 0;

chrono::system_clock::duration artificialLatency(chrono::system_clock::duration timestamp, int type, int min, int max) {
	chrono::system_clock::duration toReturn = timestamp;
	switch (type) {
	case 0: 
		toReturn = toReturn + chrono::milliseconds{ FIXED_LATENCY };
		break;

	case 1: 
		toReturn = toReturn + chrono::milliseconds{ rand() % max + min };
		break;

	case 2: 
		if (incrementalLatStep + 1 + min > max)
			incrementalLatStep = 0;
		toReturn = toReturn + chrono::milliseconds{ min + incrementalLatStep++ };
	}

	return toReturn;
}

int main(int argc, char *argv[])
{
	players = new player[4];
	for (int i = 0; i < 4; ++i)
		players[i].clientID = -1;

	/* set event handler */
	server.setOpenHandler(openHandler);
	server.setCloseHandler(closeHandler);
	server.setMessageHandler(messageHandler);
    server.setPeriodicHandler(periodicHandler);

	cout << "Would you like to simulate latency? (y/n)" << endl;
	char answer = cin.get();
	cin.ignore(1000, '\n');
	if (answer == 'y' || answer == 'Y')
		addArtLatency = true;
	else
		addArtLatency = false;


    /* start the chatroom server, listen to ip '127.0.0.1' and ports '8000'-'8003' */
    int ports[] = { 8000, 8001, 8002, 8003 };

	server.startServer(ports);

    return 1;
}
