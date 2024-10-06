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
#ifndef CRAZY_IMAGES_H
#define CRAZY_IMAGES_H


#include <stddef.h>


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
			   unsigned int* restrict maxval, size_t* restrict width, size_t* restrict height);


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
			size_t size, const char* image);


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
			  size_t* restrict new_width, size_t* restrict new_height);


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
		 size_t image_size, char** restrict scaled, size_t* restrict scaled_size);


#endif

