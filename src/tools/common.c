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
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/sendfile.h>



/**
 * Copy a file
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int copyfile(const char* src, const char* dest)
{
  int fsrc = -1;
  int fdest = -1;
  int saved_errno;
  ssize_t sent;
  struct stat attr;
  
  if (fsrc = open(src, O_RDONLY), fsrc < 0)
    goto fail;
  if (fstat(fsrc, &attr))
    goto fail;
  
  if (fdest = open(dest, O_WRONLY | O_CREAT | O_EXCL, attr.st_mode & 07777), fdest < 0)
    goto fail;
  
  do
    {
      sent = sendfile(fdest, fsrc, NULL, 0x7ffff000UL);
      if ((sent < 0) && (errno != EINTR))
	goto fail;
    }
  while (sent);
  
  while (close(fsrc)  && (errno == EINTR));
  while (close(fdest) && (errno == EINTR));
  return 0;
 fail:
  saved_errno = errno;
  if (fsrc  >= 0)  while (close(fsrc)  && (errno == EINTR));
  if (fdest >= 0)  while (close(fdest) && (errno == EINTR));
  errno = saved_errno;
  return -1;
}


/**
 * Link a file, symbolically
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int symlfile(const char* src, const char* dest)
{
}


/**
 * Link a file
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int linkfile(const char* src, const char* dest)
{
  return link(src, dest);
}


/**
 * Move a file
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int movefile(const char* src, const char* dest)
{
  if (!rename(src, dest))   return 0;
  if (errno != EXDEV)       return -1;
  if (copyfile(src, dest))  return -1;
  return unlink(src);
}


/**
 * Create directory recursively,
 * do nothing if it already exists
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int mkdirs(const char* dir)
{
}

