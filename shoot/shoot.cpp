#include <bits/stdc++.h>
#include <windows.h>
#include <conio.h>
using namespace std;
const int enemy_max = 15, opposite = 2000, fps = 60, start_blood = 10, shoot_time = 2500, map_size = 21, start_bullet = 50, bullet_max = 1000, state_max = 3, boss_time = 60000, boss_max_blood = 5, boss_hit = 3; 

void cls2(void){
    COORD coord = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD counter;
    FillConsoleOutputCharacterA(GetStdHandle(STD_OUTPUT_HANDLE), ' ',
        csbi.dwSize.X * csbi.dwSize.Y, coord, &counter);
    FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
        csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, coord, &counter);
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void cls(){
	HANDLE Hconsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD coordScreen = {0, 0};
	SetConsoleCursorPosition(Hconsole, coordScreen);
}

int mp[map_size][map_size];
int shooter_x, shooter_y, blood, counter, score, bullet, state, state_time, nstate, shooter_row, enemy_row, boss_wait, boss_blood, now_hit;
bool boss;
struct enemy
{
	int x, y, shoot_light, tm, blood;
};
vector <enemy> enemies;

struct p
{
	int x, y;
}bossxy;

p empty_xy()
{
	int x = rand() % map_size, y = rand() % map_size;
	while (mp[x][y] != -1)
	{
		x = rand() % map_size, y = rand() % map_size;
	}
	return {x, y};
}

void kill_enemy(int x)
{
	if (now_hit == x)
	{
		now_hit = -1;
	}
	else if (now_hit > x)
	{
		now_hit--;
	}
	mp[enemies[x].x][enemies[x].y] = -1;
	enemies.erase(enemies.begin()+x);
	int id = 0;
	for (auto i : enemies)
	{
		mp[i.x][i.y] = id;
		id++;
	}
	counter--;
	score++;
	int num = rand() % start_bullet;
	p point = empty_xy();
	mp[point.x][point.y] = -num-10;
	if (state == 3)
	{
		score++;
	}
}

void clean_bit(int x, int y)
{
	if (mp[x][y] <= -1 || mp[x][y] > enemy_max)
	{
		mp[x][y] = -1;
	}
}

void clean_all()
{
	memset(mp, -1, sizeof mp);
	enemies.clear();
	counter = 0;
	state = -1;
}

void kill_all()
{
	while (!enemies.empty())
	{
		kill_enemy(0);
	}
}

void start()
{
	memset(mp, -1, sizeof mp);
	cout << "上下左右对应wsad，空格对应射击\nA代表敌人，O代表正在瞄准的敌人\nk表示秒杀全场的技能\nb表示血包\ns表示双倍得分\nl表示无限子弹\n$表示子弹包\n\n按任意键继续......" << endl;
	while (!_kbhit()){cls2();}
	getch();
	cls2();
	cls2();
	cls2();
	shooter_x = map_size / 2;
	shooter_y = map_size / 2;
	blood = start_blood;
	counter = 0;
	score = 0;
	bullet = start_bullet;
	state = -1;
	nstate = -1;
	boss = 0;
	boss_wait = boss_time;
	now_hit = -1;
	shooter_row = 1;
	enemy_row = 1;
}

void new_enemy(int x, int y)
{
	enemies.push_back({x, y, 0, 0, enemy_row});
	mp[x][y] = enemies.size()-1;
	counter++;
}

void output()
{
	for (int i = 0; i < map_size; i++)
	{
		for (int j = 0; j < map_size; j++)
		{
			if (mp[i][j] > -1 && mp[i][j] < enemy_max)
			{
				switch (enemies[mp[i][j]].shoot_light)
				{
					case 0:
						cout << "A";
						break;
					case 1:
						cout << "o";
						break;
					case 2:
						cout << "O";
						break;
					case 3:
						cout << "*";
						break;
					case 4:
						cout << "#";
						break;
				}
			}
			else if (mp[i][j] == enemy_max)
			{
				cout << "W";
			}
			else if (mp[i][j] > enemy_max)
			{
				if (mp[i][j] == enemy_max + 1)
				{
					cout << "K";
				}
				else if (mp[i][j] == enemy_max + 2)
				{
					cout << "B";
				}
				else if (mp[i][j] == enemy_max + 3)
				{
					cout << "S";
				}
				else if (mp[i][j] == enemy_max + 4)
				{
					cout << "L";
				}
			}
			else if (mp[i][j] < -1)
			{
				cout << "%";
			}
			else if (shooter_x == i && shooter_y == j)
			{
				cout << "+";
			}
			
			else if (shooter_x == i-1 && shooter_y == j)
			{
				cout << "|";
			}
			
			else if (shooter_x == i+1 && shooter_y == j)
			{
				cout << "|";
			}
			
			else if (shooter_x == i && shooter_y == j-1)
			{
				cout << "-";
			}
			
			else if (shooter_x == i && shooter_y == j+1)
			{
				cout << "-";
			}
			else if (boss && abs(bossxy.x-i) + abs(bossxy.y-j) <= 2)
			{
				cout << "#";
			}
			else
			{
				cout << ".";
			}
		}
		cout << "                  \n";
	}
	cout << "剩余血量：";
	for (int i = 0; i < blood; i++)
	{
		cout << "#";
	}
	cout << "                           \n子弹数量：";
	if (state == 4)
	{
		cout << "无限"; 
	}
	else
	{
		cout << bullet;
	}
	cout << "\n敌人数量：";
	cout << enemies.size();
	if (!boss)
	{
		cout << "\nboss还有 " << 0.001 * boss_wait << " 秒到达            ";
	}
	else
	{
		cout << "\nboss血量：";
		for (int i = 0; i < boss_blood; i++)
		{
			cout << "#";
		}
		cout << "                           ";
	}
	cout << "\n得分：";
	cout << score << endl;
//	cout << enemy_row << " " << shooter_row << "\n";
	if (state == 3)
	{
		cout << "双倍得分，剩余 ";
		cout << 0.001 * state_time << " 秒。           " << endl;
	}
	if (state == 4)
	{
		cout << "无限子弹，剩余 ";
		cout << 0.001 * state_time << " 秒。          " << endl;
	}
	if (now_hit == -1)
	{
		return ;
	}
	cout << "当前敌人剩余血量：";
	if (now_hit == enemy_max)
	{
		cout << boss_blood << "      " << endl;
	}
	else
	{
		cout << enemies[now_hit].blood  << "      "<< endl;
	}
	cout << "                   \n                    \n" << endl;
}

void shoot(int x, int y)
{
	if (state != 4)
	{
		if (bullet <= 0)
		{
			return ;
		}
		bullet--;
	}
	if (mp[x][y] > -1 && mp[x][y] < enemy_max)
	{
		enemies[mp[x][y]].blood -= shooter_row;
		now_hit = mp[x][y];
		if (enemies[mp[x][y]].blood <= 0)
		{
			kill_enemy(mp[x][y]);
		}
	}
	else if (mp[x][y] < -1)
	{
		bullet += -mp[x][y] - 10;
		mp[x][y] = -1;
		bullet = min(bullet, bullet_max);
	}
	else if (boss && abs(bossxy.x-x) + abs(bossxy.y-y) <= 2)
	{
		boss_blood -= shooter_row;
		if (boss_blood <= 0)
		{
			boss = 0;
			clean_all();
			cls2();
			score += 5;
			enemy_row++;
			now_hit = -1;
		}
		else
		{
			now_hit = enemy_max;
		}
	}
	else if (mp[x][y] > enemy_max)
	{
		if (mp[x][y] == enemy_max + 1)
		{
			kill_all();
		}
		else if (mp[x][y] == enemy_max + 2)
		{
			blood += rand() % 6;
		}
		else if (mp[x][y] == enemy_max + 3)
		{
			state = 3;
			state_time = 10000.0;
		}
		else if (mp[x][y] == enemy_max + 4)
		{
			state = 4;
			state_time = 15000.0;
		}
		mp[x][y] = -1;
	}
}

void be_shooted()
{
	cls2();
	for (int i = 0; i < map_size; i++)
	{
		for (int j = 0; j < map_size; j++)
		{
			cout << "#";
		}
		cout << endl;
	}
	Sleep(500);
	cls2();
	blood--;
}

void lose()
{
	system("cls");
	cout << "#  #     ## \n# #     #  #\n##     #   #\n# #    #  # \n#  # #  ## #\n" << endl;
	Sleep(5000);
	return ; 
}

int main()
{
	system("cls");
	ios_base::sync_with_stdio(0);
	cin.tie(0ll), cout.tie(0ll);
	srand((unsigned) time (NULL));
	start();
	int new_time = opposite;
	while (1)
	{
		if (_kbhit())
		{
			char c = getch();
			if (c == 72)
			{
				if (shooter_x != 0)
				{
					shooter_x--;
				}
			}
			else if (c == 75)
			{
				if (shooter_y != 0)
				{
					shooter_y--;
				}
			}
			else if (c == 80)
			{
				if (shooter_x != map_size-1)
				{
					shooter_x++;
				}
			}
			else if (c == 77)
			{
				if (shooter_y != map_size-1)
				{
					shooter_y++;
				}
			}
			else if (c == ' ')
			{
				shoot(shooter_x, shooter_y);
			}
		}
		if (counter < enemy_max)
		{
			if (new_time <= 0)
			{
				p point1 = empty_xy();
				new_enemy(point1.x, point1.y);
				new_time = opposite;
			}
			else
			{
				new_time -= 1000.0 / fps;
			}
		}
		if (state_time <= 0)
		{
			state = -1;
		}
		else
		{
			state_time -= 1000.0 / fps;
		}
		if (nstate <= 0)
		{
			int kind = rand() % 4 + 1;
			p point = empty_xy();
			mp[point.x][point.y] = enemy_max + kind;
			nstate = rand() % 10000;
		}
		else
		{
			nstate -= 1000.0 / fps;
		}
		for (auto &i : enemies)
		{
			if (i.shoot_light == 3)
			{
				cls2();
				be_shooted();
				i.shoot_light = 0;
				i.tm = 0;
			}
			else if (i.shoot_light == 2 && i.tm <= 0)
			{
				i.shoot_light = 3;
				i.tm = shoot_time;
			}
			else if (i.shoot_light == 1 && i.tm <= 0)
			{
				i.shoot_light = 2;
				i.tm = shoot_time;
			}
			else if (i.shoot_light == 0)
			{
				int rd = rand() % 1000;
				if (rd < 50)
				{
					if (i.x > 0 && mp[i.x-1][i.y] == -1)
					{
						int id = mp[i.x][i.y];
						mp[i.x][i.y] = -1;
						i.x--;
						mp[i.x][i.y] = id;
					}
				}
				else if (rd < 100)
				{
					if (i.x < map_size-1 && mp[i.x+1][i.y] == -1)
					{
						int id = mp[i.x][i.y];
						mp[i.x][i.y] = -1;
						i.x++;
						mp[i.x][i.y] = id;
					}
				}
				else if (rd < 150)
				{
					if (i.y > 0 && mp[i.x][i.y-1] == -1)
					{
						int id = mp[i.x][i.y];
						mp[i.x][i.y] = -1;
						i.y--;
						mp[i.x][i.y] = id;
					}
				}
				else if (rd < 200 && mp[i.x][i.y+1] == -1)
				{
					if (i.y < map_size-1)
					{
						int id = mp[i.x][i.y];
						mp[i.x][i.y] = -1;
						i.y++;
						mp[i.x][i.y] = id;
					}
				}
				else if (rd < 210)
				{
					i.shoot_light = 1;
				}
			}
			else
			{
				i.tm -= 1000.0 / fps;
			}
		}
		if (boss_wait > 0 && !boss)
		{
			boss_wait -= 1000.0 / fps; 
		}
		else if (!boss)
		{
			boss_wait = boss_time;
			boss = 1, boss_blood = boss_max_blood * enemy_row;
			bossxy = empty_xy();
		}
		else
		{
			int rd = rand() % 1000;
			if (rd < 3)
			{
				blood -= boss_hit;
			}
			else if (rd < 15)
			{
				mp[bossxy.x][bossxy.y] = -1;
				bossxy = empty_xy();
				mp[bossxy.x][bossxy.y] = enemy_max;
			}
		}
		if (score >= shooter_row * 50)
		{
			score -= shooter_row * 50;
			shooter_row++;
		}
		cls();
		output();
		Sleep(1000.0 / fps);
		if (blood <= 0)
		{
			lose();
			break;
		}
	}
	return 0;
}
