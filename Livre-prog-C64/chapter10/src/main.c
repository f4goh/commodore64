/******************************************************************************
 * @file      main.c
 * @author    Stefano Coppi
 * @project   Chrome Horizon / Modern Code, Classic Steel
 * * @description
 * This source code is part of the examples from the book:
 * "Modern Code, Classic Steel" by Stefano Coppi.
 *
 * Chapter10: Sprite Multiplexer 
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

#define MAX_BULLETS 2
#define FIRE_TYPE_BASE 0

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
    uint8_t  y;
    uint8_t  active;
    uint8_t  typeID;
    uint8_t  currentFrame;

    uint8_t  bbox_x;
    uint8_t  bbox_y;
    uint8_t  bbox_w;
    uint8_t  bbox_h;
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
} Enemy;


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
    { 3        ,6    ,100   ,7    ,0     ,8     ,16    ,5 },  
    { 3        ,8    ,100   ,1    ,0     ,0     ,0     ,0  }   
};

Bullet bulletPool[MAX_BULLETS];
uint8_t currentWeaponType = FIRE_TYPE_BASE; 
uint8_t firePressedLastFrame = 0; 

WaveDefinition level1_waves[] = {
    // SpawnX, Tipo      , Conteggio, Spacing, StartY, speedX, PathID,     
    {    340 , 0         , 1        , 0      , 110   , 2     , PATH_SINUSOID},
    {    550 , 1         , 4        , 20     , 80    , 2     , PATH_LINEAR},
    {    630 , 1         , 4        , 20     , 120   , 2     , PATH_LINEAR},
    { 0xFFFF } // Terminatore della lista (indica la fine delle ondate)
};

uint8_t enemiesToSpawn = 0;
uint8_t spawnTimer = 0;     
uint8_t currentWaveIdx = 0;

Enemy enemyPool[MAX_ENEMIES_POOL];

EnemyType enemyCatalog[] = {
//    color       ,baseFrame,numFrames,animSpeed,hp  ,points,bbox_x,bbox_y,bbox_w,bbox_h
    { VCOL_LT_RED ,4        , 6       ,10       ,100 ,100   ,0     ,0     ,24    ,14 },
    { VCOL_LT_BLUE,10       , 3       ,10       ,100 ,100   ,0     ,0     ,24    ,21 }
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
void spawn_bullet(uint16_t x, uint8_t y, uint8_t typeID);
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

//***************************************************************************************************************
//* MAIN
//***************************************************************************************************************
int main() {
    
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
            update_spawner();
            update_enemy_movement();
            update_enemy_animation();
            check_bullets_enemies_collisions();
            check_player_enemy_collision();
            check_player_background_collision();
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

            bulletPool[i].bbox_x = bulletCatalog[typeID].bbox_x;
            bulletPool[i].bbox_y = bulletCatalog[typeID].bbox_y;
            bulletPool[i].bbox_w = bulletCatalog[typeID].bbox_w;
            bulletPool[i].bbox_h = bulletCatalog[typeID].bbox_h;
            
            uint8_t spriteIdx = i + 2;

             vspr_set(spriteIdx,
                bulletPool[i].x, bulletPool[i].y,
                sprite_base + bulletCatalog[typeID].baseFrame,
                bulletCatalog[typeID].color
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
                vspr_hide(i + 2);
            } else {
                vspr_move(i + 2, bulletPool[i].x, bulletPool[i].y);
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
            
            enemyPool[i].baseFrame = enemyCatalog[wave.enemyTypeID].baseFrame;
            enemyPool[i].maxFrames = enemyCatalog[wave.enemyTypeID].numFrames;
            enemyPool[i].animSpeed = enemyCatalog[wave.enemyTypeID].animSpeed;
            enemyPool[i].animTimer = 0;
            enemyPool[i].currentFrame = 0;

            vspr_set(i+4,enemyPool[i].x,enemyPool[i].y>>8,sprite_base+enemyPool[i].baseFrame,enemyPool[i].color);
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

            vspr_move(4+i,enemyPool[i].x, enemyPool[i].y>>8);

            if (enemyPool[i].x <= 0) {
                
                enemyPool[i].active = 0;
                vspr_hide(4+i);
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
            
            enemyPool[i].animTimer++;

            if (enemyPool[i].animTimer >= enemyPool[i].animSpeed) {
                
                enemyPool[i].animTimer = 0;
                
                enemyPool[i].currentFrame++;

                if (enemyPool[i].currentFrame >= enemyPool[i].maxFrames) {
                    if(enemyPool[i].state == STATE_EXPLODING) {
                        enemyPool[i].active = 0;
                        vspr_hide(4+i);
                        continue;;
                    }

                    enemyPool[i].currentFrame = 0;
                }

                uint8_t finalPtr = enemyPool[i].baseFrame + enemyPool[i].currentFrame;
                
                vspr_image(i+4,sprite_base+finalPtr);
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

        uint16_t b_hit_x = bullet->x + bullet->bbox_x;
        uint8_t  b_hit_y = bullet->y + bullet->bbox_y;
        uint16_t b_right = b_hit_x + bullet->bbox_w;
        uint8_t  b_down  = bullet->y + bullet->bbox_y + bullet->bbox_h;

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
    uint8_t hp = enemyPool[eIdx].hp;

    enemyPool[eIdx].hp -= bulletCatalog[bulletPool[bIdx].typeID].damage;

    if (enemyPool[eIdx].hp <= 0 && enemyPool[eIdx].state == STATE_ALIVE) {

        enemyPool[eIdx].state = STATE_EXPLODING;
        enemyPool[eIdx].baseFrame = 13;
        enemyPool[eIdx].currentFrame = 0;
        enemyPool[eIdx].maxFrames = 8;
        enemyPool[eIdx].animTimer = 0;
        enemyPool[eIdx].animSpeed = 5;
        enemyPool[eIdx].color = VCOL_YELLOW;
        vspr_color(4+eIdx,enemyPool[eIdx].color);

        uint8_t typeID = enemyPool[eIdx].typeID;
        uint16_t pt = enemyCatalog[typeID].points;
        score += pt;
        rirq_wait();
        update_score();
    }

    bulletPool[bIdx].active = 0;
     vspr_hide(2+bIdx);
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
    vspr_color(4+eIdx,enemyPool[eIdx].color);
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
    memset(Screen, 0, 1000);
    memset(Color, VCOL_WHITE + 8, 1000);
    init_hud();
    draw_map(0);
}

void init_gameover() {
    game_over_timer = 0;
    for (int8_t i = 0; i < VSPRITES_MAX; i++) 
        vspr_hide(i);
    draw_gameover();
}




