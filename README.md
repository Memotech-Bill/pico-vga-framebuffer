Raspberry Pi Pico - VGA 640x480 Framebuffer
===========================================

This code is a quick trial of a 640x480 framebuffer using the RPi scanvideo code
https://github.com/raspberrypi/pico-extras/tree/master/src/common/pico_scanvideo

640x480 = 307,200 pixels, but the Pico only has 264 kB of RAM. Thus each pixel can
only be 4-bits (giving 16 colours) with each byte of the frame buffer containing two
pixels.

As described in the above reference, to generate video a scanvideo_scanline_buffer->data buffer
must be filled with the pixels to be displayed. A single RAW_RUN is used for 641 pixels, including
a black pixel at the end of the run. The pixels in the scanline buffer are described as 16-bit values.

The problem is to convert the 4-bit values in the framebuffer to 16-bit values in the scanline
buffer fast enough to keep up with the video generation.

The solution is not to use a palette of 16 colours but a double palette of 256 colour pairs.
This enables each byte in the frame buffer to be rapidly converted into a 32-bit word containing
the colours of the two pixels, ready to be loaded into the scanline buffer. Even with this trick
it is necessary to over-clock the Pico.

The code has now been revised to make use of the routine provided by @kilograham to use the Pico
interpolater to accelerate the palette expansion.
https://www.raspberrypi.org/forums/viewtopic.php?f=145&t=305712&p=1848868#p1848706
Using this it is no longer necessary to overclock the Pico.

At present the code just displays the contents of fbuf[], which is filled with some sample text.
The code is only using one core, so it will be possible to use the second core to generate or
update the text to be displayed.

To use, fill an array of sixteen uint16_t values with the RGB definitions of the colours you wish
to use. Then call set_colours() with that array to populate the double palette (dblpal).

A couple of routines, plot_point() and plot_text(), for writing to the frame buffer are supplied.


Build Instructions
------------------

You require pico-sdk and pico-extras from Raspberry Pi. Then from the folder containing these:
```
git clone https://github.com/Memotech-Bill/pico-vga-framebuffer.git
cd pico-vga-framebuffer
mkdir build
cd build
cmake ..
make
```
