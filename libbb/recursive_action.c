/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) tons of folks.  Tracking down who wrote what
 * isn't something I'm going to worry about...  If you wrote something
 * here, please feel free to acknowledge your work.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Based in part on code from sash, Copyright (c) 1999 by David I. Bell 
 * Permission has been granted to redistribute this code under the GPL.
 *
 */

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include "libbb.h"

/* same conditions as recursive_action */
#define bb_need_name_too_long
#define BB_DECLARE_EXTERN
#include "../messages.c"

#undef DEBUG_RECURS_ACTION


/*
 * Walk down all the directories under the specified 
 * location, and do something (something specified
 * by the fileAction and dirAction function pointers).
 *
 * Unfortunately, while nftw(3) could replace this and reduce 
 * code size a bit, nftw() wasn't supported before GNU libc 2.1, 
 * and so isn't sufficiently portable to take over since glibc2.1
 * is so stinking huge.
 */
int recursive_action(const char *fileName,
					int recurse, int followLinks, int depthFirst,
					int (*fileAction) (const char *fileName,
									   struct stat * statbuf,
									   void* userData),
					int (*dirAction) (const char *fileName,
									  struct stat * statbuf,
									  void* userData),
					void* userData)
{
	int status;
	struct stat statbuf;
	struct dirent *next;

	if (followLinks == TRUE)
		status = stat(fileName, &statbuf);
	else
		status = lstat(fileName, &statbuf);

	if (status < 0) {
#ifdef DEBUG_RECURS_ACTION
		fprintf(stderr,
				"status=%d followLinks=%d TRUE=%d\n",
				status, followLinks, TRUE);
#endif
		perror_msg("%s", fileName);
		return FALSE;
	}

	if ((followLinks == FALSE) && (S_ISLNK(statbuf.st_mode))) {
		if (fileAction == NULL)
			return TRUE;
		else
			return fileAction(fileName, &statbuf, userData);
	}

	if (recurse == FALSE) {
		if (S_ISDIR(statbuf.st_mode)) {
			if (dirAction != NULL)
				return (dirAction(fileName, &statbuf, userData));
			else
				return TRUE;
		}
	}

	if (S_ISDIR(statbuf.st_mode)) {
		DIR *dir;

		if (dirAction != NULL && depthFirst == FALSE) {
			status = dirAction(fileName, &statbuf, userData);
			if (status == FALSE) {
				perror_msg("%s", fileName);
				return FALSE;
			} else if (status == SKIP)
				return TRUE;
		}
		dir = opendir(fileName);
		if (!dir) {
			perror_msg("%s", fileName);
			return FALSE;
		}
		status = TRUE;
		while ((next = readdir(dir)) != NULL) {
			char nextFile[PATH_MAX];

			if ((strcmp(next->d_name, "..") == 0)
					|| (strcmp(next->d_name, ".") == 0)) {
				continue;
			}
			if (strlen(fileName) + strlen(next->d_name) + 1 > PATH_MAX) {
				error_msg(name_too_long);
				return FALSE;
			}
			memset(nextFile, 0, sizeof(nextFile));
			if (fileName[strlen(fileName)-1] == '/')
				sprintf(nextFile, "%s%s", fileName, next->d_name);
			else
				sprintf(nextFile, "%s/%s", fileName, next->d_name);
			if (recursive_action(nextFile, TRUE, followLinks, depthFirst,
						fileAction, dirAction, userData) == FALSE) {
				status = FALSE;
			}
		}
		closedir(dir);
		if (dirAction != NULL && depthFirst == TRUE) {
			if (dirAction(fileName, &statbuf, userData) == FALSE) {
				perror_msg("%s", fileName);
				return FALSE;
			}
		}
		if (status == FALSE)
			return FALSE;
	} else {
		if (fileAction == NULL)
			return TRUE;
		else
			return fileAction(fileName, &statbuf, userData);
	}
	return TRUE;
}


/* END CODE */
/*
Local Variables:
c-file-style: "linux"
c-basic-offset: 4
tab-width: 4
End:
*/
