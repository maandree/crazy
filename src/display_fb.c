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


/**
 * Initialise the display system
 * 
 * @return  Zero on success, -1 on error
 */
static int display_fb_initialise(void)
{
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
  ssize_t got, i;
  size_t ptr = 0, size = 8 << 10, offset;
  char* old;
  int state = 0, comment = 0, type = 0;
  unsigned int maxval = 0;
  size_t width = 0, height = 0;
  char c;
  
  (void) crop_x;
  (void) crop_y;
  (void) crop_width;
  (void) crop_height;
  (void) split_x;
  
  if (fd < 0)
    goto reading_done;
  
  *image = malloc(size * sizeof(char));
  if (*image == NULL)
    return -1;
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
      
      for (i = 0; i < got; i++)
	if (state == 9)
	  {
	    if (c == '\n')
	      break;
	  }
	else if (c = (*image)[i], comment)
	  comment = c != '\n';
	else if (c == '#')
	  comment = 1;
	else if ((state == 0) && (c == 'P'))
	  state++;
	else if (((state == 1) || (state == 2)) && ('0' <= c) && (c <= '9'))
	  state = 2, type = type * 10 + (c & 15);
	else if (((state == 3) || (state == 4)) && ('0' <= c) && (c <= '9'))
	  state = 4, width = width * 10 + (c & 15);
	else if (((state == 5) || (state == 6)) && ('0' <= c) && (c <= '9'))
	  state = 6, height = height * 10 + (c & 15);
	else if (((state == 7) || (state == 8)) && ('0' <= c) && (c <= '9'))
	  state = 8, maxval = maxval * 10 + (c & 15);
	else if ((state % 2) == 0)
	  if ((++state == 9) && (c == '\n'))
	    break;
      
      ptr += (size_t)i;
      got -= i;
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
  
  for (i = 0, state = 0; (size_t)i < ptr; i++)
    if (state == 9)
      {
	if (c == '\n')
	  {
	    state = 10;
	    break;
	  }
      }
    else if (c = (*image)[i], comment)
      comment = c != '\n';
    else if (c == '#')
      comment = 1;
    else if ((state == 0) && (c == 'P'))
      state++;
    else if (((state == 1) || (state == 2)) && ('0' <= c) && (c <= '9'))
      state = 2, type = type * 10 + (c & 15);
    else if (((state == 3) || (state == 4)) && ('0' <= c) && (c <= '9'))
      state = 4, width = width * 10 + (c & 15);
    else if (((state == 5) || (state == 6)) && ('0' <= c) && (c <= '9'))
      state = 6, height = height * 10 + (c & 15);
    else if (((state == 7) || (state == 8)) && ('0' <= c) && (c <= '9'))
      state = 8, maxval = maxval * 10 + (c & 15);
    else if ((state % 2) == 0)
      if ((++state == 9) && (c == '\n'))
	{
	  state = 10;
	  break;
	}
  offset = (size_t)i;
  
  size = width * height * (maxval <= 0x100 ? 1 : 2) * (type == 6 ? 3 : 1);
  if (type == 4)
    size = (size + 7) / 8;
  
  /* TODO check that `(state == 10) && (ptr - offset >= size)` */
  
  old = *image;
  *image = realloc(*image, (size + offset) * sizeof(char));
  if (*image == NULL)
    {
      perror(execname);
      errno = 0;
      *image = old;
    }
  
  /* TODO display! */
  
  return 0;
 fail:
  return -1;
}


/**
 * Terminate the display system
 */
static void display_fb_terminate(void)
{
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

