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
#define _GNU_SOURCE
#include "common.h"
#include <alloca.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sendfile.h>



/**
 * Get the absolute path of a file,
 * it will remove all redundant slashes, all ./:s, and
 * all ../:s, but not resolve symbolic links
 * 
 * @param   file  The file
 * @param   ref   The current working directory
 * @return        The file's absolute pathname
 */
static char* abspath(const char* file, const char* ref)
{
  char* rc;
  char* rc_;
  size_t p, q;
  
  rc = malloc((strlen(file) + strlen(ref) + 2) * sizeof(char));
  if (rc_ = rc, rc == NULL)
    return NULL;
  
  /* Create absolute path, with //, /./, and /../. */
  if (*file != '/')
    {
      rc_ = strcpy(rc_, ref);
      *rc_++ = '/';
    }
  strcpy(rc_, file);
  
  /* Remove redundant slashes. */
  p = q = 1;
  while (rc[p])
    if ((rc[p] == '/') && (rc[p - 1] == '/'))
      p++;
    else
      rc[q++] = rc[p++];
  rc[q] = '\0';
  
  /* Remove ./:s. */
  p = q = 0;
  while (rc[p])
    if ((rc[p] == '/') && (rc[p + 1] == '.') && (rc[p + 2] == '/'))
      p += 2;
    else
      rc[q++] = rc[p++];
  rc[q] = '\0';
  
  /* Remove ../:s. */
  for (;;)
    {
      rc_ = strstr(rc, "/../");
      if (rc_ == NULL)
	break;
      memmove(rc_, rc_ + 3, strlen(rc_ + 2) * sizeof(char));
      if (rc_ == rc)
	continue;
      for (p = 0; rc_[-p] != '/'; p++);
      memmove(rc_ - p, rc_, (strlen(rc_) + 1) * sizeof(char));
    }
  
  return rc;
}


/**
 * Get the relative path of a file
 * 
 * @param   file  The file
 * @param   ref   The reference file (not directory)
 * @return        The file's pathname relative to the reference file
 */
static char* relpath(const char* file, const char* ref)
{
  int saved_errno;
  char* cwd = NULL;
  char* absfile = NULL;
  char* absref = NULL;
  char* rc = NULL;
  char* prc;
  size_t ptr, p, n = 0;
  
  if ((cwd     = get_current_dir_name()) == NULL)  goto fail;
  if ((absfile = abspath(file, cwd))     == NULL)  goto fail;
  if ((absref  = abspath(ref, cwd))      == NULL)  goto fail;
  
  strrchr(absref, '/')[1] = '\0';
  
  ptr = 0;
  while (absfile[ptr] && (absfile[ptr] == absref[ptr]))
    ptr++;
  while (absfile[ptr] != '/')
    ptr--;
  
  for (p = ptr; absref[p]; p++)
    if (absref[p] == '/')
      n += 1;
  
  rc = malloc((strlen(absfile + ptr) + 1 + 3 * n) * sizeof(char));
  if (prc = rc, rc == NULL)
    goto fail;
  
  while (n--)
    prc = stpcpy(prc, "../");
  strcpy(prc, absfile + ptr);
  
  free(cwd);
  free(absfile);
  free(absref);
  return rc;
  
 fail:
  saved_errno = errno;
  free(cwd);
  free(absfile);
  free(absref);
  free(rc);
  errno = saved_errno;
  return NULL;
}


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
  char* target = NULL;
  int saved_errno;
  
  target = relpath(src, dest);
  if (target == NULL)
    goto fail;
  
  if (symlink(target, dest))
    goto fail;
  
  free(target);
  return 0;
 fail:
  saved_errno = errno;
  free(target);
  errno = saved_errno;
  return -1;
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
  char* p;
  char* q;
  char* dir_edited;
  
  p = alloca((strlen(dir) + 1) * sizeof(char));
  strcpy(p, dir);
  dir_edited = p;
  
 next:
  while (*p == '/')
    p++;
  
  q = strchr(p, '/');
  if (*q == '/')
    {
      *q = '\0';
      if (access(dir, F_OK) && mkdir(dir_edited, 0755))
	goto fail;
      *q = '/';
      p = q + 1;
      goto next;
    }
  else if (*p)
    if (access(dir, F_OK) && mkdir(dir_edited, 0755))
      goto fail;
  
  return 0;
 fail:
  return -1;
}

