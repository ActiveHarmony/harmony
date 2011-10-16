.SUFFIXES: .h .java .class

all: Harmony.class Harmony.h libharmonyapi.so HarmonyVariable.class testHarmony.class

.java.class: 
	javac $<

.class.h:
	javah -jni ${<:.class=}

libhello.so: HelloWorldImp.c
	gcc -G -I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux \
      $? -o $@

clean:
	\rm -f *.class Harmony.h *.so a.out core

libharmonyapi.so: harmonyclient.c #api.c hclient.o hsockutil.o hutil.o hmesgs.o
	g++ -g -o $@ -fPIC -shared $? -I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux -lm -ldl -lnsl -ltcl
