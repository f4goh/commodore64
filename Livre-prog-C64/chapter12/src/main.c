/******************************************************************************
 * @file      main.c
 * @author    Stefano Coppi
 * @project   Chrome Horizon / Modern Code, Classic Steel
 * * @description
 * This source code is part of the examples from the book:
 * "Modern Code, Classic Steel" by Stefano Coppi.
 *
 * Chapter12: Power Ups
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
#include <c64/memmap.h>
#include <c64/rasterirq.h>
#include <oscar.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <conio.h>
#include <stdio.h>


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

#define MAX_BULLETS 3
#define MAX_ENEMY_BULLETS 2

#define FIRE_TYPE_BASE 0
#define FIRE_TYPE_DOUBLE 2
#define FIRE_TYPE_LASER  3
#define FIRE_TYPE_WAVE   4
#define FIRE_TYPE_BEAM   5
#define BEAM_THRESHOLD   50
#define BAR_LENGTH 22

#define PATH_LINEAR     0
#define PATH_SINUSOID   1

#define MAX_ENEMIES_POOL 8

#define STATE_ALIVE     0
#define STATE_EXPLODING 1

#define STARTING_LIVES 1

#define GAME_RUNNING 0
#define GAME_OVER    1
#define GAME_TITLE   2

#define GAME_OVER_DURATION 60*2

#define PWR_NONE           0
#define PWR_LASER          1
#define PWR_DOUBLE_MISSILE 2
#define PWR_WAVE_FIRE      3
#define PWR_SATELLITE      4

#define FLASH_DURATION 20
//***************************************************************************************************************
//* DATA STRUCTURES DEFINITIONS
//***************************************************************************************************************

typedef struct {
    uint16_t x;
    uint8_t  y;
    uint8_t  color;
    uint8_t  base_frame;

    uint8_t  currentFrame; 
    uint8_t  maxFrames;    
    uint8_t  animTimer;    
    uint8_t  animSpeed;

    uint8_t bbox_x;        
    uint8_t bbox_y;
    uint8_t bbox_w;
    uint8_t bbox_h;
    uint8_t state;
 
} PlayerShip;


typedef struct {
    uint8_t baseFrame;
    uint8_t speed;     
    uint8_t damage;
    uint8_t color;

    uint8_t bbox_x;
    uint8_t bbox_y;
    uint8_t bbox_w;
    uint8_t bbox_h;
} BulletType;


typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t  active;
    uint8_t  typeID;
    uint8_t  currentFrame;
    uint8_t  bbox_x;
    uint8_t  bbox_y;
    uint8_t  bbox_w;
    uint8_t  bbox_h;
    uint8_t  angle;
} Bullet;


typedef struct {
    uint16_t spawnX; 
    uint8_t  enemyTypeID;
    uint8_t  count;       
    uint8_t  spacing;     
    uint8_t  startY;
    uint8_t  speedX;      
    uint8_t  pathID;      
} WaveDefinition;


typedef struct {
    uint8_t color;     
    uint8_t baseFrame;   
    uint8_t numFrames;   
    uint8_t animSpeed;   
    uint8_t hp;          
    uint16_t points;
    
    uint8_t  bbox_x;
    uint8_t  bbox_y;
    uint8_t  bbox_w;
    uint8_t  bbox_h;
    int8_t  bulletTypeID;
    uint8_t bulletPeriod;
    uint16_t bulletAngle;
    uint8_t powerUpTypeID;
} EnemyType;


typedef struct {
    uint16_t x;            
    uint16_t y;            
    uint8_t  active;       
    uint8_t  color;
    
    uint8_t  pathID;       
    uint8_t  pathIdx;      
    uint8_t  speedX;
    
    uint8_t  baseFrame;    
    uint8_t  currentFrame; 
    uint8_t  maxFrames;    
    uint8_t  animTimer;    
    uint8_t  animSpeed;    
    
    int8_t   hp;           
    uint8_t  typeID;
    
    uint8_t  bbox_x;
    uint8_t  bbox_y;
    uint8_t  bbox_w;
    uint8_t  bbox_h;

    uint8_t  state;

    int8_t bulletTypeID;
    uint8_t bulletPeriod;
    uint8_t bulletTimer;
    uint8_t powerUpTypeID;
    uint8_t flashTimer;
} Enemy;

typedef struct {
    uint16_t x;
    uint8_t  y;
    uint8_t  active;

    uint8_t  color;
    uint8_t  baseFrame;
    uint8_t  spriteID;

    uint8_t  typeID;
    
    uint8_t  bbox_x;
    uint8_t  bbox_y;
    uint8_t  bbox_w;
    uint8_t  bbox_h;
} PowerUp;

//***************************************************************************************************************
//* GLOBAL VARIABLES
//***************************************************************************************************************


PlayerShip player_ship = {
    PLAYERX0, PLAYERY0,     // x,y
    VCOL_GREEN,             // color
    1,                      // base_frame
    0,                      // currentFrame
    0,                      // maxFrames
    0,                      // animTimer
    0,                      // animSpeed
    0,                      // bbox_x
    4,                      // bbox_y
    24,                     // bbox_w
    11,                     // bbox_h
    STATE_ALIVE             // state
};

char * const Screen = (char *)    0xc000;      
char * const SpriteRAM = (char *) 0xf000;
char * const Charset = (char *)   0xd000;
char * const Charset2 = (char *)  0xe000;
char * const Color = (char *)     0xd800;

uint8_t sprite_base;


const char MySpriteData[] = {
    #embed spd_sprites lzo "assets/sprites.spd"
};

const char Level1Tiles[] = { 
    #embed ctm_chars lzo "../assets/level1.ctm" 
};

const char Level1Map[] = { 
    #embed ctm_map8 "../assets/level1.ctm" 
};

uint16_t scrollX=8*40-1;

const BulletType bulletCatalog[] = {
//    baseFrame,speed,damage,color,bbox_x,bbox_y,bbox_w,bbox_h
    { 3        ,6    ,10    ,7    ,0     ,8     ,16    ,5 },  // #0: FIRE_TYPE_BASE
    { 21       ,3    ,10    ,7    ,10    ,8     ,6     ,4 },  // #1: enemy bullet  
    { 22       ,6    ,10    ,7    ,8     ,8     ,8     ,6 },  // #2: FIRE_TYPE_DOUBLE
    { 23       ,10   ,20    ,14   ,8     ,8     ,8     ,8 },  // #3: FIRE_TYPE_LASER
    { 25       ,8    ,10    ,13   ,0     ,2     ,24    ,18},  // #4: FIRE_TYPE_WAVE
    { 26       ,10   ,20    ,13   ,0     ,8     ,24    ,8 }   // #5: FIRE_TYPE_BEAM
};

Bullet bulletPool[MAX_BULLETS];
uint8_t currentWeaponType = FIRE_TYPE_BASE; 
uint8_t firePressedLastFrame = 0; 

WaveDefinition level1_waves[] = {
    // SpawnX, Tipo      , Conteggio, Spacing, StartY, speedX, PathID,     
    {    340 , 0         , 1        , 0      , 110   , 2     , PATH_SINUSOID},
    {    460 , 2         , 1        , 0      , 90    , 2     , PATH_LINEAR},
    {    670 , 1         , 4        , 20     , 80    , 2     , PATH_LINEAR},
    {    750 , 1         , 4        , 20     , 120   , 2     , PATH_LINEAR},
    { 0xFFFF }
};

uint8_t enemiesToSpawn = 0;
uint8_t spawnTimer = 0;     
uint8_t currentWaveIdx = 0;

Enemy enemyPool[MAX_ENEMIES_POOL];

EnemyType enemyCatalog[] = {
//    color       ,baseFrame,numFrames,animSpeed,hp  ,points,bbox_x,bbox_y,bbox_w,bbox_h,bulletTypeID,bulletPeriod,bulletAngle,powerUpTypeID
    { VCOL_LT_RED ,4        , 6       ,10       ,10  ,100   ,0     ,0     ,24    ,14    ,-1          ,0           ,0          ,PWR_NONE},
    { VCOL_LT_BLUE,10       , 3       ,10       ,10  ,200   ,0     ,0     ,24    ,21    ,1           ,20          ,19         ,PWR_NONE},
    { VCOL_BLUE   ,27       , 1       ,0        ,10  ,100   ,0     ,0     ,22    ,20    ,-1          ,0           ,0          ,PWR_LASER},
};

int16_t sineTableFixed[64];

const char PanelFont[] = { 
    #embed ctm_chars lzo "../assets/low_panel.ctm" 
};

const char PanelMap[] = { 
    #embed ctm_map8 "../assets/low_panel.ctm" 
};

const char PanelAttr[] = {
	#embed ctm_attr1 "../assets/low_panel.ctm"
};

RIRQCode irq_charset_change;
RIRQCode irq_panel;

uint32_t score = 0;
uint8_t lives = STARTING_LIVES;
uint8_t game_state = GAME_RUNNING;

const char GameOverMap[] = { 
    #embed ctm_map8 "../assets/game over.ctm" 
};

const char GameOverAttr[] = {
	#embed ctm_attr1 "../assets/game over.ctm"
};

int16_t game_over_timer;

Bullet enemyBulletPool[MAX_ENEMY_BULLETS];

int16_t sinTable[36];

PowerUp powerup;

uint8_t beamCharge;

//***************************************************************************************************************
//* FUNCTION PROTOTYPES
//***************************************************************************************************************
void init_video();

void init_player_ship();
void update_player_ship();
void draw_map(uint8_t mx);
void fill_column(unsigned ix, unsigned iy);
void hard_scroll();
void scroll_left();

void check_fire_on_release();
void spawn_bullet(uint16_t x, uint16_t y, uint8_t bulletType,uint8_t frame,uint8_t angle);
void update_bullets();

void update_spawner();
void init_enemy_pool();
void activate_enemy_from_pool(WaveDefinition wave);
void update_enemy_movement();
void generate_sine_fixed();
void update_enemy_animation();

void check_bullets_enemies_collisions();
void handle_enemy_hit(uint8_t eIdx, uint8_t bIdx);
void check_player_enemy_collision();
void handle_player_death(uint8_t eIdx);
void spawn_player_explosion();
void check_player_background_collision();

void init_irq();
void init_hud();

void print_string(char* string,uint16_t x,uint8_t y);
void update_score();
void update_lives();
void draw_gameover();

void init_gamestate_running();
void init_gameover();

void spawn_enemy_bullet(uint16_t x, uint8_t y, uint8_t bulletTypeID,uint8_t typeID);
void update_enemy_bullets();
void check_bullets_player_collisions();
void generate_sine_table();

uint8_t check_background_collision(uint16_t x, uint8_t y);

void spawn_powerup(uint16_t x, uint8_t y, uint8_t typeID);
void update_powerup();
void check_player_powerup_collision();
int8_t calc_free_bullets();
void update_hud_beam();

//***************************************************************************************************************
//* MAIN
//***************************************************************************************************************
int main() {
    
    generate_sine_table();
    generate_sine_fixed();
    init_video();
    
    init_player_ship();

    draw_map(0);

    init_enemy_pool();
    init_hud();
    init_irq();

    while (true) {
        switch (game_state)
        {
        case GAME_RUNNING:
            scroll_left();

            update_player_ship();
            check_fire_on_release();
            update_bullets();
            update_enemy_bullets();
            update_spawner();
            update_enemy_movement();
            update_enemy_animation();
            check_bullets_enemies_collisions();
            check_player_enemy_collision();
            check_player_background_collision();
            check_bullets_player_collisions();
            update_powerup();
            check_player_powerup_collision();
            break;

        case GAME_OVER:
            vic_waitFrame();
            game_over_timer++;
            if(game_over_timer >= GAME_OVER_DURATION) {
                init_gamestate_running();
                game_state = GAME_RUNNING;
            }

            break;
        
        default:
            break;
        }

        vspr_sort();
        rirq_wait();
        vspr_update();
        rirq_sort();
    }

    return 0;
}

//***************************************************************************************************************
//* FUNCTION DEFINITIONS
//***************************************************************************************************************

void init_player_ship() {

    player_ship.x = PLAYERX0;
    player_ship.y = PLAYERY0;
    player_ship.state = STATE_ALIVE;
    player_ship.color = VCOL_GREEN;
    player_ship.base_frame = 1;
    player_ship.currentFrame = 0;
    player_ship.maxFrames = 0;
    player_ship.animTimer = 0;
    player_ship.animSpeed = 0;

    vspr_set(0,player_ship.x,player_ship.y,
             sprite_base+player_ship.base_frame,player_ship.color);
}

void update_player_ship() {
    if (player_ship.state == STATE_ALIVE) {
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

        vspr_move(0, player_ship.x, player_ship.y);

        vspr_image(0, sprite_base + player_ship.base_frame + joyy[0]);
    } else if (player_ship.state == STATE_EXPLODING) {
        player_ship.animTimer++;
        if (player_ship.animTimer >= player_ship.animSpeed) {
            player_ship.animTimer = 0;
            player_ship.currentFrame++;
        }
        if (player_ship.currentFrame >= player_ship.maxFrames) {
            
            if (lives <= 0) {
                init_gameover();
                game_state = GAME_OVER;
            } else {
                init_player_ship();
            }
        }
        else
            vspr_image(0, sprite_base + player_ship.base_frame + player_ship.currentFrame);
            
        
    }
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
        rirq_data(&irq_charset_change, 4, ((scrollX & 7) ^ 7) | VIC_CTRL2_MCM);
        rirq_data(&irq_panel, 2, VIC_CTRL2_MCM);

        if ((scrollX & 7) == 0) {
            hard_scroll();
            fill_column(scrollX, 0);
        }
        else
            vic_waitTop();
    }
}

void check_fire_on_release() {
    if (joyb[0]) {
        firePressedLastFrame = 1;

        if (beamCharge < BEAM_THRESHOLD) {
            beamCharge++;
            update_hud_beam();
        }
        return;
    }

    if (firePressedLastFrame) {
        const BulletType *bCat = &bulletCatalog[currentWeaponType];
        uint8_t baseF = bCat->baseFrame;
        uint16_t px = player_ship.x;
        uint8_t py = player_ship.y;

        if (beamCharge >= BEAM_THRESHOLD) {
            spawn_bullet(player_ship.x, player_ship.y, FIRE_TYPE_BEAM, bulletCatalog[FIRE_TYPE_BEAM].baseFrame, 0);
        } else {

            switch (currentWeaponType) {
                case FIRE_TYPE_BASE:
                case FIRE_TYPE_WAVE:
                    spawn_bullet(px, py, currentWeaponType, baseF, 0);
                    break;

                case FIRE_TYPE_DOUBLE:
                    if (calc_free_bullets() >= 2) {
                        spawn_bullet(px, py - 3, currentWeaponType, baseF, 0);
                        spawn_bullet(px, py + 5, currentWeaponType, baseF, 0);
                    }
                    break;

                case FIRE_TYPE_LASER:
                    if (calc_free_bullets() >= 3) {
                        spawn_bullet(px, py, FIRE_TYPE_BASE, bulletCatalog[FIRE_TYPE_BASE].baseFrame, 0);
                        spawn_bullet(px, py, FIRE_TYPE_LASER, baseF + 1, 4);
                        spawn_bullet(px, py, FIRE_TYPE_LASER, baseF, 31);
                    }
                    break;
            }
        }

        firePressedLastFrame = 0;
        beamCharge = 0;
        update_hud_beam();
    }
}

void spawn_bullet(uint16_t x, uint16_t y, uint8_t bulletType, uint8_t frame, uint8_t angle) {
    const BulletType *cat = &bulletCatalog[bulletType];

    for (uint8_t i = 0; i < MAX_BULLETS; i++) {   
        Bullet *b = &bulletPool[i];

        if (!b->active) {               
            b->active = 1;
            b->typeID = bulletType;
            b->angle  = angle; 

            b->x = (x + 18) << 6;  
            b->y = (y -2) << 6;

            b->bbox_x = cat->bbox_x;
            b->bbox_y = cat->bbox_y;
            b->bbox_w = cat->bbox_w;
            b->bbox_h = cat->bbox_h;

            vspr_set(i + 3,
                x + 18, 
                y - 2,
                sprite_base + frame,
                cat->color
            );

            return;
        }
    }
}

void update_bullets() {
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &bulletPool[i];
        
        if (b->active) {
            uint8_t speed = bulletCatalog[b->typeID].speed;
            uint8_t angle = b->angle;

            int16_t dx = (int16_t)sinTable[(angle + 9) % 36] * speed;
            int16_t dy = (int16_t)sinTable[angle % 36] * speed;

            int16_t nextX = b->x + dx;
            int16_t nextY = b->y + dy;

            uint16_t px = nextX >> 6;
            uint16_t py = nextY >> 6;

            if (check_background_collision(px, py)) {
                b->angle = (36 - b->angle); 
                if (b->angle >= 36) b->angle -= 36;
     
                b->y -= (8 << 6);
                vspr_movey(i + 3, b->y >> 6);

                uint8_t newFrame = (b->angle >= 32) ? 23 : 24;
                vspr_image(i + 3, sprite_base + newFrame);
                continue;
            }

            b->x = nextX;
            b->y = nextY;

            if (px < 31 || px > 335 || py < 50 || py > 210) {
                b->active = 0;
                vspr_hide(i + 3);
            } else {
                vspr_move(i + 3, px, py);
            }
        }
    }
}

void update_spawner() {
    if (level1_waves[currentWaveIdx].spawnX == 0xFFFF) {
        return;
    }

    if (level1_waves[currentWaveIdx].spawnX == scrollX) {
        enemiesToSpawn = level1_waves[currentWaveIdx].count;
        spawnTimer = 0;
    }

    if (enemiesToSpawn > 0) {
        if (spawnTimer == 0) {
            activate_enemy_from_pool(level1_waves[currentWaveIdx]);

            enemiesToSpawn--;
            spawnTimer = level1_waves[currentWaveIdx].spacing;

            if (enemiesToSpawn == 0) currentWaveIdx++;
        } else {
            spawnTimer--;
        }
    }
}

void init_enemy_pool() {
    for (uint8_t i = 0; i < MAX_ENEMIES_POOL; i++) {
        enemyPool[i].active = 0;
    }
}

void activate_enemy_from_pool(WaveDefinition wave) {

    for (uint8_t i = 0; i < MAX_ENEMIES_POOL; i++) {
        if (enemyPool[i].active == 0) {
            enemyPool[i].active = 1;         
            enemyPool[i].x = 344;            
            enemyPool[i].y = wave.startY <<8;    
            enemyPool[i].speedX = wave.speedX;
            enemyPool[i].pathID = wave.pathID;
            enemyPool[i].pathIdx = 0;        
            uint8_t id = wave.enemyTypeID;
            uint8_t color = enemyCatalog[id].color;
            uint8_t baseFrame = enemyCatalog[id].baseFrame;
            enemyPool[i].typeID = id;
            enemyPool[i].color = enemyCatalog[wave.enemyTypeID].color;
            enemyPool[i].hp = enemyCatalog[id].hp;
            enemyPool[i].state = STATE_ALIVE;
            enemyPool[i].bbox_x = enemyCatalog[id].bbox_x;
            enemyPool[i].bbox_y = enemyCatalog[id].bbox_y;
            enemyPool[i].bbox_w = enemyCatalog[id].bbox_w;
            enemyPool[i].bbox_h = enemyCatalog[id].bbox_h;
            enemyPool[i].bulletTypeID = enemyCatalog[id].bulletTypeID;
            enemyPool[i].bulletPeriod = enemyCatalog[id].bulletPeriod;
            enemyPool[i].bulletTimer = 0;
            
            enemyPool[i].baseFrame = enemyCatalog[wave.enemyTypeID].baseFrame;
            enemyPool[i].maxFrames = enemyCatalog[wave.enemyTypeID].numFrames;
            enemyPool[i].animSpeed = enemyCatalog[wave.enemyTypeID].animSpeed;
            enemyPool[i].animTimer = 0;
            enemyPool[i].currentFrame = 0;

            enemyPool[i].powerUpTypeID = enemyCatalog[wave.enemyTypeID].powerUpTypeID;
            enemyPool[i].flashTimer = 0;

            vspr_set(i+8,enemyPool[i].x,enemyPool[i].y>>8,sprite_base+enemyPool[i].baseFrame,enemyPool[i].color);
            return;
        }
    }
}

void update_enemy_movement() {
    for (uint8_t i = 0; i < MAX_ENEMIES_POOL; i++) {
        if (enemyPool[i].active && enemyPool[i].state == STATE_ALIVE) {
            
            int8_t dx = 0;
            int16_t dy = 0;
            uint8_t idx = enemyPool[i].pathIdx;

            switch (enemyPool[i].pathID) {
                case PATH_LINEAR:
                    dx = -enemyPool[i].speedX;
                    dy = 0;
                    break;

                case PATH_SINUSOID:
                    dx = -2;
                    dy = sineTableFixed[idx & 63];
                    break;
            }

            enemyPool[i].x += dx;
            enemyPool[i].y += dy;

            enemyPool[i].pathIdx++;

            vspr_move(i+8,enemyPool[i].x, enemyPool[i].y>>8);

            enemyPool[i].bulletTimer++;
            if( enemyPool[i].bulletTimer >= enemyPool[i].bulletPeriod) {
                if (enemyPool[i].bulletTypeID != -1) {

                    spawn_enemy_bullet(enemyPool[i].x,enemyPool[i].y>>8,enemyPool[i].bulletTypeID,enemyPool[i].typeID);
                    enemyPool[i].bulletTimer = 0;
                }
            }

            if (enemyPool[i].x <= 0) {
                
                enemyPool[i].active = 0;
                vspr_hide(i+8);
            }
        }
    }
}


void generate_sine_fixed() {
    for(int i = 0; i < 64; i++) {
       
        float angle = (2.0 * PI * i) / 64.0;
        float sineValue = sin(angle);
        
        sineTableFixed[i] = (int16_t)(sineValue * 256.0);
    }
}

void update_enemy_animation() {
    for (uint8_t i = 0; i < MAX_ENEMIES_POOL; i++) {
        if (enemyPool[i].active) {

            if (enemyPool[i].flashTimer > 0) {
                enemyPool[i].flashTimer--;
                
                if (enemyPool[i].flashTimer & 1) {
                    vspr_color(i + 8, VCOL_BLACK);
                } else {
                    vspr_color(i + 8, enemyPool[i].color);
                }
            }
            
            enemyPool[i].animTimer++;

            if (enemyPool[i].animTimer >= enemyPool[i].animSpeed) {
                
                enemyPool[i].animTimer = 0;
                
                enemyPool[i].currentFrame++;

                if (enemyPool[i].currentFrame >= enemyPool[i].maxFrames) {
                    if(enemyPool[i].state == STATE_EXPLODING) {
                        enemyPool[i].active = 0;
                        vspr_hide(i+8);
                        if(enemyPool[i].powerUpTypeID != PWR_NONE) {
                            spawn_powerup(enemyPool[i].x, enemyPool[i].y >>8,enemyPool[i].powerUpTypeID);
                        }
                        continue;;
                    }

                    enemyPool[i].currentFrame = 0;
                }

                uint8_t finalPtr = enemyPool[i].baseFrame + enemyPool[i].currentFrame;
                
                vspr_image(i+8,sprite_base+finalPtr);
            }
        }
    }
}

void init_video() {
    mmap_trampoline();
    mmap_set(MMAP_RAM);

    oscar_expand_lzo(SpriteRAM, MySpriteData);
    oscar_expand_lzo(Charset, Level1Tiles);
    oscar_expand_lzo(Charset2,PanelFont);

    mmap_set(MMAP_ROM);

    memset(Screen, 0, 1000);
    memset(Color, VCOL_WHITE + 8, 1000);

    spr_init(Screen);

    vic.color_border = VCOL_BLACK;
    vic.color_back = VCOL_BLACK;
    vic.color_back1 = VCOL_LT_GREY;
	vic.color_back2 = VCOL_DARK_GREY;

    vic.spr_mcolor0 = VCOL_DARK_GREY;
    vic.spr_mcolor1 = VCOL_LT_GREY;
    vic.spr_multi = 0xff;

    sprite_base = (uint16_t)(SpriteRAM - 0xC000) / 64;

    vic_setmode(VICM_TEXT_MC, Screen, Charset);

    rirq_init_kernal();

    vspr_init(Screen);
    vspr_sort();
	vspr_update();
	rirq_sort();
	rirq_start();
}


void check_bullets_enemies_collisions() {
    for (uint8_t b = 0; b < MAX_BULLETS; b++) {
        Bullet *bullet = &bulletPool[b];
        if (!bullet->active) continue;

        uint16_t b_hit_x = (bullet->x >> 6) + bullet->bbox_x;
        uint8_t  b_hit_y = (bullet->y >> 6) + bullet->bbox_y;
        uint16_t b_right = b_hit_x + bullet->bbox_w;
        uint8_t  b_down  = b_hit_y + bullet->bbox_h;

        for (uint8_t e = 0; e < MAX_ENEMIES_POOL; e++) {
            Enemy *enemy = &enemyPool[e];
            
            if (enemy->active != 1 || enemy->state == STATE_EXPLODING) continue;

            uint16_t ex_left = enemy->x + enemy->bbox_x;
            if (b_right < ex_left) continue;

            if (b_hit_x > (ex_left + enemy->bbox_w)) continue;

            uint8_t ey = (uint8_t)(enemy->y >> 8) + enemy->bbox_y;
            if (b_down < ey) continue;
            if (b_hit_y > (ey + enemy->bbox_h)) continue;

            handle_enemy_hit(e, b);
            break; 
        }
    }
}


void handle_enemy_hit(uint8_t eIdx, uint8_t bIdx) {

    uint8_t damage = bulletCatalog[bulletPool[bIdx].typeID].damage;

    enemyPool[eIdx].hp -= damage;

    if (enemyPool[eIdx].hp <= 0 && enemyPool[eIdx].state == STATE_ALIVE) {

        enemyPool[eIdx].state = STATE_EXPLODING;
        enemyPool[eIdx].baseFrame = 13;
        enemyPool[eIdx].currentFrame = 0;
        enemyPool[eIdx].maxFrames = 8;
        enemyPool[eIdx].animTimer = 0;
        enemyPool[eIdx].animSpeed = 5;
        enemyPool[eIdx].color = VCOL_YELLOW;
        vspr_color(8+eIdx,enemyPool[eIdx].color);

        uint8_t typeID = enemyPool[eIdx].typeID;
        uint16_t pt = enemyCatalog[typeID].points;
        score += pt;
        rirq_wait();
        update_score();
    }
    else {
        enemyPool[eIdx].flashTimer = FLASH_DURATION;
    }

    if (bulletPool[bIdx].typeID != FIRE_TYPE_BEAM) {
        bulletPool[bIdx].active = 0;
        vspr_hide(3+bIdx);
    }
}


void check_player_enemy_collision() {
    if (player_ship.state != STATE_ALIVE) return;

    uint16_t p_left   = player_ship.x + player_ship.bbox_x;
    uint16_t p_right  = p_left + player_ship.bbox_w;
    uint8_t  p_top    = player_ship.y + player_ship.bbox_y;
    uint8_t  p_bottom = p_top + player_ship.bbox_h;

    for (uint8_t i = 0; i < MAX_ENEMIES_POOL; i++) {
        Enemy *enemy = &enemyPool[i];

        if (!enemy->active || enemy->state == STATE_EXPLODING) continue;

        uint16_t e_left = enemy->x + enemy->bbox_x;
        if (p_right < e_left) continue; 

        uint16_t e_right = e_left + enemy->bbox_w;
        if (p_left > e_right) continue;

        uint8_t e_top = (uint8_t)(enemy->y >> 8) + enemy->bbox_y;
        if (p_bottom < e_top) continue;

        uint8_t e_bottom = e_top + enemy->bbox_h;
        if (p_top > e_bottom) continue;

        player_ship.state = STATE_EXPLODING;
        handle_player_death(i);
        return; 
    }
}

void handle_player_death(uint8_t eIdx) {
    spawn_player_explosion();
    
    enemyPool[eIdx].state = STATE_EXPLODING;
    enemyPool[eIdx].baseFrame = 13;
    enemyPool[eIdx].currentFrame = 0;
    enemyPool[eIdx].maxFrames = 8;
    enemyPool[eIdx].animTimer = 0;
    enemyPool[eIdx].animSpeed = 5;
    enemyPool[eIdx].color = VCOL_YELLOW;
    vspr_color(8+eIdx,enemyPool[eIdx].color);
}

void spawn_player_explosion() {
    player_ship.state = STATE_EXPLODING;
    
    player_ship.base_frame = 13;
    player_ship.currentFrame = 0;
    player_ship.maxFrames = 8;
    player_ship.animTimer = 0;
    player_ship.animSpeed = 5;

    player_ship.color = VCOL_YELLOW;
    vspr_color(0,player_ship.color);

    lives--;
    update_lives();
}

void check_player_background_collision() {
    if (player_ship.state != STATE_ALIVE) return;

    uint16_t left   = player_ship.x + player_ship.bbox_x;
    uint16_t right  = left + player_ship.bbox_w;
    uint8_t  top    = player_ship.y + player_ship.bbox_y;
    uint8_t  bottom = top + player_ship.bbox_h;

    uint8_t tx1 = (uint8_t)((left - 24) >> 3);
    uint8_t tx2 = (uint8_t)((right - 24) >> 3);
    uint8_t ty1 = (uint8_t)((top - 50) >> 3);
    uint8_t ty2 = (uint8_t)((bottom - 50) >> 3);

    if (tx1 > 39 || ty1 > 24) return; 

    uint16_t row1 = (ty1 << 5) + (ty1 << 3);
    uint16_t row2 = (ty2 << 5) + (ty2 << 3);

    if (Screen[row1 + tx1] > 0 || 
        Screen[row1 + tx2] > 0 || 
        Screen[row2 + tx1] > 0 || 
        Screen[row2 + tx2] > 0) 
    {
        spawn_player_explosion();
    }
}

void init_irq() {
    
    rirq_build(&irq_charset_change, 5);

    rirq_write(&irq_charset_change, 0, &vic.memptr, 0x04);

    rirq_write(&irq_charset_change, 1, & vic.color_back, VCOL_BLACK);

    rirq_write(&irq_charset_change, 2, &vic.color_back1, VCOL_LT_GREY);

    rirq_write(&irq_charset_change, 3, &vic.color_back2, VCOL_DARK_GREY);

    rirq_write(&irq_charset_change, 4, &vic.ctrl2, 0);

    rirq_set(8, 1, &irq_charset_change);


    
    rirq_build(&irq_panel,5);

    rirq_write(&irq_panel, 0, &vic.memptr, 0x08);

    rirq_write(&irq_panel, 1, &vic.color_back, VCOL_BLACK);

    rirq_write(&irq_panel, 2, &vic.ctrl2, 0);
    
    rirq_write(&irq_panel, 3, &vic.color_back1, VCOL_DARK_GREY);

    rirq_write(&irq_panel, 4, &vic.color_back2, VCOL_LT_GREY);

    rirq_set(10, 211, &irq_panel);

    rirq_sort();
    rirq_start(); 
}

void init_hud() {
    char * sp = Screen + 20*40;
    char * cp = Color + 20*40;
    
    for(char y=0; y<5; y++) 
        for(char x=0; x<40; x++) {
            
            char tile = PanelMap[x + 40 * y];
            
            sp[0] = tile;
            cp[0] = PanelAttr[tile];
            sp++;
            cp++;
        }
    
    update_score();
    update_lives();
    update_hud_beam();
}

void print_string(char* string, uint16_t x, uint8_t y) {
    char * screen_ptr = Screen + x + (y << 5) + (y << 3);

    while (*string) {
        *screen_ptr++ = *string++;
    }
}

void update_score() {
    char s[6];
    sprintf(s,"%05ld",score);
    print_string(s,9,23);
}

void update_lives() {
    char s[4];
    int8_t i;
    char * screen_ptr = Screen + 1 + 40*21;
    memset(screen_ptr,0,4);
    for(i=0; i<lives; i++) 
        s[i] = 13;
    s[i] = 0;
    print_string(s,1,21);
}

void draw_gameover() {
    const char* mp = GameOverMap;
    const char* am = GameOverAttr;
    char* sp = Screen + 40*8 +6;
    char* cm = Color +40*8 +6;
    char tile;
    uint8_t color;

    for(int8_t y=0; y<2; y++) {
        for(int8_t x=0; x<26; x++) {
            tile = *mp;
            *sp = tile;
            mp++;
            sp++;
            color = am[tile];
            *cm = color;
            cm++;
        }
        sp += 40-26;
        cm += 40-26;
    }
}

void init_gamestate_running() {
    init_player_ship();
    scrollX = 8*40-1;
    lives = STARTING_LIVES;
    score = 0;
    currentWaveIdx = 0;
    currentWeaponType = FIRE_TYPE_BASE;
    beamCharge = 0;
    firePressedLastFrame = 0;
    memset(Screen, 0, 1000);
    memset(Color, VCOL_WHITE + 8, 1000);
    init_hud();
    draw_map(0);
}

void init_gameover() {
    game_over_timer = 0;
    for (int8_t i=0; i < MAX_ENEMIES_POOL; i++)
        enemyPool[i].active = false;
    for (int8_t i=0; i < MAX_ENEMY_BULLETS; i++)
        enemyBulletPool[i].active = false;
    for (int8_t i = 0; i < VSPRITES_MAX; i++) 
        vspr_hide(i);
    draw_gameover();
}

void spawn_enemy_bullet(uint16_t x, uint8_t y, uint8_t bulletTypeID,uint8_t typeID) {
    
    for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; i++) {   
        if (!enemyBulletPool[i].active) {               
            
            enemyBulletPool[i].active = 1;
            enemyBulletPool[i].typeID = bulletTypeID;
            enemyBulletPool[i].x = (int16_t)x << 6;  
            enemyBulletPool[i].y = (int16_t)y << 6; 
    
            enemyBulletPool[i].currentFrame = 0;

            enemyBulletPool[i].bbox_x = bulletCatalog[bulletTypeID].bbox_x;
            enemyBulletPool[i].bbox_y = bulletCatalog[bulletTypeID].bbox_y;
            enemyBulletPool[i].bbox_w = bulletCatalog[bulletTypeID].bbox_w;
            enemyBulletPool[i].bbox_h = bulletCatalog[bulletTypeID].bbox_h;
            
            enemyBulletPool[i].angle  = enemyCatalog[typeID].bulletAngle;

            uint8_t spriteIdx = i + 6;

            vspr_set(spriteIdx,
                enemyBulletPool[i].x >> 6, enemyBulletPool[i].y >> 6,
                sprite_base + bulletCatalog[bulletTypeID].baseFrame,
                bulletCatalog[bulletTypeID].color
            );

            return;
        }
    }
} 

void update_enemy_bullets() {
    for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; i++) {
        Bullet *eb = &enemyBulletPool[i];

        if (eb->active) {
            uint8_t speed = bulletCatalog[eb->typeID].speed;

            uint16_t angle = eb->angle;

            int16_t dx = (int16_t) sinTable[(angle+9) % 36] * speed;
            int16_t dy = (int16_t) sinTable[angle] * speed;
            
            eb->x += dx;
            eb->y += dy;
            
            uint16_t x_pixel = eb->x >> 6;
            uint16_t y_pixel = eb->y >> 6;

            if (x_pixel < 31 || x_pixel > 335 || y_pixel < 50 || y_pixel > 200) {
                eb->active = 0;
                vspr_hide(i + 6);
            } else {
                vspr_move(i + 6, x_pixel, y_pixel);
            }
        }
    }
}

void check_bullets_player_collisions() {
    if (player_ship.state == STATE_EXPLODING) return;

    uint16_t p_left   = player_ship.x + player_ship.bbox_x;
    uint16_t p_right  = p_left + player_ship.bbox_w;
    uint8_t  p_top    = player_ship.y + player_ship.bbox_y;
    uint8_t  p_bottom = p_top + player_ship.bbox_h;

    for (uint8_t b = 0; b < MAX_ENEMY_BULLETS; b++) {
        Bullet *eb = &enemyBulletPool[b];
        if (!eb->active) continue;

        uint16_t bx_hit = (eb->x >> 6) + eb->bbox_x;

        if ((bx_hit + +eb->bbox_w) < p_left) continue;
        if (bx_hit > p_right) continue;

        uint8_t by_hit = (uint8_t)(eb->y >> 6) + eb->bbox_y;
        if ((by_hit + eb->bbox_h) < p_top) continue;
        if (by_hit > p_bottom) continue;

        player_ship.state = STATE_EXPLODING;
        spawn_player_explosion();

        eb->active = 0;
        vspr_hide(6 + b);

        return;
    }
}

void generate_sine_table() {
    for (int i = 0; i < 36; i++) {
        float radians = (float)i*10 * (PI / 180.0);
        sinTable[i] = (int16_t)(sin(radians) * 64.0);
    }
}


uint8_t check_background_collision(uint16_t x, uint8_t y) {
    if (x < 31 || x > 335 || y < 50 || y > 249) return 0;

    uint8_t col = (uint8_t)((x - 31) >> 3);
    uint8_t row = (y - 50) >> 3;

    uint16_t offset = (uint16_t)(row << 5) + (row << 3) + col;

    if (Screen[offset] > 0) {
        return 1;
    }

    return 0;
}

void spawn_powerup(uint16_t x, uint8_t y, uint8_t typeID) 
{
    powerup.x = x;
    powerup.y = y;
    powerup.active = 1;

    powerup.baseFrame = 28;
    powerup.typeID = typeID;
    powerup.spriteID = 2;

    powerup.bbox_x = 4;
    powerup.bbox_y = 4;
    powerup.bbox_w = 18;
    powerup.bbox_h = 15;

    switch (typeID) {
        case PWR_LASER:
            powerup.color = VCOL_BLUE;
            break;
        case PWR_DOUBLE_MISSILE:
            powerup.color = VCOL_RED;
            break;
        case PWR_WAVE_FIRE:
            powerup.color = VCOL_PURPLE;
            break;
        case PWR_SATELLITE:
            powerup.color = VCOL_LT_GREEN;
            break;
    }

    vspr_set(powerup.spriteID, powerup.x, powerup.y, sprite_base + powerup.baseFrame,
             powerup.color);

    return;
}

void update_powerup()
{
    PowerUp *p = &powerup;

    if (p->active == 0)
        return;

    p->x--;
    vspr_movex(p->spriteID, p->x);

    if (p->x < 7) {
        p->active = 0;
        vspr_hide(p->spriteID);
    }
}

void check_player_powerup_collision() {
    if (player_ship.state != STATE_ALIVE) return;    
    PowerUp *p = &powerup;
    if (p->active == 0) return;

    uint16_t p_left   = player_ship.x + player_ship.bbox_x;
    uint16_t p_right  = p_left + player_ship.bbox_w;
    uint8_t  p_top    = player_ship.y + player_ship.bbox_y;
    uint8_t  p_bottom = p_top + player_ship.bbox_h;

    uint16_t i_left = p->x + p->bbox_x;
    if (i_left > p_right) return;

    uint16_t i_right = i_left + p->bbox_w;
    if (i_right < p_left) return;

    uint8_t i_top = p->y + p->bbox_y;
    if (i_top > p_bottom) return;

    uint8_t i_bottom = i_top + p->bbox_h;
    if (i_bottom < p_top) return;

    p->active = 0;
    vspr_hide(p->spriteID);

    if (p->typeID < PWR_SATELLITE) {
        switch (p->typeID) {
            case PWR_LASER:
                currentWeaponType = FIRE_TYPE_LASER;
                break;
            case PWR_DOUBLE_MISSILE:
                currentWeaponType = FIRE_TYPE_DOUBLE;
                break;
            case PWR_WAVE_FIRE:
                currentWeaponType = FIRE_TYPE_WAVE;
                break;
        }
    }
}


int8_t calc_free_bullets() {
    int8_t free_bullets = 0;
    Bullet *b = bulletPool;

    for (int8_t i = 0; i < MAX_BULLETS; i++) {
        if (b->active == 0) {
            free_bullets++;
        }
        b++;
    }
    
    return free_bullets;
}


void update_hud_beam() {
    uint8_t filledBlocks = (uint8_t)((uint16_t)beamCharge * BAR_LENGTH / BEAM_THRESHOLD);

    uint8_t* p = (uint8_t*)(Screen + 40 * 21 + 16);

    for (uint8_t i = 0; i < BAR_LENGTH; i++) {
        if (i < filledBlocks) {
            *p = 14;
        } else {
            *p = 7;
        }
        p++;
    }
}