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
 * @return               Zero on success, -1 on error
 */
static int display_fb_display(int fd, pid_t pid, char** restrict image, size_t* restrict crop_x,
			      size_t* restrict crop_y, size_t* restrict crop_width,
			      size_t* restrict crop_height, size_t* restrict split_x)
{
  pid_t reaped;
  int status;
  (void) fd;
  (void) image;
  (void) crop_x;
  (void) crop_y;
  (void) crop_width;
  (void) crop_height;
  (void) split_x;
  
  
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
  
  return 0;
 fail:
  return 1;
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

