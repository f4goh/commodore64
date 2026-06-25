//
// Chapter14: end level boss
//
#include <c64/vic.h>
#include <c64/sprites.h>
#include <c64/joystick.h>
#include <c64/memmap.h>
#include <c64/rasterirq.h>
#include <c64/keyboard.h>
#include <c64/cia.h>
#include <oscar.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <conio.h>
#include <stdio.h>


//***************************************************************************************************************
//* DEFINIZIONE DELLE COSTANTI
//***************************************************************************************************************
#define PLAYERX0 50         
#define PLAYERY0 120
#define PLAYER_SPEED 2      
#define PLAYER_XMIN 40     
#define PLAYER_XMAX 320-8   
#define PLAYER_YMIN 50      
#define PLAYER_YMAX 200 

#define MAP_MAX_X 454*8

#define MAX_BULLETS 3
#define MAX_ENEMY_BULLETS 2
#define FIRE_TYPE_BASE   0
#define FIRE_TYPE_DOUBLE 2
#define FIRE_TYPE_LASER  3
#define FIRE_TYPE_WAVE   4
#define FIRE_TYPE_BEAM   5
#define BEAM_THRESHOLD   50

#define PATH_LINEAR     0
#define PATH_SINUSOID   1
#define PATH_NONE       2

#define MAX_ENEMIES_POOL 7

#define STATE_ALIVE     0
#define STATE_EXPLODING 1

#define STARTING_LIVES 1

#define GAME_RUNNING 0
#define GAME_OVER    1
#define GAME_TITLE   2
#define GAME_LEVEL_COMPLETED 3

#define GAME_OVER_DURATION 60*2
#define GAME_LVL_COMP_DURATION 60*2

#define PWR_NONE           0
#define PWR_LASER          1
#define PWR_DOUBLE_MISSILE 2
#define PWR_WAVE_FIRE      3
#define PWR_SATELLITE      4

#define FLASH_DURATION 20

#define SAT_STATE_FREE 0
#define SAT_STATE_RECALL 1
#define SAT_STATE_RELEASE 2
#define SAT_STATE_DOCKED_FRONT 3
#define SAT_STATE_DOCKED_BACK 4

#define SAT_X0 (31+320-16-24)
#define SAT_Y0 (50+60)
#define SAT_SPEED0 -2

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
    
    uint8_t bbox_x;        
    uint8_t bbox_y;
    uint8_t bbox_w;
    uint8_t bbox_h;
    uint8_t state;
} PlayerShip;


typedef struct {
    uint8_t baseFrame;
    uint8_t speed;     
    uint16_t damage;
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
    uint16_t startX;     
    uint8_t  startY;
    uint8_t  speedX;      
    uint8_t  pathID;      
} WaveDefinition;

typedef struct {
    uint8_t color;     
    uint8_t baseFrame;   
    uint8_t numFrames;   
    uint8_t animSpeed;   
    int16_t hp;          
    uint16_t points;
    uint8_t bbox_x;
    uint8_t bbox_y;
    uint8_t bbox_w;
    uint8_t bbox_h;
    int8_t  bulletTypeID;
    uint8_t bulletPeriod;
    uint16_t bulletAngle;
    uint8_t powerUpTypeID;
    uint8_t isBoss;      
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
    
    int16_t  hp;           
    uint8_t  typeID;
    
    uint8_t bbox_x;
    uint8_t bbox_y;
    uint8_t bbox_w;
    uint8_t bbox_h;

    uint8_t  state;

    int8_t bulletTypeID;
    uint8_t bulletPeriod;
    uint8_t bulletTimer;

    uint8_t powerUpTypeID;
    uint8_t flashTimer;

    uint8_t isBoss;
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

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t pathIdx;
    int8_t speedX;

    uint8_t active;
    uint8_t state;

    uint8_t color;
    uint8_t base_frame;

    uint8_t currentFrame;
    uint8_t maxFrames;
    uint8_t animTimer;
    uint8_t animSpeed;

    uint8_t bbox_x;
    uint8_t bbox_y;
    uint8_t bbox_w;
    uint8_t bbox_h;
} Satellite;

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
    10,                      // animSpeed
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

uint8_t sprite_base;                        // frame base per gli sprite

// include gli sprites nel formato SpritePad
const char MySpriteData[] = {
    #embed spd_sprites lzo "assets/sprites.spd"
};

const char Level1Tiles[] = { 
    #embed ctm_chars lzo "../assets/level1_map_low.ctm" 
};

const char Level1Map[] = { 
    #embed ctm_map8 "../assets/level1_map_low.ctm" 
};

uint16_t scrollX= 413*8;


const BulletType bulletCatalog[] = {
//    baseFrame,speed,damage,color,bbox_x,bbox_y,bbox_w,bbox_h
    { 5        ,6    ,100   ,1    ,0     ,8     ,16    ,5 },    // #0: FIRE_TYPE_BASE
    { 14       ,3    ,100   ,7    ,10    ,8     ,6     ,4 },    // #1: enemy bullet  
    { 15       ,6    ,100   ,7    ,8     ,8     ,8     ,6 },    // #2: FIRE_TYPE_DOUBLE
    { 17       ,10   ,200   ,14   ,8     ,8     ,8     ,8 },    // #3: FIRE_TYPE_LASER
    { 16       ,8    ,100   ,1    ,0     ,2     ,24    ,18},    // #4: FIRE_TYPE_WAVE
    { 21       ,10   ,200   ,5    ,0     ,8     ,24    ,8 }     // #5: FIRE_TYPE_BEAM
};


Bullet bulletPool[MAX_BULLETS];
uint8_t currentWeaponType = FIRE_TYPE_BASE; 
uint8_t firePressedLastFrame = 0; 

WaveDefinition level1_waves[] = {
    // SpawnX,Tipo,Conteggio,Spacing,StartX,StartY,speedX,PathID,     
    {      8 ,0   ,1        ,14     ,344   ,110   ,2     ,PATH_SINUSOID},
    {    160 ,2   ,1        ,0      ,344   ,90    ,2     ,PATH_LINEAR},
    {    460 ,1   ,2        ,20     ,344   ,90    ,2     ,PATH_LINEAR},
    {    3632,3   ,1        ,0      ,255   ,90    ,0     ,PATH_NONE},
    {    3632,4   ,1        ,0      ,279   ,90    ,0     ,PATH_NONE},
    {    3632,5   ,1        ,0      ,303   ,90    ,0     ,PATH_NONE},
    {    3632,6   ,1        ,0      ,255   ,111   ,0     ,PATH_NONE},
    {    3632,7   ,1        ,0      ,279   ,111   ,0     ,PATH_NONE},
    {    3632,8   ,1        ,0      ,303   ,111   ,0     ,PATH_NONE},
    { 0xFFFF } // Terminatore della lista (indica la fine delle ondate)
};

uint8_t enemiesToSpawn = 0;
uint8_t spawnTimer = 0;     
uint8_t currentWaveIdx = 3;

Enemy enemyPool[MAX_ENEMIES_POOL];

EnemyType enemyCatalog[] = {
//    color        ,baseFrame,numFrames,animSpeed,hp  ,points,bbox_x,bbox_y,bbox_w,bbox_h,bulletTypeID,bulletPeriod,bulletAngle,powerUpTypeID,isBoss
    { VCOL_LT_BLUE ,0        , 2       ,10       ,100 ,100   ,14    ,8     ,10    ,5     ,-1          ,0           ,0          ,PWR_NONE     ,0 },
    { VCOL_YELLOW  ,6        , 1       ,0        ,200 ,200   ,0     ,4     ,24    ,14    ,1           ,20          ,17         ,PWR_NONE     ,0},
    { VCOL_MED_GREY,20       , 1       ,0        ,100 ,100   ,2     ,2     ,22    ,18    ,-1          ,0           ,0          ,PWR_SATELLITE,0 },
    { VCOL_ORANGE  ,26       , 1       ,0        ,400 ,500   ,14    ,9     ,10    ,12    ,-1          ,0           ,0          ,PWR_NONE     ,1 },
    { VCOL_ORANGE  ,27       , 1       ,0        ,400 ,500   ,0     ,0     ,24    ,21    ,-1          ,0           ,0          ,PWR_NONE     ,1 },
    { VCOL_ORANGE  ,28       , 1       ,0        ,400 ,500   ,0     ,0     ,24    ,21    ,-1          ,0           ,0          ,PWR_NONE     ,1 },
    { VCOL_ORANGE  ,29       , 1       ,0        ,400 ,500   ,14    ,0     ,10    ,15    ,1           ,30          ,17         ,PWR_NONE     ,1 },
    { VCOL_ORANGE  ,30       , 1       ,0        ,400 ,500   ,0     ,0     ,24    ,21    ,-1          ,0           ,0          ,PWR_NONE     ,1 },
    { VCOL_ORANGE  ,31       , 1       ,0        ,400 ,500   ,0     ,0     ,24    ,16    ,-1          ,0           ,0          ,PWR_NONE     ,1 }
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

Satellite satellite;

int16_t level_complete_timer;


//***************************************************************************************************************
//* PROTOTIPI DELLE FUNZIONI
//***************************************************************************************************************

void init_video();

void init_player_ship();
void update_player_ship();

void draw_map(char mx);
void fill_column(unsigned ix, unsigned iy);
void hard_scroll();
void scroll_left();

void check_fire_on_release();
void spawn_bullet(uint16_t x, uint16_t y, uint8_t bulletType,uint8_t frame,uint8_t angle);
void update_bullets();
int8_t calc_free_bullets();

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

uint8_t check_aabb_collision(int16_t x1, uint8_t y1, uint8_t w1, uint8_t h1, 
                             int16_t x2, uint8_t y2, uint8_t w2, uint8_t h2);

uint8_t check_background_collision(uint16_t x, uint8_t y);

void spawn_powerup(uint16_t x, uint8_t y, uint8_t typeID);
void update_powerup();
void check_player_powerup_collision();
void update_hud_beam();

void spawn_satellite(uint16_t x, uint16_t y);
void satellite_update();
void check_player_satellite_docking();
void check_satellite_launch();
void check_bullets_satellite_collisions();
void check_satellite_enemy_collision();

void init_level_complete();

//***************************************************************************************************************
//* MAIN
//***************************************************************************************************************
int main() {

    generate_sine_table();
    generate_sine_fixed();
    cia_init();
    init_video();
    init_player_ship();
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
            check_player_satellite_docking();
            keyb_poll();
            check_satellite_launch();
            satellite_update();
            check_bullets_satellite_collisions();
            check_satellite_enemy_collision();
            break;

        case GAME_OVER:
            vic_waitFrame();
            game_over_timer++;
            if(game_over_timer >= GAME_OVER_DURATION) {
                init_gamestate_running();
                game_state = GAME_RUNNING;
            }

            break;
        
        case GAME_LEVEL_COMPLETED:
            vic_waitFrame();
            level_complete_timer++;
            if(level_complete_timer >= GAME_LVL_COMP_DURATION) {
                
                uint8_t* sp = Screen + 40*10 + 13;

                for(uint8_t x=0; x<15; x++)
                    *sp++ = 0;

                init_gameover();
                game_state = GAME_OVER;
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
//* DEFINIZIONE DELLE FUNZIONI
//***************************************************************************************************************

void init_player_ship() {

    player_ship.x = PLAYERX0;
    player_ship.y = PLAYERY0;
    player_ship.state = STATE_ALIVE;
    player_ship.color = VCOL_LT_BLUE;
    player_ship.base_frame = 3;
    player_ship.currentFrame = 0;
    player_ship.maxFrames = 2;
    player_ship.animTimer = 0;
    player_ship.animSpeed = 10;

    vspr_set(0,player_ship.x,player_ship.y,
            sprite_base+player_ship.base_frame,player_ship.color);

    vspr_set(1,player_ship.x-24,player_ship.y+1,
            sprite_base+player_ship.eng_baseFrame,player_ship.color);
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

        vspr_move(1, player_ship.x-24, player_ship.y+1);

        player_ship.animTimer++;
        if (player_ship.animTimer >= 5) { 
            player_ship.animTimer = 0;
            player_ship.currentFrame = (player_ship.currentFrame + 1) % 2;
        }
        
        vspr_image(1, sprite_base + player_ship.eng_baseFrame + player_ship.currentFrame);
    }
    else if (player_ship.state == STATE_EXPLODING) {

        player_ship.animTimer++;
        if (player_ship.animTimer >= player_ship.animSpeed) { 
            player_ship.animTimer = 0;
            player_ship.currentFrame++;
        }
        if (player_ship.currentFrame >= player_ship.maxFrames) {
            if (lives <= 0) {
                init_gameover();
                game_state = GAME_OVER;
            }
            else {
                init_player_ship();
            }
        }
        else
            vspr_image(0, sprite_base + player_ship.base_frame + player_ship.currentFrame);
    }
}


void draw_map(char mx) {
    const char * mp = Level1Map + mx;    // (1)
    char * sp = Screen + 40*12;             // (2)   

    for(char y=0; y<8; y++) {               // (3)
        for(char x=0; x<40; x++) {          // (4) 
            *sp = *mp;                      // (5)
            sp++;                           // (6)
            mp++;                           // (7)
        }
        mp += 454-40;                       // (8)
    }   
}


void fill_column(unsigned ix, unsigned iy) {
    char * sp = Screen + 40*12 + 39;        // (1)
    unsigned sx = (ix >> 3);                // (2)
    unsigned sy = (iy >> 3);                // (3)

    for(char y=0; y<8; y++) {               // (4)
        char tile = Level1Map[sx + 454*sy]; // (5)
        
        sp[0] = tile;                       // (6)
        sp += 40;                           // (7)
        sy++;                               // (8)
    }
}


void hard_scroll() {
    
    for(char i=0; i<39; i++) {                           // (1)
        #pragma unroll(full)                             // (2)
        for(char j=12; j<20; j++)                        // (3)
            Screen[40 * j + i] = Screen[40 * j + i + 1]; // (4)
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
        
        if (beamCharge < BEAM_THRESHOLD) {
            beamCharge++;
            update_hud_beam();
        }
    } else {
        if (firePressedLastFrame == 1) {

            if (beamCharge >= BEAM_THRESHOLD) {
                spawn_bullet(player_ship.x, player_ship.y, FIRE_TYPE_BEAM, bulletCatalog[FIRE_TYPE_BEAM].baseFrame, 0);
            } else {

                switch (currentWeaponType) {
                    case FIRE_TYPE_BASE:
                    case FIRE_TYPE_WAVE:
                        spawn_bullet(player_ship.x, player_ship.y,currentWeaponType,bulletCatalog[currentWeaponType].baseFrame,0);
                        break;
                    
                    case FIRE_TYPE_DOUBLE:
                        int8_t free_bullets = calc_free_bullets();
                        if(free_bullets>=2) {
                            spawn_bullet(player_ship.x, player_ship.y-3,currentWeaponType,bulletCatalog[currentWeaponType].baseFrame,0);
                            spawn_bullet(player_ship.x, player_ship.y+5,currentWeaponType,bulletCatalog[currentWeaponType].baseFrame,0);
                        }
                        break;
                    
                    case FIRE_TYPE_LASER:
                        free_bullets = calc_free_bullets();
                        if(free_bullets>=3) {
                            spawn_bullet(player_ship.x, player_ship.y,FIRE_TYPE_BASE,bulletCatalog[FIRE_TYPE_BASE].baseFrame,0);
                            spawn_bullet(player_ship.x, player_ship.y,FIRE_TYPE_LASER,bulletCatalog[currentWeaponType].baseFrame+1,4);
                            spawn_bullet(player_ship.x, player_ship.y,FIRE_TYPE_LASER,bulletCatalog[currentWeaponType].baseFrame,27+4);
                        }
                        break;

                    default:
                        break;
                }
            }
            
            firePressedLastFrame = 0;
            beamCharge = 0;
            update_hud_beam();  
        }
    }
}


void spawn_bullet(uint16_t x, uint16_t y, uint8_t bulletType,uint8_t frame,uint8_t angle) {
    
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {   
        if (!bulletPool[i].active) {               
            
            bulletPool[i].active = 1;
            bulletPool[i].typeID = bulletType;
            bulletPool[i].x = (x + 24) << 6;  
            bulletPool[i].y = y << 6;
            bulletPool[i].angle = angle; 

            bulletPool[i].bbox_x = bulletCatalog[bulletType].bbox_x;
            bulletPool[i].bbox_y = bulletCatalog[bulletType].bbox_y;
            bulletPool[i].bbox_w = bulletCatalog[bulletType].bbox_w;
            bulletPool[i].bbox_h = bulletCatalog[bulletType].bbox_h;

            uint8_t spriteIdx = i + 3;

            vspr_set(spriteIdx,
                bulletPool[i].x >> 6, bulletPool[i].y >> 6,
                sprite_base + frame,
                bulletCatalog[bulletType].color
            );

            return;
        }
    }
}

void update_bullets() {
    for (uint8_t i = 0; i < MAX_BULLETS; i++) {
        if (bulletPool[i].active) {
            uint8_t speed = bulletCatalog[bulletPool[i].typeID].speed;
            uint8_t angle = bulletPool[i].angle;

           

            int16_t dx = (int16_t)sinTable[(angle + 9) % 36] * speed;
            int16_t dy = (int16_t)sinTable[angle % 36] * speed;

            int16_t nextX = bulletPool[i].x + dx;
            int16_t nextY = bulletPool[i].y + dy;

            if (check_background_collision(nextX >> 6, (nextY) >> 6)) {
                bulletPool[i].angle = (36 - bulletPool[i].angle) % 36;
                
                bulletPool[i].y -= (8 << 6);
                vspr_movey(i +3,bulletPool[i].y >> 6);

                uint8_t newFrame;
                if (bulletPool[i].angle >= 32)
                    newFrame = 17;
                else
                    newFrame = 18;
                vspr_image(i + 3, sprite_base + newFrame);

                //bulletPool[i].active = 0;
                continue;
            }

            bulletPool[i].x = nextX;
            bulletPool[i].y = nextY;

            if ((bulletPool[i].x < (31 << 6)) || (bulletPool[i].x > (335 << 6)) ||
                (bulletPool[i].y < (50 << 6)) || (bulletPool[i].y > (210 << 6))) 
            {
                bulletPool[i].active = 0;
                vspr_hide(i + 3);
            } else {
                vspr_move(i + 3, bulletPool[i].x >> 6, bulletPool[i].y >> 6);
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
        Enemy e = enemyPool[i];
        e.active = 0;
    }
}

void activate_enemy_from_pool(WaveDefinition wave) {

    for (uint8_t i = 0; i < MAX_ENEMIES_POOL; i++) {
        if (enemyPool[i].active == 0) {
            
            enemyPool[i].active = 1;         
            enemyPool[i].x = wave.startX;            
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
            
            // Copiamo i dati per l'animazione
            enemyPool[i].baseFrame = enemyCatalog[wave.enemyTypeID].baseFrame;
            enemyPool[i].maxFrames = enemyCatalog[wave.enemyTypeID].numFrames;
            enemyPool[i].animSpeed = enemyCatalog[wave.enemyTypeID].animSpeed;
            enemyPool[i].animTimer = 0;
            enemyPool[i].currentFrame = 0;

            enemyPool[i].powerUpTypeID = enemyCatalog[wave.enemyTypeID].powerUpTypeID;
            enemyPool[i].flashTimer = 0;

            enemyPool[i].isBoss = enemyCatalog[wave.enemyTypeID].isBoss;

            vspr_set(i+8,enemyPool[i].x,enemyPool[i].y>>8,sprite_base+enemyPool[i].baseFrame,enemyPool[i].color);
            
            return;
        }
    }
}

void update_enemy_movement() {
    for (uint8_t i = 0; i < MAX_ENEMIES_POOL; i++) {
        if (enemyPool[i].active && enemyPool[i].state != STATE_EXPLODING) {
            
            Enemy e = enemyPool[i];
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
                    dy = sineTableFixed[idx % 64];
                    break;
                
                case PATH_NONE:
                    dx = 0;
                    dy = 0;
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
                        
                        if(enemyPool[i].isBoss) {
                            init_level_complete();
                            game_state = GAME_LEVEL_COMPLETED;
                            continue;
                        }

                        enemyPool[i].active = 0;
                        vspr_hide(i+8);
                        if(enemyPool[i].powerUpTypeID != PWR_NONE) {
                            spawn_powerup(enemyPool[i].x, enemyPool[i].y >>8,enemyPool[i].powerUpTypeID);
                        }
                        
                        continue;
                    }

                    enemyPool[i].currentFrame = 0;
                }

                uint8_t finalPtr = enemyPool[i].baseFrame + enemyPool[i].currentFrame;
                
                vspr_image(i+8,sprite_base+finalPtr);
            }
        }
    }
}


void check_bullets_enemies_collisions() {
    for (uint8_t b = 0; b < MAX_BULLETS; b++) {
        if (!bulletPool[b].active) continue;

        uint16_t bx = bulletPool[b].x >> 6;
        uint8_t  by = bulletPool[b].y >> 6;
        uint8_t  bbx = bulletPool[b].bbox_x;
        uint8_t  bby = bulletPool[b].bbox_y;
        uint8_t  bw  = bulletPool[b].bbox_w;
        uint8_t  bh  = bulletPool[b].bbox_h;
       
        for (uint8_t e = 0; e < MAX_ENEMIES_POOL; e++) {
            if (!enemyPool[e].active) continue;
            if (enemyPool[e].state == STATE_EXPLODING) continue;
   
            uint16_t ex = enemyPool[e].x;
            uint16_t ey = enemyPool[e].y >> 8;
            uint8_t  ebx = enemyPool[e].bbox_x;
            uint8_t  eby = enemyPool[e].bbox_y;
            uint8_t  ebw = enemyPool[e].bbox_w;
            uint8_t  ebh = enemyPool[e].bbox_h;
            

            if (check_aabb_collision(bx+bbx, by+bby, bw, bh, 
                                     ex+ebx, ey+eby, ebw,ebh) == 1 )
            {
                handle_enemy_hit(e, b);
                break; 
            }
        }
    }
}


void handle_enemy_hit(uint8_t eIdx, uint8_t bIdx) {

    uint16_t damage = bulletCatalog[bulletPool[bIdx].typeID].damage;
    int16_t hp = enemyPool[eIdx].hp;

    enemyPool[eIdx].hp -= damage;
    hp = enemyPool[eIdx].hp;

    if (enemyPool[eIdx].hp <= 0 && enemyPool[eIdx].state == STATE_ALIVE) {

        enemyPool[eIdx].state = STATE_EXPLODING;
        enemyPool[eIdx].baseFrame = 7;
        enemyPool[eIdx].currentFrame = 0;
        enemyPool[eIdx].maxFrames = 4;
        enemyPool[eIdx].animTimer = 0;
        enemyPool[eIdx].animSpeed = 5;
        enemyPool[eIdx].color = VCOL_YELLOW;
        vspr_color(8+eIdx,enemyPool[eIdx].color);

        uint8_t typeID = enemyPool[eIdx].typeID;
        uint16_t pt = enemyCatalog[typeID].points;
        score += pt+10000;
        update_score();

        if (enemyPool[eIdx].isBoss == 1) {

            for(uint8_t i=0; i<MAX_ENEMIES_POOL; i++) {
                if (enemyPool[i].active==1 && enemyPool[i].isBoss==1 && enemyPool[i].state != STATE_EXPLODING) {
                    enemyPool[i].state = STATE_EXPLODING;

                    enemyPool[i].baseFrame = 7;
                    enemyPool[i].currentFrame = 0;
                    enemyPool[i].maxFrames = 4;
                    enemyPool[i].animTimer = 0;
                    enemyPool[i].animSpeed = 5;
                    enemyPool[i].color = VCOL_YELLOW;
                    vspr_color(8+i,enemyPool[i].color);
                }
            }
        }
    }
    else {
        if (enemyPool[eIdx].isBoss == 1) {
            for(uint8_t i=0; i<MAX_ENEMIES_POOL; i++) {
                enemyPool[i].flashTimer = FLASH_DURATION;
            }
        }

        enemyPool[eIdx].flashTimer = FLASH_DURATION;
    }

    //if (bulletPool[bIdx].typeID != FIRE_TYPE_BEAM) {
        bulletPool[bIdx].active = 0;
        vspr_hide(3+bIdx);
    //}
}


void check_player_enemy_collision() {
    if (player_ship.state != STATE_ALIVE) return;

    uint16_t px = player_ship.x;
    uint8_t  py = player_ship.y;
    uint8_t  pbx = player_ship.bbox_x;
    uint8_t  pby = player_ship.bbox_y;
    uint8_t  pbw = player_ship.bbox_w;
    uint8_t  pbh = player_ship.bbox_h;

    for (uint8_t i = 0; i < MAX_ENEMIES_POOL; i++) {
        if (!enemyPool[i].active || enemyPool[i].state == STATE_EXPLODING) continue;

        uint16_t ex = enemyPool[i].x;
        uint16_t ey = enemyPool[i].y >> 8;
        uint8_t ebx = enemyPool[i].bbox_x;
        uint8_t eby = enemyPool[i].bbox_y;
        uint8_t ebw = enemyPool[i].bbox_w;
        uint8_t ebh = enemyPool[i].bbox_h; 

        if ( check_aabb_collision(px+pbx,py+pby,pbw,pbh,ex+ebx,ey+eby,ebw,ebh) == 1 )
        {
            player_ship.state = STATE_EXPLODING;
            handle_player_death(i);
            return;
        }
    }
}


void handle_player_death(uint8_t eIdx) {
    spawn_player_explosion();
    
    enemyPool[eIdx].state = STATE_EXPLODING;
    enemyPool[eIdx].baseFrame = 7;
    enemyPool[eIdx].currentFrame = 0;
    enemyPool[eIdx].maxFrames = 4;
    enemyPool[eIdx].animTimer = 0;
    enemyPool[eIdx].animSpeed = 5;
    enemyPool[eIdx].color = VCOL_YELLOW;
    vspr_color(8+eIdx,enemyPool[eIdx].color);
}


void spawn_player_explosion() {
    player_ship.state = STATE_EXPLODING;
    
    player_ship.base_frame = 11;
    player_ship.currentFrame = 0;
    player_ship.maxFrames = 3;
    player_ship.animTimer = 0;
    player_ship.animSpeed = 5;

    player_ship.color = VCOL_WHITE;
    vspr_color(0,player_ship.color);
    vspr_hide(1);

    lives--;
    satellite.active = 0;
    vspr_hide(2);
    update_lives();
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

    vic.color_border = VCOL_BLACK;
    vic.color_back = VCOL_DARK_GREY;
    vic.color_back1 = VCOL_LT_GREY;
    vic.color_back2 = VCOL_BLACK;
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

void check_player_background_collision() {
    if (player_ship.state != STATE_ALIVE) return;

    uint16_t left = player_ship.x + player_ship.bbox_x;
    uint16_t right = left + player_ship.bbox_w;
    uint8_t top = player_ship.y + player_ship.bbox_y;
    uint8_t bottom = top + player_ship.bbox_h;

    uint8_t tx1 = (uint8_t)((left - 24) / 8);
    uint8_t tx2 = (uint8_t)((right - 24) / 8);
    uint8_t ty1 = (uint8_t)((top - 50) / 8);
    uint8_t ty2 = (uint8_t)((bottom - 50) / 8);

    if (tx1 > 39) tx1 = 39;
    if (tx2 > 39) tx2 = 39;
    if (ty1 > 24) ty1 = 24;
    if (ty2 > 24) ty2 = 24;

    uint8_t tile_tl = Screen[ty1 * 40 + tx1];
    uint8_t tile_tr = Screen[ty1 * 40 + tx2];
    uint8_t tile_bl = Screen[ty2 * 40 + tx1];
    uint8_t tile_br = Screen[ty2 * 40 + tx2];

    if (tile_tl > 0 || tile_tr > 0 || tile_bl >0 || tile_br >0) {
        spawn_player_explosion();
    }
}

void init_irq() {
    
    // IRQ alla riga 20 per cambiare il charset
    // Costruiamo un IRQ con 5 comandi
    rirq_build(&irq_charset_change, 5);

    // Scriviamo nel registro memptr il nuovo valore del charset
    rirq_write(&irq_charset_change, 0, &vic.memptr, 0x04);

    rirq_write(&irq_charset_change, 1, & vic.color_back, VCOL_DARK_GREY);

    rirq_write(&irq_charset_change, 2, &vic.color_back1, VCOL_LT_GREY);

    rirq_write(&irq_charset_change, 3, &vic.color_back2, VCOL_BLACK);

    rirq_write(&irq_charset_change, 4, &vic.ctrl2, 0);

    // Settiamo l'interrupt alla riga 1
    rirq_set(8, 1, &irq_charset_change);

    
    rirq_build(&irq_panel,5);

    // Scriviamo nel registro memptr il nuovo valore 
    rirq_write(&irq_panel, 0, &vic.memptr, 0x08);

    rirq_write(&irq_panel, 1, &vic.color_back, VCOL_BLACK);

    rirq_write(&irq_panel, 2, &vic.ctrl2, 0);
    
    rirq_write(&irq_panel, 3, &vic.color_back1, VCOL_DARK_GREY);

    rirq_write(&irq_panel, 4, &vic.color_back2, VCOL_LT_GREY);

    // Settiamo l'interrupt alla riga 210
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

void print_string(char* string,uint16_t x,uint8_t y) {
    char * screen_ptr = Screen + x + 40*y;

    int8_t str_length = strlen(string);
    for(int8_t i=0; i<str_length; i++)
        *screen_ptr++ = *string++;
}

void update_score() {
    char s[6];
    sprintf(s,"%05ld",score);
    print_string(s,5,23);
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
    scrollX = 0;
    lives = STARTING_LIVES;
    score = 0;
    currentWaveIdx = 0;
    beamCharge = 0;
    currentWeaponType = FIRE_TYPE_BASE;
    firePressedLastFrame = 0;
    memset(Screen, 0, 1000);
    memset(Color, VCOL_WHITE + 8, 1000);
    init_hud();
}

void init_gameover() {
    game_over_timer = 0;
    for (int8_t i=0; i < MAX_ENEMIES_POOL; i++)
        enemyPool[i].active = false;
    for (int8_t i=0; i < MAX_ENEMY_BULLETS; i++)
        enemyBulletPool[i].active = false;
    for (int8_t i=0; i < MAX_BULLETS; i++)
        bulletPool[i].active = false;
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
    
            enemyBulletPool[i].bbox_x = bulletCatalog[bulletTypeID].bbox_x;
            enemyBulletPool[i].bbox_y = bulletCatalog[bulletTypeID].bbox_y;
            enemyBulletPool[i].bbox_w = bulletCatalog[bulletTypeID].bbox_w;
            enemyBulletPool[i].bbox_h = bulletCatalog[bulletTypeID].bbox_h;
            
            uint16_t angle = enemyCatalog[typeID].bulletAngle;
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
        if (enemyBulletPool[i].active) {
            uint8_t speed = bulletCatalog[enemyBulletPool[i].typeID].speed;

            uint16_t angle = enemyBulletPool[i].angle;

            int16_t dx = (int16_t) sinTable[(angle+9) % 36] * speed;
            int16_t dy = (int16_t) sinTable[angle] * speed;
            
            enemyBulletPool[i].x += dx;
            enemyBulletPool[i].y += dy;
            
            if ( (enemyBulletPool[i].x  < (31<<6) ) || 
                 (enemyBulletPool[i].x  > (335<<6)) ||
                 (enemyBulletPool[i].y  < (50<<6) ) ||
                 (enemyBulletPool[i].y  > (200<<6) ) 
               )
            {
                enemyBulletPool[i].active = 0;
                vspr_hide(i + 6);
            } else {
                vspr_move(i + 6, enemyBulletPool[i].x >>6, enemyBulletPool[i].y>>6);
            }
        }
    }
}

void check_bullets_player_collisions() {

    uint16_t px = player_ship.x;
    uint8_t  py = player_ship.y;
    uint8_t  pbx = player_ship.bbox_x;
    uint8_t  pby = player_ship.bbox_y;
    uint8_t  pbw = player_ship.bbox_w;
    uint8_t  pbh = player_ship.bbox_h;

    if (player_ship.state == STATE_EXPLODING) return;

    for (uint8_t b = 0; b < MAX_ENEMY_BULLETS; b++) {
        if (!enemyBulletPool[b].active) continue;
        

        int16_t bx = enemyBulletPool[b].x >> 6;
        int16_t by = enemyBulletPool[b].y >> 6;
        uint8_t bbx = enemyBulletPool[b].bbox_x;
        uint8_t bby = enemyBulletPool[b].bbox_y;
        uint8_t bbw = enemyBulletPool[b].bbox_w;
        uint8_t bbh = enemyBulletPool[b].bbox_h;
        
        if( check_aabb_collision(px+pbx,py+pby,pbw,pbh,bx+bbx,by+bby,bbw,bbh) == 1 )
        {
            player_ship.state = STATE_EXPLODING;
            
            spawn_player_explosion();
    
            enemyBulletPool[b].active = 0;
            vspr_hide(6+b);

            return;
        }
    }
}

void generate_sine_table() {
    for (int i = 0; i < 36; i++) {
        float radians = (float)i*10 * (PI / 180.0);
        int16_t s = (int16_t)(sin(radians) * 64.0);
        sinTable[i] = (int16_t)(sin(radians) * 64.0);
    }
}

int8_t calc_free_bullets() {
    int8_t free_bullets = 0;
    for(int8_t i=0; i<MAX_BULLETS; i++)
        if(bulletPool[i].active==0)
            free_bullets++;
    
    return free_bullets;
}

uint8_t check_aabb_collision(int16_t x1, uint8_t y1, uint8_t w1, uint8_t h1, 
                             int16_t x2, uint8_t y2, uint8_t w2, uint8_t h2) 
{
    
    // Verifica sovrapposizione sull'asse X
    if (x1 + w1 < x2) return 0; // A è completamente a sinistra di B
    if (x1 > x2 + w2) return 0; // A è completamente a destra di B

    // Verifica sovrapposizione sull'asse Y
    if (y1 + h1 < y2) return 0; // A è completamente sopra B
    if (y1 > y2 + h2) return 0; // A è completamente sotto B

    // Se nessuna delle condizioni sopra è vera, c'è una collisione
    return 1;
}


uint8_t check_background_collision(uint16_t x, uint8_t y) {
    if (x < 31 || x > 335 || y < 50 || y > 249) return 0;

    uint8_t col = (x - 31) >> 3;
    uint8_t row = (y - 50) >> 3;

    char* screenPos = Screen + (row * 40) + col;
    uint8_t charValue = *(uint8_t*)screenPos;

    if (charValue > 0) {
        return 1;
    }

    return 0;
}

void spawn_powerup(uint16_t x, uint8_t y, uint8_t typeID) 
{
    powerup.x = x;
    powerup.y = y;
    powerup.active = 1;

    powerup.baseFrame = 19;
    powerup.typeID = typeID;
    powerup.spriteID = 15;

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
    if (powerup.active == 0)
        return;

    powerup.x -= 1;
    vspr_movex(powerup.spriteID,powerup.x);

    if (powerup.x < (31-24)) {
        powerup.active = 0;
        vspr_hide(powerup.spriteID);
    }
}

void check_player_powerup_collision() {
    if (player_ship.state != STATE_ALIVE) return;
    if (powerup.active == 0) return;

    uint16_t px = player_ship.x;
    uint8_t  py = player_ship.y;
    uint8_t  pbx = player_ship.bbox_x;
    uint8_t  pby = player_ship.bbox_y;
    uint8_t  pbw = player_ship.bbox_w;
    uint8_t  pbh = player_ship.bbox_h;

    uint16_t ix = powerup.x;
    uint8_t  iy = powerup.y;
    uint8_t  ibx = powerup.bbox_x;
    uint8_t  iby = powerup.bbox_y;
    uint8_t  ibw = powerup.bbox_w;
    uint8_t  ibh = powerup.bbox_h;

    if ( check_aabb_collision(px+pbx,py+pby,pbw,pbh,ix+ibx,iy+iby,ibw,ibh) == 1)
    {
        powerup.active = 0;
        vspr_hide(powerup.spriteID);

        if(powerup.typeID < PWR_SATELLITE)
        {
            switch (powerup.typeID)
            {
            case PWR_LASER:
                currentWeaponType = FIRE_TYPE_LASER;
                break;
            case PWR_DOUBLE_MISSILE:
                currentWeaponType = FIRE_TYPE_DOUBLE;
                break;
            case PWR_WAVE_FIRE:
                currentWeaponType = FIRE_TYPE_WAVE;
                break;
            
            default:
                break;
            }
        }
        else if (powerup.typeID == PWR_SATELLITE)
        {
            spawn_satellite(SAT_X0, SAT_Y0);
        }
        
    }
}

void update_hud_beam() {
    uint8_t barLength = 22;
    uint8_t filledBlocks = (beamCharge * barLength) / BEAM_THRESHOLD;
    
    for (uint8_t i = 0; i < barLength; i++) {
        uint16_t pos = (uint16_t)Screen + 40*21 + 16 + i;
        if (i < filledBlocks) {
            *(uint8_t*)pos = 14;
        } else {
            *(uint8_t*)pos = 7;
        }
    }
}

void spawn_satellite(uint16_t x, uint16_t y) {
    satellite.x = x;
    satellite.y = y << 8;
    satellite.pathIdx = 0;
    satellite.speedX = SAT_SPEED0;

    satellite.active = 1;
    satellite.state = SAT_STATE_FREE;

    satellite.color = VCOL_RED;
    satellite.base_frame = 22;

    satellite.currentFrame = 0;
    satellite.maxFrames = 4;
    satellite.animTimer = 0;
    satellite.animSpeed = 5;

    satellite.bbox_x = 0;
    satellite.bbox_y = 0;
    satellite.bbox_w = 22;
    satellite.bbox_h = 18;

    vspr_set(2, satellite.x, satellite.y >> 8,
             sprite_base + satellite.base_frame, satellite.color);
}

void satellite_update() {
    if (!satellite.active)
        return;

    if (satellite.state == SAT_STATE_DOCKED_FRONT) {
        satellite.x = (player_ship.x + 24); 
        satellite.y = (player_ship.y + 2) << 8;  
    } 
    else if (satellite.state == SAT_STATE_DOCKED_BACK) {
        satellite.x = (player_ship.x - 22); 
        satellite.y = (player_ship.y + 1) << 8;
    }
    else if (satellite.state == SAT_STATE_RELEASE || satellite.state == SAT_STATE_RECALL) {
        satellite.x += satellite.speedX;

        if (satellite.x <= 31) {
            satellite.speedX *= -1;
            satellite.state = SAT_STATE_FREE;
        }
        else if (satellite.x >= (31+320-16-24)) {
            satellite.speedX *= -1;
            satellite.state = SAT_STATE_FREE;
        }
    }
    else
    {

        uint8_t idx = satellite.pathIdx;

        int16_t dx = satellite.speedX;
        int16_t dy = 2* sineTableFixed[idx % 64];
        
        satellite.pathIdx++;

        satellite.x += dx;
        satellite.y += dy;

        if ( (satellite.x < 31) || (satellite.x > (31+320-16-24)) )
            satellite.speedX *= -1;
    }

    uint16_t x = satellite.x;
    uint16_t y = satellite.y >> 8;

    vspr_move(2,satellite.x,satellite.y >> 8);

    satellite.animTimer++;

    if (satellite.animTimer >= satellite.animSpeed) {
        satellite.animTimer = 0;
        satellite.currentFrame++;

        if (satellite.currentFrame >= satellite.maxFrames) {
            satellite.currentFrame = 0;
        }

        uint8_t finalFrame = satellite.base_frame + satellite.currentFrame;
        vspr_image(2, sprite_base + finalFrame);
    }
}

void check_player_satellite_docking() {
    if (!satellite.active || satellite.state == SAT_STATE_DOCKED_FRONT || satellite.state == SAT_STATE_DOCKED_BACK ) return;
    if (player_ship.state != STATE_ALIVE) return;

    uint16_t px = player_ship.x + player_ship.bbox_x;
    uint8_t py = player_ship.y + player_ship.bbox_y;
    uint8_t pw = player_ship.bbox_w;
    uint8_t ph = player_ship.bbox_h;

    uint16_t sx = satellite.x + satellite.bbox_x; 
    uint8_t sy = (satellite.y >> 8) + satellite.bbox_y;
    uint8_t sw = satellite.bbox_w;
    uint8_t sh = satellite.bbox_h;

    uint8_t halfWidth = pw / 2;

    if (check_aabb_collision(px + halfWidth, py, halfWidth, ph, sx, sy, sw, sh)) {
        satellite.state = SAT_STATE_DOCKED_FRONT;
        return;
    }

    if (check_aabb_collision(px, py, halfWidth, ph, sx, sy, sw, sh)) {
        satellite.state = SAT_STATE_DOCKED_BACK;
        return;
    }
}

void check_satellite_launch() {
    if (!satellite.active || satellite.state == SAT_STATE_RELEASE) return;

    if (key_pressed(KSCAN_SPACE)) {
        if (satellite.state == SAT_STATE_DOCKED_FRONT) {
            satellite.speedX = 4;
            satellite.state = SAT_STATE_RELEASE;  
        } 
        else if (satellite.state == SAT_STATE_DOCKED_BACK) {
            satellite.speedX = -4;
            satellite.state = SAT_STATE_RELEASE;
        }
        else if(satellite.state == SAT_STATE_FREE) {
            
            if (satellite.x > player_ship.x)
                satellite.speedX = -4;
            else
                satellite.speedX = 4;
            satellite.state = SAT_STATE_RECALL;
        }

        
    }
}


void check_bullets_satellite_collisions() {

    if (!satellite.active) return;

    uint16_t sx = satellite.x + satellite.bbox_x; 
    uint8_t sy = (satellite.y >> 8) + satellite.bbox_y;
    uint8_t sw = satellite.bbox_w;
    uint8_t sh = satellite.bbox_h;

    for (uint8_t b = 0; b < MAX_ENEMY_BULLETS; b++) {
        if (!enemyBulletPool[b].active) continue;
        
        int16_t bx = enemyBulletPool[b].x >> 6;
        int16_t by = enemyBulletPool[b].y >> 6;
        uint8_t bbx = enemyBulletPool[b].bbox_x;
        uint8_t bby = enemyBulletPool[b].bbox_y;
        uint8_t bbw = enemyBulletPool[b].bbox_w;
        uint8_t bbh = enemyBulletPool[b].bbox_h;
        
        if( check_aabb_collision(sx,sy,sw,sh,bx+bbx,by+bby,bbw,bbh) == 1 )
        {
            enemyBulletPool[b].active = 0;
            vspr_hide(6+b);

            return;
        }
    }
}


void check_satellite_enemy_collision() {
    if (!satellite.active) return;

    uint16_t sx = satellite.x + satellite.bbox_x; 
    uint8_t sy = (satellite.y >> 8) + satellite.bbox_y;
    uint8_t sw = satellite.bbox_w;
    uint8_t sh = satellite.bbox_h;

    for (uint8_t i = 0; i < MAX_ENEMIES_POOL; i++) {
        if (!enemyPool[i].active || enemyPool[i].state == STATE_EXPLODING) continue;

        uint16_t ex = enemyPool[i].x + enemyPool[i].bbox_x;
        uint16_t ey = (enemyPool[i].y >> 8) + enemyPool[i].bbox_y;
        uint8_t ew = enemyPool[i].bbox_w;
        uint8_t eh = enemyPool[i].bbox_h; 

        if ( check_aabb_collision(sx,sy,sw,sh,ex,ey,ew,eh) == 1 )
        {
            enemyPool[i].state = STATE_EXPLODING;
            enemyPool[i].baseFrame = 7;
            enemyPool[i].currentFrame = 0;
            enemyPool[i].maxFrames = 4;
            enemyPool[i].animTimer = 0;
            enemyPool[i].animSpeed = 5;
            enemyPool[i].color = VCOL_YELLOW;
            vspr_color(8+i,enemyPool[i].color);

            uint8_t typeID = enemyPool[i].typeID;
            uint16_t pt = enemyCatalog[typeID].points;
            score += pt;
            update_score();
            return;
        }
    }
}

void init_level_complete() {
    level_complete_timer = 0;
    for (int8_t i=0; i < MAX_ENEMIES_POOL; i++)
        enemyPool[i].active = false;
    for (int8_t i=0; i < MAX_ENEMY_BULLETS; i++)
        enemyBulletPool[i].active = false;
    for (int8_t i=0; i < MAX_BULLETS; i++)
        bulletPool[i].active = false;
    for (int8_t i = 0; i < VSPRITES_MAX; i++) 
        vspr_hide(i);
    
    uint8_t msg[] = { 201,202,203,202,201,200,204,205,206,207,201,202,208,202,209,0};
    print_string(msg,13,10);
}