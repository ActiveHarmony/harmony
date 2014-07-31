#!/bin/sh

HTML_DIR=$1.html
LATEX_DIR=$1.latex

# Exit this script immediately upon error.
set -e

# Copy the Harmony style file to the destination directory.
cat doxygen/harmony.sty >> $LATEX_DIR/doxygen.sty

# Force new sections to begin on a new page.
sed 's/^\\section/\\clearpage\\newpage\\section/' $LATEX_DIR/refman.tex > $LATEX_DIR/tmp.tex
mv -f $LATEX_DIR/tmp.tex $LATEX_DIR/refman.tex

# Use \TabularEnv for "Configuration Variables" tables.
for i in $LATEX_DIR/*.tex; do
    perl \
        -e '@a = <>;' \
        -e '$a = join("", @a);' \
        -e '$pre = " Variables\\} \\\\begin\{\\K";' \
        -e '$a =~ s/${pre}TabularC\}\{4\}(.*?)\\end\{TabularC\}/TabularEnv4}\1\\end{TabularEnv4}/isg;' \
        -e '$a =~ s/${pre}TabularC\}\{3\}(.*?)\\end\{TabularC\}/TabularEnv3}\1\\end{TabularEnv3}/isg;' \
        -e 'print $a;' $i > $i.tmp
    mv -f $i.tmp $i
done

# Fix &ndash; (--) and &#35; (#) characters.
for i in $LATEX_DIR/*.tex $HTML_DIR/*.html; do
    perl \
        -e '@a = <>;' \
        -e '$a = join("", @a);' \
        -e '$a =~ s/&ndash;/--/g;' \
        -e '$a =~ s/&amp;ndash;/--/g;' \
        -e '$a =~ s/&#35;/#/g;' \
        -e '$a =~ s/&amp;#35;/#/g;' \
        -e 'print $a;' $i > $i.tmp
    mv -f $i.tmp $i
done
