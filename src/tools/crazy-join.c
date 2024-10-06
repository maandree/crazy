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
#include "common.h"
#include <dirent.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include <argparser.h>



/**
 * Buffer for pathnames
 */
static char buffer1[sizeof(".pnm") / sizeof(char) + 3 * sizeof(size_t)];

/**
 * Buffer for pathnames
 */
static char buffer2[sizeof(".pnm") / sizeof(char) + 3 * sizeof(size_t)];



/**
 * Perform join
 * 
 * @return  Zero on success, -1 on error
 */
static int perform_join(void)
{
  DIR* dir;
  struct dirent* file;
  size_t in, out = 1, n = 0;
  char* p;
  
  dir = opendir(".");
  t (dir == NULL);
  
  while ((file = readdir(dir)) != NULL)
    {
      in = (size_t)strtoumax(file->d_name, &p, 10);
      if (strcmp(p, ".pnm"))
	continue;
      sprintf(buffer1, "%zu.pnm", in);
      if ((strcmp(buffer1, file->d_name)) || (in == SIZE_MAX))
	t ((errno = ERANGE));
      n = n < in ? in : n;
    }
  
  for (in = 1, n++; in < n; in++)
    {
      sprintf(buffer1, "%zu.pnm", in);
      sprintf(buffer2, "%zu.pnm", out);
      if (access(buffer1, F_OK))
	continue;
      
      if (in == out++)
	continue;
      
      t (movefile(buffer1, buffer2));
    }
  
  closedir(dir);
  return 0;
 fail:
  if (dir)
    closedir(dir);
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
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Waggregate-return"
# pragma GCC diagnostic ignored "-Wcast-qual"
#endif
  
  int rc = 0;
  
  
  args_init((char*)"Remove gaps from removed file",
	    (char*)"crazy-join",
	    NULL, NULL, 1, 0, args_standard_abbreviations);
  
  
  args_add_option(args_new_argumentless(NULL, 0, (char*)"--help", NULL),
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
  
  
  t (perform_join());
  
  
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
  
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
}

