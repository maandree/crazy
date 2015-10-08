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
#include <errno.h>

#include <argparser.h>



/**
 * Copy files
 */
#define MODE_COPY  0

/**
 * Symbolically link files
 */
#define MODE_SYMLINK  1

/**
 * Link files
 */
#define MODE_LINK  2

/**
 * Move files
 */
#define MODE_MOVE  3



/**
 * Buffer for pathnames
 */
static char* buffer1;

/**
 * Buffer for pathnames
 */
static char* buffer2;



/**
 * Information for a split
 */
struct split
{
  /**
   * The first image
   */
  size_t first;
  
  /**
   * The index difference between successive images
   */
  size_t diff;
  
  /**
   * The image after the last image
   */
  size_t end;
  
  /**
   * The output directory
   */
  char* dir;
};



/**
 * Perform split
 * 
 * @param   splits  Information about the splits
 * @param   count   The number of splits
 * @param   mode    `MODE_COPY`, `MODE_SYMLINK`, `MODE_LINK`, or `MODE_MOVE`
 * @return          Zero on success, -1 on error
 */
static int perform_split(struct split* splits, size_t count, int mode)
{
  struct split s;
  size_t in, out;
  
  while (count--)
    {
      s = splits[count];
      out = 0;
      for (in = s.first; in < s.end; in += s.diff)
	{
	  sprintf(buffer1, "%zu.pnm", in);
	  sprintf(buffer2, "%s/%zu.pnm", s.dir, out++);
	  switch (mode)
	    {
	    case MODE_COPY:     if (copyfile(buffer1, buffer2)) goto fail;  break;
	    case MODE_SYMLINK:  if (symlfile(buffer1, buffer2)) goto fail;  break;
	    case MODE_LINK:     if (linkfile(buffer1, buffer2)) goto fail;  break;
	    case MODE_MOVE:     if (movefile(buffer1, buffer2)) goto fail;  break;
	    default:
	      return abort(), -1;
	    }
	}
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
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Waggregate-return"
# pragma GCC diagnostic ignored "-Wcast-qual"
#endif
  
  int rc = 0, mode = MODE_COPY;
  int f_symlink = 0, f_hardlink = 0, f_move = 0;
  size_t maxlen = 0, i, n;
  struct split* splits;
  
  
  args_init((char*)"Split a directory of images in a pattern",
	    (char*)"crazy-split [-s | -h | -m] [--] (<first> <gaps+1> <last> <output-dir>)...",
	    NULL, NULL, 1, 0, args_standard_abbreviations);
  
  
  args_add_option(args_new_argumentless(NULL, 1, (char*)"--help", NULL),
		  (char*)"Prints this help message");
  
  args_add_option(args_new_argumentless(NULL, 0, (char*)"-s", (char*)"--symlink", NULL),
		  (char*)"Create symbolic links rather than copies");
  
  args_add_option(args_new_argumentless(NULL, 0, (char*)"-h", (char*)"--hardlink", (char*)"--link", NULL),
		  (char*)"Create hard links rather than copies");
  
  args_add_option(args_new_argumentless(NULL, 0, (char*)"-m", (char*)"--move", NULL),
		  (char*)"Move files rather than create copies");
  
  
  args_parse(argc, argv);
  args_support_alternatives();
  
  
  if (args_opts_used((char*)"--help"))
    {
      args_help();
      goto exit;
    }
  if (args_unrecognised_count || (args_files_count & 3) || !args_files_count)
    goto invalid_opts;
  
  f_symlink  = args_opts_used((char*)"--symlink");
  f_hardlink = args_opts_used((char*)"--hardlink");
  f_move     = args_opts_used((char*)"--move");
  if (f_symlink + f_hardlink + f_move > 1)
    goto invalid_opts;
  
  
  splits = alloca((args_files_count >> 2) * sizeof(struct split));
  for (i = 0; i < args_files_count; i += 4)
    {
      splits[i].first = parse_size(args_files[i | 0]);
      splits[i].diff  = parse_size(args_files[i | 1]);
      splits[i].end   = parse_size(args_files[i | 2]);
      splits[i].dir   = args_files[i | 3];
      
      if ((splits[i].first == 0) || (splits[i].diff == 0))  goto invalid_opts;
      if ((splits[i].last  == 0) || (splits[i].dir  == 0))  goto invalid_opts;
      if (splits[i].end++ == SIZE_MAX)
	{
	  errno = ERANGE;
	  goto fail;
	}
      
      n = strlen(args_files[i | 3]), maxlen = maxlen < n ? n : maxlen;
      if (mkdirs(args_files[i | 3]))
	goto fail;
    }
  n = sizeof("/.pnm") + (3 * sizeof(size_t) + n) * sizeof(char);
  buffer1 = alloca(n);
  buffer2 = alloca(n);
  
  
  if (f_symlink)   mode = MODE_SYMLINK;
  if (f_hardlink)  mode = MODE_LINK;
  if (f_move)      mode = MODE_MOVE;
  if (perform_split(splits, args_files_count >> 2, mode))
    goto fail;
  
  
 exit:
  args_dispose();
  return rc;
 invalid_opts:
  args_help();
 fail:
  perror(*argv);
  rc = 1;
  goto exit;
  
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
}

