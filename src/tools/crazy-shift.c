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
 * Perform shift
 * 
 * @param   first  The first image to shift
 * @param   shift  The size of the gap
 * @return         Zero on success, -1 on error
 */
static int perform_shift(size_t first, size_t shift)
{
  size_t i = first;
  
  for (;; i++)
    {
      sprintf(buffer1, "%zu.pnm", i);
      if (access(buffer1, F_OK))
	break;
    }
  
  while (i-- > first)
    {
      sprintf(buffer1, "%zu.pnm", i);
      sprintf(buffer2, "%zu.pnm", i + shift);
      if (movefile(buffer1, buffer2))
	goto fail;
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
  char* s_first;
  char* s_shift;
  size_t first, shift;
  char buf[3 * sizeof(size_t) + 2];
  
  
  args_init((char*)"Create a gap between images",
	    (char*)"crazy-shift [--] <first> <shift>",
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
  if (args_unrecognised_count || (args_files_count != 2))
    goto invalid_opts;
  
  
  first = (size_t)strtoumax(s_first = args_files[0], NULL, 10);
  shift = (size_t)strtoumax(s_shift = args_files[1], NULL, 10);
  
  if (sprintf(buf, "%zu", first), strcmp(buf, s_first))  goto invalid_opts;
  if (sprintf(buf, "%zu", shift), strcmp(buf, s_shift))  goto invalid_opts;
  if (shift == 0)                                        goto invalid_opts;
  
  
  if (perform_shift(first, shift))
    goto fail;
  
  
 exit:
  args_dispose();
  return rc;
 invalid_opts:
  args_help();
 fail:
  rc = 1;
  goto exit;
  
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
}

