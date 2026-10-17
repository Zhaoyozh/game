#include <bits/stdc++.h>
#include <windows.h>
using namespace std;
const int sz = 19;
char mp[sz][sz];
int d[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
bool vis[sz][sz];
int white = 0, black = 0;

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
}

int df(int x, int y, char color)
{
	int cnt = 0;
	for (int i = 0; i < 4; i++)
	{
		int nx = x + d[i][0], ny = y + d[i][1];
		if (nx >= 0 && nx < sz && 
			ny >= 0 && ny < sz && 
			!vis[nx][ny])
		{
			vis[nx][ny] = 1;
			if (mp[nx][ny] == '.')
			{
				cnt++;
			}
			else if (mp[nx][ny] == color)
			{
				cnt += df(nx, ny, color);
			}
		}
	}
	return cnt;
}

int dfs(int x, int y, char color)
{
	memset(vis, 0, sizeof vis);
	return df(x, y, color);
}

void dfs2(int x, int y, char color) 
{
	if (color == 'o')
	{
		white++;
	}
	else
	{
		black++;
	}
	for (int i = 0; i < 4; i++)
	{
		int nx = x + d[i][0], ny = y + d[i][1];
//		cout << nx << " " << ny << "*\n";
		if (nx >= 0 && nx < sz && 
			ny >= 0 && ny < sz && 
			mp[nx][ny] == color)
		{
//			cout << nx << " " << ny << "^\n";
			mp[nx][ny] = '.';
			dfs2(nx, ny, color);
		}
	}
}

bool eat(char color)
{
	bool flag = 0;
	for (int i = 0; i < sz; i++)
	{
		for (int j = 0; j < sz; j++)
		{
			if (mp[i][j] != color)
			{
				continue;
			}
			if (!dfs(i, j, mp[i][j]))
			{
				flag = 1;
				dfs2(i, j, mp[i][j]);
				mp[i][j] = '.';
			}
		}
	}
	return flag;
}

bool check(char color)
{
	for (int i = 0; i < sz; i++)
	{
		for (int j = 0; j < sz; j++)
		{
			if (dfs(i, j, color))
			{
				return 0;
			}
		}
	}
	return 1;
}

int main()
{
	for (int i = 0; i < sz; i++)
	{
		for (int j = 0; j < sz; j++)
		{
			mp[i][j] = '.';
		}
	}
	int x, y;
	for (int i = 0; i < sz * sz; i++)
	{
		if (check(i % 2? 'o': '@'))
		{
			break;
		}
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
		cout << "请输入落子位置：（x, y）";
		cin >> x >> y;
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
			mp[x][y] = (i % 2? 'o': '@');
			if(!dfs(x, y, (i % 2? 'o': '@')) && !eat((i % 2? '@': 'o')))
			{
				i--;
				cout << "请重新输入！该位置不合法！";
				Sleep(2000); 
			}
			else
			{
				eat('@'), eat('o');
			}
		}
		system("cls");
	}
	output();
	printf("白棋吃掉了 %d 颗黑棋，\n黑棋吃掉了 %d 颗白棋，\n%s棋胜利！！", black, white, (black > white? "白": "黑"));
	Sleep(10000);
	return 0;
}
