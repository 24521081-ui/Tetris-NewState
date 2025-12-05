void setColor(char ch) {
    switch (ch) {
    case 'I': SetConsoleTextAttribute(hConsole, 11); break; 
    case 'O': SetConsoleTextAttribute(hConsole, 14); break; 
    case 'T': SetConsoleTextAttribute(hConsole, 13); break; 
    case 'S': SetConsoleTextAttribute(hConsole, 10); break;
    case 'Z': SetConsoleTextAttribute(hConsole, 12); break; 
    case 'J': SetConsoleTextAttribute(hConsole, 9);  break; 
    case 'L': SetConsoleTextAttribute(hConsole, 6);  break; 
    case '#': SetConsoleTextAttribute(hConsole, 8);  break; 
    default : SetConsoleTextAttribute(hConsole, 7);  break; 
    }
}

void resetColor() {
    SetConsoleTextAttribute(hConsole, 7);
}



void boardDelBlock() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (curBlock[i][j] != ' ') {
                int by = y + i;
                int bx = x + j;
          
                if (by >= 0 && by < H && bx >= 0 && bx < W && board[by][bx] != '#') {
                    board[by][bx] = ' ';
                }
            }
        }
    }
}


void block2Board() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (curBlock[i][j] != ' ') {
                int by = y + i;
                int bx = x + j;
     
                if (by >= 0 && by < H && bx >= 0 && bx < W) {
                    board[by][bx] = curBlock[i][j];
                }
            }
        }
    }
}



bool canPlaceBlockAt(int nx, int ny, const char block[4][4]) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (block[i][j] != ' ') { 
                int tx = nx + j;
                int ty = ny + i;

             
                if (tx < 1 || tx >= W - 1 || ty >= H - 1)
                    return false;

              
                if (ty >= 0 && board[ty][tx] != ' ')
                    return false;
            }
        }
    }
    return true;
}


bool canMove(int dx, int dy) {
    return canPlaceBlockAt(x + dx, y + dy, curBlock);
}


bool handleInput() {
    if (!_kbhit()) return true; 
    int c = _getch();

    
    if (c == 0 || c == 224) {
        int code = _getch();
        if (code == 75) {            
            if (canMove(-1, 0)) x--;
        }
        else if (code == 77) {       
            if (canMove(1, 0)) x++;
        }
        else if (code == 80) {       
            if (canMove(0, 1)) y++;
        }
        else if (code == 72) {       
          
            rotateBlockCW(); 
        }
    } else {
      
        if (c == 'q' || c == 'Q') {
            return false; 
        }
        if (c == ' ') {
           
            while (canMove(0, 1)) {
                y++;
            }
        }
    }

    return true; 
}