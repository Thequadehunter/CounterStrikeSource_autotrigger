#include <Windows.h>  
#include <iostream> 
#include "HookingTool.h" 

CHackProcess fProcess;
using namespace std;

#define LEFT_CLICK 0x01

/*
m_iHealth = 0x90

m_iTeam = 0x980  - 8 bytes in front of health

Base Address: "client.dll"+004C6708

m_flags = 0x350


Enemy Base: "client.dll"+004D3904

Enemy distance = 0x10

crosshair id offset = 0x145C

NUM OF PLAYERS: server.dll+50F864
*/
const DWORD dw_playerBase = 0x004C6708; //client
const DWORD dw_enemyBase = 0x004D3904; //client
const DWORD dw_teamOffset = 0x9C; //client
//const DWORD dw_crosshairIdOffset = 0x145C; //from player base - this gets another interesting value
const DWORD dw_crosshairIdOffset = 0x14F0; //from player base
const DWORD dw_deadCheckOffSet = 0xF0;
const DWORD dw_enemyEntityOffset = 0x10; //client
const DWORD dw_playerCount = 0X5EF83C; //ENGINE
const DWORD dw_inMenu = 0x00186CB0;
const DWORD dw_menuOffset = 0x2F4;

int getPlayerCount()
{
	int players = 0;

	ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)( fProcess.__dwordEngine + dw_playerCount), &players, sizeof(int), 0);
	return players;
}

void LeftClick()
{
	INPUT    Input = { 0 };
	// left down 
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	::SendInput(1, &Input, sizeof(INPUT));

	// left up
	::ZeroMemory(&Input, sizeof(INPUT));
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	::SendInput(1, &Input, sizeof(INPUT));
}

int GetWindowString(HWND hwnd, string &s)
{
	char buffer[65536];

	int txtlen = GetWindowTextLength(hwnd) + 1; //idk why this works like this it just does
	GetWindowText(hwnd, buffer, txtlen); //read window text into char buffer

	s = buffer;
	return txtlen;
}

string GetWindowTitle(HWND window)
{
	string s;
	int len = GetWindowString(window, s); //read window name into s
	return s;
}

struct MyPlayer_t
{
	DWORD localPlayer;
	DWORD menuBase;
	int cursorPosition;
	int team;
	int menuInt;
	float deadCheck;
	bool inMenu;
	bool isDead;

	void ReadInformation()
	{
		//read player's team
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + dw_playerBase), &localPlayer, sizeof(DWORD), 0);
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(localPlayer + dw_teamOffset), &team, sizeof(int), 0);

		//read cursor position
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(localPlayer + dw_crosshairIdOffset), &cursorPosition, sizeof(int), 0);	

		//read if dead
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(localPlayer + dw_deadCheckOffSet), &deadCheck, sizeof(int), 0);

		//read if in menu
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordShaderApid + dw_inMenu), &menuBase, sizeof(DWORD), 0);
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(menuBase + dw_menuOffset), &menuInt, sizeof(int), 0);

		if (menuInt == 25)
			inMenu = false;
		else
			inMenu = true;

		if (deadCheck != 0)
			isDead = false;
		else
			isDead = true;

		cout << menuInt << endl;
	}
}MyPlayer;

struct EnemyPlayerList_t
{
	DWORD localPlayer;
	int team;

	void ReadInformation(int player)
	{
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + dw_enemyBase + (player * dw_enemyEntityOffset)), &localPlayer, sizeof(DWORD), 0);

		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(localPlayer + dw_teamOffset), &team, sizeof(int), 0);
	}

}EnemyPlayerList[32];

void triggerBot()
{
	HWND hwnd = GetForegroundWindow();
	int playerCount = getPlayerCount();

	if (MyPlayer.cursorPosition == 0)
		return;
	else if (MyPlayer.cursorPosition > playerCount)
		return;
	else if (EnemyPlayerList[MyPlayer.cursorPosition - 1].team == MyPlayer.team)
		return;
	else if (MyPlayer.team == 1)
		return;
	else if (MyPlayer.inMenu)
		return;
	else if (MyPlayer.isDead)
		return;
	else if (EnemyPlayerList[MyPlayer.cursorPosition - 1].team != MyPlayer.team) 
	{
		if (GetWindowTitle(hwnd).find("Counter-Strike") != string::npos)
		{
			cout << "enemy team: " << EnemyPlayerList[MyPlayer.cursorPosition - 1].team << endl;
			LeftClick();
			Sleep(20);
		}
	}
}


int main()
{
	cout << "test" << endl;
	fProcess.RunProcess();
	cout << "CS Source found" << endl;

	int playerCount = getPlayerCount();

	for (;;)
	{
		MyPlayer.ReadInformation();

		for (int i = 0; i < playerCount; i++)
		{
			EnemyPlayerList[i].ReadInformation(i);
		}
		triggerBot();
	}

	return 0;
}