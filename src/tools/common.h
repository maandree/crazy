/**
 * crazy — A crazy simple and usable scanning utility
 * Copyright © 2015, 2016  Mattias Andrée (maandree@member.fsf.org)
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
#ifndef CRAZY_TOOLS_COMMON_H
#define CRAZY_TOOLS_COMMON_H



/**
 * Go to the label `fail` unless the given expression
 * evaluates to zero.
 * 
 * @param  ...  The expression, may have side-effects.
 */
#define t(...)  do { if (__VA_ARGS__) goto fail; } while (0)


/**
 * Copy a file
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int copyfile(const char* src, const char* dest);

/**
 * Link a file, symbolically
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int symlfile(const char* src, const char* dest);

/**
 * Link a file
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int linkfile(const char* src, const char* dest);

/**
 * Move a file
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int movefile(const char* src, const char* dest);

/**
 * Create directory recursively,
 * do nothing if it already exists
 * 
 * @param   src   The original file
 * @param   dest  The new file
 * @return        Zero on success, -1 on error
 */
int mkdirs(const char* dir);



#endif

