#!/bin/sh

# Rudimentary build script.

ANTLR_CLASSPATH=/usr/share/java/antlr3.jar:/usr/share/java/stringtemplate.jar:/usr/share/java/antlr3-runtime.jar

antlr3 *.g
javac -d . -cp $ANTLR_CLASSPATH *.java
cp *.java *.tokens org/umd/periCSL
jar cf periCSL.jar org
