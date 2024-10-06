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
#include "util.h"
#include "crazy.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



/**
 * Create a subprocess and read it's stdout
 * 
 * @param   file    The filename of the command to start
 * @param   argv    The command line arguments for the new process
 * @param   lang_c  Whether to set $LANG to "C"
 * @param   pid     Output parameter for the new process's PID
 * @return          The file descriptor of the new process's stdout, -1 on error
 */
int subprocess_rd(const char* file, const char* const argv[], int lang_c, pid_t* pid)
{
  int pipe_rw[2];
  
  if (pipe(pipe_rw) < 0)
    {
      perror(execname);
      errno = 0;
      return -1;
    }
  
  *pid = fork();
  if (*pid < 0)
    {
      perror(execname);
      close(pipe_rw[0]);
      close(pipe_rw[1]);
      errno = 0;
      return -1;
    }
  else if (*pid == 0)
    {
      if (lang_c)
	setenv("LANG", "C", 1);
      if (pipe_rw[1] != STDOUT_FILENO)
	{
	  close(STDOUT_FILENO);
	  dup2(pipe_rw[1], STDOUT_FILENO);
	  close(pipe_rw[1]);
	}
      close(pipe_rw[0]);
      
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-qual"
#endif
      execvp(file, (char* const*)argv);
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
      perror(execname);
      exit(1);
    }
  
  close(pipe_rw[1]);
  return pipe_rw[0];
}


/**
 * Get the next line from a file
 * 
 * @param   fd    The file descriptor
 * @param   line  Pointer to the buffer for the line
 * @param   size  Pointer to the size of `*line`
 * @return        The length of the line, `-1` on error, `0` on end of file
 */
ssize_t fd_getline(int fd, char** line, size_t* size)
{
  static char buf[128];
  static size_t saved = 0;
  size_t ptr = 0;
  ssize_t got;
  char *p;
  
  for (;;)
    {
      if (saved > 0)
	{
	  got = (ssize_t)saved;
	  saved = 0;
	}
      else
	{
	  got = read(fd, buf, sizeof(buf));
	  t (got < 0);
	  if (got == 0)
	    break;
	}
      
      if (ptr + (size_t)got > *size)
	{
	  void* new = realloc(*line, ptr + (size_t)got);
	  t (new == NULL);
	  *line = new;
	  *size = ptr + (size_t)got;
	}
      
      p = memchr(buf, '\n', (size_t)got);
      if (p == NULL)
	{
	  memcpy(*line + ptr, buf, (size_t)got);
	  ptr += (size_t)got;
	}
      else
	{
	  p += 1;
	  saved = (size_t)got - (size_t)(p - buf);
	  memcpy(*line + ptr, buf, (size_t)(p - buf));
	  memmove(buf, p, saved);
	  ptr += (size_t)(p - buf);
	  break;
	}
    }
  
  if (ptr + (size_t)got >= *size)
    {
      void* new = realloc(*line, ptr + (size_t)got + 1);
      t (new == NULL);
      *line = new;
      *size = ptr + (size_t)got;
    }
  
  (*line)[ptr] = '\0';
  return (ssize_t)ptr;
  
 fail:
  perror(execname);
  errno = 0;
  return -1;
}

