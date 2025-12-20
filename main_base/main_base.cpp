#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <string>

using namespace std;
using namespace sf;

const int H = 20;
const int W = 15;
const int CELL_SIZE = 30;
const int LEFT_UI_WIDTH = 250;
const int RIGHT_UI_WIDTH = 250;
const int OFFSET_X = LEFT_UI_WIDTH;

const char BLOCK_SHAPES[8][4][4] = {
    {{' ','I',' ',' '}, {' ','I',' ',' '}, {' ','I',' ',' '}, {' ','I',' ',' '}},
    {{' ',' ',' ',' '}, {' ','O','O',' '}, {' ','O','O',' '}, {' ',' ',' ',' '}},
    {{' ',' ',' ',' '}, {'I','I','I','I'}, {' ',' ',' ',' '}, {' ',' ',' ',' '}},
    {{' ',' ',' ',' '}, {' ','T',' ',' '}, {'T','T','T',' '}, {' ',' ',' ',' '}},
    {{' ',' ',' ',' '}, {' ','S','S',' '}, {'S','S',' ',' '}, {' ',' ',' ',' '}},
    {{' ',' ',' ',' '}, {'Z','Z',' ',' '}, {' ','Z','Z',' '}, {' ',' ',' ',' '}},
    {{' ',' ',' ',' '}, {'J',' ',' ',' '}, {'J','J','J',' '}, {' ',' ',' ',' '}},
    {{' ',' ',' ',' '}, {' ',' ','L',' '}, {'L','L','L',' '}, {' ',' ',' ',' '}}
};

struct Particle {
    Vector2f pos;
    Vector2f vel;
    float lifetime;
    Color color;
};

class TetrisGame {
private:
    RenderWindow window;
    Font font;

    char board[H][W];
    char currentBlock[4][4];
    char nextBlock[4][4];
    int x, y;

    int score;
    int highScore;
    int speedMs;
    const int MIN_SPEED = 50;

    Clock clock;
    float timer;

    bool gameOver;
    bool isPaused = false;

    vector<Particle> particles;

    Color getColor(char c, int alpha = 255) {
        switch (c) {
        case 'I': return Color(0, 255, 255, alpha);
        case 'O': return Color(255, 0, 0, alpha);
        case 'T': return Color(255, 255, 0, alpha);
        case 'S': return Color(0, 255, 0, alpha);
        case 'Z': return Color(255, 0, 0, alpha);
        case 'J': return Color(0, 0, 200, alpha);
        case 'L': return Color(255, 127, 0, alpha);
        case '#': return Color(80, 80, 80, alpha);
        default: return Color::White;
        }
    }

    void initBoard() {
        for (int i = 0; i < H; i++)
            for (int j = 0; j < W; j++)
                board[i][j] = (i == H - 1 || j == 0 || j == W - 1) ? '#' : ' ';
    }

    void generateNext() {
        int b = rand() % 8;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                nextBlock[i][j] = BLOCK_SHAPES[b][i][j];
    }

    void spawnBlock() {
        memcpy(currentBlock, nextBlock, sizeof(currentBlock));
        x = W / 2 - 2;
        y = 0;
        generateNext();
    }

    bool canMove(int dx, int dy) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (currentBlock[i][j] != ' ') {
                    int nx = x + j + dx;
                    int ny = y + i + dy;
                    if (board[ny][nx] != ' ') return false;
                }
        return true;
    }

    void rotate() {
        char t[4][4];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                t[i][j] = currentBlock[3 - j][i];

        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (t[i][j] != ' ' && board[y + i][x + j] != ' ')
                    return;

        memcpy(currentBlock, t, sizeof(t));
    }

    void lockBlock() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (currentBlock[i][j] != ' ')
                    board[y + i][x + j] = currentBlock[i][j];
    }

    void clearLines() {
        for (int i = H - 2; i > 0; i--) {
            bool full = true;
            for (int j = 1; j < W - 1; j++)
                if (board[i][j] == ' ') full = false;

            if (full) {
                score += 100;
                for (int ii = i; ii > 0; ii--)
                    for (int jj = 1; jj < W - 1; jj++)
                        board[ii][jj] = board[ii - 1][jj];
                i++;
            }
        }
    }

    void drawTile(int gx, int gy, char c) {
        if (c == ' ') return;
        RectangleShape r(Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
        r.setPosition(OFFSET_X + gx * CELL_SIZE + 1, gy * CELL_SIZE + 1);
        r.setFillColor(getColor(c));
        window.draw(r);
    }

    void drawPause() {
        RectangleShape overlay(Vector2f(window.getSize()));
        overlay.setFillColor(Color(0, 0, 0, 160));
        window.draw(overlay);

        Text txt("PAUSED", font, 50);
        txt.setFillColor(Color::Yellow);
        txt.setPosition(window.getSize().x / 2 - 100, window.getSize().y / 2 - 40);
        window.draw(txt);
    }

public:
    TetrisGame() {
        window.create(VideoMode(LEFT_UI_WIDTH + W * CELL_SIZE + RIGHT_UI_WIDTH, H * CELL_SIZE), "Tetris - Pause Ready");
        window.setFramerateLimit(60);

        font.loadFromFile("arial.ttf");

        speedMs = 300;
        score = 0;
        timer = 0;
        gameOver = false;

        initBoard();
        generateNext();
        spawnBlock();
    }

    void run() {
        while (window.isOpen()) {
            float dt = clock.restart().asSeconds() * 1000;
            if (!isPaused) timer += dt;

            Event e;
            while (window.pollEvent(e)) {
                if (e.type == Event::Closed) window.close();

                if (e.type == Event::KeyPressed) {
                    if (e.key.code == Keyboard::P && !gameOver)
                        isPaused = !isPaused;

                    if (!gameOver && !isPaused) {
                        if (e.key.code == Keyboard::A && canMove(-1, 0)) x--;
                        if (e.key.code == Keyboard::D && canMove(1, 0)) x++;
                        if (e.key.code == Keyboard::S && canMove(0, 1)) y++;
                        if (e.key.code == Keyboard::W) rotate();
                    }
                }
            }

            if (!gameOver && !isPaused && timer > speedMs) {
                if (canMove(0, 1)) y++;
                else {
                    lockBlock();
                    clearLines();
                    spawnBlock();
                    if (!canMove(0, 0)) gameOver = true;
                }
                timer = 0;
            }

            window.clear(Color(30, 30, 40));

            for (int i = 0; i < H; i++)
                for (int j = 0; j < W; j++)
                    drawTile(j, i, board[i][j]);

            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    if (currentBlock[i][j] != ' ')
                        drawTile(x + j, y + i, currentBlock[i][j]);

            if (isPaused) drawPause();

            window.display();
        }
    }
};

int main() {
    srand(time(0));
    TetrisGame game;
    game.run();
    return 0;
}
