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
#include <sys/wait.h>

#include "crazy.h"
#include "images.h"



/**
 * The width of the frame buffer
 */
static size_t fb_width;

/**
 * The height of the frame buffer
 */
static size_t fb_height;



/**
 * Initialise the display system
 * 
 * @return  Zero on success, -1 on error
 */
static int display_fb_initialise(void)
{
  /* TODO display_fb_initialise */
  
  return 0;
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
  int status;
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
  
  /* TODO fail if ((state < 10) || (ptr - offset < size) || (maxval < 1)) */
  
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
  
  if (resize_image(display_width, display_height, resize_vertically, *image, size + offset, &scaled_image))
    goto fail;
  
  pnm_init_parse_header(&state, &comment, &type, &maxval, &width, &height);
  offset = pnm_parse_header(&state, &comment, &type, &maxval, &width, &height, ptr, scaled_image);
  
  size = width * height * (maxval <= 0x100 ? 1 : 2) * (type == 6 ? 3 : 1);
  if (type == 4)
    size = (size + 7) / 8;
  
  /* TODO fail if ((state < 10) || (ptr - offset < size) || (maxval < 1)) */
  
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
 fail:
  free(scaled_image);
  return -1;
}


/**
 * Terminate the display system
 */
static void display_fb_terminate(void)
{
  /* TODO display_fb_terminate */
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

