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
#include <stddef.h>
#include <alloca.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include <argparser.h>



/**
 * Buffer for pathnames
 */
static char buffer[sizeof(".pnm") / sizeof(char) + 3 * sizeof(size_t)];



#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Waggregate-return"
# pragma GCC diagnostic ignored "-Wcast-qual"
#endif


/**
 * Perform compilation
 * 
 * @return  Zero on success, -1 on error
 */
static int perform_compile(void)
{
  size_t i, n, len = 0;
  char** command;
  char* buf;
  ssize_t arglen;
  int saved_errno
  
  for (n = 1;; n++)
    {
      sprintf(buffer, "%zu.pnm", n);
      if (access(buffer, F_OK))
	break;
      len += strlen(buffer) + 1;
    }
  
  command = malloc((4 + n) * sizeof(char*));
  arg = buf = malloc(len * sizeof(char));
  
  command[0] = "gm";
  command[1] = "convert";
  command[2] = "-adjoin";
  for (i = 1; i < n; i++)
    {
      command[i + 2] = arg;
      sprintf(arg, "%zu.pnm%zn", i, &arglen);
      arg += (size_t)arglen + 1;
    }
  command[n + 2] = "pdf:-";
  command[n + 3] = NULL;
  
  execvp(*command, command);
  saved_errno = errno;
  free(command);
  free(buf);
  errno = saved_errno;
  return -1;
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
  int rc = 0, dispose = 1;
  
  
  args_init((char*)"Compile a multipage-document from a directory of images",
	    (char*)"crazy-compile > <output-file>",
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
  if (args_unrecognised_count || args_files_count)
    goto invalid_opts;
  
  
  args_dispose(), dispose = 0;
  
  if (perform_compile())
    goto fail;
  
  
 exit:
  if (dispose)
    args_dispose();
  return rc;
 invalid_opts:
  args_help();
 fail:
  rc = 1;
  goto exit;
}

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

