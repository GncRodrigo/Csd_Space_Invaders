#include <hf-risc.h>
#include "vga/vga_drv.h"

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

char getInput(){
    /* PS/2 scancode reader
     * Reads raw scancodes from the AXI peripheral and sends the corresponding
     * ASCII character (for common keys) or a human-readable name for special keys.
     * Only sends on key press (make code) and handles the E0 and F0 prefixes.
     */

    uint8_t pressed = 0;
    while (1)
    {
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
                    case 0x66: ch = '\b'; putchar(ch); ch =' '; putchar(ch); ch = '\b'; break; 
                    case 0x5A: ch = '\n'; break;
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
                }
        }
    }
}


/* ---------- objetcs.c content (object & ship logic) ---------- */

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
};

void shoot(struct object_s *obj);
void enemy_shoot(struct object_s *obj, int e_type);
void player_handle_inputs(struct object_s *obj);


struct ship_s { // estrutura para o jogador
    struct object_s *obj;
    int health; 
    int bulletspeed; // so pra ficar mais facil de mudar a velocidade dos tiros
    int bulletcount; // numero de tiros que o jogador pode ter na tela ao mesmo tempo, pode aumentar com o tempo ou fazer power ups se tiver tempo
    void (*shoot)(struct object_s *obj); // funcao de atirar
    void (*player_handle_inputs)(struct object_s *obj); // funcao para lidar com os inputs do jogador, fiz a logica de mover dentro dela mesmo
};

struct enemies_s { // estrutura para os inimigos
    struct object_s *obj;
    int type; // tipo do inimigo, pode ser usado pra definir comportamento diferente
    int points; // pontos que o inimigo vale quando destruido
    void (*move_enemy)(struct object_s *obj); // funcao para mover o inimigo, pode ser mais de uma dependendo do tipo de inimigo
    void (*enemy_shoot)(struct object_s *obj, int e_type); // funcao de atirar, nem todos os inimigos vao atirar, mas alguns vao
};



void init_object(struct object_s *obj, char *spritea, char *spriteb,
    char *spritec, char spriteszx, char spriteszy, int posx, int posy, 
    int dx, int dy, int spx, int spy)
{
    obj->sprite_frame[0] = spritea;
    obj->sprite_frame[1] = spriteb;
    obj->sprite_frame[2] = spritec;
    obj->spriteszx = spriteszx;
    obj->spriteszy = spriteszy;
    obj->cursprite = 0;
    obj->posx = posx;
    obj->posy = posy;
    obj->dx = dx;
    obj->dy = dy;
    obj->speedx = spx;
    obj->speedy = spy;
    obj->speedxcnt = spx;
    obj->speedycnt = spy;
}

void init_ship(struct ship_s *ship, struct object_s *obj, int health,
    int bulletspeed, int bulletcount,
    void (*shoot)(struct object_s *obj),
    void (*player_handle_inputs)(struct object_s *obj))
{
    ship->obj = obj;
    ship->health = health;
    ship->bulletspeed = bulletspeed;
    ship->bulletcount = bulletcount;
    ship->shoot = shoot;
    ship->player_handle_inputs = player_handle_inputs;
}

void init_enemies(struct enemies_s *enemies, struct object_s *obj, int type,
    int points,
    void (*move_enemy)(struct object_s *obj),
    void (*enemy_shoot)(struct object_s *obj, int e_type))
{
    enemies->obj = obj;
    enemies->type = type;
    enemies->points = points;
    enemies->move_enemy = move_enemy;
    enemies->enemy_shoot = enemy_shoot;
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

void move_enemy(struct object_s *obj) // tem que mudar depois, fazer ele virar quando chegar na borda, adicionar comportamento diferente pra cada inimigo
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

void shoot(struct object_s *obj){ // tem que lembrar de fazer um delay entre os tiros, normalmente so deixa atirar um por vez
    //code to shoot
    printf("Shoot from position (%d, %d)\n", obj->posx, obj->posy);
}

void enemy_shoot(struct object_s *obj, int e_type){ // inimigos atirando, pode ser diferente dependendo do tipo de inimigo
    //code for enemy shoot
    printf("Enemy of type %d shooting from position (%d, %d)\n", e_type, obj->posx, obj->posy);
}

void player_handle_inputs(struct object_s *obj){
    char input = getInput();
    if(input == 'a'){
        //move left
        if(obj->posx > (0 + obj->spriteszx)){ // nao passar dos cantos
            obj->dx = -1;
        }
    } else if(input == 'd'){
        //move right
        if(obj->posx < (320 - obj->spriteszx)){ // mesma coisa
            obj->dx = 1;
        }
    } else if (input == ' '){
        //shoot
        shoot(obj);
    }
}


/* ---------- sprites + main (merged from original space_invaders.c) ---------- */
/* sprites and sprite drawing */
char monster1a[8][11] = {
    {0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0},
    {2, 0, 0, 2, 0, 0, 0, 2, 0, 0, 2},
    {2, 0, 2, 2, 2, 2, 2, 2, 2, 0, 2},
    {2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0},
    {0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0},
    {0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0}
};

char monster1b[8][11] = {
    {0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0},
    {0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0},
    {0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0},
    {0, 2, 2, 0, 2, 2, 2, 0, 2, 2, 0},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 0, 2, 0, 0, 0, 0, 0, 2, 0, 2},
    {0, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0}
};

char player[8][11] = { // fiz com o gpt, 3 e ciano
    {0, 0, 0, 3, 3, 0, 3, 3, 0, 0, 0},
    {0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0},
    {0, 3, 3, 0, 3, 3, 3, 3, 0, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 3, 3, 0, 3, 3, 3, 0, 3, 3, 3},
    {0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0},
    {0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0},
    {3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3}
};


void init_display()
{
    display_background(BLACK); // acho que seria so isso por enquanto, podemos colocar algumas animacoes depois
}


int main()
{
    struct object_s player_obj;
    struct ship_s player_ship;

    init_display();

    init_object(&player_obj, (char *)player, 0, 0, 11, 8, 100, 200, 0, 0, 1, 1);

    init_ship(&player_ship, &player_obj, 3, 5, 1, shoot, player_handle_inputs);

    while (1) {
        player_ship.player_handle_inputs(player_ship.obj);
        draw_object(player_ship.obj, 0, -1);
    }

    return 0;
}