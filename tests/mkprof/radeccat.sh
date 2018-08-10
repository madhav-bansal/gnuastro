# Create a mock image from radeccat.txt
#
# See the Tests subsection of the manual for a complete explanation
# (in the Installing gnuastro section).
#
# Original author:
#     Mohammad Akhlaghi <mohammad@akhlaghi.org>
# Contributing author(s):
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.





# Preliminaries
# =============
#
# Set the variables (The executable is in the build tree). Do the
# basic checks to see if the executable is made or if the defaults
# file exists (basicchecks.sh is in the source tree).
prog=mkprof
execname=../bin/$prog/ast$prog
cat=$topsrc/tests/$prog/radeccat.txt





# Skip?
# =====
#
# If the dependencies of the test don't exist, then skip it. There are two
# types of dependencies:
#
#   - The executable was not made (for example due to a configure option).
#
#   - Catalog doesn't exist (problem in tarball release).
if [ ! -f $execname ]; then echo "$execname not created."; exit 77; fi
if [ ! -f $cat      ]; then echo "$cat does not exist.";   exit 77; fi





# Actual test script
# ==================
#
# `check_with_program' can be something like `Valgrind' or an empty
# string. Such programs will execute the command if present and help in
# debugging when the developer doesn't have access to the user's system.
$check_with_program $execname $cat --ccol=RA --ccol=Dec --mode=wcs   \
                              --mergedsize=100,100
