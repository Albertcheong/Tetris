#include <iostream>
#include <thread>
#include <memory>
#include <vector>

#include <Windows.h>

struct Vector2
{
	int x, y;

	Vector2() : x(0), y(0) {}
	Vector2(int _x, int _y) : x(_x), y(_y) {}
};

class Application
{
	public:
	HANDLE hConsole;
	DWORD dwBytesWritten;

	const int SCREENWIDTH  = 120;
	const int SCREENHEIGHT =  30;
	const int FIELDWIDTH   =  12;
	const int FIELDHEIGHT  =  18;

	wchar_t* screen;
	wchar_t* blocks;

	unsigned char* field;

	std::wstring pieces[7];

	Vector2 position;

	int speed			= 20;
	int speedCount		=  0;
	int currentPiece    =  0;
	int currentRotation =  0;
	int pieceCount      =  0;
	int currentScore    =  0;

	bool key[4];
	bool gameover   = false;
	bool forceDown  = false;
	bool holdRotate =  true;

	std::vector<int> vlines;

	Application()
		: hConsole(CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, NULL, NULL, CONSOLE_TEXTMODE_BUFFER, NULL)), dwBytesWritten(0),
		  position(FIELDWIDTH / 2, 0)
	{
		SetConsoleActiveScreenBuffer(hConsole);
		SetConsoleTitleW(L"Tetris");

		screen = new wchar_t[SCREENWIDTH * SCREENHEIGHT];
		for (int i = 0; i < SCREENWIDTH * SCREENHEIGHT; i++)
			screen[i] = L' ';

		field = new unsigned char[FIELDWIDTH * FIELDHEIGHT];
		for (int x = 0; x < FIELDWIDTH; x++)
			for (int y = 0; y < FIELDHEIGHT; y++)
				field[y * FIELDWIDTH + x] = (x == 0 || x == FIELDWIDTH - 1 || y == FIELDHEIGHT - 1) ? 9 : 0;

		pieces[0].append(L"..X...X...X...X."); // Tetronimos 4x4
		pieces[1].append(L"..X..XX...X.....");
		pieces[2].append(L".....XX..XX.....");
		pieces[3].append(L"..X..XX..X......");
		pieces[4].append(L".X...XX...X.....");
		pieces[5].append(L".X...X...XX.....");
		pieces[6].append(L"..X...X..XX.....");

		blocks = new wchar_t[10] { L' ', L'$', L'@', L'&', L'%', L'\u2327', L'\u30B3', L'\u30A8', L'=', L'#' };
	}

	~Application()
	{
		delete[] screen;
		delete[] field;
		delete[] blocks;
		CloseHandle(hConsole);
	}

	int rotate(int x, int y, int rot)
	{
		int index = 0;
		switch (rot % 4)
		{
			case 0: index = y * 4 + x;        break; //   0 derajat
			case 1: index = 12 + y - (x * 4); break; //  90 derajat
			case 2: index = 15 - (y * 4) - x; break; // 180 derajat
			case 3: index = 3 - y + (x * 4);  break; // 270 derajat
		}
		return index;
	}

	bool checkPiece(int piece, int rot, int _x, int _y)
	{
		for (int x = 0; x < 4; x++)
			for (int y = 0; y < 4; y++)
			{
				int pieceIndex = rotate(x, y, rot);
				int fieldIndex = (_y + y) * FIELDWIDTH + (_x + x);

				if (_x + x >= 0 && _x + x < FIELDWIDTH)
				{
					if (_y + y >= 0 && _y + y < FIELDHEIGHT)
					{
						if (pieces[piece][pieceIndex] != L'.' && field[fieldIndex] != 0)
							return false;
					}
				}
			}
		return true;
	}

	void process()
	{
		while (!gameover)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50)); // limit frame
			speedCount++;
			forceDown = (speedCount == speed);

			for (int k = 0; k < 4; k++)
				key[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;

			position.x += (key[0] && checkPiece(currentPiece, currentRotation, position.x + 1, position.y)) ? 1 : 0;
			position.x -= (key[1] && checkPiece(currentPiece, currentRotation, position.x - 1, position.y)) ? 1 : 0;
			position.y += (key[2] && checkPiece(currentPiece, currentRotation, position.x, position.y + 1)) ? 1 : 0;

			if (key[3])
			{
				currentRotation += (holdRotate && checkPiece(currentPiece, currentRotation + 1, position.x, position.y)) ? 1 : 0;
				holdRotate = false;
			}
			else
				holdRotate = true;

			if (forceDown)
			{
				speedCount = 0;
				pieceCount++;
				if (pieceCount % 50 == 0)
					if (speed >= 10) speed--;

				if (checkPiece(currentPiece, currentRotation, position.x, position.y + 1))
					position.y++;
				else
				{
					for (int x = 0; x < 4; x++)
						for (int y = 0; y < 4; y++)
							if (pieces[currentPiece][rotate(x, y, currentRotation)] != L'.')
								field[(position.y + y) * FIELDWIDTH + (position.x + x)] = currentPiece + 1;

					for (int y = 0; y < 4; y++)
						if (position.y + y < FIELDHEIGHT - 1)
						{
							bool bLine = true;
							for (int x = 1; x < FIELDWIDTH - 1; x++)
								bLine &= (field[(position.y + y) * FIELDWIDTH + x]) != 0;

							if (bLine)
							{
								for (int x = 1; x < FIELDWIDTH - 1; x++)
									field[(position.y + y) * FIELDWIDTH + x] = 8;
								vlines.push_back(position.y + y);
							}
						}

					currentScore += 25;
					if (!vlines.empty())
						currentScore += (1 << vlines.size()) * 100;

					position.x      = FIELDWIDTH / 2;
					position.y      = 0;
					currentRotation = 0;
					currentPiece    = rand() % 7;

					gameover = !checkPiece(currentPiece, currentRotation, position.x, position.y);
				}
			}

			for (int x = 0; x < FIELDWIDTH; x++)
				for (int y = 0; y < FIELDHEIGHT; y++)
					screen[(y + 2) * SCREENWIDTH + (x + 2)] = L" ABCDEFG=#"[field[y * FIELDWIDTH + x]];

			for (int x = 0; x < 4; x++)
				for (int y = 0; y < 4; y++)
					if (pieces[currentPiece][rotate(x, y, currentRotation)] != L'.')
						screen[(position.y + y + 2) * SCREENWIDTH + (position.x + x + 2)] = currentPiece + 65;

			swprintf_s(&screen[2 * SCREENWIDTH + FIELDWIDTH + 6], 16, L"SCORE: %8d", currentScore);

			if (!vlines.empty())
			{
				WriteConsoleOutputCharacterW(hConsole, screen, SCREENWIDTH * SCREENHEIGHT, { 0,0 }, &dwBytesWritten);
				std::this_thread::sleep_for(std::chrono::milliseconds(400)); // Delay a bit

				for (auto& v : vlines)
					for (int x = 1; x < FIELDWIDTH - 1; x++)
					{
						for (int y = v; y > 0; y--)
							field[y * FIELDWIDTH + x] = field[(y - 1) * FIELDWIDTH + x];
						field[x] = 0;
					}

				vlines.clear();
			}
			WriteConsoleOutputCharacterW(hConsole, screen, SCREENWIDTH * SCREENHEIGHT, { 0,0 }, &dwBytesWritten);
		}
	}
};

int main()
{
	int score;
	{
		std::unique_ptr<Application> tetris = std::make_unique<Application>();
		tetris->process();
		score = tetris->currentScore;
	}

	std::cout << "Game over!! Score: " << score << std::endl;
	system("pause");
	return 0;
}