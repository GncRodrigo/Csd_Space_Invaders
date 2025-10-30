#include <hf-risc.h>
#include "vga/vga_drv.h"

#define VGA_MIDDLE_X (VGA_WIDTH / 2)
#define VGA_MIDDLE_Y (VGA_HEIGHT / 2)

#define QUANT 11 // quantidade por linha. Da pra mudar depois, ou aumentar por fase


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

char getInput(){ // tentei deixar sem while, talvez de merda, veremos
    /* PS/2 scancode reader
     * Reads raw scancodes from the AXI peripheral and sends the corresponding
     * ASCII character (for common keys) or a human-readable name for special keys.
     * Only sends on key press (make code) and handles the E0 and F0 prefixes.
     */

     if(!sw_axi_data_available()){
        return 0; // no input
     }

    uint8_t pressed = 0;
    uint8_t x = sw_axi();

    if (x == 0xF0){
        pressed = 1;
    } else if(pressed){
        pressed = 0;
        char ch = 0;
        switch (x) {
            /* numbers (top row) */
            case 0x16: ch = '1'; break;
            case 0x1E: ch = '2'; break;
            case 0x26: ch = '3'; break;
            case 0x25: ch = '4'; break;
            case 0x2E: ch = '5'; break;
            case 0x36: ch = '6'; break;
            case 0x3D: ch = '7'; break;
            case 0x3E: ch = '8'; break;
            case 0x46: ch = '9'; break;
            case 0x45: ch = '0'; break;
            /* letters */
            case 0x15: ch = 'q'; break; case 0x1D: ch = 'w'; break; case 0x24: ch = 'e'; break; case 0x2D: ch = 'r'; break; case 0x2C: ch = 't'; break;
            case 0x35: ch = 'y'; break; case 0x3C: ch = 'u'; break; case 0x43: ch = 'i'; break; case 0x44: ch = 'o'; break; case 0x4D: ch = 'p'; break;
            case 0x1C: ch = 'a'; break; case 0x1B: ch = 's'; break; case 0x23: ch = 'd'; break; case 0x2B: ch = 'f'; break; case 0x34: ch = 'g'; break;
            case 0x33: ch = 'h'; break; case 0x3B: ch = 'j'; break; case 0x42: ch = 'k'; break; case 0x4B: ch = 'l'; break;
            case 0x1A: ch = 'z'; break; case 0x22: ch = 'x'; break; case 0x21: ch = 'c'; break; case 0x2A: ch = 'v'; break; case 0x32: ch = 'b'; break;
            case 0x31: ch = 'n'; break; case 0x3A: ch = 'm'; break; 
            //case 0x66: ch = '\b'; putchar(ch); ch =' '; putchar(ch); ch = '\b'; break; 
            //case 0x5A: ch = '\n'; break;
            case 0x41: ch = ','; break;
            case 0x49: ch = '.'; break;
            /* special keys */
            case 0x29: ch = ' '; break; /* space bar */
            default:
                ch = 0; /* ignore other keys */
                break;
        }
        if (ch) {
            return ch;
        } else {
            return 0;
        }
    }
    return 0;
}

/* ---------- objects.c content (object logic) ---------- */
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
    {0, 0, 0, 7, 7, 0, 0, 0},
    {0, 0, 7, 7, 7, 7, 0, 0},
    {0, 7, 7, 7, 7, 7, 7, 0},
    {7, 7, 0, 7, 7, 0, 7, 7},
    {7, 7, 7, 7, 7, 7, 7, 7},
    {0, 7, 0, 7, 7, 0, 7, 0},
    {7, 0, 0, 0, 0, 0, 0, 7},
    {0, 7, 0, 0, 0, 0, 7, 0}
};

char enemy1b[8][8] = {
    {0, 0, 0, 7, 7, 0, 0, 0},
    {0, 0, 7, 7, 7, 7, 0, 0},
    {0, 7, 7, 7, 7, 7, 7, 0},
    {7, 7, 0, 7, 7, 0, 7, 7},
    {7, 7, 7, 7, 7, 7, 7, 7},
    {0, 0, 7, 0, 0, 7, 0, 0},
    {0, 7, 0, 7, 7, 0, 7, 0},
    {7, 0, 7, 0, 0, 7, 0, 7}
};

char enemy2a[8][11] = {
    {0, 0, 7, 0, 0, 0, 0, 0, 7, 0, 0},
    {7, 0, 0, 7, 0, 0, 0, 7, 0, 0, 7},
    {7, 0, 7, 7, 7, 7, 7, 7, 7, 0, 7},
    {7, 7, 7, 0, 7, 7, 7, 0, 7, 7, 7},
    {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},
    {0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 0},
    {0, 0, 7, 0, 0, 0, 0, 0, 7, 0, 0},
    {0, 7, 0, 0, 0, 0, 0, 0, 0, 7, 0}
};

char enemy2b[8][11] = { // todos em branco
    {0, 0, 7, 0, 0, 0, 0, 0, 7, 0, 0},
    {0, 0, 0, 7, 0, 0, 0, 7, 0, 0, 0},
    {0, 0, 7, 7, 7, 7, 7, 7, 7, 0, 0},
    {0, 7, 7, 0, 7, 7, 7, 0, 7, 7, 0},
    {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},
    {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},
    {7, 0, 7, 0, 0, 0, 0, 0, 7, 0, 7},
    {0, 0, 0, 7, 7, 0, 7, 7, 0, 0, 0}
};

char ship[8][13] = { // ta em verde
    {0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0},
    {0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}
};

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
    int bullets;
};


void init_object(struct object_s *obj, char *spritea, char *spriteb,
	char *spritec, char spriteszx, char spriteszy, int posx, int posy, 
	int dx, int dy, int spx, int spy, int health, int score, int bullets)
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
    obj->bullets = bullets;         /* balas do objeto, a ideia eh limitar em um eu acho*/
}

void init_enemies(struct object_s *enemies, int type, int line)
{
    int startX = 20;
    int spcX = 20;
    int spcY = 20;

    switch (type)
    {
    case 1:
        for (int i = 0; i < QUANT; i++) {
            init_object(&enemies[i], enemy1a[0], enemy1b[0], 0, 8, 8, startX + i * spcX, line * spcY, 1, 0, 5, 5, 1, 30, 1);
        }
        break;
    case 2:
        for (int i = 0; i < QUANT; i++) {
            init_object(&enemies[i], enemy2a[0], enemy2b[0], 0, 11, 8, startX + i * spcX, line * spcY, 1, 0, 5, 5, 1, 20, 1);
        }
    default:
        break;
    }
}

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

void move_enemies(struct object_s *enemies, int count){
    int minX = VGA_WIDTH + 1, maxX = -1;
    for (int i = 0; i < count; i++){ // pega a posicao dos extremos
        if (enemies[i].health <= 0){ // morto nao conta
            continue;
        }
        if (enemies[i].posx < minX) minX = enemies[i].posx;
        if (enemies[i].posx > maxX) maxX = enemies[i].posx;
        }
    

    for (int i = 0; i < count; i++){
         if (maxX + enemies[i].spriteszx >= (VGA_WIDTH - 10) || minX <= 10){ // 10 de margem
        for (int i = 0; i < count; i++){
            enemies[i].dx = -enemies[i].dx; // inverte a direção
            enemies[i].posy += 10; // vai descendo, acho que nao precisa de limite aqui
        }
    }

    for (int i = 0; i < count; i++){
        if(enemies[i].health > 0){
            move_object(&enemies[i]);
        }
    }
    
    }
}

void move_ship(struct object_s *obj, char inputKey)
{
    if(inputKey == 'a' && obj->posx > (0 + obj->spriteszx)){ 
        obj->posx -= 2; 
    } else if(inputKey == 'd' && obj->posx < (VGA_WIDTH - obj->spriteszx)){
        obj->posx += 2;
    }

    draw_object(obj, 0, -1);
}

void ship_fire_bullet(struct object_s *obj)
{
    // implementar depois
}


void start_menu(struct object_s *mysteryShip){
    draw_object(mysteryShip, 1, -1);
    display_print("SPACE INVADERS", VGA_MIDDLE_X - 50, VGA_MIDDLE_Y - 20, 2, WHITE);
    display_print("PRESS ANY KEY TO START", VGA_MIDDLE_X - 70, VGA_MIDDLE_Y + 10, 1, WHITE);
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
    struct object_s mysteryShipObj;
    struct object_s enemies_type1_a[QUANT];
    struct object_s enemies_type1_b[QUANT];
    struct object_s enemies_type2[QUANT];
    struct object_s playerShip;
    
    
    init_display();

    init_enemies(enemies_type1_a, 1, 1); // tipo 1 na linha 1
    init_enemies(enemies_type2, 2, 2); // tipo 2 na linha 2
    init_enemies(enemies_type1_b, 1, 3); // tipo 1 na linha 3
    init_object(&playerShip, ship[0], 0, 0, 13, 8, VGA_MIDDLE_X, VGA_HEIGHT - 20, 0, 0, 0, 0, 3, 0, 2);
    init_object(&mysteryShipObj, mysteryShip[0], 0, 0, 16, 7, VGA_WIDTH, 5, -1, 0, 2, 2, 1, 100, 7);

    char player1_score[6] = "00000"; // nao gostaria de usar char, mas o display_print so aceita string, foda
    int running = 0; 
    int menu = 1;

    while(menu){
        start_menu(&mysteryShipObj);
        move_object(&mysteryShipObj);
        char inputKey = getInput();
        if(inputKey != 0){
            menu = 0;
            running = 1;
        }
    }

    while (running)  {
        display_scores(player1_score);
        draw_object(&playerShip, 0, -1);

        for(int i = 0; i < QUANT; i++){
            draw_object(&enemies_type1_a[i], 1, -1);
            draw_object(&enemies_type2[i], 1, -1);
            draw_object(&enemies_type1_b[i], 1, -1);
        }

        char inputKey = getInput();

        if(inputKey == 'a' || inputKey == 'd'){
            move_ship(&playerShip, inputKey);
        } else if(inputKey == ' '){
            ship_fire_bullet(&playerShip);
        }

        // move de baixo pra cima, videozao que eu vi tava assim eu acho
        move_enemies(enemies_type1_b, QUANT);
        move_enemies(enemies_type2, QUANT);
        move_enemies(enemies_type1_a, QUANT);

    }

    return 0;
}