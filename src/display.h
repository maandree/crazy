/**
 * crazy — A crazy simple and usable scanning utility
 * Copyright © 2015, 2016  Mattias Andrée (m@maandree.se)
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
#ifndef CRAZY_DISPLAY_H
#define CRAZY_DISPLAY_H


#include <stddef.h>
#include <sys/types.h>


/**
 * Function collection for image display systems
 */
typedef struct display
{
  /**
   * Initialise the display system
   * 
   * @return  Zero on success, -1 on error
   */
  int (*initialise)(void);
  
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
  int (*display)(int fd, pid_t pid, char** restrict image, size_t* restrict crop_x, size_t* restrict crop_y,
		 size_t* restrict crop_width, size_t* restrict crop_height, size_t* restrict split_x);
  
  /**
   * Terminate the display system
   */
  void (*terminate)(void);
  
} display_t;


#endif

