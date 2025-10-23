#include <hf-risc.h>

/* endereço do periférico AXI */
#define SW_AXI_BASE			0xe4a90000

/* registradores STATUS e DATA, implementados no hardware */
#define SW_AXI_STATUS			(*(volatile uint32_t *)(SW_AXI_BASE + 0x010))
#define SW_AXI_SDATA			(*(volatile uint32_t *)(SW_AXI_BASE + 0x020))

/* máscada dos bits READY e VALID, presentes no registrador STATUS */
#define SW_AXI_STREADY			(1 << 0)
#define SW_AXI_STVALID			(1 << 1)


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

struct KeyboardInput getKeyInput(){

	struct KeyboardInput input = {0};
	uint8_t pressed = 0;
	while (1)
	{
		uint8_t x = sw_axi();
		if (x == 0xF0){
			pressed = 1;
		} else if(pressed){
			pressed = 0;
			input.Left = 0;
            input.Right = 0;
            input.Shoot = 0;

				switch (x) { 
                    
					case 0x1C: input.Left = 1; break; // A
					case 0x23: input.Right = 1; break; // D
					case 0x29: input.Shoot = 1; break; // Space

					default:
						break;
				}
	    }
		}
        return input;
}

/* ------ Fim Teclado ------ */

/* ------ Sprites ------ */

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

char player[8][11] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}
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


/* --------------------- */

void init_display()
{
	display_background(BLACK);
	draw
}

void Move_Shoot_Player(){
    struct KeyboardInput input = getKeyInput();
    if(input.Left){
        //move left
    }
    if(input.Right){
        //move right
    }
    if(input.Shoot){
        //shoot
    }
}

int main()
{
    struct object_s player_obj;
    
    init_display();
    init_object(&player_obj, (char *)player, 0, 0, 11, 8, 100, 200, 0, 0, 1, 1);

    while(1){
        Move_Shoot_Player();
        draw_object(&player_obj, 0, -1);
    }

    return 0;
}