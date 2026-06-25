/******************************************************************************
 * @file      main.c
 * @author    Stefano Coppi
 * @project   Chrome Horizon / Modern Code, Classic Steel
 * * @description
 * This source code is part of the examples from the book:
 * "Modern Code, Classic Steel" by Stefano Coppi.
 *
 * Chapter05: Player ship's firing system
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

#define MAP_WIDTH 159
#define SCREEN_WIDTH 40
#define MAP_MAX_X (MAP_WIDTH*8)

#define MAX_BULLETS 2
#define FIRE_TYPE_BASE 0


//***************************************************************************************************************
//* DATA STRUCTURES DEFINITIONS
//***************************************************************************************************************

typedef struct {
    uint16_t x;
    uint8_t  y;
    uint8_t  color;
    uint8_t  base_frame;
} PlayerShip;


typedef struct {
    uint8_t baseFrame;
    uint8_t speed;     
    uint8_t damage;
    uint8_t color;
} BulletType;


typedef struct {
    uint16_t x;
    uint8_t  y;
    uint8_t  active;
    uint8_t  typeID;
    uint8_t  currentFrame;
} Bullet;


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

uint16_t scrollX=8*40-1;


const BulletType bulletCatalog[] = {
//    baseFrame,speed,damage,color
    { 3        ,6    ,100   ,7     },  
    { 3        ,8    ,100   ,1     }   
};


Bullet bulletPool[MAX_BULLETS];
uint8_t currentWeaponType = FIRE_TYPE_BASE; 
uint8_t firePressedLastFrame = 0; 

//***************************************************************************************************************
//* FUNCTION PROTOTYPES
//***************************************************************************************************************

void init_player_ship();
void update_player_ship();
void draw_map(uint8_t mx);
void fill_column(unsigned ix, unsigned iy);
void hard_scroll();
void scroll_left();

void check_fire_on_release();
void spawn_bullet(uint16_t x, uint8_t y, uint8_t typeID);
void update_bullets();


//***************************************************************************************************************
//* MAIN
//***************************************************************************************************************
int main() {

    memcpy(SpriteRAM,MySpriteData,sizeof(MySpriteData));
    
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

    draw_map(0);

    vic_setmode(VICM_TEXT_MC, Screen, Charset);

    while (true) {
        scroll_left();

        update_player_ship();
        check_fire_on_release();
        update_bullets();
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
        mp += MAP_WIDTH-SCREEN_WIDTH;
    }   
}


void fill_column(unsigned ix, unsigned iy) {
    char * sp = Screen + 40*13 + 39;
    unsigned sx = (ix >> 3);
    unsigned sy = (iy >> 3);

    for(char y=0; y<7; y++) {
        char tile = Level1Map[sx + MAP_WIDTH*sy];
        
        sp[0] = tile;
        sp += 40;
        sy++;
    }
}


void hard_scroll() {
    
    for(char i=0; i<39; i++) {
        #pragma unroll(full)
        for(char j=13; j<20; j++)
            Screen[40 * j + i] = Screen[40 * j + i + 1];
    }
}

void scroll_left() {
    if (scrollX < MAP_MAX_X) {
        vic_waitBottom();
        scrollX++;
        
        vic.ctrl2 = ((scrollX & 7) ^ 7) | VIC_CTRL2_MCM;
        vic.ctrl2 &= ~VIC_CTRL2_CSEL;

        if ((scrollX & 7) == 0) {
            hard_scroll();
            fill_column(scrollX, 0);
        }
        else
            vic_waitTop();
    }
}

void check_fire_on_release() {
    uint8_t fireIsPressed = joyb[0];

    if (fireIsPressed) {
        firePressedLastFrame = 1;
    } else {
        if (firePressedLastFrame == 1) {
            spawn_bullet(player_ship.x, player_ship.y, FIRE_TYPE_BASE);

            firePressedLastFrame = 0;
        }
    }
}


void spawn_bullet(uint16_t x, uint8_t y, uint8_t typeID) {
    
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {   
        if (!bulletPool[i].active) {               
            
            bulletPool[i].active = 1;
            bulletPool[i].typeID = typeID;
            bulletPool[i].x = x + 18;  
            bulletPool[i].y = y - 2; 

            bulletPool[i].currentFrame = 0;
            
            uint8_t spriteIdx = i + 2;

            spr_set(spriteIdx,true,
                bulletPool[i].x, bulletPool[i].y,
                sprite_base + bulletCatalog[typeID].baseFrame,
                bulletCatalog[typeID].color,
                true,
                false,false
            );

            return;
        }
    }
}

void update_bullets() {
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (bulletPool[i].active) {
            uint8_t speed = bulletCatalog[bulletPool[i].typeID].speed;
            bulletPool[i].x += speed;

            if (bulletPool[i].x > 344) {
                bulletPool[i].active = 0;
                spr_show(i + 2,false);
            } else {
                spr_move(i + 2, bulletPool[i].x, bulletPool[i].y);
            }
        }
    }
}