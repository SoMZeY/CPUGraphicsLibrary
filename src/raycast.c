// raycast.c

// Used this website to learn and create this https://lodev.org/cgtutor/raycasting.html

#include "graphics.h"

// For timespec structs 
#include <time.h>
// For sin and cos
#include <math.h>

// Define the map and screensize
#define mapWidth 24
#define mapHeight 24
#define screenWidth 640
#define screenHeight 480

// Our map. 0 is walkable; i > 0 is not
int worldMap[mapWidth][mapHeight]=
{
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Helper function to dim the color
color_t darken(color_t color) 
{
    // ================================================================================
    // Extract RGB bits
    // Shift the color right 16-5(11) times since the first 5 bits are red
    // And then and mask the 5 right most bits to remove all the other colors bits
    // ================================================================================
    unsigned short r = (color >> 11) & 0x1F;
    unsigned short g = (color >> 5)  & 0x3F;
    unsigned short b = color         & 0x1F;

    // Darken by dividing by 2 using shift
    r = r >> 1;
    g = g >> 1;
    b = b >> 1;

    // Reassemble color
    return RGB(r, g, b);
}

int main()
{
    // Position of the player
    double posX = 22, posY = 12;
    // Direction of the player
    double dirX = -1, dirY = 0;

    // Camera plane perpendicular to the direction vector
    double planeX = 0, planeY = 0.66;

    struct timespec start, now;

    // Initialize the framebuffer
    init_graphics();

    // Create a second offscreen buffer
    void *buffer = new_offscreen_buffer();
    if (!buffer) 
    {
        exit_graphics();
        return 1;
    }

    // Clear
    clear_screen(buffer);
    
    double w = screenWidth;

    // Start the timer
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Game loop
    while(1)
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        // Calculate how many seconds passed since last frame
        double frameTime = (now.tv_sec - start.tv_sec)
                     + (now.tv_nsec - start.tv_nsec) / 1.0e9;

        // Update start for the next iteration
        start = now;

        // So your move/turn speed won't be zero:
        double moveSpeed = frameTime * 10.0;
        double rotSpeed  = frameTime * 8.0;
        int x;

        // Loop per each vertical line of the screen
        for (x = 0; x < w; x ++)
        {
            // Calculate ray position and direction
            // Normalizes each point along the camera axis between [-1, 1] from [0, 640]
            double cameraX = (2 * x) / w - 1;  

            // RayDirx will stay constant initially until the direction moves
            // Initially RayDirY is the one that fans the rays 
            double rayDirX = dirX + planeX * cameraX;
            double rayDirY = dirY + planeY * cameraX;

            // Which grid cell of the map we're in
            int mapX = (int)(posX);
            int mapY = (int)(posY);

            // Length of ray from current position to next x or y-side
            double sideDistX;
            double sideDistY;

            // ===================================================================================
            // Length of ray from one x or y-side to next x or y-side
            // 1e30 is meant to control perfectly horizontal/vertical rays, in which it will
            // help us skip over all the gridlines, and helps us avoid the division by zero
            // Also need to make value absolute so check for the negative numbers
            // ===================================================================================
            double deltaDistX = (rayDirX == 0) ? 1e30 : (1 / rayDirX);
            if (deltaDistX < 0) deltaDistX = -deltaDistX;
            double deltaDistY = (rayDirY == 0) ? 1e30 : (1 / rayDirY);
            if (deltaDistY < 0) deltaDistY = -deltaDistY;

            // Variables; Will be used later to calculate the length of the ray
            double perpWallDist;
            // What direction to step in x or y-direction (either +1 or -1)
            int stepX;
            int stepY;

            // If there was a wall hit
            int hit = 0;
            // What side North/South or West/East
            int side;

            // Figure out the step and initial step distance
            if (rayDirX < 0)
            {
                stepX = -1;
                sideDistX = (posX - mapX) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (mapX + 1.0 - posX) * deltaDistX;
            }
            if (rayDirY < 0)
            {
                stepY = -1;
                sideDistY = (posY - mapY) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (mapY + 1.0 - posY) * deltaDistY;
            }

            // Actual DDA now 
            while (hit == 0)
            {
                // Jump to next map square, either in x-direction, or in y-direction
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                }
                else
                {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }

                //Check if ray has hit a wall
                if (worldMap[mapX][mapY] > 0) hit = 1;
            }
            
            // ============================================================================
            // Now we need to calculate the distance to the wall
            // to avoid fish eye effect we need to get the distance
            // from the camera place instead of the position of the player
            // But to avoid being "inside the wall" we need to subtract the deltaDist
            // ============================================================================
            if(side == 0) perpWallDist = (sideDistX - deltaDistX);
            else perpWallDist = (sideDistY - deltaDistY);

            // Calculate height of line to draw on screen
            int lineHeight = (int)(screenHeight / perpWallDist);

            // calculate lowest and highest pixel to fill in current stripe
            int drawStart = -lineHeight / 2 + screenHeight / 2;
            if(drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + screenHeight / 2;
            if(drawEnd >= screenHeight) drawEnd = screenHeight - 1;

            // Choose wall color based on the number that was hit
            color_t color;
            switch(worldMap[mapX][mapY])
            {
                // 1 red
                case 1:  color = RGB(255, 0, 0); break;
                // 2 green
                case 2:  color = RGB(0, 255, 0); break;
                // 3 blue
                case 3:  color = RGB(0, 0, 255); break;
                // 4 white
                case 4:  color = RGB(255, 255, 255); break;
                // non yellow
                default: color = RGB(255, 255, 0); break;
            }

            //give x and y sides different brightness
            if (side == 1) color = darken(color);

            //draw the pixels of the stripe as a vertical line
            draw_line(buffer, x, drawStart, x, drawEnd, color);
        }

        blit(buffer);
        clear_screen(buffer);

        char keypressed = getkey();

        // Move forward if no wall in front of you
        if (keypressed == 'w')
        {
            if(worldMap[(int)(posX + dirX * moveSpeed)][(int)(posY)] == 0) posX += dirX * moveSpeed;
            if(worldMap[(int)(posX)][(int)(posY + dirY * moveSpeed)] == 0) posY += dirY * moveSpeed;
        }
        // Move backwards if no wall behind you
        if (keypressed == 's')
        {
            if(worldMap[(int)(posX - dirX * moveSpeed)][(int)(posY)] == 0) posX -= dirX * moveSpeed;
            if(worldMap[(int)(posX)][(int)(posY - dirY * moveSpeed)] == 0) posY -= dirY * moveSpeed;
        }
        // Rotate to the right
        if (keypressed == 'd')
        {
            // Both camera direction and camera plane must be rotated
            double oldDirX = dirX;
            dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
            dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
            double oldPlaneX = planeX;
            planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
            planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
        }
        //rotate to the left
        if (keypressed == 'a')
        {
            // Both camera direction and camera plane must be rotated
            double oldDirX = dirX;
            dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
            dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
            double oldPlaneX = planeX;
            planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
            planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
        }
        // Break out of main loop
        if (keypressed == 'q') 
        {
            exit_graphics();
            break;
        }
    }
}