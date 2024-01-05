# JcShell
This is an implementation of a job submission shell, which is programming assignment 1 of HKU COMP3230 course.

Author: Guo Zebin

Platform: Linux

This shell is capable of:
+ It accepts a single command or a job that consists of a sequence of commands linked together with pipes (|) and executes the corresponding command(s) with the given argument list(s).
+ It can locate and execute any valid program (i.e., compiled programs) by giving an absolute path (starting with /) or a relative path (starting with ./ or ../) or by searching directories under the $PATH environment variable.
+ It can be terminated by the built-in exit command but it cannot be terminated by the Cltr-c key or the SIGINT signal.
+ After the submitted command/job terminated, It prints the running statistics of all terminated command(s) and waits for the next command/job from the user.




