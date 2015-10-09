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
#include <alloca.h>
#include <string.h>

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
 * Perform merge
 * 
 * @param   input_dirs    The directories to merge
 * @param   input_dirs_n  The number of directories to merge
 * @param   output_dir    The output directory
 * @param   mode          `MODE_COPY`, `MODE_SYMLINK`, `MODE_LINK`, or `MODE_MOVE`
 * @return                Zero on success, -1 on error
 */
static int perform_merge(char** input_dirs, size_t input_dirs_n, char* output_dir, int mode)
{
  char* stopped = alloca(input_dirs_n * sizeof(char));
  size_t in = 1, out = 1, dir = 0, stop_count = 0;
  
  memset(stopped, 0, input_dirs_n * sizeof(char));
  
  for (; stop_count < input_dirs_n; in += !(dir = (dir + 1) % input_dirs_n))
    {
      if (stopped[dir])
	continue;
      
      sprintf(buffer1, "%s/%zu.pnm", input_dirs[dir], in);
      if (access(buffer1, F_OK))
	{
	  stopped[dir] = 1;
	  stop_count++;
	  continue;
	}
      
      sprintf(buffer2, "%s/%zu.pnm", output_dir, out++);
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
  
  int rc = 0, mode = MODE_COPY;
  int f_symlink = 0, f_hardlink = 0, f_move = 0;
  size_t maxlen = 0, i, n;
  
  
  args_init((char*)"Zigzag-merge directories",
	    (char*)"crazy-merge [-s | -h | -m] [--] <input-dir>... <output-dir>",
	    NULL, NULL, 1, 0, args_standard_abbreviations);
  
  
  args_add_option(args_new_argumentless(NULL, 0, (char*)"--help", NULL),
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
  if (args_unrecognised_count || (args_files_count < 3))
    goto invalid_opts;
  
  f_symlink  = !!args_opts_used((char*)"--symlink");
  f_hardlink = !!args_opts_used((char*)"--hardlink");
  f_move     = !!args_opts_used((char*)"--move");
  if (f_symlink + f_hardlink + f_move > 1)
    goto invalid_opts;
  
  
  for (i = 0; i < (size_t)args_files_count; i++)
    n = strlen(args_files[i]), maxlen = maxlen < n ? n : maxlen;
  maxlen = sizeof("/.pnm") + (3 * sizeof(size_t) + maxlen) * sizeof(char);
  buffer1 = alloca(maxlen);
  buffer2 = alloca(maxlen);
  
  
  if (mkdirs(args_files[args_files_count - 1]))
    goto fail;
  
  if (f_symlink)   mode = MODE_SYMLINK;
  if (f_hardlink)  mode = MODE_LINK;
  if (f_move)      mode = MODE_MOVE;
  if (perform_merge(args_files, (size_t)args_files_count - 1, args_files[args_files_count - 1], mode))
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

