#include <hf-risc.h>
#include "vga/vga_drv.h"

#define VGA_MIDDLE_X (VGA_WIDTH / 2)
#define VGA_MIDDLE_Y (VGA_HEIGHT / 2)

#define QUANT 11 // quantidade por linha. Da pra mudar depois, ou aumentar por fase
#define NUM_ENEMY_ROWS 3 // número de linhas de inimigos

// Matriz que define o tipo de inimigo em cada linha
int enemy_types[NUM_ENEMY_ROWS] = {1, 2, 3};


/* ---------- getInputKey.c content (PS/2 AXI peripheral reader) ---------- */

/* endereço do periférico AXI */
#define SW_AXI_BASE            0xe4a90000

/* registradores STATUS e DATA, implementados no hardware */
#define SW_AXI_STATUS          (*(volatile uint32_t *)(SW_AXI_BASE + 0x010))
#define SW_AXI_SDATA           (*(volatile uint32_t *)(SW_AXI_BASE + 0x020))

/* máscada dos bits READY e VALID, presentes no registrador STATUS */
#define SW_AXI_STREADY         (1 << 0)
#define SW_AXI_STVALID         (1 << 1)


uint8_t sw_axi()
{
    uint8_t data;
    
    /* leitura 'fake' do registrador DATA, ativa uma transferência do periférico */
    data = SW_AXI_SDATA;
    /* leitura do registrador STATUS e espera ocupada, até que o periférico tenha um dado válido */
    while (!(SW_AXI_STATUS & SW_AXI_STVALID));
    /* leitura 'real' do registrador DATA, contendo os dados transferidos do periférico */
    data = SW_AXI_SDATA;
    
    return data;
}

/* função para verificar se há dados disponíveis */
int sw_axi_data_available()
{
    return (SW_AXI_STATUS & SW_AXI_STVALID);
}

char getInput(){ // non-blocking, stateful: mapeia apenas 'a','d' e 'space'
    static uint8_t saw_f0 = 0;
    static uint8_t saw_e0 = 0;
    char ret = 0;

    /* se nao houver dado, retorna imediatamente */
    while (sw_axi_data_available()) {
        uint8_t x = sw_axi(); /* só chamado se há dado */

        if (x == 0xE0) { /* prefixo extended */
            saw_e0 = 1;
            continue;
        }
        if (x == 0xF0) { /* prefixo de release */
            saw_f0 = 1;
            continue;
        }

        if (saw_f0) { /* release: descarta e limpa flags */
            saw_f0 = 0;
            saw_e0 = 0;
            continue;
        }

        /* make code: mapear apenas as teclas necessárias */
        if (!saw_e0) {
            if (x == 0x1C) ret = 'a';     /* 'a' make */
            else if (x == 0x23) ret = 'd';/* 'd' make */
            else if (x == 0x29) ret = ' ';/* space */
        } else {
            /* extended make: sem mapeamento por enquanto */
            ret = 0;
        }

        saw_e0 = 0; /* limpar E0 após uso */

        if (ret) return ret;
        /* se não mapeou, continua consumindo scancodes disponíveis */
    }

    return 0;
}

/* ---------- sprites ---------- */
/**/

char mysteryShip[7][16] = {
    {0, 0, 0, 0, 0, 7, 7, 7, 7, 7, 7, 0, 0, 0, 0, 0},
    {0, 0, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 0, 0, 0, 0},
    {0, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 0, 0},
    {0, 7, 7, 0, 7, 7, 0, 7, 7, 0, 7, 7, 0, 7, 7, 0},
    {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},
    {0, 0, 7, 7, 7, 0, 0, 7, 7, 0, 0, 7, 7, 7, 0, 0},
    {0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0}
};

char enemy1a[8][8] = {
    {0, 0, 0, 5, 5, 0, 0, 0},
    {0, 0, 5, 5, 5, 5, 0, 0},
    {0, 5, 5, 5, 5, 5, 5, 0},
    {5, 5, 0, 5, 5, 0, 5, 5},
    {5, 5, 5, 5, 5, 5, 5, 5},
    {0, 5, 0, 5, 5, 0, 5, 0},
    {5, 0, 0, 0, 0, 0, 0, 5},
    {0, 5, 0, 0, 0, 0, 5, 0}
};

char enemy1b[8][8] = {
    {0, 0, 0, 5, 5, 0, 0, 0},
    {0, 0, 5, 5, 5, 5, 0, 0},
    {0, 5, 5, 5, 5, 5, 5, 0},
    {5, 5, 0, 5, 5, 0, 5, 5},
    {5, 5, 5, 5, 5, 5, 5, 5},
    {0, 0, 5, 0, 0, 5, 0, 0},
    {0, 5, 0, 5, 5, 0, 5, 0},
    {5, 0, 5, 0, 0, 5, 0, 5}
};

char enemy2a[8][11] = {
    {0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0},
    {3, 0, 0, 3, 0, 0, 0, 3, 0, 0, 3},
    {3, 0, 3, 3, 3, 3, 3, 3, 3, 0, 3},
    {3, 3, 3, 0, 3, 3, 3, 0, 3, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0},
    {0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0},
    {0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0}
};

char enemy2b[8][11] = {
    {0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0},
    {0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0},
    {0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0},
    {0, 3, 3, 0, 3, 3, 3, 0, 3, 3, 0},
    {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 0, 3, 0, 0, 0, 0, 0, 3, 0, 3},
    {0, 0, 0, 3, 3, 0, 3, 3, 0, 0, 0}
};

char enemy3a[8][12] = {
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1}
};

char enemy3b[8][12] = {
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0},
    {0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0},
    {0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0}
};

char ship[8][13] = { // ta em verde
    {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}

};

char barrier[16][22] = {
    {0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0},
    {0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0},
    {0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0},
    {0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2},
    {2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2},
    {2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2},
    {2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2}
};

char bullet_s[3][1] = {
    {7},
    {7},
    {7}
};

/*-------------------------------------------------------*/
/*---------------- structs e funcoes relacionadas ------------------------*/

void draw_sprite(unsigned int x, unsigned int y, char *sprite,
	unsigned int sizex, unsigned int sizey, int color)
{
	unsigned int px, py;
	
	if (color < 0) {
		for (py = 0; py < sizey; py++)
			for (px = 0; px < sizex; px++)
				display_pixel(x + px, y + py, sprite[py * sizex + px]);
	} else {
		for (py = 0; py < sizey; py++)
			for (px = 0; px < sizex; px++)
				display_pixel(x + px, y + py, color & 0xf);
	}
	
}

struct bullet
{
    unsigned int posx, posy;
    char sprite;
    int spriteszx, spriteszy;
    int dy;
    int speed;
    int quantity;
};

void init_bullet(struct bullet *b, unsigned int posx, unsigned int posy, char sprite,
    int spriteszx, int spriteszy, int dy, int speed)
{
    b->posx = posx;
    b->posy = posy;
    b->sprite = sprite;
    b->spriteszx = spriteszx;
    b->spriteszy = spriteszy;
    b->dy = dy;
    b->speed = speed;
};

/* sprite based objects */
struct object_s {
	char *sprite_frame[3];
	char spriteszx, spriteszy, sprites;
	int cursprite;
	unsigned int posx, posy;
	int dx, dy;
	int speedx, speedy;
	int speedxcnt, speedycnt;
    int health;
    int score;
    struct bullet bullets;
};


void init_object(struct object_s *obj, char *spritea, char *spriteb,
	char *spritec, char spriteszx, char spriteszy, int posx, int posy, 
	int dx, int dy, int spx, int spy, int health, int score)
{
	obj->sprite_frame[0] = spritea; /* frames de animação */
	obj->sprite_frame[1] = spriteb; /* frames de animação */
	obj->sprite_frame[2] = spritec; /* frames de animação */
	obj->spriteszx = spriteszx;     /* tamanho do sprite */
	obj->spriteszy = spriteszy;     /* tamanho do sprite */
	obj->cursprite = 0;             /* frame atual */
	obj->posx = posx;   
	obj->posy = posy;
	obj->dx = dx;
	obj->dy = dy;
	obj->speedx = spx;
	obj->speedy = spy;
	obj->speedxcnt = spx;           /* contadores de velocidade */
	obj->speedycnt = spy;           /* contadores de velocidade */
    obj->health = health;           /* vida do objeto */
    obj->score = score;             /* pontuação do objeto, podemos usar tanto para os inimigos quanto para o jogador, vai somando do que mata na do jogador */
    init_bullet(&obj->bullets, 0, 0, bullet_s[0][0], 1, 3, dy, 2);
}

void init_all_enemies(struct object_s enemies_all[NUM_ENEMY_ROWS][QUANT])
{
    int startX = 20;
    int startY = 40;
    int spcX = 20;
    int spcY = 20;
    int line, i, type;
    
    for (line = 0; line < NUM_ENEMY_ROWS; line++) {
        type = enemy_types[line];
        
        switch (type) {
            case 1:
                for (i = 0; i < QUANT; i++) {
                    init_object(&enemies_all[line][i], enemy1a[0], enemy1b[0], 0, 8, 8, 
                               startX + i * spcX, startY + line * spcY, 1, 0, 5, 5, 1, 30, 1);
                }
                break;
            case 2:
                for (i = 0; i < QUANT; i++) {
                    init_object(&enemies_all[line][i], enemy2a[0], enemy2b[0], 0, 11, 8, 
                               startX + i * spcX, startY + line * spcY, 1, 0, 5, 5, 1, 20, 1);
                }
                break;
            case 3:
                for (i = 0; i < QUANT; i++) {
                    init_object(&enemies_all[line][i], enemy3a[0], enemy3b[0], 0, 12, 8, 
                               startX + i * spcX, startY + line * spcY, 1, 0, 5, 5, 1, 10, 1);
                }
                break;
        }
    }
}

/*-------------------------------------------------------*/
/*-----------------funcoes uteis (desenhar, mover...) --------------------*/

void draw_object(struct object_s *obj, char chgsprite, int color)
{
	if (chgsprite) {
		obj->cursprite++;
		if (obj->sprite_frame[obj->cursprite] == 0)
			obj->cursprite = 0;
	}
	
	draw_sprite(obj->posx, obj->posy, obj->sprite_frame[obj->cursprite],
		obj->spriteszx, obj->spriteszy, color);
}

void move_object(struct object_s *obj)
{
	struct object_s oldobj;
	
	memcpy(&oldobj, obj, sizeof(struct object_s));
	
	if (--obj->speedxcnt == 0) {
		obj->speedxcnt = obj->speedx;
		obj->posx = obj->posx + obj->dx;
	}
	if (--obj->speedycnt == 0) {
		obj->speedycnt = obj->speedy;
		obj->posy = obj->posy + obj->dy;
	}

	if ((obj->speedx == obj->speedxcnt) || (obj->speedy == obj->speedycnt)) {
		draw_object(&oldobj, 0, 0);
		draw_object(obj, 1, -1);
	}
}

void move_all_enemies(struct object_s enemies_all[NUM_ENEMY_ROWS][QUANT])
{
    int line, i, minX, maxX, any_line_touching_border;
    
    any_line_touching_border = 0;
    
    for (line = 0; line < NUM_ENEMY_ROWS; line++) { // passa de linha em linha para verificar se o movimento futuro de algum inimigo bate na borda, se sim, inverte o dx de todos e desce 5px
        minX = VGA_WIDTH + 1;
        maxX = -1;
        
        for (i = 0; i < QUANT; i++){
            if (enemies_all[line][i].health <= 0) continue;
            
            int next_x = enemies_all[line][i].posx + enemies_all[line][i].dx;
            
            if (next_x < minX) minX = next_x;
            if (next_x + enemies_all[line][i].spriteszx > maxX) 
                maxX = next_x + enemies_all[line][i].spriteszx;
        }
        
        if (maxX >= VGA_WIDTH || minX <= 0) {
            any_line_touching_border = 1;
            break;
        }
    }
    
    if (any_line_touching_border) {
        for (line = 0; line < NUM_ENEMY_ROWS; line++) {
            for (i = 0; i < QUANT; i++){
                draw_object(&enemies_all[line][i], 0, 0);
                enemies_all[line][i].dx = -enemies_all[line][i].dx;
                enemies_all[line][i].posy += 5;
            }
        }
    }
    
    for (line = 0; line < NUM_ENEMY_ROWS; line++) {
        for (i = 0; i < QUANT; i++){
            if(enemies_all[line][i].health > 0){
                move_object(&enemies_all[line][i]);
            }
        }
    }
}

void move_ship(struct object_s *obj, char inputKey)
{
    struct object_s oldobj;
	memcpy(&oldobj, obj, sizeof(struct object_s));

    if(inputKey == 'a' && obj->posx > (0 + obj->spriteszx)){ 
        obj->posx -= 3; 
    } else if(inputKey == 'd' && obj->posx < (VGA_WIDTH - (obj->spriteszx + 2))){
        obj->posx += 3;
    }

    draw_object(&oldobj, 0, 0);
    draw_object(obj, 0, -1);
}

void ship_fire_bullet(struct object_s *obj)
{
    struct bullet oldbullet;
    memcpy(&oldbullet, &obj->bullets, sizeof(struct bullet));
    
    if(obj->bullets.posy == 0 && obj->bullets.posy == 0){
        obj->bullets.posx = obj->posx; // nao tenho certeza se a posx eh o centro
        obj->bullets.posy = obj->posy;
    }

    obj->bullets.posy = obj->bullets.posy + (obj->bullets.dy * obj->bullets.speed);

    draw_sprite(oldbullet.posx, oldbullet.posy, &oldbullet.sprite,
        oldbullet.spriteszx, oldbullet.spriteszy, 0);

    draw_sprite(obj->bullets.posx, obj->bullets.posy, obj->bullets.sprite[0],
		obj->bullets.spriteszx, obj->bullets.spriteszy, -1);
}

void enemy_fire_bullet(struct object_s *obj)
{
    struct bullet oldbullet;
    memcpy(&oldbullet, &obj->bullets, sizeof(struct bullet));
    
    if(obj->bullets.posy == 0 && obj->bullets.posy == 0){
        obj->bullets.posx = obj->posx; // nao tenho certeza se a posx eh o centro
        obj->bullets.posy = obj->posy;
    }

    obj->bullets.posy = obj->bullets.posy + (obj->bullets.dy * obj->bullets.speed);

    draw_sprite(oldbullet.posx, oldbullet.posy, &oldbullet.sprite,
        oldbullet.spriteszx, oldbullet.spriteszy, 0);

    draw_sprite(obj->bullets.posx, obj->bullets.posy, obj->bullets.sprite[0],
        obj->bullets.spriteszx, obj->bullets.spriteszy, -1);
}


/*-------------------------------------------------------*/
/*------------------menus e display---------------------*/


void start_menu(struct object_s *mysteryShip){
    draw_object(mysteryShip, 1, -1);
    display_print("SPACE INVADERS", VGA_MIDDLE_X - 90, 20, 2, WHITE);
    display_print("PRESS 'space' TO START", VGA_MIDDLE_X - 70, VGA_MIDDLE_Y + 10, 1, WHITE);
    display_print("USE 'a' / 'd' TO MOVE", VGA_MIDDLE_X - 70, VGA_MIDDLE_Y + 30, 1, WHITE);
    display_print("PRESS 'space' TO SHOOT", VGA_MIDDLE_X - 70, VGA_MIDDLE_Y + 50, 1, WHITE);
}

void init_display()
{
    display_background(BLACK); // acho que seria so isso por enquanto, podemos colocar algumas animacoes depois
}

void display_scores(char *player1_score){
    display_print("SCORE:", 10, 10, 1, WHITE); // aquele 1 eh o tamanho, talvez tenha que aumentar, testamos depois
    display_print("HIGH SCORE:", VGA_MIDDLE_X - 40, 10, 1, WHITE);
    display_print(player1_score, 10, 20, 1, WHITE); // chutei a altura por enquanto
    display_print("00000", VGA_MIDDLE_X - 20, 20, 1, WHITE); // high score temporario
}

int main()
{

/*------------------- inicializacoes -------------------------*/

    struct object_s mysteryShipObj;
    struct object_s enemies_all[NUM_ENEMY_ROWS][QUANT];
    struct object_s playerShip;
    struct object_s barriers[4];
    int line, i;
    
    init_display();

    init_all_enemies(enemies_all);
    init_object(&playerShip, ship[0], 0, 0, 13, 8, VGA_MIDDLE_X, VGA_HEIGHT - 20, 0, 0, 0, 0, 3, 0, 2);
    init_object(&mysteryShipObj, mysteryShip[0], 0, 0, 16, 7, VGA_WIDTH, 5, -1, 0, 2, 2, 1, 100, 7);

    // declarações barreiras
    init_object(&barriers[0], barrier[0], 0, 0, 22, 16, 40, VGA_HEIGHT - 60, 0, 0, 0, 0, 5, 0, 0);
    init_object(&barriers[1], barrier[0], 0, 0, 22, 16, 100, VGA_HEIGHT - 60, 0, 0, 0, 0, 5, 0, 0);
    init_object(&barriers[2], barrier[0], 0, 0, 22, 16, 180, VGA_HEIGHT - 60, 0, 0, 0, 0, 5, 0, 0);
    init_object(&barriers[3], barrier[0], 0, 0, 22, 16, 260, VGA_HEIGHT - 60, 0, 0, 0, 0, 5, 0, 0);

    char player1_score[6] = "00000";
    int running = 0; 
    int menu = 1;

/*------------------- menu loop -------------------------*/

    while(menu){
        start_menu(&mysteryShipObj);
        move_object(&mysteryShipObj);
        char inputKey = getInput();
        if(inputKey == ' '){
            menu = 0;
            init_display();
            running = 1;
        }
        if (inputKey) putchar(inputKey);
    }

/*------------------- game loop -------------------------*/

    while (running)  {
        display_scores(player1_score);
        draw_object(&playerShip, 0, -1);

        draw_object(&barriers[0], 0, -1);
        draw_object(&barriers[1], 0, -1);
        draw_object(&barriers[2], 0, -1);
        draw_object(&barriers[3], 0, -1);

        for(line = 0; line < NUM_ENEMY_ROWS; line++){
            for(i = 0; i < QUANT; i++){
                draw_object(&enemies_all[line][i], 1, -1);
            }
        }

        char inputKey = getInput();

        if(inputKey == 'a' || inputKey == 'd'){
            move_ship(&playerShip, inputKey);
        } else if(inputKey == ' '){
            ship_fire_bullet(&playerShip);
        }
        if (inputKey) putchar(inputKey);

        // move todos os inimigos
        move_all_enemies(enemies_all);

    }

    return 0;
}   