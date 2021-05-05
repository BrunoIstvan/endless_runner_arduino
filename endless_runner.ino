#include <LiquidCrystal.h>

enum STATUS { INITIAL, WIN, LOSE, NO_ENERGY };

#define PIEZO                             12
#define INITIAL_VELOCITY                  500
int VELOCITY                              = INITIAL_VELOCITY;

//#define NOTE_G4  392
//#define NOTE_G5  784
//#define NOTE_G6  1568

#define START_BUTTON                      13
#define PANEL_POSITION                    13
#define MAX_INITIAL_OBJECTS_POSITION_X    12
#define VELOCITY_DECREASE                 25
#define MAX_OBSTACLE_SIZE                 8
#define MIN_SCORE                         0

#define PLAYER_1                          1
#define PLAYER_2                          2
#define PLAYER_3                          3
#define WALL                              4
#define SPIKE_UP                          5
#define SPIKE_DOWN                        6
#define EXPLOSION                         7

const byte playerForm1[8]                 = {B00110,B00110,B00101,B11110,B00100,B01010,B10010,B10010}; // OK - Player 1
const byte playerForm2[8]                 = {B00110,B00110,B10100,B01111,B00100,B01010,B01001,B01001}; // OK - Player 2
//const byte playerForm1[8]={B00110,B00110,B10100,B01111,B00100,B00110,B00101,B01001};
//const byte playerForm2[8]={B00110,B00110,B00101,B01111,B10100,B01111,B01001,B01001};
const byte playerForm3[8]                 = {B00110,B00110,B10101,B01110,B00100,B00111,B00101,B01001}; // OK - Player 3
const byte wallForm[8]                    = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111}; // OK - Wall
const byte spikeForm1[8]                  = {B00100,B00100,B00100,B01110,B01110,B11111,B11111,B10001}; // OK - Spike Up
const byte spikeForm2[8]                  = {B10001,B11111,B11111,B01110,B01110,B00100,B00100,B00100}; // OK - Spike Dowm
const byte explosionForm[8]               = {B01010,B10101,B01010,B10001,B10001,B01010,B10101,B01010}; // OK

int pyPlayer                              = 1;
int pxPlayer                              = 0;
int formPlayer                            = 1;
int pxExplosion                           = 0;
int pyExplosion                           = 0;
int pxObstacle                            = MAX_INITIAL_OBJECTS_POSITION_X;
int pyObstacle                            = 1;
int pxSpike                               = MAX_INITIAL_OBJECTS_POSITION_X;
int pySpike                               = 1;

bool game                                 = false;
bool hasObstacle                          = false;
bool hasSpike                             = false;
bool playerJumped                         = false;
bool playSound                            = false;
int totalObstacles                        = 0;
int sizeObstacle                          = 0;
int sizeSpike                             = 0;
int score                                 = 0;
int switchValue                           = 0; 
long freqIn                               = 0;
long blow1                                = 0;
long blow2                                = 0;

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

void setup() {
  
  // inicializar serial
  Serial.begin(9600);
  
  // inicializar display
  lcd.begin(16, 2);
  lcd.clear();

  randomSeed(millis());

  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(PIEZO, OUTPUT);
 
  // incluir personagens na memoria do display
  createCharacters();

}

void loop() {

  if(game) {

    lcd.clear();
    
    // imprimir pontos e nivel
    drawPanel(PANEL_POSITION);

    // avalidar se o joystick esta direcionado para algum lugar e desenhar o carro
    availablePlayer();  

    // verificar se o obstaculo chegou no limite da tela
    availableObstacle();

    // verificar se o espeto chegou no limite da tela
    // availableSpike();

    // avaliar se houve colisao entre personagem e obstaculo
     availablePlayerAndObstacleCollision();

    // avaliar se houve colisao entre personagem e espeto
    // availablePlayerAndSpikeCollision();

    // avalia se deve incrementar a velocidade
    availableIncreaseVelocity();

    // toca musica continua
    playPlayerRunning();
    
    delay(VELOCITY);
    
  } else {

    drawMessageStoppedGame(INITIAL);
    
    // se jogador apertou botao, entao iniciar o jogo
    if(digitalRead(START_BUTTON) == LOW) {
      resetGame();
    }

  }

}

// avalia se deve incrementar a velocidade
void availableIncreaseVelocity() {
  
  // a cada 3 obstaculos, deixar a velocidade do jogo mais rapida
  if(totalObstacles % 3 == 0) {
    if(VELOCITY - VELOCITY_DECREASE == 50) {
      
    } else {
      VELOCITY -= VELOCITY_DECREASE;
    }
    Serial.println(VELOCITY);
    totalObstacles++;
  }
  
}

// avaliar se houve colisao entre personagem e obstaculo
void availablePlayerAndObstacleCollision() {

  bool checkCollision = (pxPlayer == pxObstacle) && (pyPlayer == pyObstacle);
  bool alternativeCheckCollision = (pxPlayer == pxObstacle+1) && (pyPlayer == pyObstacle);

  // houve uma colisao, remover obstaculo e carro e desenhar explosao
  if(checkCollision || alternativeCheckCollision) {    
    game = false;
    drawSomething(pxPlayer, pyPlayer, EXPLOSION);
    playExplosionSound();
    drawMessageStoppedGame(LOSE);
    pxObstacle = MAX_INITIAL_OBJECTS_POSITION_X;
    delay(5000);
    lcd.clear();
  }
  
}

// avaliar se houve colisao entre personagem e espeto
void availablePlayerAndSpikeCollision() {

  bool checkCollision = (pxPlayer == pxSpike) && (pyPlayer == pySpike);
  bool alternativeCheckCollision = (pxPlayer == pxSpike+1) && (pyPlayer == pySpike);

  // houve uma colisao, remover espeto e carro e desenhar explosao
  if(checkCollision || alternativeCheckCollision) {    
    game = false;
    drawSomething(pxPlayer, pyPlayer, EXPLOSION);
    playExplosionSound();
    drawMessageStoppedGame(LOSE);
    pxSpike = MAX_INITIAL_OBJECTS_POSITION_X;
    delay(5000);
    lcd.clear();
  }
  
}

// verificar se obstaculo chegou no limite da tela e desenhar em caso negativo
void availableObstacle() {

  if(hasObstacle) { // se existir obstaculo, mover ele ate o comeco da tela para dar a dinamica do jogo
    
    // diminuir a posicao de um obstaculo
    pxObstacle--;

    // se o ultimo bloco do obstaculo atingiu limite da tela, voltar ele para a posicao inicial
    if((pxObstacle + sizeObstacle) <= 0) {
      
      pxObstacle = -10; 
      hasObstacle = false;
      score++;
      
    } else {

      for(int tot = 0; tot < sizeObstacle; tot++) {
        if(pxObstacle + tot <= MAX_INITIAL_OBJECTS_POSITION_X) {
          drawSomething(pxObstacle + tot, pyObstacle, WALL); // cria um muro
        }
      }
      
    }
    
  } else { // caso nao tenha obstaculo, criar um

    pyObstacle = possibilityGeneratingRow();  // dar so 30% de chance de criar obstaculo na linha 0
    sizeObstacle = random(2, MAX_OBSTACLE_SIZE); // gerar colunas entre 2 e MAX_OBSTACLE_SIZE colunas
    pxObstacle = MAX_INITIAL_OBJECTS_POSITION_X;
    
    for(int tot = 0; tot < sizeObstacle; tot++) {
      if(pxObstacle + tot <= MAX_INITIAL_OBJECTS_POSITION_X) {
        drawSomething(pxObstacle + tot, pyObstacle, WALL); // cria um muro
      }
    }

    hasObstacle = true;
    totalObstacles++;
    
  }

}


// verificar se espeto chegou no limite da tela e desenhar em caso negativo
void availableSpike() {

  if(hasSpike) { // se existir espeto, mover ele ate o comeco da tela para dar a dinamica do jogo
    
    // diminuir a posicao de um espeto
    pxSpike--;

    // se o ultimo bloco do espeto atingiu limite da tela, voltar ele para a posicao inicial
    if((hasSpike + sizeSpike) <= 0) {
      
      pxSpike = -10; 
      hasSpike = false;
      score++;
      
    } else {

      for(int tot = 0; tot < sizeSpike; tot++) {
        if(pxSpike + tot <= MAX_INITIAL_OBJECTS_POSITION_X) {
          drawSpike(tot);
        }
      }
      
    }
    
  } else { // caso nao tenha espeto, criar um

    // se possivel criar depois 
    if (pxObstacle == MAX_INITIAL_OBJECTS_POSITION_X/2) {
        
      pySpike = possibilityGeneratingRow();  // dar so 30% de chance de criar espeto na linha 0
      pxSpike = MAX_INITIAL_OBJECTS_POSITION_X;
      Serial.println(pxSpike);
      sizeSpike = random(2, MAX_OBSTACLE_SIZE); // gerar colunas entre 2 e MAX_OBSTACLE_SIZE colunas
      
      for(int tot = 0; tot < sizeSpike; tot++) {
        if(pxSpike + tot <= MAX_INITIAL_OBJECTS_POSITION_X) {
          drawSpike(tot);
        }
      }
  
      hasSpike = true;
      totalObstacles++;

    }
      
  }
 
}

// dar so 30% de chance de criar obstaculo na linnha 0
int possibilityGeneratingRow() {
  return random(0, 10) > 6 ? 0 : 1; // criar o obstaculo randomicamente entre 0 e 9, dar so 30% de chance de criar obstaculo na linnha 0
}

void drawSpike(int tot) {
  
  if(pySpike == 0) { 
    drawSomething(pxSpike + tot, pySpike, SPIKE_DOWN);
  } else if(pySpike == 1) { 
    drawSomething(pxSpike + tot, pySpike, SPIKE_UP);
  }
  
}

// avaliar se o botao foi apertado e desenhar o player
void availablePlayer() {
  
  if(digitalRead(START_BUTTON) == LOW) { // se botao foi apertado, desenhar personagem na linha 0 da tela

    pyPlayer = 0;
    drawSomething(pxPlayer, pyPlayer, PLAYER_3); 

  } else { // caso o botao nao esteja apertado, fazer o personagem correr

    // verificar se ao soltar o botao algum bloco de obstaculo esta abaixo do personagem, isso deve fazer ele andar acima do bloco, ou seja, andar na linha 0
    if(pyPlayer == 0 && (pxObstacle <= pxPlayer) && ((pxObstacle + sizeObstacle) > pxPlayer) || 
       pyPlayer == 0 && (pxSpike <= pxPlayer) && ((pxSpike + sizeSpike) > pxPlayer)) {
      
      pyPlayer = 0;
      
    } else {
    
      pyPlayer = 1;

    }
    
    // desenhar personagem
    drawSomething(pxPlayer, pyPlayer, (formPlayer > 0 ? PLAYER_1 : PLAYER_2)); 
    formPlayer *= -1;

  }

}

void resetGame() {
  
  score = MIN_SCORE;
  game = true;
  hasObstacle = false;
  hasSpike = false;
  VELOCITY = INITIAL_VELOCITY;
  playerJumped = false;
  totalObstacles = 0;
  
}

//void drawExplosionShip(int px, int py) {
//  lcd.clear();
//  drawSomething(px, py, EXPLOSION);
//  delay(1000);
//  lcd.clear();
//}

void drawSomething(int px, int py, int character) {
  lcd.setCursor(px, py);
  lcd.write(character);
}

String getPadded(int num) {
  char buff[6];
  char padded[7];
  
  //sprintf function will convert the long to a string
  sprintf(buff, "%.3u", num); // buff will be "01238"

  padded[0] = buff[0];
  padded[1] = buff[1];
  padded[2] = buff[2];
//  padded[3] = buff[3];
//  padded[4] = buff[4];
//  padded[5] = buff[5];
  padded[3] = '\0'; // The terminating NULL

  return String(padded);
}

void drawPanel(int px) {
  lcd.setCursor(px, 0);
  lcd.print(getPadded(score));
  lcd.setCursor(px, 1);
  lcd.print(getPadded(INITIAL_VELOCITY - VELOCITY));
}

// desenhar a tela conforme status passado no parametro
void drawMessageStoppedGame(STATUS pStatus) { 

  String showScore = "";
  lcd.setCursor(0, 0);
  switch (pStatus) {
    case INITIAL: 
      lcd.print(" ENDLESS RUNNER ");
      lcd.setCursor(2, 1);
      lcd.print("PRESS BUTTON");
      break;
    case WIN: 
    case LOSE: 
    case NO_ENERGY: 
      lcd.clear();
      lcd.print(pStatus == WIN ? ":)  YOU WIN  :)" : (pStatus == LOSE ? ":( GAME OVER :(" : "BATTERY IS OVER :(") ); 
      lcd.setCursor(0, 1);
      showScore = " SCORE: " + getPadded(score) + " PTS";
      lcd.print(showScore);
      break;
  }
  
}

void playPlayerRunning() {
  tone(PIEZO, 500, 100);
}

void playExplosionSound() {
  for(int k = 0; k < 250; k++){
    blow1 = random(500,1000);
    blow2 = random(1,3);
    piezoTone(blow1,blow2);
  } 
}

//void playTakeBattery() {
//  
//  // Play Fireball sound
//  tone(PIEZO, NOTE_G4, 35);
//  delay(35);
//  tone(PIEZO, NOTE_G5, 35);
//  delay(35);
//  tone(PIEZO, NOTE_G6, 35);
//  delay(35);
//  noTone(8);
//  
//}

void piezoTone(long freq, long duration){
  
  long aSecond = 1000000;
  long period = aSecond/freq;
  duration = duration*1000;
  duration = duration/period;
  for(long k = 0; k < duration; k++){
    digitalWrite(PIEZO, HIGH);
    delayMicroseconds(period/2);
    digitalWrite(PIEZO, LOW);
    delayMicroseconds(period/2);
  }
  
} 

// Cria os personagens na memoria do display
void createCharacters() {
  
  // cria o personagem forma 1
  lcd.createChar(PLAYER_1, playerForm1);
  
  // cria o personagem forma 2
  lcd.createChar(PLAYER_2, playerForm2);

  // cria o personagem forma 2
  lcd.createChar(PLAYER_3, playerForm3);

  // cria o muro
  lcd.createChar(WALL, wallForm);

  // cria o espeto para cima
  lcd.createChar(SPIKE_UP, spikeForm1);

  // cria o espeto para baixo
  lcd.createChar(SPIKE_DOWN, spikeForm2);

  // cria a explosao
  lcd.createChar(EXPLOSION, explosionForm);
  
//  // cria a bateria
//  lcd.createChar(BATTERY, batteryForm);

//  // cria a bandeira 
//  lcd.createChar(FLAG, flagForm);
  
}
