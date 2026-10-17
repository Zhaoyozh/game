#include <bits/stdc++.h>
#include <windows.h>
using namespace std;
const int sz = 19, jns = 4;
char mp[sz][sz];
int d[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
bool vis[sz][sz];
int jin[2][2], cd[2][4];
string alljn[jns];
struct nd
{
	int x, y;
} c[sz*sz];//记录每一步的落子情况 

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

void setjn()
{
	alljn[0] = "悔一步棋\n";
	alljn[1] = "吃掉对面一个棋子\n";
	alljn[2] = "将一颗棋子变为你的，但这颗棋子周围不能有你的棋子\n";
	alljn[3] = "清空棋盘\n";
}

char tochar(int color)
{
	return (color % 2? 'o': '@');
}

void output()
{
	cout << "  ";
	for (int i = 0; i < sz; )
	{
		cout << setw(2) << ++i;
	}
	for (int i = 0; i < sz; i++)
	{
		cout << "\n" << setw(2) << i+1;
		for (int j = 0; j < sz; j++)
		{
			cout << " " << mp[i][j];
		}
	}
	cout << "\n";
	for (int i = 0; i < jns; i++)
	{
		cout << "技能" << i << "：" << alljn[i];
	}
}

bool check(int x, int y) 
{
	for (int i = 0; i < 4; i++)
	{
		int cnt = 1;
		char c = mp[x][y];
		for (int j = 1; j < 5; j++)
		{
			int nx = x + j * d[i][0], ny = y + j * d[i][1];
			if (nx < 0 || nx >= sz || 
				ny < 0 || ny >= sz || 
				mp[nx][ny] != c)
			{
				break;
			}
			cnt++;
		}
		for (int j = -1; j > -5; j--)
		{
			int nx = x + j * d[i][0], ny = y + j * d[i][1];
			if (nx < 0 || nx >= sz || 
				ny < 0 || ny >= sz || 
				mp[nx][ny] != c)
			{
				break;
			}
			cnt++;
		}
		if (cnt >= 5)
		{
			return 1;
		}
	}
	return 0;
}

void win(int color)
{
	cls();
	output();
	cout << (color? "白": "黑") << "棋取得了胜利！！";
	Sleep (10000) ;
}

bool only(int x, int y, int color)
{
	int nx = x + 1, ny = y + 1;
	if (nx < sz && nx >= 0 && ny < sz && ny >= 0 && mp[nx][ny] == tochar(color))
	{
		return 0;
	}
	nx = x + 0, ny = y + 1;
	if (nx < sz && nx >= 0 && ny < sz && ny >= 0 && mp[nx][ny] == tochar(color))
	{
		return 0;
	}
	nx = x + -1, ny = y + 1;
	if (nx < sz && nx >= 0 && ny < sz && ny >= 0 && mp[nx][ny] == tochar(color))
	{
		return 0;
	}
	nx = x + 1, ny = y + 0;
	if (nx < sz && nx >= 0 && ny < sz && ny >= 0 && mp[nx][ny] == tochar(color))
	{
		return 0;
	}
	nx = x + 1, ny = y + -1;
	if (nx < sz && nx >= 0 && ny < sz && ny >= 0 && mp[nx][ny] == tochar(color))
	{
		return 0;
	}
	nx = x + 0, ny = y + -1;
	if (nx < sz && nx >= 0 && ny < sz && ny >= 0 && mp[nx][ny] == tochar(color))
	{
		return 0;
	}
	nx = x + -1, ny = y + 0;
	if (nx < sz && nx >= 0 && ny < sz && ny >= 0 && mp[nx][ny] == tochar(color))
	{
		return 0;
	}
	nx = x + -1, ny = y + -1;
	if (nx < sz && nx >= 0 && ny < sz && ny >= 0 && mp[nx][ny] == tochar(color))
	{
		return 0;
	}
	return 1;
}

void givejn(int color, int id)
{
	int rd = rand() % 1000;
	if (rd < 400)
	{
		jin[color][id] = 0;
	}
	else if (rd < 750)
	{
		jin[color][id] = 1;
	}
	else if (rd < 950)
	{
		jin[color][id] = 2;
	}
	else
	{
		jin[color][id] = 3;
	}
}

int main()
{
	memset(cd, 0, sizeof cd);
	setjn();
	srand ((unsigned) time (NULL)); 
	for (int i = 0; i < sz; i++)
	{
		for (int j = 0; j < sz; j++)
		{
			mp[i][j] = '.';
		}
	}
	int x, y;
	givejn(0, 0), givejn(1, 0), givejn(0, 1), givejn(1, 1);
	for (int i = 0; i < sz * sz; i++)
	{
		output();
		if (i % 2)
		{
			cout << "轮到了白棋，"; 
			//白棋 
		}
		else
		{
			cout << "轮到了黑棋，"; 
			//黑棋 
		}
		cout << "请输入落子位置：（x, y），\n如果想使用技能，请输入-1 技能编号\n你拥有的技能："
			 << jin[i%2][0] << ", " << jin[i%2][1] << "\n";
		cin >> x >> y;
		if (x == -1 && (y == jin[i%2][0] || y == jin[i%2][1]))
		{
			bool flag = 1;
			if (cd[i%2][y] > 0)
			{
				cout << "你无法使用本技能\n本技能还有 " << cd[i%2][y] << " 回合的冷却时间\n";
				cls();
				i--;
				continue;
			}
			if (y == 0)
			{
				if (i < 2)
				{
					cout << "你无法使用本技能！";
//						i--;
					flag = 0;
					Sleep(2000); 
				}
				else
				{
					mp[c[i-1].x][c[i-1].y] = '.', mp[c[i-2].x][c[i-2].y] = '.';
					cd[i%2][0] = 3;
					i-= 3;
					//传输使用技能0的消息 
				}
				cls();
			}
			else if (y == 1)
			{
				cout << "请输入目标棋子位置（x，y）";
				int x2, y2;
				cin >> x2 >> y2;
				x2--, y2--;
				if (x2 < 0 || x2 >= sz || 
					y2 < 0 || y2 >= sz || 
					mp[x2][y2] != tochar(i+1))
				{
					cout << "你无法使用本技能！";
					flag = 0;
					Sleep(2000); 
				}
				else
				{
					mp[x2][y2] = '.';
					cd[i%2][1] = 4;
					//传输使用技能1的消息 
				}
				cls();
			}
			else if (y == 2)
			{
				cout << "请输入目标棋子位置（x，y）";
				int x11, y11;
				cin >> x11 >> y11;
				x11--, y11--;
				if (x11 < 0 || x11 >= sz || 
					y11 < 0 || y11 >= sz || 
					mp[x11][y11] != tochar(i+1) || 
					!only(x11, y11, i))
				{
					cout << "你无法使用本技能！";
					flag = 0;
					Sleep(2000); 
				}
				else
				{
					mp[x11][y11] = tochar(i);
					cd[i%2][2] = 5;
					//传输使用技能2的消息 
				}
				cls();
			}
			else if (y == 3)
			{
				memset(mp, '.', sizeof mp);
				cd[i%2][3] = 15;
				i = -1;
				cls();
				//传输使用技能3的消息 
			}
			if (flag)
			{
				if (y == jin[i%2][0])
				{
					givejn(i%2, 0);
				}
				else
				{
					givejn(i%2, 1);
				}
			}
			continue;
		}
		x--, y--;
		if (x < 0 || x >= sz || 
			y < 0 || y >= sz || 
			mp[x][y] != '.')
		{
			i--;
			cout << "请重新输入！该位置不合法！";
			Sleep(2000); 
		}
		else
		{
			mp[x][y] = tochar(i);
			//传输落子位置 
			if (check(x, y))
			{
				win(i % 2);
				//传输胜利信号 
				return 0;
			}
		}
		cls();
		c[i].x = x, c[i].y = y; 
		for (int j = 0; j < jns; j++)
		{
			cd[i%2][j] = max(0, cd[i%2][j]-1);
		}
	}
	//传输平局信号 
	cout << "平局！";
	Sleep(10000);
	return 0;
}
