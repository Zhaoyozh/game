#include <bits/stdc++.h>
#include <windows.h>
using namespace std;
const int unopen = 22, empty = '.', bomb = '+', walk = '*', findit = '!';
int n, m, b;
int d[8][2] = {{1, 0}, {0, 1}, {1, -1}, {-1, 1}, {0, -1}, {-1, 0}, {1, 1}, {-1, -1}};
char mp1[110][110], mp2[110][110];

struct nd
{
	int x, y;
};

void output1()
{
	cout << "    ";
	for (int i = 1; i <= m; i++)
	{
		cout << setw(2) << i;
	}
	cout << "\n";
	for (int i = 1; i <= n; i++)
	{
		cout << "\n" << setw(2) << i << "  ";
		for (int j = 1; j <= m; j++)
		{
			cout << " " << mp1[i][j];
		}
	}
	cout << "\n";
}

void output2()
{
	cout << "    ";
	for (int i = 1; i <= m; i++)
	{
		cout << setw(2) << i;
	}
	cout << "\n";
	for (int i = 1; i <= n; i++)
	{
		cout << "\n" << setw(2) << i << "  ";
		for (int j = 1; j <= m; j++)
		{
			cout << " " << mp2[i][j];
		}
	}
	cout << "\n";
}

void openbit(int x, int y)
{
	for (int i = 0; i < 8; i++)
	{
		int nx = x + d[i][0], ny = y + d[i][1];
		if (nx <= n && nx > 0 && ny <= m && ny > 0 && mp1[nx][ny] == unopen)
		{
			if (mp2[nx][ny] == bomb)
			{
				continue;
			}
			else if (mp2[nx][ny] == empty)
			{
//				printf("%d %d %d %d .\n", x, y, nx, ny);
				if (abs(nx-x) + abs(ny-y) < 2)
				{
//					printf("%d %d %d %d ..\n", x, y, nx, ny);
					mp1[nx][ny] = mp2[nx][ny];
					openbit(nx, ny);
				}
			}
			else
			{
//				printf("%d %d %d %d 123456789\n", x, y, nx, ny);
				mp1[nx][ny] = mp2[nx][ny];
			}
		}
	}
}

bool open(int x, int y)
{
	if (mp2[x][y] == bomb)
	{
		mp2[x][y] = walk;
		return 1;
	}
	if (mp2[x][y] != empty)
	{
		mp1[x][y] = mp2[x][y];
		return 0;
	}
	mp1[x][y] = mp2[x][y];
	openbit(x, y);
	return 0;
}

void setbomb(int b)
{
	srand((unsigned) time (NULL));
	for (int i = 1; i <= b; i++)
	{
		int x, y;
		do
		{
			x = rand() % n + 1, y = rand() % m + 1;
		} while(mp2[x][y] != empty);
		mp2[x][y] = bomb;
	}
}

void start(int n, int m, int b, int x, int y)
{
	memset(mp1, unopen, sizeof mp1);
	memset(mp2, empty, sizeof mp2);
	mp2[x][y] = bomb;
	for (int i = 0; i < 8; i++)
	{
		int nx = x + d[i][0], ny = y + d[i][1];
		if (nx <= n && nx > 0 && ny <= m && ny > 0)
		{
			mp2[nx][ny] = bomb;
		}
	}
	setbomb(b);
	mp2[x][y] = empty;
	for (int i = 0; i < 8; i++)
	{
		int nx = x + d[i][0], ny = y + d[i][1];
		if (nx <= n && nx > 0 && ny <= m && ny > 0)
		{
			mp2[nx][ny] = empty;
		}
	}
	for (int i = 1; i <= n; i++)
	{
		for (int j = 1; j <= m; j++)
		{
			if (mp2[i][j] == bomb)
			{
				continue;
			}
			int cnt = 0;
			for (int k = 0; k < 8; k++)
			{
				int nx = i + d[k][0], ny = j + d[k][1];
				if (nx <= n && nx > 0 && ny <= m && ny > 0 && mp2[nx][ny] == bomb)
				{
					cnt++;
				}
			}
			if (cnt)
			{
				mp2[i][j] = cnt + '0';
			}
		}
	}
	open(x, y);
}

void findbomb(int x, int y)
{
	mp1[x][y] = findit;
}

bool win()
{
	for (int i = 1; i <= n; i++)
	{
		for (int j = 1; j <= n; j++)
		{
			if (mp1[i][j] == unopen && mp2[i][j] != bomb)
			{
				return 0;
			}
		}
	}
	return 1;
}

void bombing(int x, int y)
{
	int dist[110][110];
	memset(dist, -1, sizeof dist);
	dist[x][y] = 0;
	queue<nd> q;
	q.push({x, y});
	while (!q.empty())
	{
		int x = q.front().x, y = q.front().y;
		q.pop();
		for (int i = 0; i < 8; i++)
		{
			int nx = x + d[i][0], ny = y + d[i][1];
			if (nx <= n && nx > 0 && ny <= m && ny > 0 && dist[nx][ny] == -1)
			{
				dist[nx][ny] = dist[x][y] + 1;
				q.push({nx, ny});
			}
		}
	}
	for (int i = 1; i < max(n, m); i++)
	{
		for (int j = 1; j <= n; j++)
		{
			for (int k = 1; k <= m; k++)
			{
				if (mp2[j][k] == bomb && dist[j][k] <= i)
				{
					mp2[j][k] = walk;
				}
			}
		}
//		cout << i << "\n";
		output2();
		Sleep(20);
		system("cls");
	}
	output2();
}

int main()
{
	bool flag = 1;
	while (flag)
	{
		cout << "请输入地图的长和宽，以及地雷的数量。\n注意，长和宽必须在 5 和 99 之间，\n地雷的数量必须在 5 和地图面积的一半之间\n"; 
		cin >> n >> m >> b;
		if (n >= 5 && n <= 99 && m >= 5 && m <= 99 && b >= 5 && b <= n*m/2)
		{
			flag = 0;
		}
		else
		{
			cout << "输入不合法！！请重新输入。";
			Sleep(2000);
			system("cls"); 
		}
	}
	memset(mp1, unopen, sizeof mp1);
	memset(mp2, empty, sizeof mp2);
	flag = 1;
	int x, y;
	while (flag)
	{
		output1();
		cout << "请输入要翻开的格子的位置（先行后列）:\n";
		cin >> x >> y;
		if (x > 0 && x <= n && y > 0 && y <= m)
		{
			flag = 0;
		}
		else
		{
			cout << "该位置不在地图内！请重新输入。";
			Sleep(2000);
			system("cls"); 
		}
	}
	start(n, m, b, x, y);
	while (!win())
	{
		system("cls");
		int x, y;
		char c;
		output1();
		cout << "如果你想要翻开一个格子，请输入 o\n如果你想标记一颗地雷，请输入 b\n";
		cin >> c;
		if (c == 'o')
		{
			cout << "请输入要翻开的格子的位置（先行后列）:\n";
			cin >> x >> y;
			if (x > 0 && x <= n && y > 0 && y <= m)
			{
				if (mp1[x][y] == unopen || mp1[x][y] == findit)
				{
					if (open(x, y))//touch the bomb
					{
						system("cls");
						cout << "你踩到雷了！！\n"; 
						bombing(x, y);
						Sleep(10000); 
						return 0;
					}
				}
				else
				{
					cout << "该位置已翻开！！\n";
					Sleep(2000);
				}
			}
			else
			{
				cout << "该位置不在地图内！请重新输入。";
				Sleep(2000);
			}
		}
		else if (c == 'b')
		{
			cout << "请输入地雷位置（先行后列）:\n";
			cin >> x >> y; 
			if (x > 0 && x <= n && y > 0 && y <= m)
			{
				if (mp1[x][y] == unopen)
				{
					findbomb(x, y);
				}
				else
				{
					cout << "该位置已翻开或已标记！！\n";
					Sleep(2000);
				}
			}
			else
			{
				cout << "该位置不在地图内！请重新输入。";
				Sleep(2000) ; 
			}
		}
		else
		{
			cout << "请输入 o 或 b ！！";
			Sleep(2000);
		}
	}
	system("cls");
	cout << "恭喜你赢了！！！\n";
	output2();
	Sleep(10000);
	return 0;
}
