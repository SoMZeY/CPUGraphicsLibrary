# TinyCore-FB Graphics + Raycaster (C)
### What
A tiny **framebuffer graphics library** in C (double-buffered) plus a Wolfenstein-style raycaster.
* **Graphics lib**: pure Linux syscalls (open/ioctl/mmap/select/read/nanosleep) — no graphics libraries.
* **Demo**: uses only minimal time.h/math.h (linked with -lm -lrt) for timing & sin/cos.
### Why
Educational project to learn low-level rendering and OS interfaces. **Goal**: run on **Tiny Core Linux** in a **Raspberry Pi** console.

# Demo (Screens / GIF)
![Demo GIF](media/raycastGIF.gif)

# Features
* **Double buffering**: offscreen buffer + blit()
* **Primitives**: draw_pixel, Bresenham draw_line, scanline fill_triangle
* **Input**: non-blocking keyboard via select()
* **Raycaster**: classic DDA with side-based shading (W/A/S/D, Q to quit)

# Requirements
>⚠️ Compatibility Warning
This project has been tested only on Tiny Core Linux (console, using /dev/fb0 in 16-bpp RGB565).
It has not been tested on other distros, desktop TTYs, or Raspberry Pi OS.
Running elsewhere is experimental and may fail if /dev/fb0 is missing or not 16-bpp.

* Linux framebuffer device: `/dev/fb0`
* 16-bpp (RGB565) mode (this build assumes RGB565)
* Run from a native console (no desktop compositor)
* Needs `sudo` to access `/dev/fb0`

# Build
```
gcc -o myprogram library.c raycast.c -lm -lrt
```
> -lm for sin/cos; -lrt for timing.

# Run
```
sudo ./myprogram
# Controls: W/A/S/D move/rotate, Q quit
```

# Notes / Limits
* Designed around **Tiny Core Linux** console; **no Wayland/X** support.
* If your framebuffer is 24/32-bpp, colors/pixels will be wrong (current code packs RGB565).
* Restores terminal settings on exit.

## License
This project is licensed under the [MIT License](./LICENSE).
