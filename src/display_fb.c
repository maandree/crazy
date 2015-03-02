/**
 * crazy — A crazy simple and usable scanning utility
 * Copyright © 2015  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "display_fb.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stropts.h>
#include <stdint.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include "crazy.h"
#include "images.h"


/**
 * The psuedodevice pathname used to access a framebuffer
 */
#ifndef FB_DEVICE
# define FB_DEVICE  "/dev/fb0"
#endif



/**
 * The width of the framebuffer
 */
static size_t fb_width;

/**
 * The height of the framebuffer
 */
static size_t fb_height;

/**
 * The file descriptor for the framebuffer
 */
static int fb_fd = -1;

/**
 * Framebuffer pointer
 */
static int8_t* fb_mem = MAP_FAILED;

/**
 * Increment for `mem` to move to next pixel on the line
 */
static size_t fb_bytes_per_pixel;

/**
 * Increment for `mem` to move down one line but stay in the same column
 */
static size_t fb_line_length;



/**
 * Terminate the display system
 */
static void display_fb_terminate(void)
{
  if (fb_fd >= 0)
    close(fb_fd), fb_fd = -1;
}


/**
 * Initialise the display system
 * 
 * @return  Zero on success, -1 on error
 */
static int display_fb_initialise(void)
{
  struct fb_fix_screeninfo fix_info;
  struct fb_var_screeninfo var_info;
  int fds[256];
  long ptr = 0;
  int saved_errno;
  
  fb_fd = open(FB_DEVICE, O_RDWR | O_CLOEXEC);
  if (fb_fd < 0)
    goto fail;
  while ((fb_fd == STDIN_FILENO) || (fb_fd == STDOUT_FILENO) || (fb_fd == STDERR_FILENO))
    {
      fds[ptr++] = fb_fd;
      fb_fd = dup(fb_fd);
      if (fb_fd < 0)
	goto fail;
    }
  while (ptr--)
    close(fds[ptr]);
  
  if (ioctl(fb_fd, (unsigned long int)FBIOGET_FSCREENINFO, &fix_info) ||
      ioctl(fb_fd, (unsigned long int)FBIOGET_VSCREENINFO, &var_info))
    goto fail;
  
  fb_mem = mmap(NULL, (size_t)(fix_info.smem_len), PROT_WRITE, MAP_PRIVATE, fb_fd, (off_t)0);
  if (fb_mem == MAP_FAILED)
    goto fail;
  
  fb_mem += var_info.xoffset * (var_info.bits_per_pixel / 8);
  fb_mem += var_info.yoffset * fix_info.line_length;
  
  fb_width           = var_info.xres;
  fb_height          = var_info.yres;
  fb_bytes_per_pixel = var_info.bits_per_pixel / 8;
  fb_line_length     = fix_info.line_length;
  
  return 0;
 fail:
  saved_errno = errno;
  while (ptr--)
    close(fds[ptr]);
  display_fb_terminate();
  errno = saved_errno;
  return -1;
}


/**
 * Draw an PNM image onto the framebuffer
 * 
 * @param  xoff       The where onto the framebuffer the top-left corner of the image is drawn on the X-axis
 * @param  yoff       The where onto the framebuffer the top-left corner of the image is drawn on the Y-axis
 * @param  width      The width of the image
 * @param  height     The height of the image
 * @param  maxval     The maximum value subpixel can have
 * @param  type       4: raw lineart, 5: raw greyscale, 6: raw colour
 * @param  pixeldata  Pixel data of the type described by `type`
 */
static void display_fb_draw_image(size_t xoff, size_t yoff, size_t width, size_t height,
				  int maxval, int type, const unsigned char* restrict pixeldata)
{
  int8_t* mem = fb_mem + yoff * fb_line_length + xoff * fb_bytes_per_pixel;
  size_t next_line = fb_line_length - width * fb_bytes_per_pixel;
  size_t i, x, y;
  uint32_t maxval_;
  
  if (maxval > 255)
    maxval >>= 7;
  maxval_ = (uint32_t)maxval;
  
  if (type == 4)
    for (i = y = 0; y < height; y++, mem += next_line)
      for (x = 0; x < width; x++, i++, mem += fb_bytes_per_pixel)
	*(uint32_t*)mem = (uint32_t)((pixeldata[x >> 3] & (1 << (i & 7))) ? ~0 : 0);
  else if ((type == 5) && (maxval == 255))
    for (y = 0; y < height; y++, mem += next_line)
      for (x = 0; x < width; x++, mem += fb_bytes_per_pixel)
	{
	  uint32_t colour = (uint32_t)*pixeldata++;
	  *(uint32_t*)mem = colour | (colour << 8) | (colour << 16);
	}
  else if ((type == 5) && (maxval < 255))
    for (y = 0; y < height; y++, mem += next_line)
      for (x = 0; x < width; x++, mem += fb_bytes_per_pixel)
	{
	  uint32_t colour = (uint32_t)*pixeldata++;
	  colour = 255 * colour / maxval_;
	  *(uint32_t*)mem = colour | (colour << 8) | (colour << 16);
	}
  else if (type == 5)
    for (y = 0; y < height; y++, mem += next_line)
      for (x = 0; x < width; x++, mem += fb_bytes_per_pixel)
	{
	  uint32_t colour = (uint32_t)*pixeldata++ << 1;
	  colour |= (uint32_t)(*pixeldata++) >> 7;
	  colour = 511 * colour / maxval_;
	  *(uint32_t*)mem = colour | (colour << 8) | (colour << 16);
	}
  else if ((type == 6) && (maxval == 255))
    for (y = 0; y < height; y++, mem += next_line)
      for (x = 0; x < width; x++, mem += fb_bytes_per_pixel)
	{
	  uint32_t colour = (uint32_t)*pixeldata++ << 16;
	  colour |= (uint32_t)*pixeldata++ << 8;
	  colour |= (uint32_t)*pixeldata++;
	  *(uint32_t*)mem = colour;
	}
  else if ((type == 6) && (maxval < 255))
    for (y = 0; y < height; y++, mem += next_line)
      for (x = 0; x < width; x++, mem += fb_bytes_per_pixel)
	{
	  uint32_t colour;
	  colour  = (255 * (uint32_t)*pixeldata++ / maxval_) << 16;
	  colour |= (255 * (uint32_t)*pixeldata++ / maxval_) <<  8;
	  colour |=  255 * (uint32_t)*pixeldata++ / maxval_;
	  *(uint32_t*)mem = colour;
	}
  else if (type == 6)
    for (y = 0; y < height; y++, mem += next_line)
      for (x = 0; x < width; x++, mem += fb_bytes_per_pixel)
	{
	  uint32_t colour_r, colour_g, colour_b;
	  colour_r  = (uint32_t)*pixeldata++ << 1;
	  colour_r |= (uint32_t)*pixeldata++ >> 7;
	  colour_g  = (uint32_t)*pixeldata++ << 1;
	  colour_g |= (uint32_t)*pixeldata++ >> 7;
	  colour_b  = (uint32_t)*pixeldata++ << 1;
	  colour_b |= (uint32_t)*pixeldata++ >> 7;
	  colour_r = 511 * colour_r / maxval_;
	  colour_g = 511 * colour_g / maxval_;
	  colour_b = 511 * colour_b / maxval_;
	  *(uint32_t*)mem = (colour_r << 16) | (colour_g << 8) | colour_b;
	}
}


/**
 * Display the scanning
 * 
 * @param   fd           The file descriptor for the image scanning, read from `*image` if negative
 * @param   pid          The PID of the process writting to `fd`
 * @param   image        Output parameter for the image buffer
 * @param   crop_x       Output parameter for the X-position of the top-left corner of the cropped image
 * @param   crop_y       Output parameter for the Y-position of the top-left corner of the cropped image
 * @param   crop_width   Output parameter for the width of the image after cropping, 0 if not cropped
 * @param   crop_height  Output parameter for the height of the image after cropping, 0 if not cropped
 * @param   split_x      Output parameter for where on the X-axis to split the cropped image, 0 if not splitted
 * @return               Zero on success, -1 on error, `errno` will be set appropriately (may be zero)
 */
static int display_fb_display(int fd, pid_t pid, char** restrict image, size_t* restrict crop_x,
			      size_t* restrict crop_y, size_t* restrict crop_width,
			      size_t* restrict crop_height, size_t* restrict split_x)
{
  pid_t reaped;
  int status, saved_errno;
  ssize_t got;
  size_t ptr = 0, size = 8 << 10, offset;
  char* old;
  int state, comment, type;
  unsigned int maxval = 0;
  size_t width, height;
  int resize_vertically;
  size_t display_width, display_height;
  char* scaled_image = NULL;
  
  (void) crop_x;
  (void) crop_y;
  (void) crop_width;
  (void) crop_height;
  (void) split_x;
  
  if (fd < 0)
    goto reading_done;
  
  pnm_init_parse_header(&state, &comment, &type, &maxval, &width, &height);
  *image = malloc(size * sizeof(char));
  if (*image == NULL)
    goto fail;
  for (;;)
    {
      if (size - ptr < 1024)
	{
	  old = *image;
	  *image = realloc(*image, size <<= 1);
	  if (*image == NULL)
	    {
	      *image = old;
	      goto fail;
	    }
	}
      got = read(fd, *image, size - ptr);
      if (got == 0)
	break;
      else if ((got < 0) && (errno == EINTR))
	continue;
      else if (got < 0)
	goto fail;
      
      offset = 0;
      if (state < 10)
	offset = pnm_parse_header(&state, &comment, &type, &maxval, &width, &height, (size_t)got, *image + ptr);
      if (state == 10)
	{
	  state++;
	  get_resize_dimensions(width, height, fb_width, fb_height, &display_width, &display_height);
	}
      
      ptr += offset;
      got -= (ssize_t)offset;
      if (got == 0)
	continue;
      
      /* TODO display! */
      
      ptr += (size_t)got;
    }
  
  for (;;)
    {
      reaped = wait(&status);
      if (reaped < 0)
	goto fail;
      else if (reaped == pid)
	{
	  if (status)
	    {
	      errno = 0;
	      goto fail;
	    }
	  break;
	}
    }
  
 reading_done:
  
  pnm_init_parse_header(&state, &comment, &type, &maxval, &width, &height);
  offset = pnm_parse_header(&state, &comment, &type, &maxval, &width, &height, ptr, *image);
  
  size = width * height * (maxval <= 0x100 ? 1 : 2) * (type == 6 ? 3 : 1);
  if (type == 4)
    size = (size + 7) / 8;
  
  if ((state < 10) || (ptr - offset < size) || (maxval < 1))
    goto incomplete_scan;
  
  old = *image;
  *image = realloc(*image, (size + offset) * sizeof(char));
  if (*image == NULL)
    {
      perror(execname);
      errno = 0;
      *image = old;
    }
  
  resize_vertically = get_resize_dimensions(width, height, fb_width, fb_height,
					    &display_width, &display_height);
  
  if (resize_image(display_width, display_height, resize_vertically,
		   *image, size + offset, &scaled_image, &ptr))
    goto fail;
  
  pnm_init_parse_header(&state, &comment, &type, &maxval, &display_width, &height);
  offset = pnm_parse_header(&state, &comment, &type, &maxval, &display_width, &height, ptr, scaled_image);
  
  size = display_width * display_height * (maxval <= 0x100 ? 1 : 2) * (type == 6 ? 3 : 1);
  if (type == 4)
    size = (size + 7) / 8;
  
  if ((state < 10) || (ptr - offset < size) || (maxval < 1))
    goto incomplete_scan;
  
  old = scaled_image;
  scaled_image = realloc(scaled_image, (size + offset) * sizeof(char));
  if (scaled_image == NULL)
    {
      perror(execname);
      errno = 0;
      scaled_image = old;
    }
  
  /* TODO display! */
  
  free(scaled_image);
  return 0;
 incomplete_scan:
  fprintf(stderr, "%s: scan failed, image incomplete", execname);
  errno = 0;
 fail:
  saved_errno = errno;
  free(scaled_image);
  errno = saved_errno;
  return -1;
}


/**
 * Get the functions associated with the framebuffer display system
 * 
 * @param  display  Output parameter the functions
 */
void display_fb_get(display_t* restrict display)
{
  display->initialise = display_fb_initialise;
  display->display    = display_fb_display;
  display->terminate  = display_fb_terminate;
}

