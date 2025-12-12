#include <iostream>
#include <conio.h>
#include <windows.h>
using namespace std;

#define H 20
#define W 15

int speedMs = 200;
const int MIN_SPEED = 50;

char board[H][W] = {};

char blocks[][4][4] = {
    {{' ','I',' ',' '},
     {' ','I',' ',' '},
     {' ','I',' ',' '},
     {' ','I',' ',' '}},

    {{' ',' ',' ',' '},
     {' ','O','O',' '},
     {' ','O','O',' '},
     {' ',' ',' ',' '}},

    {{' ',' ',' ',' '},
     {'I','I','I','I'},
     {' ',' ',' ',' '},
     {' ',' ',' ',' '}},

    {{' ',' ',' ',' '},
     {' ','T',' ',' '},
     {'T','T','T',' '},
     {' ',' ',' ',' '}},

    {{' ',' ',' ',' '},
     {' ','S','S',' '},
     {'S','S',' ',' '},
     {' ',' ',' ',' '}},

    {{' ',' ',' ',' '},
     {'Z','Z',' ',' '},
     {' ','Z','Z',' '},
     {' ',' ',' ',' '}},

    {{' ',' ',' ',' '},
     {'J',' ',' ',' '},
     {'J','J','J',' '},
     {' ',' ',' ',' '}},

    {{' ',' ',' ',' '},
     {' ',' ','L',' '},
     {'L','L','L',' '},
     {' ',' ',' ',' '}}
};

int x = 4, y = 0, b = 1;
int score = 0;   // <-- Thêm tính điểm

void gotoxy(int x, int y) {
    COORD c = { x, y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void boardDelBlock() {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (blocks[b][i][j] != ' ' && y + j < H)
                board[y + i][x + j] = ' ';
}

void block2Board() {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (blocks[b][i][j] != ' ')
                board[y + i][x + j] = blocks[b][i][j];
}

void initBoard() {
    for (int i = 0; i < H; i++)
        for (int j = 0; j < W; j++)
            if (i == H - 1 || j == 0 || j == W - 1)
                board[i][j] = '#';
            else
                board[i][j] = ' ';
}

void draw() {
    gotoxy(0, 0);
    cout << "Controls:\n";
    cout << "A: Move Left\n";
    cout << "D: Move Right\n";
    cout << "W: Rotate\n";
    cout << "X: Move Down\n";
    cout << "Q: Quit\n";
    cout << "\nScore: " << score << "\n\n";

    for (int i = 0; i < H; i++, cout << endl)
        for (int j = 0; j < W; j++)
            cout << board[i][j];
}

bool canMove(int dx, int dy) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (blocks[b][i][j] != ' ') {
                int tx = x + j + dx;
                int ty = y + i + dy;

                if (tx < 1 || tx >= W - 1 || ty >= H - 1) return false;
                if (board[ty][tx] != ' ') return false;
            }
    return true;
}

void removeLine() {
    int j;
    int linesCleared = 0;

    for (int i = H - 2; i > 0; i--) {
        for (j = 1; j < W - 1; j++)
            if (board[i][j] == ' ') break;

        if (j == W - 1) {
            linesCleared++;

            for (int ii = i; ii > 0; ii--)
                for (int jj = 1; jj < W - 1; jj++)
                    board[ii][jj] = board[ii - 1][jj];

            for (int jj = 1; jj < W - 1; jj++)
                board[1][jj] = ' ';

            i++;
        }
    }

    score += linesCleared * 100;  // <-- Cộng điểm
}

void rotateBlock() {
    char temp[4][4];
    char rotated[4][4];

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            temp[i][j] = blocks[b][i][j];

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            rotated[i][j] = temp[3 - j][i];

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (rotated[i][j] != ' ') {
                int tx = x + j;
                int ty = y + i;

                if (tx < 1 || tx >= W - 1 || ty >= H - 1)
                    return;

                if (board[ty][tx] != ' ')
                    return;
            }

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            blocks[b][i][j] = rotated[i][j];
}

int main() {
    srand(time(0));
    b = rand() % 8;
    system("cls");
    initBoard();

    while (1) {
        boardDelBlock();

        if (_kbhit()) {
            char c = _getch();
            if (c == 'a' && canMove(-1, 0)) x--;
            if (c == 'd' && canMove(1, 0))  x++;
            if (c == 'x' && canMove(0, 1))  y++;
            if (c == 'w') rotateBlock();
            if (c == 'q') break;
        }

        if (canMove(0, 1)) {
            y++;
        }
        else {
            block2Board();
            removeLine();

            // Tạo block mới
            x = 5;
            y = 0;
            b = rand() % 8;

            // --- GAME OVER ---
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    if (blocks[b][i][j] != ' ' && board[y + i][x + j] != ' ') {
                        system("cls");
                        cout << "GAME OVER!\nYour score: " << score << endl;
                        Sleep(4000);
                        return 0;
                    }
        }

        block2Board();
        draw();
        Sleep(speedMs);
    }
    return 0;
}
