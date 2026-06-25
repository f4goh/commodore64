//
// Chapter03B: fiamma del motore dell'astronave
//
#include <c64/vic.h>
#include <c64/sprites.h>
#include <c64/joystick.h>
#include <oscar.h>
#include <string.h>
#include <stdint.h>

//***************************************************************************************************************
//* DEFINIZIONE DELLE COSTANTI
//***************************************************************************************************************
#define PLAYERX0 50         // posizione iniziale
#define PLAYERY0 120
#define PLAYER_SPEED 2      
#define PLAYER_XMIN 40      // coordinata x minima
#define PLAYER_XMAX 320-8   // coordinata x massima
#define PLAYER_YMIN 50      // coordinata y minima
#define PLAYER_YMAX 200     // coordinata y massima



//***************************************************************************************************************
//* DEFINIZIONE DELLE STRUTTURE DATI
//***************************************************************************************************************

typedef struct {
    uint16_t x;
    uint8_t  y;
    uint8_t  color;
    uint8_t  base_frame;

    uint8_t  eng_baseFrame;
    uint8_t  currentFrame; 
    uint8_t  maxFrames;    
    uint8_t  animTimer;    
    uint8_t  animSpeed;    
} PlayerShip;


//***************************************************************************************************************
//* VARIABILI GLOBALI
//***************************************************************************************************************

PlayerShip player_ship = {
    PLAYERX0, PLAYERY0,     // x,y
    VCOL_LT_BLUE,           // color
    3,                      // base_frame
    0,                      // eng_baseFrame
    0,                      // currentFrame
    2,                      // maxFrames
    0,                      // animTimer
    10                      // animSpeed
};

char * const Screen = (char *) 0x0400;      // memoria schermo standard
char * const SpriteRAM = (char *) 0x3000;   // memoria destinata agli sprite

uint8_t sprite_base;                        // frame base per gli sprite

// include gli sprites nel formato SpritePad
const char MySpriteData[] = {
    #embed spd_sprites "assets/player_ship.spd"
};



//***************************************************************************************************************
//* PROTOTIPI DELLE FUNZIONI
//***************************************************************************************************************

void init_player_ship();
void update_player_ship();


//***************************************************************************************************************
//* MAIN
//***************************************************************************************************************
int main() {

    // copia i dati degli sprite nella memoria destinata agli sprite
    memcpy(SpriteRAM,MySpriteData,64*5);

    // pulisce lo schermo
    memset(Screen, 32, 1000);

    // inizializzazione degli sprite
    spr_init(Screen);

    // Configurazione dei colori
    vic.color_back = VCOL_BLACK;      // Imposta il colore di sfondo dello schermo
    vic.color_border = VCOL_BLACK;    // Imposta il colore di bordo dello schermo
    vic.spr_mcolor0 = VCOL_DARK_GREY; // Primo colore multicolor condiviso per tutti gli sprite
    vic.spr_mcolor1 = VCOL_LT_GREY;   // Secondo colore multicolor condiviso per tutti gli sprite

    // calcola il frame base per gli sprite
    sprite_base = (uint16_t) (SpriteRAM) / 64;

    init_player_ship();

    // Loop infinito per mantenere la visualizzazione
    while (true) {
        update_player_ship();

        // Sincronizzazione con il refresh video per evitare sfarfallio
        vic_waitFrame();
    }

    return 0;
}


//***************************************************************************************************************
//* DEFINIZIONE DELLE FUNZIONI
//***************************************************************************************************************

void init_player_ship() {

    // (1)
    spr_set(0,true,player_ship.x,player_ship.y,
            sprite_base+player_ship.base_frame,player_ship.color,
            true,false,false);

    // (2)
    spr_set(1,true,player_ship.x-24,player_ship.y+1,
            sprite_base+player_ship.eng_baseFrame,player_ship.color,
            true,false,false);
}

void update_player_ship() {
    // legge lo stato del joystick in porta 2
    joy_poll(0);

    player_ship.x += joyx[0] * PLAYER_SPEED;
    player_ship.y += joyy[0] * PLAYER_SPEED;

    // Clipping: impedisce all'astronave di uscire dall'area visibile
    if (player_ship.x < PLAYER_XMIN)
        player_ship.x = PLAYER_XMIN;
    else if (player_ship.x > PLAYER_XMAX)
        player_ship.x = PLAYER_XMAX;

    if (player_ship.y < PLAYER_YMIN)
        player_ship.y = PLAYER_YMIN;
    else if (player_ship.y > PLAYER_YMAX)
        player_ship.y = PLAYER_YMAX;

    // Aggiorna la posizione dello sprite
    spr_move(0, player_ship.x, player_ship.y);

    // CAMBIO FRAME DI ANIMAZIONE:
    // Se joyy è -1, l'indice diventa neutro - 1 (Su)
    // Se joyy è 0, l'indice resta neutro (Fermo)
    // Se joyy è 1, l'indice diventa neutro + 1 (Giù)
    spr_image(0, sprite_base + player_ship.base_frame + joyy[0]);

    // Muovi la fiammella con un offset
    spr_move(1, player_ship.x-24, player_ship.y+1);

    // LOGICA DI ANIMAZIONE (5 volte al secondo)
    player_ship.animTimer++;
    if (player_ship.animTimer >= 5) { // Ogni 10 frame video (10 * 1/50 sec = 0.2 sec)
        player_ship.animTimer = 0;
        player_ship.currentFrame = (player_ship.currentFrame + 1) % 2; // Alterna tra 0 e 1 (per 2 frame)
    }
    
    // Imposta il nuovo frame per lo sprite della fiammella (indice 1)
    spr_image(1, sprite_base + player_ship.eng_baseFrame + player_ship.currentFrame);
}
