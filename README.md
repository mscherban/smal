# smal
msp430 micro os project.

## what it is
a small project where i am using what i learned in my os class to build a context switching os on the msp430.
a lot of the methods here burned into my brain from studying the xinu kernel.
the msp430 interests me because it's a low barrier-to-entry cpu with huge documentation and free tools.
msp430fr2443 launchpad.

## how to use
just add the project files to your directory and ```#include <smal.h>```

## progress
currently swaps between processes using a priority based preemption scheduler

## to-do
sleep queue
synchronization, due to interrupt based preemption
