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
#include "common.h"
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/wait.h>

#include <argparser.h>



/**
 * Buffer for pathnames
 */
static char buffer1[sizeof(".temp.pnm") / sizeof(char) + 3 * sizeof(size_t)];

/**
 * Buffer for pathnames
 */
static char buffer2[sizeof(".temp.pnm") / sizeof(char) + 3 * sizeof(size_t)];




#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Waggregate-return"
# pragma GCC diagnostic ignored "-Wcast-qual"
#endif

/**
 * Perform rotation
 * 
 * @param   first  The first image
 * @param   diff   The index difference between successive images
 * @param   end    The image after the last image
 * @return         Zero on success, -1 on error
 */
static int perform_rotate(size_t first, size_t diff, size_t end)
{
  char* command[] = { (char*)"gm", (char*)"convert", buffer1,
		      (char*)"-flip", (char*)"-flop", buffer2, NULL };
  size_t i;
  pid_t pid;
  int status;
  
  for (i = first; i < end; i += diff)
    {
      sprintf(buffer1, "%zu.pnm", i);
      sprintf(buffer2, "%zu.temp.pnm", i);
      if (access(buffer1, F_OK))
	break;
      
      pid = fork();
      if (pid < 0)
	goto fail;
      
      if (pid == 0)
	execvp(*command, command), perror(*command), exit(1);
      
    rewait:
      if (waitpid(pid, &status, 0) < 0)
	{
	  if (errno == EINTR)
	    goto rewait;
	  goto fail;
	}
      if (status)
	return errno = 0, -1;
      
      if (unlink(buffer1) || movefile(buffer2, buffer1))
	goto fail;
    }
  
  return 0;
 fail:
  return -1;
}


/**
 * Convert a `char*` to `a size_t`
 * 
 * @param   str  The `char*`
 * @return       The `size_t`
 */
static size_t parse_size(const char* str)
{
  char buf[3 * sizeof(size_t) + 2];
  size_t rc;
  
  rc = (size_t)strtoumax(str, NULL, 10);
  if (sprintf(buf, "%zu", rc), strcmp(buf, str))
    return 0;
  
  return rc;
}


/**
 * Everything begins "here"
 * 
 * @param   argc  The number of command line arguments
 * @param   argv  Command line arguments
 * @return        Zero on and only on success
 */
int main(int argc, char* argv[])
{
  int rc = 0;
  size_t first, diff, end;
  
  
  args_init((char*)"Rotate images in a pattern",
	    (char*)"crazy-rotate [--] <first> <gaps+1> [<last>]",
	    NULL, NULL, 1, 0, args_standard_abbreviations);
  
  
  args_add_option(args_new_argumentless(NULL, 1, (char*)"--help", NULL),
		  (char*)"Prints this help message");
  
  
  args_parse(argc, argv);
  args_support_alternatives();
  
  
  if (args_opts_used((char*)"--help"))
    {
      args_help();
      goto exit;
    }
  if (args_unrecognised_count || (args_files_count < 2) || (args_files_count > 3))
    goto invalid_opts;
  
  
  first = parse_size(args_files[0]);
  diff  = parse_size(args_files[1]);
  end   = args_files_count == 3 ? parse_size(args_files[2]) : (SIZE_MAX - 1);
  
  if (!first || !diff || !end)
    goto invalid_opts;
  if (end++ == SIZE_MAX)
    {
      errno = ERANGE;
      goto fail;
    }
  
  if (perform_rotate(first, diff, end))
    goto fail;
  
  
 exit:
  args_dispose();
  return rc;
 invalid_opts:
  args_help();
 fail:
  if (errno)
    perror(*argv);
  rc = 1;
  goto exit;
}

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

