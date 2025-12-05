These are little test programs which exercise a variety of Linux system calls.  They are intended for use under any architecture.

See arch-makefiles for examples of how to compile.  
In general make a architecture specific directory as a sibling to this directory.
 e.g. If this repos is cloned to /home/you/git/generic-tests then create a directory /home/you/git/generic-x86
 (or you can modify the VPATH to point from where you are building back to this directory)

You will also need cross compilers of course.  
The file base.Dockerfile includes all the packages I commonly use which is more than is needed for generic-tests but does include 
compiler infrastructure for x86, arm, mips, and ppc.

Many of the tests are careful to flush stdout after printf's so that they result in writes when they are executed and not saved up to the end.

Certainly the tests could be refactored to avoid repetition and thrash but for now they are meant to be simple to use
 
