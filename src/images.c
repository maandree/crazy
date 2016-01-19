/**
 * crazy — A crazy simple and usable scanning utility
 * Copyright © 2015, 2016  Mattias Andrée (maandree@member.fsf.org)
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
#include "images.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "crazy.h"


/**
 * Initialise the state for the PNM header-parsing
 * 
 * @param  state    The major state of the parser, 10 when done
 * @param  comment  Whether the parser is in a comment section
 * @param  type     The PNM type: 4 for raw lineart, 5 for raw greyscale 6 for raw RGB
 * @param  maxval   The maximum value on a subpixel
 * @param  width    The width of the image, in pixels
 * @param  height   The height of the image, in pixels
 */
void pnm_init_parse_header(int* restrict state, int* restrict comment, int* restrict type,
			   unsigned int* restrict maxval, size_t* restrict width, size_t* restrict height)
{
  *state = *comment = *type = 0;
  *maxval = 0;
  *width = *height = 0;
}


/**
 * Parse the header of a PNM file
 * 
 * @param   state    The major state of the parser, 10 when done
 * @param   comment  Whether the parser is in a comment section
 * @param   type     The PNM type: 4 for raw lineart, 5 for raw greyscale 6 for raw RGB
 * @param   maxval   The maximum value on a subpixel
 * @param   width    The width of the image, in pixels
 * @param   height   The height of the image, in pixels
 * @param   size     The bytes ready to be read from `image`
 * @param   image    The image
 * @return           The number of bytes read from `image`
 */
size_t pnm_parse_header(int* restrict state, int* restrict comment, int* restrict type,
			unsigned int* restrict maxval, size_t* restrict width, size_t* restrict height,
			size_t size, const char* image)
{
#define X(S)  ((*state == S) || (*state == S + 1)) && ('0' <= c) && (c <= '9')
  
  size_t i;
  char c;
  
  for (i = 0; i < size; i++)
    if (c = image[i], *state == 9)
      {
	if (c == '\n')
	  return *state = 10, i;
      }
    else if (comment)                      *comment = c != '\n';
    else if (c == '#')                     *comment = 1;
    else if ((*state == 0) && (c == 'P'))  ++*state;
    else if (X(1))                         *state = 2, *type   = *type   * 10 + (c & 15);
    else if (X(3))                         *state = 4, *width  = *width  * 10 + (c & 15);
    else if (X(5))                         *state = 6, *height = *height * 10 + (c & 15);
    else if (X(7))                         *state = 8, *maxval = *maxval * 10 + (c & 15);
    else if ((*state % 2) == 0)
      {
	++*state;
	if ((*state == 7) && (*type == 4))  *state = 9, *maxval = 1;
	if ((*state == 9) && (c == '\n'))   return *state = 10, i;
      }
  
  return i;
  
#undef X
}


/**
 * Get the maximum size to which an image can be scaled up
 * 
 * @param   img_width   The original width of the image
 * @param   img_height  The original height of the image
 * @param   max_width   The width of the display area
 * @param   max_height  The heiht of the display area
 * @param   new_width   Output parameter for the maximum width to which the image can be scaled up
 * @param   new_height  Output parameter for the maximum height to which the image can be scaled up
 * @return              Whether `*new_height == max_height`, otherwise `*new_width == max_width`
 */
int get_resize_dimensions(size_t img_width, size_t img_height, size_t max_width, size_t max_height,
			  size_t* restrict new_width, size_t* restrict new_height)
{
  *new_width = max_height * img_width / img_height;
  (*new_height = max_height);
  if (*new_width <= max_width)
    return 1;
  
  *new_height = max_width * img_width / img_height;
  *new_width = max_width;
  return 0;
}


/**
 * Resize an image
 * 
 * @param   width              The new width of the image
 * @param   height             The new height of the image
 * @param   resize_vertically  The return value of `get_resize_dimensions`
 * @param   image              The image to resize
 * @param   image_size         The number of bytes stored in `image`
 * @param   scaled             Output parameter for the resized image
 * @param   scaled_size        Output parameter for the number of bytes stored in `scaled`
 * @return                     Zero on success, -1 on error
 */
int resize_image(size_t width, size_t height, int resize_vertically, const char* image,
		 size_t image_size, char** restrict scaled, size_t* restrict scaled_size)
{
  char scale[3 * sizeof(size_t) + 2];
  pid_t pid_1 = -1, pid_2 = -1, reaped;
  int in_rw[2];
  int out_rw[2];
  int status, reap_count = 0;
  size_t ptr = 0, size = 8 << 10;
  ssize_t got;
  char* old;
  
  *scaled = NULL;
  *scaled_size = 0;
  
  if (resize_vertically)  sprintf(scale, "x%zu", height);
  else                    sprintf(scale, "%zux", width);
  
  t (pipe(in_rw));
  t (pipe(out_rw));
  
  pid_1 = fork();
  t (pid_1 < 0);
  
  if (pid_1 == 0)
    {
      close(in_rw[1]), in_rw[1] = -1;
      close(out_rw[0]), out_rw[0] = -1;
      if (STDIN_FILENO != in_rw[0])
	{
	  if (STDIN_FILENO == out_rw[1])
	    {
	      int fd = dup(out_rw[1]);
	      close(out_rw[1]);
	      out_rw[1] = fd;
	    }
	  else
	    close(STDIN_FILENO);
	  dup2(in_rw[0], STDIN_FILENO);
	  close(in_rw[0]), in_rw[0] = -1;
	}
      if (STDOUT_FILENO != out_rw[1])
	{
	  close(STDOUT_FILENO);
	  dup2(out_rw[1], STDOUT_FILENO);
	  close(out_rw[1]), out_rw[1] = -1;
	}
      
      execlp("gm", "gm", "convert", "-", "-format", "pnm", "-resize", scale, "-filter", "Lanczos", "-", NULL);
      goto fail;
    }
  
  close(in_rw[0]), in_rw[0] = -1;
  close(out_rw[1]), out_rw[1] = -1;
  
  pid_2 = fork();
  t (pid_2 < 0);
  
  if (pid_2 == 0)
    {
      close(out_rw[0]), out_rw[0] = -1;
      
      while (ptr < image_size)
	{
	  got = write(in_rw[1], image + ptr, image_size - ptr);
	  t (got < 0);
	  ptr += (size_t)got;
	}
      
      close(in_rw[1]);
      exit(0);
    }
  
  close(in_rw[1]), in_rw[1] = -1;
  
  *scaled = malloc(size);
  t (*scaled == NULL);
  for (;;)
    {
      if (size - ptr < 1024)
	{
	  old = *scaled;
	  *scaled = realloc(*scaled, size <<= 1);
	  if (*scaled == NULL)
	    {
	      *scaled = old;
	      goto fail;
	    }
	}
      got = read(out_rw[0], *scaled, size - ptr);
      if (got == 0)
	break;
      if (got < 0)
	{
	  t (errno != EINTR);
	  continue;
	}
      
      ptr += (size_t)got;
    }
  *scaled_size = ptr;
  
  for (;;)
    if (reaped = wait(&status), reaped < 0)
      goto fail;
    else if (reaped == pid_1)
      {
	if (status)
	  {
	    errno = 0;
	    goto fail;
	  }
	if (++reap_count == 2)
	  break;
      }
    else if (reaped == pid_2)
      {
	if (status)
	  {
	    errno = 0;
	    goto fail;
	  }
	if (++reap_count == 2)
	  break;
      }
  
  close(out_rw[0]);
  return 0;
 fail:
  if ((pid_1 == 0) && (pid_2 == 0))
    perror(execname);
  if (in_rw[0] >= 0)   close(in_rw[0]);
  if (in_rw[1] >= 0)   close(in_rw[1]);
  if (out_rw[0] >= 0)  close(out_rw[0]);
  if (out_rw[1] >= 0)  close(out_rw[1]);
  free(*scaled), *scaled = NULL;
  *scaled_size = 0;
  if ((pid_1 == 0) && (pid_2 == 0))
    exit(1);
  return -1;
}

