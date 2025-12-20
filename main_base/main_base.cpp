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
    Texture bgTexture;
    Sprite bgSprite;
    bool hasBackground = false;

    char board[H][W];
    char currentBlock[4][4];
    char nextBlock[4][4];
    int x, y;

    int score = 0;
    int highScore = 0;
    int speedMs = 300;
    const int MIN_SPEED = 50;

    Clock clock;
    float timer = 0;
    bool gameOver = false;
    bool isPaused = false;
    bool isMuted = false;

    SoundBuffer bufMove, bufRotate, bufDrop, bufLine, bufGameOver;
    Sound sMove, sRotate, sDrop, sLine, sGameOver;
    Music bgMusic;

    vector<Particle> particles;

    Color getColor(char c, int a = 255) {
        switch (c) {
        case 'I': return Color(0, 255, 255, a);
        case 'O': return Color(255, 0, 0, a);
        case 'T': return Color(255, 255, 0, a);
        case 'S': return Color(0, 255, 0, a);
        case 'Z': return Color(255, 0, 0, a);
        case 'J': return Color(0, 0, 200, a);
        case 'L': return Color(255, 127, 0, a);
        case '#': return Color(80, 80, 80, a);
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

        memcpy(currentBlock, t, sizeof(currentBlock));
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
                for (int k = i; k > 0; k--)
                    for (int j = 1; j < W - 1; j++)
                        board[k][j] = board[k - 1][j];
                score += 100;
                i++;
            }
        }
    }

    void drawPause() {
        RectangleShape bg(Vector2f(window.getSize()));
        bg.setFillColor(Color(0, 0, 0, 150));
        window.draw(bg);

        Text t("PAUSED", font, 50);
        t.setFillColor(Color::Yellow);
        t.setPosition(window.getSize().x / 2 - 100, window.getSize().y / 2 - 40);
        window.draw(t);
    }

public:
    TetrisGame() {
        window.create(VideoMode(LEFT_UI_WIDTH + W * CELL_SIZE + RIGHT_UI_WIDTH, H * CELL_SIZE), "Tetris");
        window.setFramerateLimit(60);

        font.loadFromFile("arial.ttf");
        initBoard();
        generateNext();
        spawnBlock();
    }

    void run() {
        while (window.isOpen()) {
            float dt = clock.restart().asSeconds() * 1000;
            timer += dt;

            Event e;
            while (window.pollEvent(e)) {
                if (e.type == Event::Closed) window.close();
                if (e.type == Event::KeyPressed) {
                    if (e.key.code == Keyboard::Q) window.close();
                    if (e.key.code == Keyboard::P) isPaused = !isPaused;

                    if (!isPaused && !gameOver) {
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
                }
                timer = 0;
            }

            window.clear();
            for (int i = 0; i < H; i++)
                for (int j = 0; j < W; j++)
                    if (board[i][j] != ' ') {
                        RectangleShape r(Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
                        r.setPosition(OFFSET_X + j * CELL_SIZE, i * CELL_SIZE);
                        r.setFillColor(getColor(board[i][j]));
                        window.draw(r);
                    }

            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    if (currentBlock[i][j] != ' ') {
                        RectangleShape r(Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
                        r.setPosition(OFFSET_X + (x + j) * CELL_SIZE, (y + i) * CELL_SIZE);
                        r.setFillColor(getColor(currentBlock[i][j]));
                        window.draw(r);
                    }

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
