cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD vgaboard)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
project(vga_framebuf_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
add_executable(vga_framebuf
  main.c
  monprom.c
  framebuffer.S
)
target_compile_definitions(vga_framebuf PUBLIC -DPICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS=322
  -DPICO_SCANVIDEO_SCANLINE_BUFFER_COUNT=8)

pico_enable_stdio_uart(vga_framebuf 0)
pico_enable_stdio_usb(vga_framebuf 0)
pico_add_extra_outputs(vga_framebuf)
target_link_libraries(vga_framebuf PRIVATE
  pico_stdlib
  pico_scanvideo_dpi
  pico_multicore
  hardware_interp)
