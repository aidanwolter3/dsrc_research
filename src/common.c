/*
 * common.c
 *
 *  Created on: Jul 22, 2015
 *      Author: Aidan
 */

#include "common.h"

//modified strtok that takes into account empty strings
//courtesy of http://www.tek-tips.com/viewthread.cfm?qid=294161
char* xstrtok(char *line, char *delims) {
  static char *saveline = NULL;
  char *p;
  int n;

  if(line != NULL) {
    saveline = line;
  }

  /*
  *see if we have reached the end of the line
  */
  if(saveline == NULL || *saveline == '\0') {
    return(NULL);
  }

  /*
  *return the number of characters that aren't delims
  */
  n = strcspn(saveline, delims);
  p = saveline; /*save start of this token*/

  saveline += n; /*bump past the delim*/

  /*trash the delim if necessary*/
  if(*saveline != '\0') {
    *saveline++ = '\0';
  }

  return p;
}
