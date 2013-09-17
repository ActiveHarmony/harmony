#!/bin/sh

DIR=$1.latex

# Exit this script immediately upon error.
set -e

# Copy the Harmony style file to the destination directory.
cat doxygen/harmony.sty >> $DIR/doxygen.sty

# Force new sections to begin on a new page.
sed 's/^\\section/\\newpage\\section/' $DIR/refman.tex > $DIR/tmp.tex
mv -f $DIR/tmp.tex $DIR/refman.tex

# Use \TabularEnv for "Configuration Variables" tables.
for i in $DIR/*.tex; do
    perl \
        -e '@a = <>;' \
        -e '$a = join("", @a);' \
        -e '$pre = "Configuration Variables\\} \\\\begin\{\\K";' \
        -e '$a =~ s/${pre}TabularC\}\{4\}(.*?)\\end\{TabularC\}/TabularEnv}\1\\end{TabularEnv}/isg;' \
        -e 'print $a;' $i > $i.tmp
    mv -f $i.tmp $i
done
