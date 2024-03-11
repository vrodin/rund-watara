#include <supervision.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bitmap.h"
#include "font.h"

#define WORDS_PER_LINE 24
#define BYTES_PER_LINE 48
#define VISIBLE_BYTES_PER_LINE 40

#define LEFT_BUTTON 	JOY_LEFT_MASK
#define RIGHT_BUTTON 	JOY_RIGHT_MASK
#define UP_BUTTON 	JOY_UP_MASK
#define DOWN_BUTTON 	JOY_DOWN_MASK
#define A_BUTTON 	JOY_BTN_A_MASK
#define B_BUTTON 	JOY_BTN_B_MASK

#define WHITE 1
#define BLACK 0

#define HEIGHT 0xA0
#define WIGHT 0xA0

static void clearDisplay(void)
{
    memset(SV_VIDEO, 0x00, 0x1E00);
}

static void clearDisplay2(void)
{
    unsigned int i;
    unsigned int offset = 0xF00;
    for(i = offset; i < 0x1E00; i += 0x30) {
        memset(SV_VIDEO + i, 0x00, 0x30);
    }
}

static void clearDisplay3(void)
{
    unsigned int i;
    unsigned int offset = 0xF00;
    for(i = offset; i < 0x1E00; i += 0x30) {
        memset(SV_VIDEO + i, 0x00, 5);
    }
}

static void delay(int count) {
    while(count) {
	__asm__("nop");
	count--;
    }
}

/* Necessary conversion to have 2 bits per pixel with darkest hue */
/* Remark: The Supervision uses 2 bits per pixel, and bits are mapped into pixels in reversed order */
static unsigned int __fastcall__ double_reversed_bits(unsigned char)
{
    __asm__("stz ptr2");
    __asm__("stz ptr2+1");
    __asm__("ldy #$08");
L1: __asm__("lsr a");
    __asm__("php");
    __asm__("rol ptr2");
    __asm__("rol ptr2+1");
    __asm__("plp");
    __asm__("rol ptr2");
    __asm__("rol ptr2+1");
    __asm__("dey");
    __asm__("bne %g", L1);
    __asm__("lda ptr2");
    __asm__("ldx ptr2+1");
    return __AX__;
}

// frame counter
unsigned int frame = 0;

// general use string buffer
char text[16];

// distance ran
unsigned char  d, delta, cloud_1_y, d_jump, d_jump_t, d_run, d_tumble_t, ox;

unsigned char  cursorX = 0;

unsigned char  cursorY = 0;

static void setCursor(char x, char y)
{
    cursorX = x;
    cursorY = y;
}

void intro()
{
    int i;
    for(i = 0; i < 28; i += 2)
    {
        clearDisplay();
        setCursor(46, i);
      //arduboy.print("ARDUBOY");
      	delay(16);
    }

    //arduboy.tunes.tone(987, 120);
    //delay(120);
    //arduboy.tunes.tone(1318, 400);

    //arduboy.setCursor(42, 48);
    //arduboy.print("presents");
    //arduboy.display();

    //delay(2000);

    //arduboy.tunes.tone(1318, 1);
}

unsigned char pressed(unsigned char buttonMask)
{
    return ~SV_CONTROL & buttonMask;
}

void drawPixel(unsigned char x, unsigned  char y, unsigned  char color)
{
  unsigned int addr;
  unsigned char offset;
  
  if (x < 0 || x > WIGHT || y < 0 || y > HEIGHT)
  {
    return;
  }

  addr = (HEIGHT - y) * BYTES_PER_LINE + (x >> 2);
  
  offset = (x << 1) % 8;
  if (color)
  {
    *(unsigned int *) (0x4000|addr) |=  3 << (offset + offset % 2);
  }
  else
  {
    *(unsigned int *) (0x4000|addr) &= ~ (3 << (offset + offset % 2));
  }
}

void drawBitmap(unsigned char x, unsigned char y, const unsigned char *bitmap, unsigned char w, unsigned char h, unsigned char color)
{
    unsigned char row, cols,iCol,xOffset;
    unsigned int lineaddr, startBitmapAddr;
     
    if (x+w < 0 || x > WIGHT || y+h < 0 || y > SV_LCD.height)
        return;
    
    startBitmapAddr = (HEIGHT - y) * BYTES_PER_LINE + (x >> 2);

    xOffset = (x << 1) % 8;
    cols = w >> 3;
    if (w%8!=0) cols++;
    
    for( row = 0; row < h; row++ ) {
        lineaddr = startBitmapAddr - row * BYTES_PER_LINE;
        
        for(iCol = 0; iCol < cols; iCol++){
            if(color) {
        	*(unsigned long *) (0x4000|lineaddr + (iCol<<1)) |= double_reversed_bits(bitmap[row * cols + iCol]) << (xOffset + xOffset % 2);
            }
            else {
                *(unsigned long *) (0x4000|lineaddr + (iCol<<1)) &= ~(double_reversed_bits(bitmap[row * cols + iCol]) << (xOffset + xOffset % 2) & 0xFFFFFFFF);
            }

        }
        
    }
    delay(10);
}

static void print(char* str, int len)
{
    unsigned char i;
    
    for(i = 0; i< len; i++) {
    	char character = *(str + i * sizeof(char ));
    	drawBitmap(cursorX, cursorY, font + (character - 32) * sizeof(unsigned char ) * 8, 8, 8, 0);
    }
}

void drawCircle(unsigned char x0, unsigned char y0, unsigned char r, unsigned char color)
{
  unsigned char  f, ddF_x, ddF_y, x, y;
  
  f = 1 - r;
  x = 0;
  y = r;
  ddF_x = 1;
  ddF_y = -2 * r;
  
  drawPixel(x0, y0+r, color);
  drawPixel(x0, y0-r, color);
  drawPixel(x0+r, y0, color);
  drawPixel(x0-r, y0, color);

  while (x<y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void swap(unsigned char a, unsigned char b)
{
  unsigned char temp;
  temp = a;
  a = b;
  b = temp;
}

void drawLine(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char color)
{
  unsigned char dx, dy, ystep, err, steep;
  // bresenham's algorithm - thx wikpedia
  steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(&x0, &y0);
    swap(&x1, &y1);
  }

  if (x0 > x1) {
    swap(&x0, &x1);
    swap(&y0, &y1);
  }

  
  dx = x1 - x0;
  dy = abs(y1 - y0);

  err = dx >> 1;

  if (y0 < y1)
  {
    ystep = 1;
  }
  else
  {
    ystep = -1;
  }

  for (; x0 <= x1; x0++)
  {
    if (steep)
    {
      drawPixel(y0, x0, color);
    }
    else
    {
      drawPixel(x0, y0, color);
    }

    err -= dy;
    if (err < 0)
    {
      y0 += ystep;
      err += dx;
    }
  }
}

void setup(void)
{
    SV_LCD.width = WIGHT;
    SV_LCD.height = HEIGHT;
    SV_LCD.xpos = 0;
    SV_LCD.ypos = 0;
    
    clearDisplay();
    drawLine(0,19,127,19,WHITE);
    //intro();
    d = 0;
    delta = 0;

    cloud_1_y = 50;

    d_jump = 0;
    d_jump_t = 0;

    d_tumble_t = 0;
    d_run = 0;

    ox = 130;
    
}

void loop()
{
    unsigned char  dy;
    if (!d_run && pressed(A_BUTTON)) {
        d_run = 1;
    }

    if (d_tumble_t && pressed(A_BUTTON)) {
       setup();
       return;
    }

    ++frame;
    if (frame>16000) frame = 0;

    // increase distance whilst running
    if (d_run && (++delta > 4)) {
        delta = 0; ++d; 
    }

    // obstacles
    if (d_run) {
        ox -= (frame%2)*(d/100) + 2;
        if (ox < -15) ox += 180 + rand() % 61;
    }

    // jump!
    if (!d_jump_t && pressed(A_BUTTON)) {
         d_jump_t = 1;
         d_jump=5;

         //arduboy.tunes.tone(440, 40);

    } else if (d_jump_t) {
        //if (d_jump_t == 3) arduboy.tunes.tone(880, 80);

        ++d_jump_t;

        if (d_jump_t<6) {
            d_jump +=6;
        } else if (d_jump_t<9) {
            d_jump +=2;
        } else if (d_jump_t<13) {
            d_jump +=1;
        } else if (d_jump_t == 16 || d_jump_t == 18) {
            d_jump +=1;
        } else if (d_jump_t == 20 || d_jump_t == 22) {
            d_jump -=1;
        } else if (d_jump_t>38) {
            d_jump = 0;
            d_jump_t = 0;
        } else if (d_jump_t>32) {
            d_jump -=6;
        } else if (d_jump_t>29) {
            d_jump -=2;
        } else if (d_jump_t>25) {
            d_jump -=1;
        }
    }

  // hit detect
  if (!d_tumble_t && ox > -10 && ox <16 && d_jump_t < 14) {
    d_tumble_t = 1;    
  }

  if (d_tumble_t) {
    if (d_tumble_t == 1) {
      //arduboy.tunes.tone(246, 80);
    } else if (d_tumble_t == 6) {
      //arduboy.tunes.tone(174, 200);
    }

    ++d_tumble_t;
    if (d_jump > 0) {
      d_jump -= 1;
      ox -= 1;
    } else {
      d_run = 0;
    }
  }

  clearDisplay();

  setCursor(100, 0);
  sprintf(text,"%d",d);
  //print(text, 4);


  // parallax clouds
  drawBitmap(128 -(d%128),cloud_1_y,cloud_1,17,7,WHITE);

  //if (d%128 == 0) {
  //  cloud_1_y = cloud_1_y + rand() % 11;
  //}

  // terrain
  if (d_jump <= 4) {
    drawLine(3,20,12,20,BLACK);
  }

  // obstacles
  drawBitmap(ox,20,cactus_1,12,24,WHITE);

  // dino
  dy = 20+d_jump;

  // tumbles!
  if (d_tumble_t) {
    drawBitmap(0,dy,dino_tumble,30,18,WHITE);

  // runs!
  } else {
    drawBitmap(0,dy,dino_top,20,18,WHITE);
  
    // Run, Dino, Run!
    //if (d_run && !d_jump) {
    //  if ((frame%8)/4) {
    //    drawBitmap(0,dy+18,dino_leg_1,20,5,WHITE);
    //  } else {
    //    drawBitmap(0,dy+18,dino_leg_2,20,5,WHITE);
    //  }
    //} else {
    //  drawBitmap(0,dy+18,dino_leg_0,20,5,WHITE);
    //}
  }

  delay(1);
}

void main(void)
{
    setup();
    
    while (1) {
    	loop();

    }
}
