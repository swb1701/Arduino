#include "WaveHC.h"
#include "WaveUtil.h"

/*

Todo:

S/W

Toward the end-game when not many options, the BLOCK, DEFEAT logic is wrong
Should compliment you for blocking it's attempt to win.

Start more levels of minimax for more challenging play.

Introduce robot move mode (which turns off the prompts to put the checker
in) and feeds robot checkers from a chute dropping them in the slots.

H/W

Might benefit from a board which masks the detectors to prevent ambient light
from interfering.

In the future design the LEDs on the side of the slots away from the cable connection.

*/


SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the volumes root directory
FatReader file;   // This object represent the WAV file for a pi digit or period
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

#define MSG_PLAYGAME 0
#define MSG_YOURMOVE 1
#define MSG_YOUWON 2
#define MSG_IWON 3
#define MSG_PLAYAGAIN 4
#define MSG_BLOCK 5
#define MSG_DEFEAT 6
#define MSG_MOVEFORME 7
#define MSG_THANKSMOVEFORME 8

#define MAX_MSG 9
#define MAX_ALT 5

uint16_t fileIndex[MAX_MSG][MAX_ALT];
int msg_total;
int num_alts[MAX_MSG];

/*
 * Define macro to put error messages in flash memory
 */

#define error(msg) error_P(PSTR(msg))

int rows=6;
int cols=7;
int board[6][7];
int height[7];
int move=0;
int winmag=10000;

boolean defeat=false;
boolean block=false;

int pin0=22; //3 on uno, 22 on mega
//3-9 on uno, 22-28 on mega

int result=0;
int last=-1;
int run=0;
int runplayer=-1;
int p=0;
boolean debug=false;

/*
 * print error message and halt
 */
void error_P(const char *str)
{
  PgmPrint("Error: ");
  SerialPrint_P(str);
  sdErrorCheck();
  while(1);
}

/*
 * print error message and halt if SD I/O error
 */
void sdErrorCheck(void)
{
  if (!card.errorCode()) return;
  PgmPrint("\r\nSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  PgmPrint(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}

void setup() {
  pinMode(11,INPUT); //make these inputs since we're playing mega SPI to them as a side-effect
  pinMode(12,INPUT);
  pinMode(13,INPUT);
  cfinput();
  Serial.begin(115200);
  randomSeed(analogRead(0));
  clearBoard();
  //ready card for audio playback
 if (!card.init()) {
    error("Card init. failed!");
  }
  if (!vol.init(card)) {
    error("No partition!");
  }
  if (!root.openRoot(vol)) {
    error("Couldn't open dir");
  }
  //PgmPrintln("Files found:");
  //root.ls();
  indexFiles();
  playmsg(MSG_PLAYGAME);
}

void clearBoard() {
  move=0;
  Serial.println("Connect4 Starting:");
  for(int i=0;i<7;i++) height[i]=0;
  for(int i=0;i<7;i++) {
    for(int j=0;j<6;j++) {
      board[j][i]=0;
    }
  }
  cfstrobe();
  showBoard();
}

int updateEval(int rows,int cols) {
  last=p;
  p=board[rows][cols]; //get new piece
  if (debug) Serial.print(rows);
  if (debug) Serial.print(",");
  if (debug) Serial.print(cols);
  if (debug) Serial.print(" last,p=");
  if (debug) Serial.print(last);
  if (debug) Serial.print(",");
  if (debug) Serial.println(p);
  if (p>0 && p==last) { //if a piece and same as before
    runplayer=p; //record whose run it is
    run++; //and increment the run count
    if (debug) Serial.print("run=");
    if (debug) Serial.println(run);
  } else {
    if (run>2) { //win (4 in a row)
      if (runplayer==1) {
        return(winmag); //high pos number for computer
      } else {
        return(-1*winmag); //high neg number for human
      }
    } else if (run>0) { //if we had at least two in a row
      if (runplayer==1) {
        result+=run; //give points for computer
      } else {
        result-=run; //give points for human
      }
    }
    run=0;
  }
  return(0);
}

int checkLast() { //same logic as above but when hit end of row
  if (run>2) { //win
    if (runplayer==1) {
      return(winmag);
    } else {
      return(-1*winmag);
    }
  } else if (run>0) {
    if (runplayer==1) {
      result+=run;
    } else {
      result-=run;
    }
  }
  run=0;
  return(0);
}


int eval() {
  result=0; //zero result
  runplayer=-1; //who has the last rund
  //horizontal rows
  for(int r=0;r<rows;r++) {
    p=-1; //reset last
    run=0; //reset run count
    for(int c=0;c<cols;c++) {
      int res=updateEval(r,c);
      if (res!=0) return(res); //win
    }
    int res=checkLast();
    if (res!=0) return(res); //win
  }
  //vertical columns
  debug=false;
  for(int c=0;c<cols;c++) {
    p=-1;
    run=0;
    for(int r=0;r<rows;r++) {
      int res=updateEval(r,c);
      if (res!=0) return(res); //win
    }
    int res=checkLast();
    if (res!=0) return(res); //win
  }
  debug=false;
  //left-up diagonals (those longer than 3)
  int c=0;
  for(int r=3;r<5;r++) {
    p=-1;
    run=0;
    for(int i=0;i<5;i++) {
      if (r-i<0) break;
      int res=updateEval(r-i,c+i);
      if (res!=0) return(res); //win
    }
  }
  for(int c=0;c<4;c++) {
    int r=5;
    p=-1;
    run=0;
    for(int i=0;i<6;i++) {
      if (r-i<0 || c>6) break;
      int res=updateEval(r-i,c+i);
      if (res!=0) return(res); //win
    }
    int res=checkLast();
    if (res!=0) return(res); //win
  }
  //right-up diagonals (those longer than 3)
  c=6;
  for(int r=3;r<5;r++) {
    p=-1;
    run=0;
    for(int i=0;i<5;i++) {
      if (r-i<0) break;
      int res=updateEval(r-i,c-i);
      if (res!=0) return(res); //win
    }
    int res=checkLast();
    if (res!=0) return(res); //win
  }
  for(int c=6;c>2;c--) {
    int r=5;
    p=-1;
    run=0;
    for(int i=0;i<6;i++) {
      if (r-i<0 || c<0) break;
      int res=updateEval(r-i,c-i);
      if (res!=0) return(res); //win
    }
    int res=checkLast();
    if (res!=0) return(res); //win
  }
  return(result);
}

void addPiece(int col,int player) {
  int cur=height[col];
  height[col]++;
  board[5-cur][col]=player;
}

int chooseMove(int player) {
  defeat=false;
  block=false;
  //kind of a shallow minimax here, give Emma a chance ;-)
  int best=-1;
  int max=-2*winmag;
  for(int c=0;c<cols;c++) {
    if (height[c]<6) {
      int m=evalMove(c,player);
      if (m>max) {
        best=c;
        max=m;
      }
    }
  }
  Serial.print("best isolated comp move is ");
  Serial.print(best);
  Serial.print(" with score ");
  Serial.println(max);
  if (max>=winmag) {
    return(best); //a sure win
  }
  //otherwise evaluate other player's responses
  int equiv=0;
  int equivMoves[7];
  int oldheight;
  int bestcol=-1;
  int bestval=-2*winmag;
  int activecols=0;
  int blockedcols=0;
  for(int c=0;c<cols;c++) {
    if (height[c]<6) {
      activecols++;
      oldheight=height[c];
      board[5-oldheight][c]=1; //computer
      height[c]++;
      int besthval=2*winmag;
      for(int c2=0;c2<cols;c2++) {
        if (height[c2]<6) {
          int m=evalMove(c2,2); //human
          Serial.print("comp=");Serial.print(c);
          Serial.print(" = human=");Serial.print(c2);
          Serial.print(" for score ");Serial.println(m);
          if (m<besthval) {
            besthval=m;
          }
        }
      }
      if (besthval<=-1*winmag) {
        blockedcols++;
      }
      if (besthval==bestval) {
        equiv++;
        equivMoves[equiv-1]=c;
      } else if (besthval>bestval) {
        bestcol=c;
        bestval=besthval;
        equiv=1;
        equivMoves[0]=bestcol;
      }
      board[5-oldheight][c]=0; //undo computer move
      height[c]=oldheight; //set back height
    }
  }
  if (activecols>1) {
    if (activecols==(blockedcols+1)) {
      block=true;
    }
    if (bestval<=-0.5*winmag) {
      defeat=true;
    }
  }
  if (equiv>1) {
    Serial.print("best 2ply comp moves are:");
    for(int i=0;i<equiv;i++) {
      Serial.print(" ");
      Serial.print(equivMoves[i]);
    }
    Serial.print(" with score ");
    Serial.println(bestval);
    bestcol=equivMoves[random(equiv)]; //choose one randomly
  } else {
    Serial.print("best 2ply comp move is ");
    Serial.print(bestcol);
    Serial.print(" with score ");
    Serial.println(bestval);
  }
  return(bestcol);
}

int evalMove(int col,int player) {
  int cur=height[col];
  board[5-cur][col]=player;
  int r=eval();
  board[5-cur][col]=0;
  return(r);
}

void loop() {
  playmsg(MSG_YOURMOVE);
  int col=getInput();
  addPiece(col,2);
  move++;
  showBoard();
  if (eval()<=-1*winmag) {
    Serial.println("Human won!");
    playmsg(MSG_YOUWON);
    clearBoard();
    playmsg(MSG_PLAYAGAIN);
  } else {
    int c=chooseMove(1);
    if (defeat) {
      playmsg(MSG_DEFEAT);
    } else if (block) {
      playmsg(MSG_BLOCK);
    }
    playmsg(MSG_MOVEFORME);
    waitForMove(c);
    playmsg(MSG_THANKSMOVEFORME);
    addPiece(c,1);
    move++;
    showBoard();
    //highlightColumn(c);
    if (eval()>=winmag) {
      Serial.println("Computer won!");
      playmsg(MSG_IWON);
      clearBoard();
      playmsg(MSG_PLAYAGAIN);
    }
  }
}

void showBoard() {
  Serial.print("Move=");
  Serial.println(move);
  for(int i=0;i<6;i++) {
    for(int j=0;j<7;j++) {
      int v=board[i][j];
      if (v==0) {
        Serial.print("_");
      } else if (v==1) {
        Serial.print("X");
      } else if (v==2) {
        Serial.print("O");
      } else {
        Serial.print("!");
      }
    }
    Serial.println("");
  }
}

void cfinput() {
  for(int i=pin0;i<pin0+7;i++) {
    pinMode(i,INPUT);
  }
}

void waitForMove(int col) {
  int i=pin0+col;
  int ratio=10;
  while(true) {
    pinMode(i,INPUT);
    for(int j=0;j<ratio;j++) {
      if (digitalRead(i)==LOW) {
        return;
      } 
    }
    pinMode(i,OUTPUT);
    digitalWrite(i,LOW);
  }
  while(digitalRead(i)==LOW) {
    delay(100);
  }
}

void highlightColumn(int col) {
  int pin=pin0+col;
  pinMode(pin,OUTPUT);
  for(int i=0;i<5;i++) {
    delay(200);
    digitalWrite(pin,LOW);
    delay(200);
    digitalWrite(pin,HIGH);
  }
  pinMode(pin,INPUT);
}

void cfstrobe() {
  for(int i=pin0;i<pin0+7;i++) {
    pinMode(i,OUTPUT);
    digitalWrite(i,LOW);
    delay(50);
    pinMode(i,INPUT);
  }
  for(int i=pin0+6;i>(pin0-1);i--) {
    pinMode(i,OUTPUT);
    digitalWrite(i,LOW);
    delay(50);
    pinMode(i,INPUT);
  }
}

int getInput() {
  int result=-1;
  while(result==-1) {
    for(int i=pin0;i<pin0+7;i++) {
      if (digitalRead(i)==LOW) {
        result=i;
        break;
      }
    }
  }
  while (digitalRead(result)==LOW) {
    delay(100);
  }
  return(result-pin0);
}

/*
 * Play a file and wait for it to complete
 */
void playcomplete(char *name) {
  playfile(name);
  while (wave.isplaying);
  
  // see if an error occurred while playing
  sdErrorCheck();
}

/*
 * Open and start playing a WAV file
 */
void playfile(char *name) 
{
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  if (!file.open(root, name)) {
    PgmPrint("Couldn't open file ");
    Serial.print(name); 
    return;
  }
  if (!wave.create(file)) {
    PgmPrintln("Not a valid WAV");
    return;
  }
  // ok time to play!
  wave.play();
}

void playmsg(int num) {
  int alt=random(num_alts[num]);
  playindex(fileIndex[num][alt]);
  while (wave.isplaying);
  sdErrorCheck();
}

void playindex(uint16_t index) 
{
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  if (!file.open(root, index)) {
    PgmPrint("Couldn't open file at index ");
    Serial.print(index); 
    return;
  }
  if (!wave.create(file)) {
    PgmPrintln("Not a valid WAV");
    return;
  }
  // ok time to play!
  wave.play();
}

void indexFiles(void) {
  char name[10];
  msg_total=0;
  strcpy_P(name,PSTR("eX-X.wav"));
  for(uint8_t i=0;i<MAX_MSG;i++) {
    boolean found=false;
    int maxalt=0;
    for(uint8_t j=0;j<MAX_ALT;j++) {
      name[1]=48+i;
      name[3]=48+j;
      //Serial.print("Trying ");
      //Serial.print(name);
      if (file.open(root,name)) {
        //Serial.println(" ...indexed");
        found=true;
        maxalt++;
        fileIndex[i][j]=root.readPosition()/32-1;
      } else {
        //Serial.println(" ...not found");
        break;
      }
    }
    num_alts[i]=maxalt;
    if (found) msg_total++;
  }
}

/*

Game phrases:

0-0: Would you like to play a game of connect four?
0-1: Ready for a game?
0-2: I'm waiting to take on a challenger.

1-0: It's your move.
1-1: Your turn.
1-2: Your move.

2-0: Congratulations, you've won!
2-1: I guess you got the best of me this time.
2-2: Congratulations, that was a good game.

3-0: Sorry, looks like I win.
3-1: I've won!
3-2: I win, better luck next time.

4-0: Would you like to play again?
4-1: Would you like to play another game?
4-2: Another game?
4-3: Ready for another game?
4-4: I like this game, could we please play it again?

5-0: Hmm, I think I better block you.
5-1: Don't mind if I stop your row of four.
5-2: I almost didn't see that.

6-0: Looks like you've got me beat.
6-1: I'm not sure how I can get out of this one.
6-2: Hmm.. difficult situation.

7-0: Could you please put my checker in the lighted column?
7-1: Could you please put my checker by the light?

8-0: Thanks!
8-1: I appreciate it.
8-2: Thanks for moving for me.

*/
