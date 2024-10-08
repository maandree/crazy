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
#ifndef CRAZY_CRAZY_H
#define CRAZY_CRAZY_H


/**
 * Go to the label `fail` unless the given expression
 * evaluates to zero.
 * 
 * @param  ...  The expression, may have side-effects.
 */
#define t(...)  do { if (__VA_ARGS__) goto fail; } while (0)


/**
 * `argv[0]` from `main`
 */
extern char* execname;

/**
 * The index of the corner in the the top-left corner after transformation
 */
extern int c1;

/**
 * The index of the corner in the the top-right corner after transformation
 */
extern int c2;

/**
 * The index of the corner in the the bottom-left corner after transformation
 */
extern int c3;

/**
 * The index of the corner in the the bottom-right corner after transformation
 */
extern int c4;


#endif

