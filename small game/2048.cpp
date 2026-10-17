#include <bits/stdc++.h>
#include <windows.h>
#include <conio.h>
using namespace std;
const int sz = 4, win_number = 2048;
int mp[110][110], score = 0;

void cls()
{
	HANDLE Hstdout;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD count;
	DWORD cellcount;
	COORD homeCoords = {0, 0};
	Hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (Hstdout == INVALID_HANDLE_VALUE)
	{
		return ;
	}
	if (!GetConsoleScreenBufferInfo(Hstdout, &csbi))
	{
		return ;
	}
	cellcount = csbi.dwSize.X * csbi.dwSize.Y;
	if (!FillConsoleOutputCharacter(Hstdout, (TCHAR)' ', cellcount, homeCoords, &count))
	{
		return ;
	}
	if (!FillConsoleOutputAttribute(Hstdout, csbi.wAttributes, cellcount, homeCoords, &count))
	{
		return ;
	}
	SetConsoleCursorPosition(Hstdout, homeCoords);
//	HANDLE Hconsole = GetStdHandle(STD_OUTPUT_HANDLE);
//	COORD coordScreen = {0, 0};
//	SetConsoleCursorPosition(Hconsole, coordScreen);
}

void output()
{
	for (int i = 1; i <= sz; i++)
	{
		cout << "\n\n";
		for (int j = 1; j <= sz; j++)
		{
			cout << setw(5) << mp[i][j];
		}
	}
	cout << "\n";
}

void setbit()
{
	int x, y;
	do
	{
		x = rand() % sz + 1, y = rand() % sz + 1;
	} while (mp[x][y]);
	int rd = rand() % 10;
	if (rd)
	{
		mp[x][y] = 2;
	}
	else
	{
		mp[x][y] = 4;
	}
}

bool win()
{
	for (int i = 1; i <= sz; i++)
	{
		for (int j = 1; j <= sz; j++)
		{
			if (mp[i][j] == win_number)
			{
				return 1;
			}
		}
	}
	return 0;
}

bool lose()
{
	for (int i = 1; i <= sz; i++)
	{
		for (int j = 1; j <= sz; j++)
		{
			if (mp[i][j] == 0)
			{
				return 0;
			}
		}
	}
	for (int i = 1; i <= sz; i++)
	{
		for (int j = 1; j < sz; j++)
		{
			if (mp[i][j] == mp[i][j+1] || mp[j][i] == mp[j+1][i])
			{
				return 0;
			}
		}
	}
	return 1;
}

void start()
{
	score = 0;
	memset(mp, 0, sizeof mp);
	setbit(), setbit();
}

bool allup()
{
	bool flag = 0;
	for (int i = 1; i <= sz; i++)
	{
		for (int j = 1; j <= sz; j++)
		{
			int i2 = i-1; 
			while (i2 > 0 && mp[i2][j] == 0)
			{
				mp[i2][j] = mp[i2+1][j], mp[i2+1][j] = 0;
				i2--;
				flag = 1;
			}
		}
	}
	return flag;
}

bool alldown()
{
	bool flag = 0;
	for (int i = sz; i > 0; i--)
	{
		for (int j = 1; j <= sz; j++)
		{
			int i2 = i+1; 
			while (i2 <= sz && mp[i2][j] == 0)
			{
				mp[i2][j] = mp[i2-1][j], mp[i2-1][j] = 0;
				i2++;
				flag = 1;
			}
		}
	}
	return flag;
}

bool allleft()
{
	bool flag = 0;
	for (int i = 1; i <= sz; i++)
	{
		for (int j = 1; j <= sz; j++)
		{
			int i2 = i-1; 
			while (i2 > 0 && mp[j][i2] == 0)
			{
				mp[j][i2] = mp[j][i2+1], mp[j][i2+1] = 0;
				i2--;
				flag = 1;
			}
		}
	}
	return flag;
}

bool allright()
{
	bool flag = 0;
	for (int i = sz; i > 0; i--)
	{
		for (int j = 1; j <= sz; j++)
		{
			int i2 = i+1; 
			while (i2 <= sz && mp[j][i2] == 0)
			{
				mp[j][i2] = mp[j][i2-1], mp[j][i2-1] = 0;
				i2++;
				flag = 1;
			}
		}
	}
	return flag;
}

bool mixup()
{
	bool flag = 0;
	for (int i = 1; i < sz; i++)
	{
		for (int j = 1; j <= sz; j++)
		{
			if (mp[i][j] == mp[i+1][j])
			{
				mp[i][j] += mp[i+1][j];
				mp[i+1][j] = 0;
				score += mp[i][j];
				flag = 1;
			}
		}
	}
	allup();
	return flag;
}

bool mixdown()
{
	bool flag = 0;
	for (int i = sz; i > 1; i--)
	{
		for (int j = 1; j <= sz; j++)
		{
			if (mp[i][j] == mp[i-1][j])
			{
				mp[i][j] += mp[i-1][j];
				mp[i-1][j] = 0;
				score += mp[i][j];
				flag = 1;
			}
		}
	}
	alldown();
	return flag;
}

bool mixleft()
{
	bool flag = 0;
	for (int i = 1; i < sz; i++)
	{
		for (int j = 1; j <= sz; j++)
		{
			if (mp[j][i] == mp[j][i+1])
			{
				mp[j][i] += mp[j][i+1];
				mp[j][i+1] = 0;
				score += mp[j][i];
				flag = 1;
			}
		}
	}
	allleft();
	return flag;
}

bool mixright()
{
	bool flag = 0;
	for (int i = sz; i > 1; i--)
	{
		for (int j = 1; j <= sz; j++)
		{
			if (mp[j][i] == mp[j][i-1])
			{
				mp[j][i] += mp[j][i-1];
				mp[j][i-1] = 0;
				score += mp[j][i];
				flag = 1;
			}
		}
	}
	allright();
	return flag;
}

bool b1, b2;

void w()
{
	b1 = allup(), b2 = mixup();
	if (b1 || b2)
	{
		setbit();
	}
}

void a()
{
	b1 = allleft(), b2 = mixleft();
	if (b1 || b2)
	{
		setbit();
	}
}

void d()
{
	b1 = allright(), b2 = mixright();
	if (b1 || b2)
	{
		setbit();
	}
}

void s()
{
	b1 = alldown(), b2 = mixdown();
	if (b1 || b2)
	{
		setbit();
	}
}

int main()
{
	bool iswin = 0;
	srand((unsigned) time (NULL));
	start();
	while (!lose()) 
	{
		if (win() && !iswin)
		{
			cout << "恭喜你赢了！！继续吗？\n如果要继续，请输入y，否则输出n：\n";
			char c;
			cin >> c;
			if (c == 'y')
			{
				iswin = 1;
			}
			else if (c == 'n')
			{
				return 0;
			}
			else
			{
				cout << "请输入y或n！！";
				Sleep(2000);
			}
			continue;
		}
		output();
		cout << "你的得分：" << score;
		cout << "\n请输入方向（上下左右对应w,s,a,d）：\n";
		while(1){if(_kbhit()) break;}
		char ch=getch();
		if(ch=='w') w();
		else if(ch=='a') a();
		else if(ch=='d') d();
		else if(ch=='s') s();
		else{
			cout << "请输入w、s、a或d！！！\n";
			Sleep(2000);
		}
		cls();
	}
	cout << "你输了......\n得分：  " << score;
	Sleep(10000);
	return 0;
}
