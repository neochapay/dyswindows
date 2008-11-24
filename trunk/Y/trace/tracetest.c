#include <stdlib.h>
#include <stdint.h>
#include <Y/trace/trace.h>

int main(void);

int main()
{
  int32_t i32 = 7;
  char ev[] = "Y_TRACE=1";
  putenv(ev);
  trace_init();
  trace("No variables");
  trace("int32_t variable, should say \"  int32_t i32 = 7\"",trace_var(i32,trace_int32));
  trace("string variable, should say \"  char *ev = (char *)########\" and \"Y_TRACE=1\"",trace_var(ev,trace_string));
  return 0;
}
