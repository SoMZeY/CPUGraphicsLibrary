#include "graphics.h"
#include <unistd.h>

int main() 
{
    // Init
    init_graphics();

    // Create a second offscreen buffer
    void *buffer = new_offscreen_buffer();
    if (!buffer) 
    {
        exit_graphics();
        return 1;
    }

    // Clear just in case
    clear_screen(buffer);

    // Individual pixels somewhere on the top left
    draw_pixel(buffer, 10, 10, RGB(31, 0, 0));   // red
    draw_pixel(buffer, 12, 12, RGB(0, 63, 0));   // green
    draw_pixel(buffer, 14, 14, RGB(0, 0, 31));   // blue 

    // red line somewhere on the top left quadrant
    draw_line(buffer, 50, 50, 250, 150, RGB(31, 0, 0));

    // Weird shaped triangl in green
    fill_triangle(buffer, 100, 100, 120, 150, 60, 180, RGB(0, 63, 0));

    // Switch the buffers
    blit(buffer);

    // If not a null terminator (any key u can press), then terminate
    char key = '\0';
    while (!key) 
    {
        key = getkey();
    }
    
    // Cleanup
    exit_graphics();

    // Will sleep for a bit before terminating
    sleep_ms(5000);
    return 0;
}