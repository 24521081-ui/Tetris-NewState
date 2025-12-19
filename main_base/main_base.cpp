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

class TetrisGame {
private:
    RenderWindow window;
    Font font;
    Texture bgTexture;
    Sprite bgSprite;
    bool hasBackground;

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

    // ===== PAUSE GAME =====
    bool isPaused;   // true = game đang pause

    SoundBuffer bufMove, bufRotate, bufDrop, bufLine, bufGameOver;
    Sound sMove, sRotate, sDrop, sLine, sGameOver;
    Music bgMusic;
    bool isMuted = false;

    Color getSfmlColor(char c, int alpha = 255) {
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

    void loadHighScore() {
        ifstream f("highscore.txt");
        if (f.is_open()) { f >> highScore; f.close(); }
        else highScore = 0;
    }

    void saveHighScore() {
        ofstream f("highscore.txt");
        if (f.is_open()) { f << highScore; f.close(); }
    }

    void initBoard() {
        for (int i = 0; i < H; i++)
            for (int j = 0; j < W; j++)
                board[i][j] = (i == H - 1 || j == 0 || j == W - 1) ? '#' : ' ';
    }

    void generateNextData() {
        int b = rand() % 8;
        if (b == 2) b = 0;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                nextBlock[i][j] = BLOCK_SHAPES[b][i][j];
    }

    void spawnBlock() {
        memcpy(currentBlock, nextBlock, sizeof(currentBlock));
        x = W / 2 - 2;
        y = 0;
        generateNextData();
    }

    bool canMove(int dx, int dy) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (currentBlock[i][j] != ' ')
                    if (board[y + i + dy][x + j + dx] != ' ')
                        return false;
        return true;
    }

    void rotateBlock() {
        char tmp[4][4];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                tmp[i][j] = currentBlock[3 - j][i];

        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (tmp[i][j] != ' ' && board[y + i][x + j] != ' ')
                    return;

        memcpy(currentBlock, tmp, sizeof(tmp));
    }

    void lockBlock() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (currentBlock[i][j] != ' ')
                    board[y + i][x + j] = currentBlock[i][j];
    }

    void removeLine() {
        int lines = 0;
        for (int i = H - 2; i > 0; i--) {
            bool full = true;
            for (int j = 1; j < W - 1; j++)
                if (board[i][j] == ' ') full = false;

            if (full) {
                lines++;
                for (int k = i; k > 0; k--)
                    for (int j = 1; j < W - 1; j++)
                        board[k][j] = board[k - 1][j];
                i++;
            }
        }
        if (lines) {
            if (!isMuted) sLine.play();
            score += lines * 100;
            if (score > highScore) highScore = score;
        }
    }

    // ===== PAUSE GAME =====
    void drawPauseScreen() {
        RectangleShape overlay(Vector2f(window.getSize()));
        overlay.setFillColor(Color(0, 0, 0, 180));
        window.draw(overlay);

        Text t;
        t.setFont(font);
        t.setString("PAUSED");
        t.setCharacterSize(50);
        t.setFillColor(Color::Yellow);
        t.setStyle(Text::Bold);

        FloatRect r = t.getLocalBounds();
        t.setOrigin(r.left + r.width / 2, r.top + r.height / 2);
        t.setPosition(window.getSize().x / 2,
                      window.getSize().y / 2);
        window.draw(t);
    }

public:
    TetrisGame() {
        window.create(VideoMode(LEFT_UI_WIDTH + W * CELL_SIZE + RIGHT_UI_WIDTH,
                                H * CELL_SIZE),
                      "GROUP 10 - TETRIS");
        window.setFramerateLimit(60);

        font.loadFromFile("arial.ttf");

        speedMs = 300;
        score = 0;
        timer = 0;
        gameOver = false;

        // ===== PAUSE GAME =====
        isPaused = false;

        loadHighScore();
        initBoard();
        generateNextData();
        spawnBlock();

        bgMusic.openFromFile("assets/sound/bgm.ogg");
        bgMusic.setLoop(true);
        bgMusic.setVolume(40);
        bgMusic.play();
    }

    void run() {
        while (window.isOpen()) {
            float time = clock.restart().asSeconds();
            timer += time * 1000;

            Event e;
            while (window.pollEvent(e)) {
                if (e.type == Event::Closed) window.close();

                if (e.type == Event::KeyPressed) {

                    // ===== PAUSE GAME =====
                    if (e.key.code == Keyboard::P && !gameOver) {
                        isPaused = !isPaused;

                        if (isPaused) bgMusic.pause();
                        else {
                            bgMusic.play();
                            clock.restart(); // tránh block rơi ngay
                        }
                    }

                    if (!gameOver && !isPaused) {
                        if (e.key.code == Keyboard::Left) x--;
                        if (e.key.code == Keyboard::Right) x++;
                        if (e.key.code == Keyboard::Down) y++;
                        if (e.key.code == Keyboard::Up) rotateBlock();
                    }
                }
            }

            // ===== PAUSE GAME =====
            if (!gameOver && !isPaused && timer > speedMs) {
                if (canMove(0, 1)) y++;
                else {
                    lockBlock();
                    removeLine();
                    spawnBlock();
                }
                timer = 0;
            }

            window.clear(Color(20, 20, 30));

            for (int i = 0; i < H; i++)
                for (int j = 0; j < W; j++)
                    if (board[i][j] != ' ')
                        window.draw(RectangleShape());

            if (isPaused) drawPauseScreen();

            window.display();
        }
    }
};

int main() {
    srand((unsigned)time(0));
    TetrisGame game;
    game.run();
    return 0;
}
