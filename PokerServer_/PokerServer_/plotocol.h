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
constexpr int MAX_USER = 100;


constexpr unsigned char C2S_Req_Login  = 1;
constexpr unsigned char C2S_Req_DrawCard = 2;

constexpr unsigned char S2C_Ack_Login_OK = 3;
constexpr unsigned char S2C_Ack_DrawCard = 4;

constexpr unsigned char S2C_Add_Player = 5;
constexpr unsigned char S2C_Result = 6;


#pragma pack(push,1)
struct c2s_login {
	unsigned char size;
	unsigned char type;
	char name[MAX_NAME];


};


struct S2CAckLoginOK {
	unsigned char size;
	unsigned char type;
	int id;

};


struct S2CAckDrawCard {
	unsigned char size;
	unsigned char type;
	int id;
	int CardNum;
	int CardType;

};

struct S2CAddPlayer {
	unsigned char size;
	unsigned char type;
	int id;
	char name[MAX_NAME];

};

struct S2CResult {
	unsigned char size;
	unsigned char type;
	int id;
	


};
#pragma pack(pop)