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
#include "crazy.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#include <argparser.h>

#include "display.h"
#include "display_fb.h"
#include "util.h"



/**
 * `argv[0]` from `main`
 */
char* execname;

/**
 * The index of the corner in the the top-left corner after transformation
 */
int c1 = 1;

/**
 * The index of the corner in the the top-right corner after transformation
 */
int c2 = 2;

/**
 * The index of the corner in the the bottom-left corner after transformation
 */
int c3 = 3;

/**
 * The index of the corner in the the bottom-right corner after transformation
 */
int c4 = 4;


/**
 * The scanning device
 */
static char* device = NULL;

/**
 * The scanning mode: 0=monochrome, 1=grey, 2=colour
 */
static int mode = -1;

/**
 * The scanning resolution
 */
static int dpi = -1;

/**
 * The brightness threshold for a white point
 */
static int white = 128;

/**
 * Shell sequence to pipe the image through while scanning
 */
static char* pipeimg = NULL;

/**
 * Shell sequence to pipe the image through after scanning
 */
static char* postimg = NULL;

/**
 * Image display system
 */
static display_t display;



/**
 * Get a list of all available devices
 * 
 * @param   count  Output parameter for the number of devices, -1 on error
 * @return         List of devices
 */
static char** get_devices(ssize_t* restrict count)
{
  pid_t pid, reaped;
  int fd, status;
  char** rc = NULL;
  char* line = NULL;
  size_t linesize = 0;
  ssize_t len;
  void* new;
  char* p;
  char* q;
  
  /* Start listing devices. */
  fd = subprocess_rd("scanimage", (const char* const[]){"scanimage", "-L", NULL}, 1, &pid);
  /* Output line format: device `DEVICE_ADDRESS' is a DEVICE_NAME_AND_TYPE */
  t (fd < 0);
  
  /* Retrieve device list. */
  *count = 0;
  for (;;)
    {
      len = fd_getline(fd, &line, &linesize);
      t (len < 0);
      if (len == 0)
	break;

      /* Check end of list. (For when nothing is found, scanimage is a bit odd here) */
      if (*line == '\n')
	break;
      
      /* Find start of column 2. */
      p = strchr(line, ' ');
      if ((p == NULL) || (*++p == ' '))
	{
	  fprintf(stderr, "%s: invalid line read from `scanimage -L`, skipping\n", execname);
	  continue;
	}
      /* Skip first character. (` is currently used, who knows, maybe it will be ‘ in the future.) */
      for (p++; (*p & 0xC0) == 0x80; p++);
      /* Find end of column 2. */
      q = strchr(p, ' ');
      if (q == NULL)
	{
	  fprintf(stderr, "%s: invalid line read from `scanimage -L`, skipping\n", execname);
	  continue;
	}
      /* Skip last character. (' is currently used, who knows, maybe it will be ’ in the future.) */
      while ((q[-1] & 0xC0) == 0x80)
	q--;
      q--;
      /* NUL-terminate item. */
      *q = '\0';
      
      /* Add device to list. */
      new = realloc(rc, ((size_t)*count + 1) * sizeof(*rc));
      t (new == NULL);
      rc = new;
      rc[*count] = malloc(strlen(p) + 1);
      t (rc[*count] == NULL);
      strcpy(rc[*count], p);
      ++*count;
      
    }
  free(line), line = NULL;
  
  /* Reap device lister. */
  for (;;)
    {
      reaped = wait(&status);
      t (reaped < 0);
      if (reaped == pid)
	{
	  if (!status)
	    break;
	  errno = 0;
	  goto fail;
	}
    }
  
  /* Done. */
  close(fd);
  return rc;
 fail:
  if (errno)
    perror(execname);
  free(line);
  if (rc != NULL)
    {
      while (*count-- > 0)
	free(rc[*count]);
      free(rc);
    }
  if (fd >= 0)
    close(fd);
  return *count = -1, NULL;
}


/**
 * Let the user select an element from a list
 * 
 * @param   selected  Output parameter for the selected item
 * @param   list      The list of selectable items
 * @param   n         The number of elements in `list`
 * @param   text      The instructional text to display
 * @return            Zero on success, -1 on error
 */
static int select_item(char** restrict selected, char** restrict list, size_t n, const char* text)
{
  size_t i, sel = 0;
  struct termios stty;
  struct termios saved_stty;
  
  printf("%s\n\n", text);
  
  for (i = 0; i < n; i++)
    if (i == sel)
      printf("\033[00;01;07;34;47m%s\033[00m\n", list[i]);
    else
      printf("\033[00;34m%s\033[00m\n", list[i]);
  
  tcgetattr(STDIN_FILENO, &stty);
  saved_stty = stty;
  stty.c_lflag &= (tcflag_t)~(ICANON | ECHO | ISIG);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &stty);
  
  for (;;)
    {
      char c = (char)getchar();
      if (c == '\n')
	break;
      else if (((c == 'N' - '@') || (c == 'B') || (c == ' ')) && (sel + 1 < n))
	{
	  printf("\033[%zuA\033[00;34m%s\033[00m\n", n - sel, list[sel]);
	  printf("\033[00;01;07;34;47m%s\033[00m\n", list[++sel]);
	  if (n - sel > 0)
	    printf("\033[%zuB", n - sel - 1);
	  fflush(stdout);
	}
      else if (((c == 'P' - '@') || (c == 'A') || (c == 8) || (c == 127)) && (sel > 0))
	{
	  printf("\033[%zuA\033[00;34m%s\033[00m\n", n - sel, list[sel]);
	  printf("\033[2A\033[00;01;07;34;47m%s\033[00m\n", list[--sel]);
	  printf("\033[%zuB", n - sel);
	  fflush(stdout);
	}
    }
  
  printf("\033[%zuA\033[00;01;34m%s\033[00m\n", n - sel, list[sel]);
  if (n - sel > 0)
    printf("\033[%zuB", n - sel);
  fflush(stdout);
  
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_stty);
  *selected = list[sel];
  printf("\n\n\n");
  return 0;
}


/**
 * Scan an image
 * 
 * @param   image  Output parameter for the image buffer
 * @return         Zero on and only on success
 */
static int scan_image(char** image)
{
  char* sh = NULL;
  const char* mode_ = mode == 0 ? "lineart" : mode == 1 ? "gray" : "color"; /* [sic!] */
  char threshold[sizeof(" --threshold -") + 3 * sizeof(int)];
  int fd = -1;
  pid_t pid;
  
  /* Init. */
  *image = NULL;
  *threshold = '\0';
  
  /* Construct scan command. */
  if (mode == 0)
    sprintf(threshold, " --threshold %i", white); /* TODO this may not be supported (inactive)*/
  aprintf(&sh, "scanimage -d '%s' --format pnm --mode %s --resolution %idpi%s%s%s%zn",
	  device, mode_, dpi, threshold, pipeimg ? " | " : "", pipeimg ?: "", &n);
  t (sh == NULL);
  
  /* Start scanner process. */
  fd = subprocess_rd("sh", (const char* const[]){"sh", "-c", sh}, 0, &pid);
  t (fd < 0);
  
  /* TODO */
  
  /* Done. */
  close(pipe_rw[0]);
  return 0;
 fail:
  if (errno)
    perror(execname);
  free(*image);
  *image = NULL;
  free(sh);
  if (pipe_rw[0] >= 0)  close(pipe_rw[0]);
  if (pipe_rw[1] >= 0)  close(pipe_rw[1]);
  return -1;
}


/**
 * Apply a transformation, rotation is applied first
 * 
 * @param  r  The number of degrees {0, 90, 180, 270} to rotate the page
 * @param  x  Whether to mirror the page horizontally
 * @param  y  Whether to mirror the page vertically
 */
static void apply_transformation(int r, int x, int y)
{
#define swap(a, b)  (a ^= b, b ^= a, a ^= b)
  
  for (; r; r -= 90)
    {
      /* Transpose. */
      swap(c2, c3);
      
      /* Mirror x-axis. */
      swap(c1, c2);
      swap(c3, c4);
    }
  
  /* Mirror x-axis. */
  if (x)  swap(c1, c2);
  if (x)  swap(c3, c4);
  
  /* Mirror y-axis. */
  if (y)  swap(c1, c3);
  if (y)  swap(c2, c4);
  
#undef swap
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
  char** args;
  ssize_t device_count;
  char** devices = NULL;
  int mirrorx = 0, mirrory = 0, rotation = 0;
  int display_initialised = 0;
  
  
  /* Parse command line. */
  
  execname = argv[0];
  
  args_init((char*)"A crazy simple and usable scanning utility",
	    (char*)"crazy [options]",
	    NULL, NULL, 1, 0, args_standard_abbreviations);
  
  
  args_add_option(args_new_argumentless(NULL, 1, (char*)"-h", (char*)"-?", (char*)"--help", NULL),
		  (char*)"Prints this help message");
  
  args_add_option(args_new_argumentless(NULL, 0, (char*)"-L", (char*)"--list-devices", NULL),
		  (char*)"List available scanning device");
  
  args_add_option(args_new_argumented(NULL, (char*)"DEVICE", 0, (char*)"-d", (char*)"--device", NULL),
		  (char*)"Select scanning device");
  
  args_add_option(args_new_argumented(NULL, (char*)"MODE", 0, (char*)"-m", (char*)"--mode", NULL),
		  (char*)"Select scan mode: monochrome|grey|colour");
  
  args_add_option(args_new_argumented(NULL, (char*)"DPI", 0, (char*)"-q", (char*)"--resolution", NULL),
		  (char*)"Select resolution: 75|150|300|600|1200|2400");
  
  args_add_option(args_new_argumented(NULL, (char*)"LEVEL", 0, (char*)"-t", (char*)"--threshold", NULL),
		  (char*)"Select minimum brightness to get a white point");
  
  args_add_option(args_new_argumented(NULL, (char*)"COMMAND", 0, (char*)"-p", (char*)"--pipe", NULL),
		  (char*)"Select shell sequence to pipe the scanned images through while scanning");
  
  args_add_option(args_new_argumented(NULL, (char*)"COMMAND", 0, (char*)"-P", (char*)"--postprocess", NULL),
		  (char*)"Select shell sequence to pipe the scanned images through after scanning");
  
  args_add_option(args_new_argumentless(NULL, 0, (char*)"-x", (char*)"--mirror-x", NULL),
		  (char*)"Mirror scanned images horizontally");
  
  args_add_option(args_new_argumentless(NULL, 0, (char*)"-y", (char*)"--mirror-y", NULL),
		  (char*)"Mirror scanned images vertically");
  
  args_add_option(args_new_argumented(NULL, (char*)"ROTATION", 0, (char*)"-r", (char*)"--rotate", NULL),
		  (char*)"Select rotation: 0|90|180|270");
  
  
  args_parse(argc, argv);
  args_support_alternatives();
  
  
  if (args_opts_used((char*)"--help"))
    {
      args_help();
      goto exit;
    }
  if (args_opts_used((char*)"--list-devices"))
    {
      ssize_t i;
      devices = get_devices(&device_count);
      if (device_count < 0)
	rc = 1;
      else
	for (i = 0; i < device_count; i++)
	  printf("%s\n", devices[i]);
      goto exit;
    }
  if (args_opts_used((char*)"--device"))
    {
      args = args_opts_get((char*)"--device");
      if ((args_opts_get_count((char*)"--device") != 1) || (*args == NULL))
	goto invalid_opts;
      device = *args;
    }
  if (args_opts_used((char*)"--mode"))
    {
      args = args_opts_get((char*)"--mode");
      if ((args_opts_get_count((char*)"--mode") != 1) || (*args == NULL))
	goto invalid_opts;
      if (!strcasecmp(*args, "monochrome") || !strcasecmp(*args, "lineart"))
	mode = 0;
      else if (!strcasecmp(*args, "grey") || !strcasecmp(*args, "gray"))
	mode = 1;
      else if (!strcasecmp(*args, "colour") || !strcasecmp(*args, "color"))
	mode = 2;
      else
	goto invalid_opts;
    }
  if (args_opts_used((char*)"--resolution"))
    {
      args = args_opts_get((char*)"--resolution");
      if ((args_opts_get_count((char*)"--resolution") != 1) || (*args == NULL))
	goto invalid_opts;
      dpi = atoi(*args);
      if (dpi <= 0)
	goto invalid_opts;
    }
  if (args_opts_used((char*)"--threshold"))
    {
      args = args_opts_get((char*)"--threshold");
      if ((args_opts_get_count((char*)"--threshold") != 1) || (*args == NULL))
	goto invalid_opts;
      white = atoi(*args);
      if (!((0 <= white) && (white <= 255)))
	goto invalid_opts;
    }
  if (args_opts_used((char*)"--pipe"))
    {
      args = args_opts_get((char*)"--pipe");
      if ((args_opts_get_count((char*)"--pipe") != 1) || (*args == NULL))
	goto invalid_opts;
      pipeimg = *args;
    }
  if (args_opts_used((char*)"--postprocess"))
    {
      args = args_opts_get((char*)"--postprocess");
      if ((args_opts_get_count((char*)"--postprocess") != 1) || (*args == NULL))
	goto invalid_opts;
      postimg = *args;
    }
  mirrorx = !!args_opts_used((char*)"--mirror-x");
  mirrory = !!args_opts_used((char*)"--mirror-y");
  if (args_opts_used((char*)"--rotation"))
    {
      args = args_opts_get((char*)"--rotation");
      if ((args_opts_get_count((char*)"--rotation") != 1) || (*args == NULL))
	goto invalid_opts;
      rotation = atoi(*args) % 360;
      if (rotation < 0)
	rotation += 360;
      if (rotation % 90)
	goto invalid_opts;
    }
  
  /* Start. */
  
  /* Find scanner. */
  if (device == NULL)
    {
      printf("Please wait while searching for scanners...\n");
      devices = get_devices(&device_count);
      if (device_count < 0)
	{
	  rc = 1;
	  goto exit;
	}
      else if (device_count == 0)
	{
	  fprintf(stderr, "%s: no scanning device available\n", execname);
	  rc = 1;
	  goto exit;
	}
      printf("\033[A\033[2K");
      if (device_count > 1)
	rc = select_item(&device, devices, (size_t)device_count,
			 "Select scanning device");
      else
	device = *devices;
      if (rc)
	goto exit;
    }
  /* Select colour level. */
  if (mode < 0)
    {
      char* mode_;
      rc = select_item(&mode_, (char*[]){(char*)"monochrome", (char*)"grey", (char*)"colour"}, 3,
		       "Select scanning mode");
      if (rc)
	goto exit;
      else if (!strcmp(mode_, "monochrome"))  mode = 0;
      else if (!strcmp(mode_, "grey"))        mode = 1;
      else if (!strcmp(mode_, "colour"))      mode = 2;
    }
  /* Select resolution. */
  if (dpi < 0)
    {
      char* dpi_;
      rc = select_item(&dpi_, (char*[]){(char*)"75 dpi", (char*)"150 dpi",
 	                                (char*)"300 dpi", (char*)"600 dpi",
 	                                (char*)"1200 dpi", (char*)"2400 dpi"}, 6,
		       "Select scanning resolution");
      if (rc)
	goto exit;
      else
	dpi = atoi(dpi_);
    }
  
  
  /* Get transformation. */
  apply_transformation(rotation, mirrorx, mirrory);
  
  
  /* Select display system. */
  display_fb_get(&display);
  /*
  if (strchr(getenv("DISPLAY") ?: "", ':'))
    display_x_get(&display);
  */
  
  
  /* Start display system. */
  t (display.initialise());
  display_initialised = 1;
  
  
  /* TODO  Start scanning. */
  
  /* Done. */
 exit:
  if (display_initialised)
    display.terminate();
  if (devices != NULL)
    {
      while (device_count)
	free(devices[--device_count]);
      free(devices);
    }
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

