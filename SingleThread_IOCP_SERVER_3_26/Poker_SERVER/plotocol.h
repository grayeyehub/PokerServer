#pragma once
#include <iostream>
#include <unordered_map>
#include <thread>
#include <array>
#include <mutex>

using namespace std;

#include <WS2tcpip.h>
#include <MSWSock.h>

constexpr int MAX_NAME = 100;
constexpr int MAX_BUFFER      =  1024;
constexpr short SERVER_PORT   =    3500;
constexpr int WORLD_X_SIZE = 400;
constexpr int WORLD_Y_SIZE = 400;
constexpr int MAX_USER = 10000;
constexpr int NPC_START = 3500;

constexpr unsigned char C2S_LOGIN  = 1;
constexpr unsigned char C2S_MOVE = 2;
constexpr unsigned char S2C_LOGIN_OK = 3;
constexpr unsigned char S2C_ADD_PLAYER = 4;
constexpr unsigned char S2C_MOVE_PLAYER = 5;
constexpr unsigned char S2C_REMOVE_PLAYER = 6;


#pragma pack(push,1)
struct c2s_login {
	unsigned char size;
	unsigned char type;
	char name[MAX_NAME];


};


enum DIRECTION {D_N, D_S , D_W ,D_E, D_NO};
struct c2s_move {
	unsigned char size;
	unsigned char type;
	DIRECTION dr;				// 0 -> North , 


};

struct s2c_login_ok {
	unsigned char size;
	unsigned char type;
	int id;
	short x, y;
	int hp, level;
	int race; // Á¾Á·

};

struct s2c_add_player {
	unsigned char size;
	unsigned char type;
	int id;
	short x, y;
	int race;
	char name[MAX_NAME];

};

struct s2c_move_player {
	unsigned char size;
	unsigned char type;
	int id;
	short x, y;


};
struct s2c_remove_player {
	unsigned char size;
	unsigned char type;
	int id;
	


};
#pragma pack(pop)