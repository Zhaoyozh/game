#include <bits/stdc++.h>
#include <windows.h>
#include <conio.h>
using namespace std;
double n;

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

int main()
{
	system("cls"); 
	ios_base::sync_with_stdio(0);
	cin.tie(0ll), cout.tie(0ll);
	srand((unsigned) time (NULL));
	n = 0;
	while (n < 20.0)
	{
		n += rand() * 1.0 / (1 << 18);
		cout << "更新中······\n";
		for (int i = 1; i <= n; i++)
		{
			cout << "#";
		}
		cout << " " << n * 5.0 << "%" << endl;
		Sleep(20);
		cls();
	}
	system("cls");
	cout << "更新完毕！！" ;
}
