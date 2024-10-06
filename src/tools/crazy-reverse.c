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
#include <errno.h>

#include <argparser.h>



/**
 * Buffer for pathnames
 */
static char buffer1[sizeof(".temp.pnm") / sizeof(char) + 3 * sizeof(size_t)];

/**
 * Buffer for pathnames
 */
static char buffer2[sizeof(".temp.pnm") / sizeof(char) + 3 * sizeof(size_t)];



/**
 * Perform reverse
 * 
 * @return  Zero on success, -1 on error
 */
static int perform_reverse(void)
{
  size_t i, n;
  
  for (n = 1;; n++)
    {
      sprintf(buffer1, "%zu.pnm", n);
      sprintf(buffer2, "%zu.temp.pnm", n);
      if (access(buffer1, F_OK))
	break;
      t (movefile(buffer1, buffer2));
    }
  
  for (i = 1; i < n; i++)
    {
      sprintf(buffer1, "%zu.temp.pnm", i);
      sprintf(buffer2, "%zu.pnm", n - i);
      t (movefile(buffer1, buffer2));
    }
  
  return 0;
 fail:
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
  
  
  args_init((char*)"Reverse the order of files",
	    (char*)"crazy-reverse",
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
  
  
  t (perform_reverse());
  
  
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

