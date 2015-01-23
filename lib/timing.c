/*********************************************************************
Functions to report timing in verbose mode.
This is part of GNU Astronomy Utilities (AstrUtils) package.

Copyright (C) 2013-2015 Mohammad Akhlaghi
Tohoku University Astronomical Institute, Sendai, Japan.
http://astr.tohoku.ac.jp/~akhlaghi/

AstrUtils is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

AstrUtils is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with AstrUtils. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <stdio.h>

#include "timing.h"

void
reporttiming(struct timeval *t1, char *jobname, size_t level)
{
  double dt=1e30;
  struct timeval t2;

  if(level<2 && t1)
    {
      gettimeofday(&t2, NULL);

      dt= ( ((double)t2.tv_sec+(double)t2.tv_usec/1e6) -
	    ((double)t1->tv_sec+(double)t1->tv_usec/1e6) );
    }

  if(level==0)
    printf("%s %f (seconds)\n", jobname, dt);
  else if(level==1)
    {
      if(t1)
	printf("  - %-"VERBMSGLENGTH_T"s in %f seconds\n", jobname, dt);
      else
	printf("  - %-"VERBMSGLENGTH_T"s\n", jobname);
    }
  else if(level==2)
    printf("  ---- %s\n", jobname);
}