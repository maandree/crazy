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
#ifndef CRAZY_UTIL_H
#define CRAZY_UTIL_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



/**
 * Allocate and format a string
 * 
 * @param  bufp    Output pointer to the string
 * @param  format  The format string
 * @param  ...     Format arguments
 */
#define aprintf(bufp, format, ...)				\
  do								\
    {								\
      size_t n;							\
      snprintf(NULL, 0, format "%zn", __VA_ARGS__, &n);		\
      *(bufp) = malloc(((size_t)n + 1) * sizeof(char*));	\
      if (*(bufp) != NULL)					\
	sprintf(*(bufp), format, __VA_ARGS__);			\
    }								\
  while (0)


/**
 * Create a subprocess and read it's stdout
 * 
 * @param   file    The filename of the command to start
 * @param   argv    The command line arguments for the new process
 * @param   lang_c  Whether to set $LANG to "C"
 * @param   pid     Output parameter for the new process's PID
 * @return          The file descriptor of the new process's stdout, -1 on error
 */
int subprocess_rd(const char* file, const char* const argv[], int lang_c, pid_t* pid);

/**
 * Get the next line from a file
 * 
 * @param   fd    The file descriptor
 * @param   line  Pointer to the buffer for the line
 * @param   size  Pointer to the size of `*line`
 * @return        The length of the line, `-1` on error, `0` on end of file
 */
ssize_t fd_getline(int fd, char** line, size_t* size);



#endif

