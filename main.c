/*  main.c - The main program

    WJB  8/ 4/21 Experimenting with a 640x480x4-bit (16 colour) framebuffer.
    WJB  9/ 4/21 Use Pico Interpolater code from @kilograham

*/

#define USE_INTERP 1

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#if USE_INTERP == 1
#include "hardware/interp.h"
#endif
#include "monprom.h"
#include <stdio.h>
#include <string.h>

#define WIDTH   640
#define HEIGHT  480
#define NCLR    16

uint16_t    colours[NCLR] =
    {
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(  0u,   0u,   0u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(128u,   0u,   0u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(  0u, 128u,   0u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(128u, 128u,   0u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(  0u,   0u, 128u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(128u,   0u, 128u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(  0u, 128u, 128u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(128u, 128u, 128u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8( 64u,  64u,  64u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(255u,   0u,   0u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(  0u, 255u,   0u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(255u, 255u,   0u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(  0u,   0u, 255u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(255u,   0u, 255u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(  0u, 255u, 255u),
    PICO_SCANVIDEO_PIXEL_FROM_RGB8(255u, 255u, 255u)
    };

uint32_t    dblpal[NCLR * NCLR];

uint8_t     fbuf[WIDTH * HEIGHT / 2];

#if USE_INTERP == 1
extern void convert_from_pal16(uint32_t *dest, uint8_t *src, uint count);
#endif

void __time_critical_func(render_loop) (void)
    {
#ifdef DEBUG
    printf ("Starting render\n");
#endif
    while (true)
        {
        struct scanvideo_scanline_buffer *buffer = scanvideo_begin_scanline_generation (true);
        int iScan = scanvideo_scanline_number (buffer->scanline_id);
        uint32_t *twoclr = buffer->data;
        uint8_t *twopix = &fbuf[(WIDTH / 2) * iScan];
#if USE_INTERP == 1
        ++twoclr;
        convert_from_pal16 (twoclr, twopix, WIDTH / 2);
        twoclr += WIDTH / 2;
#else
        for ( int iCol = 0; iCol < WIDTH / 2; ++iCol )
            {
            ++twoclr;
            *twoclr = dblpal[*twopix];
            ++twopix;
            }
        ++twoclr;
#endif
        *twoclr = COMPOSABLE_EOL_ALIGN << 16;
        twoclr = buffer->data;
        twoclr[0] = ( twoclr[1] << 16 ) | COMPOSABLE_RAW_RUN;
        twoclr[1] = ( twoclr[1] & 0xFFFF0000 ) | ( WIDTH - 2 );
        buffer->data_used = ( WIDTH + 4 ) / 2;
        scanvideo_end_scanline_generation (buffer);
        }
    }

void setup_video (void)
    {
#ifdef DEBUG
    printf ("System clock speed %d kHz\n", clock_get_hz (clk_sys) / 1000);
    printf ("Starting video\n");
#endif
#if USE_INTERP == 1
    // Configure interpolater lanes
    interp_config c = interp_default_config();
    interp_config_set_shift(&c, 22);
    interp_config_set_mask(&c, 2, 9);
    interp_set_config(interp0, 0, &c);
    interp_config_set_shift(&c, 14);
    interp_config_set_cross_input(&c, true);
    interp_set_config(interp0, 1, &c);
    interp_set_base(interp0, 0, (uintptr_t)dblpal);
    interp_set_base(interp0, 1, (uintptr_t)dblpal);
#endif
    scanvideo_setup(&vga_mode_640x480_60);
    scanvideo_timing_enable(true);
#ifdef DEBUG
    printf ("System clock speed %d kHz\n", clock_get_hz (clk_sys) / 1000);
#endif
    }

void set_colours (uint16_t *pclr)
    {
    uint32_t *dpal = dblpal;
    for ( int i = 0; i < NCLR; ++i )
        {
        for ( int j = 0; j < NCLR; ++j )
            {
            *dpal = ( pclr[i] << 16 ) | pclr[j];
            ++dpal;
            }
        }
    }

void plot_point (int x, int y, int clr)
    {
    bool    odd = x & 1;
    int     n = ( WIDTH / 2 ) * y + x / 2;
    clr &= 0x0F;
    if ( ( n >= 0 ) && ( n < WIDTH * HEIGHT / 2 ) )
        {
        uint8_t *p = &fbuf[n];
        if ( odd ) *p = ( clr << 4 ) | ( *p & 0x0F );
        else *p = ( *p & 0xF0 ) | clr;
        }
    }

void plot_text (const char *ps, int row, int col, int fg, int bg)
    {
    if ( ( row < 0 ) || ( row >= ( HEIGHT / GLYPH_HEIGHT / 2 ) )
        || ( col < 0 ) || ( col >= ( WIDTH / GLYPH_WIDTH ) ) ) return;
    int nch = strlen (ps);
    if ( col + nch > ( WIDTH / GLYPH_WIDTH ) ) nch = ( WIDTH / GLYPH_WIDTH ) - col;
    fg &= 0x0F;
    if ( bg > 0 ) bg = bg &= 0x0F;
    for ( int iScan = 0; iScan < 2 * GLYPH_HEIGHT; ++iScan )
        {
        uint8_t *p = &fbuf[(WIDTH / 2) * ((2 * GLYPH_HEIGHT) * row + iScan) + ( GLYPH_WIDTH / 2 ) * col];
        for ( int ich = 0; ich < nch; ++ich )
            {
            uint8_t line = mon_alpha_prom[ps[ich]][iScan / 2];
            if ( bg >= 0 )
                {
                for ( int i = 0; i < 4; ++i )
                    {
                    switch (line & 0xC0)
                        {
                        case 0x00: *p = ( bg << 4 ) | bg; break;
                        case 0x40: *p = ( fg << 4 ) | bg; break;
                        case 0x80: *p = ( bg << 4 ) | fg; break;
                        case 0xC0: *p = ( fg << 4 ) | fg; break;
                        }
                    line <<= 2;
                    ++p;
                    }
                }
            else
                {
                for ( int i = 0; i < 4; ++i )
                    {
                    switch (line & 0xC0)
                        {
                        case 0x00: break;
                        case 0x40: *p = ( fg << 4 ) | ( *p & 0x0F ); break;
                        case 0x80: *p = ( *p & 0xF0 ) | fg; break;
                        case 0xC0: *p = ( fg << 4 ) | fg; break;
                        }
                    line <<= 2;
                    ++p;
                    }
                }
            }
        }
    }

void core1_entry() {
    char row[2][WIDTH];
    int r=0, Y=0, S=0;

    sleep_ms(5000);

    // continuous Sierpinski triangle on "tube" framebuffer
    // https://en.wikipedia.org/wiki/Sierpinski_triangle
    memset(row[r], 0, WIDTH);
    row[r][WIDTH/2]=1;

    for(;;)
    {
      memset(fbuf+((Y+10)%HEIGHT)*WIDTH/2, 0, WIDTH/2);
      for(int x=0; x<WIDTH; ++x)
        plot_point(x, Y, row[r][x] ? 12 : 8);

      row[1-r][0]       = row[r][1] ^ row[r][WIDTH-1];
      row[1-r][WIDTH-1] = row[r][WIDTH-2] ^ row[r][0];
      for(int x=1; x<WIDTH-1; ++x)
        row[1-r][x] = row[r][x-1] ^ row[r][x+1];

      r=1-r;  
      Y=(Y+1)%HEIGHT;

      sleep_ms(1);
    }
}

int main (void)
    {
    char text[80];
#if USE_INTERP != 1
    set_sys_clock_khz (200000, true);
#else
    set_sys_clock_khz (150000, true);
#endif
    stdio_init_all();
#ifdef DEBUG
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    for (int i = 10; i > 0; --i )
        {
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
        printf ("%d seconds to start\n", i);
        }
    printf ("Building screen.\n");
#endif
    memset (fbuf, 0, sizeof (fbuf));
    set_colours (colours);
    for ( int i = 0; i < NCLR; ++i )
        {
        sprintf (text, "Colour %d", i);
        plot_text (text, i, i, i, -1);
        plot_text (text, i, i + 40, i, i + 8);
        }
    setup_video ();
    multicore_launch_core1(core1_entry);
    render_loop ();
    }
