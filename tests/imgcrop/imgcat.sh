# Crop from a catalog using x and y coordinates in Image mode.
#
# See the Tests subsection of the manual for a complete explanation
# (in the Installing AstrUtils section).
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.





# Preliminaries:
################
# Set the variabels (The executable is in the build tree). Do the
# basic checks to see if the executable is made or if the defaults
# file exists (basicchecks.sh is in the source tree).
prog=imgcrop
execname=../src/$prog/astr$prog





# If the executable was not made (the user chose to not install this
# package), skip this test:
if [ ! -f $execname ]; then
    exit 77
fi





# Actual test script:
#####################
img=mkprofcat1.fits
cat=$topsrc/tests/$prog/cat.txt
$execname $img $cat --imgmode --suffix=_imgcat.fits --zeroisnotblank