/* Reads from a file into the code segment, which is read-only.
   The process must be terminated with -1 exit code.
   Same check as tests/vm/pt-write-code2, run in the userprog build. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  int handle;

  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  read (handle, (void *) test_main, 1);
  fail ("survived reading data into code segment");
}
