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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <argparser.h>


/**
 * `argv[0]` from `main`
 */
static char* execname;


/**
 * Get a list of all available devices
 * 
 * @param   count  Output parameter for the number of devices, -1 on error
 * @return         List of devices
 */
static char** get_devices(ssize_t* restrict count)
{
  int pipe_rw[2];
  int r;
  pid_t pid, reaped;
  char buf[512];
  ssize_t got, i;
  char** rc = NULL;
  size_t ptr, allocated;
  int state = 0, status;
  
  r = pipe(pipe_rw);
  if (r < 0)
    {
      perror(execname);
      *count = -1;
      return NULL;
    }
  
  pid = fork();
  if (pid < 0)
    {
      perror(execname);
      close(pipe_rw[0]);
      close(pipe_rw[1]);
      *count = -1;
      return NULL;
    }
  
  if (pid == 0)
    {
      if (pipe_rw[1] != STDOUT_FILENO)
	{
	  close(STDOUT_FILENO);
	  dup2(pipe_rw[1], STDOUT_FILENO);
	  close(pipe_rw[1]);
	}
      close(pipe_rw[0]);
      execlp("sh", "sh", "-c", "scanimage -L | cut -d ' ' -f 2", NULL);
      perror(execname);
      exit(1);
    }
  
  close(pipe_rw[1]);
  *count = 0;
  rc = malloc(sizeof(char*));
  if (rc == NULL)
      goto fail;
  ptr = 0, allocated = 32;
  rc[0] = malloc(allocated * sizeof(char));
  if (rc[0] == NULL)
    goto fail;
  state = 0;
  
  for (;;)
    {
      got = read(pipe_rw[0], buf, sizeof(buf) / sizeof(*buf));
      if (got == 0)
	break;
      else if (got < 0)
	goto fail;
      for (i = 0; i < got; i++)
	{
	  if (state == 0)
	    state++;
	  else if ((state == 1) && ((buf[i] & 0xC0) != 0x80))
	    state++;
	  if (buf[i] == '\n')
	    {
	      char** old;
	      state = 0;
	      if ((rc[*count][--ptr] & 0x80))
		while ((rc[*count][ptr - 1] & 0xC0) != 0x80)
		  ptr--;
	      rc[*count][ptr] = '\0';
	      rc = realloc(old = rc, (size_t)(++*count) * sizeof(char*));
	      if (rc == NULL)
		{
		  rc = old;
		  goto fail;
		}
	      ptr = 0, allocated = 32;
	      rc[*count] = malloc(allocated * sizeof(char));
	      if (rc[*count] == NULL)
		goto fail;
	    }
	  else if (state == 2)
	    {
	      char* old;
	      if (ptr == allocated)
		{
		  rc[*count] = realloc(old = rc[*count], (allocated <<= 1) * sizeof(char));
		  if (rc[*count] == NULL)
		    {
		      rc[*count] = old;
		      goto fail;
		    }
		}
	      rc[*count][ptr++] = buf[i];
	    }
	}
    }

  for (;;)
    {
      reaped = wait(&status);
      if (reaped < 0)
	goto fail;
      else if (reaped == pid)
	{
	  if (status)
	    {
	      errno = 0;
	      goto fail;
	    }
	  break;
	}
    }
  
  free(rc[*count]);
  return rc;
 fail:
  if (rc != NULL)
    {
      while (*count >= 0)
	free(rc[*count--]);
      free(rc);
    }
  if (errno)
    perror(execname);
  close(pipe_rw[0]);
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
  stty.c_lflag &= (typeof(stty.c_lflag))~(ICANON | ECHO | ISIG);
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
	  if (n - sel - 1 > 0)
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
  
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_stty);
  *selected = list[sel];
  return 0;
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
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Waggregate-return"
# pragma GCC diagnostic ignored "-Wcast-qual"
  
  int rc = 0;
  char* device = NULL;
  char** args;
  ssize_t device_count;
  char** devices = NULL;
  
  execname = argv[0];
  
  args_init((char*)"A crazy simple and usable scanning utility",
	    (char*)"crazy [options]",
	    NULL, NULL, 1, 0, args_standard_abbreviations);
  
  
  args_add_option(args_new_argumentless(NULL, 1, (char*)"-h", (char*)"-?", (char*)"--help", NULL),
		  (char*)"Prints this help message");
  
  args_add_option(args_new_argumented(NULL, (char*)"DEVICE", 0, (char*)"-d", (char*)"--device", NULL),
		  (char*)"Select scanning device");
  
  args_add_option(args_new_argumentless(NULL, 0, (char*)"-L", (char*)"--list-devices", NULL),
		  (char*)"List available scanning device");
  
  
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
  
  if (device == NULL)
    {
      printf("Please wait while searching for scanners...\n");
      devices = get_devices(&device_count);
      if (device_count == 0)
	{
	  fprintf(stderr, "%s: no scanning device available\n", execname);
	  rc = 1;
	  goto exit;
	}
      printf("\033[A\033[2K");
      if (device_count > 1)
	rc = select_item(&device, devices, (size_t)device_count, "Select scanning device");
      else
	device = *devices;
      if (rc)
	goto exit;
    }
  
 exit:
  if (devices != NULL)
    {
      while (device_count)
	free(devices[--device_count]);
      free(devices);
    }
  args_dispose();
  return rc;
 invalid_opts:
  rc = 1;
  args_help();
  goto exit;
  
# pragma GCC diagnostic pop
}

