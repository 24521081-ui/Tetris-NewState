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

    sf::SoundBuffer bufMove, bufRotate, bufDrop, bufLine, bufGameOver;
    sf::Sound sMove, sRotate, sDrop, sLine, sGameOver;
    sf::Music bgMusic;

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
                if (i == H - 1 || j == 0 || j == W - 1)
                    board[i][j] = '#';
                else
                    board[i][j] = ' ';
    }

    void generateNextData() {
        int b = rand() % 8;
        if (b == 2) b = 0;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                nextBlock[i][j] = BLOCK_SHAPES[b][i][j];
    }

    void spawnBlock() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                currentBlock[i][j] = nextBlock[i][j];

        x = W / 2 - 2;
        y = 0;
        generateNextData();
    }

    bool canMove(int dx, int dy) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (currentBlock[i][j] != ' ') {
                    int tx = x + j + dx;
                    int ty = y + i + dy;
                    if (tx < 0 || tx >= W || ty < 0 || ty >= H) return false;
                    if (board[ty][tx] != ' ') return false;
                }
            }
        }
        return true;
    }

    void rotateBlock() {
        char temp[4][4];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                temp[i][j] = currentBlock[3 - j][i];

        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                if (temp[i][j] != ' ') {
                    int tx = x + j;
                    int ty = y + i;
                    if (tx < 0 || tx >= W || ty >= H || board[ty][tx] != ' ') return;
                }

        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                currentBlock[i][j] = temp[i][j];
    }

    void lockBlock() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (currentBlock[i][j] != ' ')
                    board[y + i][x + j] = currentBlock[i][j];
    }

    void removeLine() {
        int linesCleared = 0;
        for (int i = H - 2; i > 0; i--) {
            bool full = true;
            for (int j = 1; j < W - 1; j++)
                if (board[i][j] == ' ') { full = false; break; }

            if (full) {
                linesCleared++;
                for (int ii = i; ii > 0; ii--)
                    for (int jj = 1; jj < W - 1; jj++)
                        board[ii][jj] = board[ii - 1][jj];
                for (int jj = 1; jj < W - 1; jj++) board[1][jj] = ' ';
                i++;
            }
        }
        if (linesCleared > 0) {
            if (!isMuted) sLine.play();
            score += linesCleared * 100;
            if (score > highScore) highScore = score;
            if (score % 500 == 0 && speedMs > MIN_SPEED) speedMs -= 20;
        }
    }

    int getGhostY() {
        int originalY = y;
        while (canMove(0, 1)) y++;
        int ghostY = y;
        y = originalY;
        return ghostY;
    }

    bool checkGameOver() {
        return !canMove(0, 0);
    }


    void drawTile(int gridX, int gridY, char type, int alpha = 255, bool isGhost = false) {
        if (type == ' ') return;

        RectangleShape rect(Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
        rect.setPosition(OFFSET_X + gridX * CELL_SIZE + 1, gridY * CELL_SIZE + 1);

        Color c = getSfmlColor(type, alpha);

        if (isGhost) {
            rect.setFillColor(Color::Transparent);
            rect.setOutlineThickness(2);
            rect.setOutlineColor(Color(c.r, c.g, c.b, 150));
        }
        else {
            rect.setFillColor(c);
            rect.setOutlineThickness(1);
            rect.setOutlineColor(Color(255, 255, 255, 100));
        }
        window.draw(rect);
    }

    void drawKeyGuide(float px, float py, string key, string description) {
        RectangleShape keyRect(Vector2f(40, 40));
        keyRect.setPosition(px, py);
        keyRect.setFillColor(Color(50, 50, 50, 200));
        keyRect.setOutlineThickness(2);
        keyRect.setOutlineColor(Color(150, 150, 150, 200));

        Text keyText;
        keyText.setFont(font);
        keyText.setString(key);
        keyText.setCharacterSize(20);
        keyText.setFillColor(Color::Yellow);

        FloatRect textBounds = keyText.getLocalBounds();
        keyText.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f);
        keyText.setPosition(px + 20, py + 20);

        Text descText;
        descText.setFont(font);
        descText.setString(description);
        descText.setCharacterSize(18);
        descText.setFillColor(Color::White);
        descText.setPosition(px + 55, py + 8);

        window.draw(keyRect);
        window.draw(keyText);
        window.draw(descText);
    }

    void drawNextBlockPreview(float startX, float startY) {
        Text nextText;
        nextText.setFont(font);
        nextText.setString("Next Block:");
        nextText.setCharacterSize(22);
        nextText.setFillColor(Color::Cyan);
        nextText.setPosition(startX, startY);
        window.draw(nextText);

        RectangleShape box(Vector2f(4 * CELL_SIZE, 4 * CELL_SIZE));
        box.setPosition(startX, startY + 30);
        box.setFillColor(Color(0, 0, 0, 100));
        box.setOutlineThickness(1);
        box.setOutlineColor(Color(100, 100, 100));
        window.draw(box);

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (nextBlock[i][j] != ' ') {
                    RectangleShape rect(Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
                    rect.setPosition(startX + j * CELL_SIZE + 1, (startY + 30) + i * CELL_SIZE + 1);
                    rect.setFillColor(getSfmlColor(nextBlock[i][j]));
                    rect.setOutlineThickness(1);
                    rect.setOutlineColor(Color(255, 255, 255, 100));
                    window.draw(rect);
                }
            }
        }
    }

    void drawUI() {

        float leftX = 15;


        Text groupName;
        groupName.setFont(font);
        groupName.setString("GROUP 10 - TETRIS");
        groupName.setCharacterSize(22);
        groupName.setFillColor(Color(255, 0, 255));
        groupName.setStyle(Text::Bold);
        groupName.setPosition(leftX, 40);

        Text groupShadow = groupName;
        groupShadow.setFillColor(Color(50, 0, 50));
        groupShadow.setPosition(leftX + 2, 42);

        window.draw(groupShadow);
        window.draw(groupName);

        Text lblGuide;
        lblGuide.setFont(font);
        lblGuide.setString("CONTROLS");
        lblGuide.setCharacterSize(20);
        lblGuide.setFillColor(Color(200, 200, 200));
        lblGuide.setStyle(Text::Bold);
        lblGuide.setPosition(leftX, 100);
        window.draw(lblGuide);

        float guideY = 140;
        float gap = 55;
        drawKeyGuide(leftX, guideY + gap * 0, "W", "Rotate");
        drawKeyGuide(leftX, guideY + gap * 1, "A", "Move Left");
        drawKeyGuide(leftX, guideY + gap * 2, "D", "Move Right");
        drawKeyGuide(leftX, guideY + gap * 3, "S", "Soft Drop");
        drawKeyGuide(leftX, guideY + gap * 4, "Q", "Quit Game");
        drawKeyGuide(leftX, guideY + gap * 5, "M", "Mute/Unmute");


        float rightGap = 35;
        float rightX = OFFSET_X + (W * CELL_SIZE) + rightGap;

        Text title;
        title.setFont(font);
        title.setString("STATISTICS");
        title.setCharacterSize(25);
        title.setFillColor(Color(0, 255, 255));
        title.setStyle(Text::Bold);
        title.setPosition(rightX, 30);
        window.draw(title);

        Text txtScore, txtHigh;
        txtScore.setFont(font); txtScore.setCharacterSize(22); txtScore.setFillColor(Color::White);
        txtHigh.setFont(font);  txtHigh.setCharacterSize(22);  txtHigh.setFillColor(Color(200, 200, 200));

        txtScore.setString("Score: " + to_string(score));
        txtScore.setPosition(rightX, 80);

        txtHigh.setString("Best : " + to_string(highScore));
        txtHigh.setPosition(rightX, 110);

        window.draw(txtScore);
        window.draw(txtHigh);

        RectangleShape line(Vector2f(200, 2));
        line.setPosition(rightX, 150);
        line.setFillColor(Color(100, 100, 100, 180));
        window.draw(line);

        drawNextBlockPreview(rightX, 170);
    }

    void drawGameOver() {
        int wWidth = window.getSize().x;
        int wHeight = window.getSize().y;

        RectangleShape gameOverOverlay(Vector2f(wWidth, wHeight));
        gameOverOverlay.setFillColor(Color(0, 0, 0, 200));
        window.draw(gameOverOverlay);

        Text overText;
        overText.setFont(font);
        overText.setString("GAME OVER");
        overText.setCharacterSize(50);
        overText.setFillColor(Color::Red);
        overText.setStyle(Text::Bold);
        overText.setOutlineThickness(2);
        overText.setOutlineColor(Color::White);

        FloatRect textRect = overText.getLocalBounds();
        overText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
        overText.setPosition(wWidth / 2.0f, wHeight / 2.0f);
        window.draw(overText);

        Text quitText;
        quitText.setFont(font);
        quitText.setString("Press Q to Quit");
        quitText.setCharacterSize(20);
        quitText.setFillColor(Color::White);
        FloatRect qRect = quitText.getLocalBounds();
        quitText.setOrigin(qRect.left + qRect.width / 2.0f, qRect.top + qRect.height / 2.0f);
        quitText.setPosition(wWidth / 2.0f, wHeight / 2.0f + 50);
        window.draw(quitText);
    }

public:

    TetrisGame() {
        int windowWidth = LEFT_UI_WIDTH + (W * CELL_SIZE) + RIGHT_UI_WIDTH;
        int windowHeight = H * CELL_SIZE;


        window.create(VideoMode(windowWidth, windowHeight), "Group 10 - Tetris");
        window.setFramerateLimit(60);


        if (!font.loadFromFile("arial.ttf")) {
            cout << "LOI: Khong tim thay file arial.ttf!" << endl;
        }

        hasBackground = bgTexture.loadFromFile("background.jpg");
        if (hasBackground) {
            bgTexture.setSmooth(true);
            bgSprite.setTexture(bgTexture);
            Vector2u textureSize = bgTexture.getSize();
            float scaleX = (float)windowWidth / textureSize.x;
            float scaleY = (float)windowHeight / textureSize.y;
            bgSprite.setScale(scaleX, scaleY);
            bgSprite.setColor(Color(150, 150, 150, 255));
        }


        speedMs = 300;
        score = 0;
        gameOver = false;
        timer = 0;
        loadHighScore();
        initBoard();
        generateNextData();
        spawnBlock();

        bufMove.loadFromFile("assets/sound/move.wav");
        bufRotate.loadFromFile("assets/sound/rotate.wav");
        bufDrop.loadFromFile("assets/sound/drop.wav");
        bufLine.loadFromFile("assets/sound/line.wav");
        bufGameOver.loadFromFile("assets/sound/gameover.wav");

        sMove.setBuffer(bufMove);
        sRotate.setBuffer(bufRotate);
        sDrop.setBuffer(bufDrop);
        sLine.setBuffer(bufLine);
        sGameOver.setBuffer(bufGameOver);

        sMove.setVolume(50);
        sRotate.setVolume(60);
        sDrop.setVolume(70);
        sLine.setVolume(80);
        sGameOver.setVolume(90);

        bgMusic.openFromFile("assets/sound/bgm.ogg");
        bgMusic.setLoop(true);
        bgMusic.setVolume(40);
        bgMusic.play();
    }


    void run() {
        while (window.isOpen()) {
            float time = clock.getElapsedTime().asSeconds();
            clock.restart();
            timer += time * 1000;

            Event e;
            while (window.pollEvent(e)) {
                if (e.type == Event::Closed) window.close();

                if (e.type == Event::KeyPressed) {
                    if (e.key.code == Keyboard::Escape || e.key.code == Keyboard::Q) {
                        window.close();
                    }

                    if (!gameOver) {
                        if (e.key.code == Keyboard::Up || e.key.code == Keyboard::W) {
                            rotateBlock();
                            if (!isMuted) sRotate.play();
                        }
                        else if (e.key.code == Keyboard::Left || e.key.code == Keyboard::A) {
                            if (canMove(-1, 0)) x--;
                            if (!isMuted) sMove.play();
                        }
                        else if (e.key.code == Keyboard::Right || e.key.code == Keyboard::D) {
                            if (canMove(1, 0)) x++;
                            if (!isMuted) sMove.play();
                        }
                        else if (e.key.code == Keyboard::Down || e.key.code == Keyboard::S) {
                            if (canMove(0, 1)) y++;
                            if (!isMuted) sDrop.play();
                        }
                        if (e.key.code == Keyboard::M) {
                            isMuted = !isMuted;
                            if (isMuted) bgMusic.pause();
                            else bgMusic.play();
                        }
                    }
                }
            }


            if (!gameOver) {
                if (timer > speedMs) {
                    if (canMove(0, 1)) y++;
                    else {
                        lockBlock();
                        removeLine();
                        spawnBlock();
                        if (checkGameOver()) {
                            gameOver = true;
                            saveHighScore();
                            bgMusic.stop();
                            if (!isMuted) sGameOver.play();
                        }
                    }
                    timer = 0;
                }
            }


            window.clear(Color(20, 20, 30));

            if (hasBackground) window.draw(bgSprite);


            RectangleShape playAreaOverlay(Vector2f(W * CELL_SIZE, window.getSize().y));
            playAreaOverlay.setPosition(OFFSET_X, 0);
            playAreaOverlay.setFillColor(Color(0, 0, 0, 150));
            window.draw(playAreaOverlay);


            for (int i = 0; i < H; i++) {
                for (int j = 0; j < W; j++) drawTile(j, i, board[i][j]);
            }


            if (!gameOver) {
                int ghostY = getGhostY();
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 4; j++) {
                        if (currentBlock[i][j] != ' ') {
                            drawTile(x + j, ghostY + i, currentBlock[i][j], 80, true);
                            drawTile(x + j, y + i, currentBlock[i][j]);
                        }
                    }
                }
            }

            drawUI();

            if (gameOver) {
                drawGameOver();
            }

            window.display();
        }
    }
};




int main() {
    srand((unsigned int)time(0));


    TetrisGame game;
    game.run();

    return 0;
}