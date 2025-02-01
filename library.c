#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/fb.h>

#include "graphics.h"

// Global variables
int fd = -1;
color_t* fb_ptr = NULL;
unsigned int screensize;
fd_set fdescriptor;
// Global structs
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
struct termios tcinfo;
struct timeval time;
struct timespec sleep_time;

void init_graphics()
{
    // Open the fb0 file
    fd = open("/dev/fb0", O_RDWR);
    
    if (fd == -1)
    {
        // Log the error
        return;
    }

    // Call the first ioctl for the virtual screen height VS
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        // Log the error
        return;
    }

    // Call the second ioctl for the fixed screen line length
    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        // Log the error
        return;
    }   
    
    // Multiply the screen height with line length, and pixel is 2 bytes
    screensize = vinfo.yres_virtual * finfo.line_length;
    
    // Map the contents of the file to a memory
    fb_ptr = (color_t*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // mmap returns (void*)-1 on fail
    if ((void*)fb_ptr == (void*)-1)
    {
        // Log the error
        return;
    }

    // Get canonical/echo, disable them, and then set them back in
    ioctl(STDIN_FILENO, TCGETS, &tcinfo);
    tcinfo.c_lflag &= ~ICANON;
    tcinfo.c_lflag &= ~ECHO;
    ioctl(STDIN_FILENO, TCSETS, &tcinfo);

    // Even though this is part of the getkey method, it's better be created here
    time.tv_sec = 0;
    time.tv_usec = 0;

    return;
}

void exit_graphics() 
{
    // Enable canonical and echo
    tcinfo.c_lflag |= ICANON;
    tcinfo.c_lflag |= ECHO;
    ioctl(STDIN_FILENO, TCSETS, &tcinfo);

    // unmap the mapped buffer
    if (munmap(fb_ptr, screensize) == -1)
    {
        // Log the error
    }

    // Close the file descriptor of the frame buffer file
    close(fd);
}

char getkey() 
{
    // Clear and Init the file descriptor set
    FD_ZERO(&fdescriptor);
    FD_SET(STDIN_FILENO, &fdescriptor);

    char key_pressed = '\0';

    // Monitor the fds, with the nfds + 1 according to the documentation (which is weird)
    int ret = select(STDIN_FILENO + 1, &fdescriptor, NULL, NULL, &time);

    // Check for the input
    if (ret > 0) 
    {
        read(STDIN_FILENO, &key_pressed, 1);
    }

    return key_pressed;
}

void sleep_ms(long ms) 
{
    if (ms < 0)
    {
        // Log error
        return;
    }
    
    // Hnadle the sleeptime assignment
    struct timespec sleep_time;
    sleep_time.tv_sec = ms / 1000;
    sleep_time.tv_nsec = (ms % 1000) * 1000000;

    // Call the nanosleep syscall
    nanosleep(&sleep_time, NULL);
}

void clear_screen(void *img) 
{
    if (img == NULL)
    {
        // Log error
        return;
    }

    // For good sake cast everything to char to ensure byte by byte copying
    char* destination = (char*)img;

    int i;
    for (i = 0; i < vinfo.yres_virtual * finfo.line_length; i++)
    {
        destination[i] = 0;
    }
}

void draw_pixel(void *img, int x, int y, color_t color) 
{
    // Check the inputs are valid
    if (img == NULL || x < 0 || y < 0 || x >= finfo.line_length / 2 || y >= vinfo.yres_virtual)
    {
        // Log error
        return;
    }

    // Cast to the color_t to work with pixels directly and not seperate bytes
    color_t* img_new = (color_t*)img;

    // Now we need to calculate the offset for the row_major order buffer.
    // We can do this by multiplying the y by line_length so we will be in the correct line width buffer
    // Then need to find the correct pixel in that line buffer and we can just do it by adding the x
    unsigned int pixel_offset = ((finfo.line_length / 2) * y) + x;

    img_new[pixel_offset] = color; 
}

void draw_line(void *img, int x1, int y1, int x2, int y2, color_t c) 
{
    // Got the algorithm from https://www.baeldung.com/cs/bresenhams-line-algorithm
    // Check the inputs are valid
    if (img == NULL || x1 < 0 || y1 < 0 || 
        x1 >= finfo.line_length / 2 || y1 >= vinfo.yres_virtual || 
        x2 < 0 || y2 < 0 || 
        x2 >= finfo.line_length / 2 || y2 >= vinfo.yres_virtual)
    {
        // Log error
        return;
    }

    // DX
    int dx = x2 - x1;
    
    // Get the absolute value
    if (dx < 0) dx = -dx;

    // Calculate the increment variable s_x
    int s_x = (x1 < x2) ? 1 : -1;

    // DY
    int dy = y2 - y1;
    if (dy < 0) dy = -dy;
    dy = -dy; // Make dy negative as per Bresenham's algorithm

    // Calculate the increment variable s_y
    int s_y = (y1 < y2) ? 1 : -1;

    // Initialize the error term
    int e = dx + dy;

    while (1)
    {
        draw_pixel(img, x1, y1, c);

        // Check if the end point is reached
        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * e;

        // Move in the x-direction if needed
        if (e2 >= dy)
        {
            e += dy;
            x1 += s_x;
        }

        // Move in the y-direction if needed
        if (e2 <= dx)
        {
            e += dx;
            y1 += s_y;
        }
    }
}

void fill_triangle(void *img, int x1, int y1, int x2, int y2, int x3, int y3, color_t c) 
{
    // Used this resource for this https://www.gabrielgambetta.com/computer-graphics-from-scratch/07-filled-triangles.html
    // Check the inputs are valid
    if (img == NULL || 
        x1 < 0 || y1 < 0 || 
        x1 >= finfo.line_length / sizeof(color_t) || y1 >= vinfo.yres_virtual || 
        x2 < 0 || y2 < 0 || 
        x2 >= finfo.line_length / sizeof(color_t) || y2 >= vinfo.yres_virtual ||
        x3 < 0 || y3 < 0 || 
        x3 >= finfo.line_length / sizeof(color_t) || y3 >= vinfo.yres_virtual)
    {
        // Log error
        return;
    }

    // y0 <= y1 <= y2
    int px0 = x1, py0 = y1;
    int px1 = x2, py1 = y2;
    int px2 = x3, py2 = y3;

    // Sort the first and second points if necessary
    if (py1 < py0) 
    {
        // Swap px0, py0 to px1, py1
        int tmp_x = px0;
        int tmp_y = py0;
        px0 = px1;
        py0 = py1;
        px1 = tmp_x;
        py1 = tmp_y;
    }

    // Sort the first and third points if necessary
    if (py2 < py0) 
    {
        // Swap px0, py0 with px2, py2
        int tmp_x = px0;
        int tmp_y = py0;
        px0 = px2;
        py0 = py2;
        px2 = tmp_x;
        py2 = tmp_y;
    }

    // Sort the second and third points if necessary
    if (py2 < py1) 
    {
        // Swap px1, py1 with px2, py2
        int tmp_x = px1;
        int tmp_y = py1;
        px1 = px2;
        py1 = py2;
        px2 = tmp_x;
        py2 = tmp_y;
    }

    // now py0 <= py1 <= py2

    int y;
    for (y = py0; y <= py2; y++) 
    {
        int x_left, x_right;

        if (y < py1) 
        {
            // Left and right edges we need to interpolate
            if (py2 == py0) x_left = px0;
            else x_left = px0 + (px2 - px0) * (y - py0) / (py2 - py0);

            if (py1 == py0) x_right = px0;
            else x_right = px0 + (px1 - px0) * (y - py0) / (py1 - py0);
        }
        else 
        {
            // Left and right edges we need to interpolate
            if (py2 == py0) x_left = px0;
            else x_left = px0 + (px2 - px0) * (y - py0) / (py2 - py0);

            if (py2 == py1) x_right = px1;
            else x_right = px1 + (px2 - px1) * (y - py1) / (py2 - py1);
        }

        // x_left needs to be less then or equal to x_right
        if (x_left > x_right) 
        {
            int tmp = x_left;
            x_left = x_right;
            x_right = tmp;
        }

        // Draw the horizontal line from x_left to x_right at the current y
        int x;
        for (x = x_left; x <= x_right; x++) draw_pixel(img, x, y, c);
    }
}

void *new_offscreen_buffer() 
{
    // Multiply the screen height with line length
    screensize = vinfo.yres_virtual * finfo.line_length;
    
    // Map the contents of the file to a memory
    void* second_fb = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // mmap returns (void*)-1 on fail
    if (second_fb == (void*)-1)
    {
        // Log the error
        return NULL;
    }

    return second_fb;
}
void blit(void *src) 
{
    if (src == NULL)
    {
        // Log error
        return;
    }

    // For good sake cast everything to char to ensure byte by byte copying
    char* destination = (char*)fb_ptr;
    char* source = (char*)src;

    int i;
    for(i = 0; i < vinfo.yres_virtual * finfo.line_length; i++)
    {
        destination[i] = source[i];
    }
}