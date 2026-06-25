/******************************************************************************
 * @file      main.c
 * @author    Stefano Coppi
 * @project   Chrome Horizon / Modern Code, Classic Steel
 * * @description
 * This source code is part of the examples from the book:
 * "Modern Code, Classic Steel" by Stefano Coppi.
 *
 * Chapter04: Drawing tilemap
 * 
 * @copyright
 * All rights reserved. 
 * For educational use only. Distribution or commercial use of this 
 * source code without explicit permission is prohibited.
 * * Built with: Oscar64 C Compiler for Commodore 64
 ******************************************************************************/


//***************************************************************************************************************
//* INCLUDES
//***************************************************************************************************************
#include <c64/vic.h>
#include <c64/sprites.h>
#include <c64/joystick.h>
#include <oscar.h>
#include <string.h>
#include <stdint.h>

//***************************************************************************************************************
//* CONSTANTS
//***************************************************************************************************************
#define PLAYERX0 50         
#define PLAYERY0 120
#define PLAYER_SPEED 2      
#define PLAYER_XMIN 40
#define PLAYER_XMAX 320-8 
#define PLAYER_YMIN 50
#define PLAYER_YMAX 200


//***************************************************************************************************************
//* DATA STRUCTURES DEFINITIONS
//***************************************************************************************************************

typedef struct {
    uint16_t x;
    uint8_t  y;
    uint8_t  color;
    uint8_t  base_frame;
} PlayerShip;


//***************************************************************************************************************
//* GLOBAL VARIABLES
//***************************************************************************************************************

PlayerShip player_ship = {
    PLAYERX0, PLAYERY0,
    VCOL_GREEN,
    1
};

char * const Screen = (char *)    0x0400;      
char * const SpriteRAM = (char *) 0x3000;
char * const Charset = (char *)   0x2800;
char * const Color = (char *)     0xd800;

uint8_t sprite_base;

const char MySpriteData[] = {
    #embed spd_sprites "assets/sprites.spd"
};

const char Level1Tiles[] = { 
    #embed ctm_chars "../assets/level1.ctm" 
};

const char Level1Map[] = { 
    #embed ctm_map8 "../assets/level1.ctm" 
};


//***************************************************************************************************************
//* FUNCTION PROTOTYPES
//***************************************************************************************************************

void init_player_ship();
void update_player_ship();
void draw_map(uint8_t mx);


//***************************************************************************************************************
//* MAIN
//***************************************************************************************************************
int main() {

    memcpy(SpriteRAM,MySpriteData,64*3);
    
    memcpy(Charset,Level1Tiles,sizeof(Level1Tiles));

    memset(Screen, 0, 1000);
    memset(Color, VCOL_WHITE + 8, 1000);

    spr_init(Screen);

    vic.color_border = VCOL_BLACK;
    vic.color_back = VCOL_BLACK;
    vic.color_back1 = VCOL_LT_GREY;
	vic.color_back2 = VCOL_DARK_GREY;

    vic.spr_mcolor0 = VCOL_DARK_GREY;
    vic.spr_mcolor1 = VCOL_LT_GREY;

    sprite_base = (uint16_t) (SpriteRAM) / 64;

    init_player_ship();

    vic_setmode(VICM_TEXT_MC, Screen, Charset);

    draw_map(0);

    while (true) {
        update_player_ship();

        vic_waitFrame();
    }

    return 0;
}


//***************************************************************************************************************
//* FUNCTION DEFINITIONS
//***************************************************************************************************************

void init_player_ship() {

    spr_set(0,true,player_ship.x,player_ship.y,sprite_base+player_ship.base_frame,player_ship.color,true,false,false);
}


void update_player_ship() {
    joy_poll(0);

    player_ship.x += joyx[0] * PLAYER_SPEED;
    player_ship.y += joyy[0] * PLAYER_SPEED;

    if (player_ship.x < PLAYER_XMIN)
        player_ship.x = PLAYER_XMIN;
    else if (player_ship.x > PLAYER_XMAX)
        player_ship.x = PLAYER_XMAX;

    if (player_ship.y < PLAYER_YMIN)
        player_ship.y = PLAYER_YMIN;
    else if (player_ship.y > PLAYER_YMAX)
        player_ship.y = PLAYER_YMAX;

    spr_move(0, player_ship.x, player_ship.y);

    spr_image(0, sprite_base + player_ship.base_frame + joyy[0]);
}


void draw_map(uint8_t mx) {
    const char * mp = Level1Map + mx;
    char * sp = Screen + 40*13;   

    for(char y=0; y<7; y++) {
        for(char x=0; x<40; x++) { 
            *sp = *mp;
            sp++; 
            mp++;
        }
        mp += 159-40;
    }   
}