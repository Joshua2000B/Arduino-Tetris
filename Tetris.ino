#include "LedControl.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
//#include <cstdlib>

LiquidCrystal_I2C lcd(0x27,16,2);
LedControl lc=LedControl(12,11,10,1);
const int RECV_PIN = 7;
IRrecv irrecv(RECV_PIN);
decode_results results;
unsigned long key_value = 0;

int board[8][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
};
int piece[4][2] = {
  {3,-2},
  {3,-1},
  {4,-2},
  {4,-1},
}, type = 0, rotation = 0;
int spee = 500;
int score = 0, level = 1, counter = 0, lines = 0;
bool gameGoing = true;

byte* convert() { // Converts the board array to an array of bytes for display on the matrix
  byte* rtn = new byte[8];
  int value;
  // 1 1 0 1 1 0 0 0
  // 0 1 2 3 4 5 6 7
  // 1 2 4 8 16 32 64 128
  for(int y = 0; y < 8; y++) {
    value = 0;
    for(int x = 0; x < 8; x++) {
      value *= 2;
      value += board[y][x];
      for(int i = 0; i < 4; ++i) {
        if(piece[i][0] == x && piece[i][1] == y) {
          value += 1;
          break;
        }
      }
    }
    /*
    for(int i = 0; i < 4; ++i) {
      if(piece[i][1] == y) {
        value += (pow(piece[i][0]));
      }
    }*/
    rtn[y] = value;
  }
  return rtn;
}

void setup() {
  randomSeed(analogRead(0));
  Serial.begin(9600);
  lcd.init();
  lcd.backlight(); 
  
//  lcd.setCursor(0,0);
//  lcd.print("LEVEL: ");
//  lcd.setCursor(0,1);
//  lcd.print("SCORE: ");

  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);

  irrecv.enableIRIn();
  irrecv.blink13(true);

}

void loop() {
  if(gameGoing) {
  WRITE(convert());
  updateScreen();

  if(checkForGround()) {
    //Serial.println("In if");
    settlePiece();
    clearLines();
    genNextPiece();
    //spee /= 2;
  }
  else {
    lowerPiece();
  }
  ++counter;
  while(counter % spee != 0) {
    if(irrecv.decode(&results)) { 
      //Serial.println("Recieving signal"); Serial.print(results.value, HEX);
      if(results.value == 0xFFFFFFFF) { results.value = key_value; }
      switch(results.value){
          case 0x20DF1BE4:
          //Serial.println("<");
          if(!checkLeft()) {
            leftShiftPiece();
          }
          break;
          case 0x20DF9B64:
          //Serial.println(">");
          if(!checkRight()) {
            rightShiftPiece();
          }
          break;
          case 0x20DF5BA4:
          //turnPiece();
          break;
      }
      WRITE(convert());
      key_value = results.value;
      irrecv.resume();
    }
    delay(1);
    ++counter;
  }
  delay(1);
}
else {
  lc.clearDisplay(0);
  lcd.setCursor(0,0);
  lcd.print("   GAME OVER");
  lcd.setCursor(0,1);
  lcd.print("          ");
}
}

void updateScreen() {
  lcd.setCursor(0,0);
  lcd.print("LEVEL: ");
  lcd.print(level);
  lcd.setCursor(0,1);
  lcd.print("SCORE: ");
  lcd.print(score);
}

void WRITE(byte* a) {
  for(int i = 0; i < 8; ++i) {
    lc.setRow(0,i,a[i]);
    //Serial.println(a[i]);
  }
  delete[] a;
}

bool checkForGround() {
  for(int i = 0; i < 4; ++i) {
    //Serial.println(piece[i][1]);
    if(piece[i][1] == 7) { return true; }
    else if(piece[i][1] < -1) { continue; }
    else if(board[piece[i][1]+1][piece[i][0]] == 1) { return true; }
  }
  return false;
}

bool checkLeft() {
  for(int i = 0; i < 4; ++i) {
    if(piece[i][0] == 0) { return true; }
    else if(board[piece[i][1]][piece[i][0]-1] == 1) { return true; }
  }
  return false;
}

bool checkRight() {
  for(int i = 0; i < 4; ++i) {
    if(piece[i][0] == 7) { return true; }
    else if(board[piece[i][1]][piece[i][0]+1] == 1) { return true; }
  }
  return false;
}

bool checkTurn(int turn[][2]) {
  for(int i = 0; i < 4; ++i) {
    if(turn[i][0] < 0 || turn[i] > 7) { return true; }
    else if(board[turn[i][1]][turn[i][0]] == 1) { return true; }
  }
  return false;
}

void lowerPiece() { for(int i = 0; i < 4; ++i) { ++(piece[i][1]); } }

void leftShiftPiece() { for(int i = 0; i < 4; ++i) { --(piece[i][0]); } }

void rightShiftPiece() { for(int i = 0; i < 4; ++i) { ++(piece[i][0]); } }

void settlePiece() { for(int i = 0; i < 4; ++i) { board[piece[i][1]][piece[i][0]] = 1; } }

void clearLines() {
  int numOfLines = 0, checker = 0, firstLine = -1;
  for(int y = 0; y < 8; ++y) {
    for(int x = 0; x < 8; ++x) {
      checker += board[y][x];
    }
    if(checker == 8) {
      firstLine = y;
      ++numOfLines;
      ++lines;
      for(int x = 0; x < 8; ++x) { board[y][x] = 0; }
    }
    checker = 0;
  }
  int points;
  switch(numOfLines) {
    case 0:
    points = 0;
    break;
    case 1:
    points = 40;
    break;
    case 2:
    points = 100;
    break;
    case 3:
    points = 300;
    break;
    case 4:
    points = 1200;
    break;
  }
  score += level * points;
  level = (lines / 4) + 1;
  spee = 500 / level;

  if(numOfLines != 0) { settleBoard(numOfLines, firstLine); }
}

void settleBoard(int num, int start) {

  for(int i = 0; i < num; ++i) {
    for(int y = start; y != -1; --y) {
      for(int x = 0; x < 8; ++x) {
        if(board[y][x] == 1 && board[y+1][x] == 0) {
          board[y][x] = 0;
          board[y+1][x] = 1;
        }
      }
    }
  } 
}

void setPiece(int turn[][2]) {
  for(int i = 0; i < 4; ++i) {
    piece[i][0] = turn[i][0];
    piece[i][1] = turn[i][1];
  }
}
/*
void turnPiece() {
  //int turn[4][2]
  if(type == 0) { // Do nothing, squares don't need to turn
    int turn[4][2] = {
      {piece[3][0],piece[3][1]},
      {piece[2][0],piece[2][1]},
      {piece[1][0],piece[2][1]},
      {piece[0][0],piece[0][1]}
    };
    if(!checkTurn(turn)) {
      ++rotation;
      setPiece(turn);
    }
  } 
  else if(type == 1) { // Long
    if(rotation % 2 == 0) { // upright->horizontal
      int turn[4][2] = {
        {piece[3][0]-1,piece[3][1]-1},
        {piece[2][0],piece[2][1]},
        {piece[1][0]+1,piece[2][1]+1},
        {piece[0][0]+2,piece[0][1]+2}
      };
      if(!checkTurn(turn)) {
        ++rotation;
        setPiece(turn);
      }
    }
    else { // horizontal->upright
      int turn[4][2] = {
        {piece[3][0]+1,piece[3][1]+1},
        {piece[2][0],piece[2][1]},
        {piece[1][0]-1,piece[1][1]-1},
        {piece[0][0]-2,piece[0][1]-2}
      };
      if(!checkTurn(turn)) {
        ++rotation;
        setPiece(turn);
      }
    }
  }
  else if(type == 2) { // J
    if(rotation % 4 == 0) { // bottom->right
      int turn[4][2] = {
        {piece[0][0]+2,piece[0][1]-1},
        {piece[1][0]+1,piece[1][1]-2},
        {piece[2][0],piece[2][1]-1},
        {piece[3][0]-1,piece[3][1]}
      };
      if(!checkTurn(turn)) {
        ++rotation;
        setPiece(turn);
      }
    }
    else if(rotation % 4 == 1)  { //right->top
      int turn[4][2] = {
        {piece[0][0]-1,piece[0][1]-2},
        {piece[1][0]-2,piece[1][1]-1},
        {piece[2][0]-1,piece[2][1]},
        {piece[3][0],piece[3][1]+1}
      };
      if(!checkTurn(turn)) {
        ++rotation;
        setPiece(turn);
      }
    }
    else if(rotation % 4 == 2) { // top->left
      int turn[4][2] = {
        {piece[0][0]-1,piece[0][1]+1},
        {piece[1][0]-1,piece[1][1]+2},
        {piece[2][0],piece[2][1]+1},
        {piece[3][0]+1,piece[3][1]}
      };
      if(!checkTurn(turn)) {
        ++rotation;
        setPiece(turn);
      }
    }
    else { // left->bottom
      int turn[4][2] = {
        {piece[0][0],piece[0][1]+2},
        {piece[1][0]+1,piece[1][1]+1},
        {piece[2][0]+1,piece[2][1]},
        {piece[3][0]-1,piece[3][1]-1}
      };
      if(!checkTurn(turn)) {
        ++rotation;
        setPiece(turn);
      }
    }
  }
  else if(type == 2) { //L
    if(rotation % 4 == 0) { // bottom->right
      int turn[4][2] = {
        {piece[3][0]+1,piece[3][1]-2},
        {piece[2][0]+2,piece[2][1]-1},
        {piece[1][0]+1,piece[2][1]},
        {piece[0][0]+1,piece[0][1]+1}
      };
      if(!checkTurn(turn)) {
        ++rotation;
        setPiece(turn);
      }
    }
  }
}
*/


void genNextPiece() { //Sets up piece array for next piece to be lowered
  type = random(7);
  switch(type){
    case 0: // square 
    piece[0][0] = 3; piece[0][1] = -2;
    piece[1][0] = 3; piece[1][1] = -1;
    piece[2][0] = 4; piece[2][1] = -2;
    piece[3][0] = 4; piece[3][1] = -1;
    break;
    case 1: // long
    piece[0][0] = 3; piece[0][1] = -1;
    piece[1][0] = 3; piece[1][1] = -2;
    piece[2][0] = 3; piece[2][1] = -3;
    piece[3][0] = 3; piece[3][1] = -4;
    break;
    case 2: // J
    piece[0][0] = 3; piece[0][1] = -1;
    piece[1][0] = 4; piece[1][1] = -1;
    piece[2][0] = 4; piece[2][1] = -2;
    piece[3][0] = 4; piece[3][1] = -3;
    break;
    case 3: // L
    piece[0][0] = 4; piece[0][1] = -1;
    piece[1][0] = 3; piece[1][1] = -1;
    piece[2][0] = 3; piece[2][1] = -2;
    piece[3][0] = 3; piece[3][1] = -3;
    break;
    case 4: // S
    piece[0][0] = 3; piece[0][1] = -3;
    piece[1][0] = 3; piece[1][1] = -2;
    piece[2][0] = 4; piece[2][1] = -2;
    piece[3][0] = 4; piece[3][1] = -1;
    break;
    case 5: // Z
    piece[0][0] = 4; piece[0][1] = -3;
    piece[1][0] = 4; piece[1][1] = -2;
    piece[2][0] = 3; piece[2][1] = -2;
    piece[3][0] = 3; piece[3][1] = -1;
    break;
    case 6: // T
    piece[0][0] = 3; piece[0][1] = -1;
    piece[1][0] = 3; piece[1][1] = -2;
    piece[2][0] = 3; piece[2][1] = -3;
    piece[3][0] = 4; piece[3][1] = -2;
    break;
  }
  if(checkForGround()) { gameGoing = false; }
}
