//
// Created by domla on 2/15/2018.
//

#include "pong50926235.h"
#include <math.h>
#include <string>
using namespace std;


	Pong::Pong(){
		init();
	}
	Pong::Pong(unsigned int width, unsigned int height) {
		this->gameBoard.width = width;
		this->gameBoard.height = height;
		init();
	}
	Pong::~Pong(){}

	void Pong::updateBall(int ballX, int ballY, int ballVelX, int ballVelY){
		int newBallXPos = ballX;
		int newBallYPos = ballY;
		int newBallVelX = ballVelX;
		int newBallVelY = ballVelY;

		if (newBallXPos + gameBall.radius > gameBoard.width) {
			gameBall.v.x = -abs(gameBall.v.x);
		}
		if (newBallYPos + gameBall.radius > gameBoard.height) {
			gameBall.v.y = -abs(gameBall.v.y);
		}
		if (newBallXPos < 0) {
			gameBall.v.x = abs(gameBall.v.x);
		}
		if (newBallYPos < 0) {
			gameBall.v.y = abs(gameBall.v.y);
		}
		gameBall.x += gameBall.v.x;
		gameBall.y += gameBall.v.y;

		//Check if the ball has hit a goal
		if (gameBall.x <  gameBall.radius) {
			if (Intersect(player1left, gameBall))
				gameBall.owner = p1;
			else
				playerScore(gameBall.owner);
		}
		else if (gameBall.x + gameBall.radius > gameBoard.width - gameBall.radius ) {
			if (Intersect(player2right, gameBall))
				gameBall.owner = p2;
			else
				playerScore(gameBall.owner);
		}
		else if (gameBall.y < gameBall.radius ) {
			if (Intersect(player3top, gameBall))
				gameBall.owner = p3;
			else
				playerScore(gameBall.owner);
		}
		else if (gameBall.y + gameBall.radius > gameBoard.height - gameBall.radius) {
			if (Intersect(player4bottom, gameBall))
				gameBall.owner = p4;
			else
				playerScore(gameBall.owner);
		}
	}

	float Pong::randomDirection(float Min, float Max)
	{
		return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min;
	}

	void Pong::playerScore(PLAYER player) {
		//reset ball to center screen and choose random direction
		gameBall.x = this->gameBoard.width / 2;
		gameBall.y = this->gameBoard.height / 2;
		gameBall.v.x = randomDirection(gameBall.speed * -1, gameBall.speed);
		gameBall.v.y = gameBall.speed - abs(gameBall.v.x);

		// player scores 
		switch (player) {
		case p1: score.p1++;
			gameBall.owner = p1;
			break;
		case p2: score.p2++;
			gameBall.owner = p2;
			break;
		case p3: score.p3++;
			gameBall.owner = p3;
			break;
		case p4: score.p4++;
			gameBall.owner = p4;
			break;					
		}
	}

	void Pong::updatePaddle(PLAYER player, int paddleMove){
		switch (player) {
		case p1: if (paddleMove == -1)
						this->player1left.top += paddleSpeed;
					else if (paddleMove == 1)
						this->player1left.top -= paddleSpeed;
			break;
		case p2: if (paddleMove == -1)
						this->player2right.top += paddleSpeed;
					else if (paddleMove == 1)
						this->player2right.top -= paddleSpeed;
			break;
		case p3: if (paddleMove == -1)
						this->player3top.left -= paddleSpeed;
					else if (paddleMove == 1)
						this->player3top.left += paddleSpeed;
			break;
		case p4: if (paddleMove == -1)
						this->player4bottom.left -= paddleSpeed;
					else if (paddleMove == 1)
						this->player4bottom.left += paddleSpeed;
			break;
		}
	}

	void Pong::updateInputs(PLAYER player, string inputs){
		int input = 0;
		for (int i = 0; i < inputs.length(); i++) {
			switch (inputs.at(i)) {
			case 'l':
			case 'L': input = -1; //LEFT
				break; 
			case 'u':
			case 'U': input = 1; //UP
				break;
			case 'r':
			case 'R': input = 1; //RIGHT
				break;
			case 'd': 
			case 'D': input = -1; //DOWN
				break;
			default: input = 0;
			}
			if (input != 0) {
				updatePaddle(player, input);
			}
		}
	}

	void Pong::init(){
		paddleSpeed = 4;
		gameBall.owner = p1;
		gameBall.radius = 10;
		gameBall.speed = 1;
		gameBall.x = gameBoard.width / 2;
		gameBall.y = gameBoard.height / 2;
		gameBall.v.x = randomDirection(gameBall.speed * -1, gameBall.speed);
		gameBall.v.y = gameBall.speed - abs(gameBall.v.x);
		player1left.top = gameBoard.height / 2;
		player1left.left = 5;
		player1left.height = 75;
		player1left.width = 3;
		player2right.top = gameBoard.height / 2;
		player2right.left = gameBoard.width - 5;
		player2right.height = 75;
		player2right.width = 3;
		player3top.top = 5;
		player3top.left = gameBoard.width / 2;
		player3top.height = 3;
		player3top.width = 75;
		player4bottom.top = gameBoard.height - 5;
		player4bottom.left = gameBoard.width / 2;
		player4bottom.height = 3;
		player4bottom.width = 75;
		score.p1 = 0;
		score.p2 = 0;
		score.p3 = 0;
		score.p4 = 0;
	}

	void Pong::pause() {
		init();
		gameBall.v.x = 0;
		gameBall.v.y = 0;
	}

	/***************************************************************
		Send gamestate as string in format (without spaces):
		"ballPosX | ballPosY | ballDirX | ballDirY | paddle1top | paddle2top | paddle3left | paddle4left | scoreP1 | scoreP2 | scoreP3 | scoreP4 | ballOwner"
		where:
		paddle1 is p1(left)
		paddle2 is p2(right)
		paddle3 is p3(top)
		paddle4 is p4(bottom)
	****************************************************************/
	string Pong::getGameState() 
	{ 
		updateBall(gameBall.x, gameBall.y, gameBall.v.x, gameBall.v.y);
		if (player1left.buttonDownMovement != 0)
			updatePaddle(p1, player1left.buttonDownMovement);
		if (player2right.buttonDownMovement != 0)
			updatePaddle(p2, player2right.buttonDownMovement);
		if (player3top.buttonDownMovement != 0)
			updatePaddle(p3, player3top.buttonDownMovement);
		if (player4bottom.buttonDownMovement != 0)
			updatePaddle(p4, player4bottom.buttonDownMovement);

		string colors[] = { "red", "blue", "yellow", "green" };
		string returnString = ""; 
		returnString += to_string(gameBall.x)				+ '|' + 
						to_string(gameBall.y)				+ '|' + 
						to_string(gameBall.v.x)				+ '|' + 
						to_string(gameBall.v.y)				+ '|' + 
						to_string(player1left.top)			+ '|' + 
						to_string(player2right.top)			+ '|' + 
						to_string(player3top.left)			+ '|' + 
						to_string(player4bottom.left)		+ '|' +
						to_string(score.p1)					+ '|' +
						to_string(score.p2)					+ '|' +
						to_string(score.p3)					+ '|' +
						to_string(score.p4)					+ '|' +
						colors[(int)gameBall.owner];
		return returnString; 
	}

	
	bool Pong::Intersect(paddle paddle, ball ball) {
		return (ball.x < paddle.left + paddle.width && ball.y + ball.radius >= paddle.top && ball.y  <= paddle.top + paddle.height)	||		//intersection on left 
				(ball.x + ball.radius > paddle.left && ball.y + ball.radius >= paddle.top && ball.y <= paddle.top + paddle.height)	||		//intersection on right
				(ball.y + ball.radius > paddle.top && ball.x + ball.radius > paddle.left && ball.x < paddle.left + paddle.width)	||						//intersection at bottom
				(ball.y < paddle.top + paddle.height && ball.x + ball.radius > paddle.left && ball.x < paddle.left + paddle.width);			//intersection at top
	}


