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
#ifndef CRAZY_UTIL_H
#define CRAZY_UTIL_H


#include <unistd.h>



/**
 * Create a subprocess and read it's stdout
 * 
 * @param   file  The filename of the command to start
 * @param   argv  The command line arguments for the new process
 * @param   pid   Output parameter for the new process's PID
 * @return        The file descriptor of the new process's stdout, -1 on error
 */
int subprocess_rd(const char* file, const char* const argv[], pid_t* pid);

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

