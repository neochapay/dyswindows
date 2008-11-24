#include <Y/trace/trace.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

static FILE *tracefile;

static int tracing = 0;

void trace_(const char *file, const int line, const char *function, const char *comment, ...)
{
  /* The ... is a sequence of these 3[4] parameters:
   * 
   * const char *variablename,
   * enum tracetype type,
   * const <type> variable,
   * [size_t size,] <- only for trace_buffer
   * 
   * The final parameter must be a NULL, and it must appear in 
   * a position which would otherwise be a valid place to find
   * a variablename.
   */
  if (!tracing) return;
  fprintf(tracefile, "%s:%d:in %s:%s\n", file, line, function, comment);
  va_list args;
  const char *varname;
  int ival;
  unsigned int uival;
  int64_t i64val;
  uint64_t ui64val;
  const char *strval;
  const void *ptrval;
  const unsigned char *bufval;
  double dblval;
  long double longdblval;
  enum tracetype type;
  size_t size;
  va_start(args, comment);
  while ((varname = va_arg(args, const char *)) != NULL)
  {
    type = va_arg(args, enum tracetype);
    switch (type)
    {
      case trace_uint8:
	uival = va_arg(args, unsigned int); /* unsigned integer types <= unsigned int are promoted to unsigned int in unprototyped argument lists */
	if (uival > UCHAR_MAX)
	  fprintf(tracefile, "  uint8_t %s has impossible value %u\n", varname, uival);
	else
	  fprintf(tracefile, "  uint8_t %s = %u\n", varname, uival);
	break;
      case trace_uint16:
	uival = va_arg(args, unsigned int); /* unsigned integer types <= unsigned int are promoted to unsigned int in unprototyped argument lists */
	if (uival > USHRT_MAX)
	  fprintf(tracefile, "  uint16_t %s has impossible value %u\n", varname, uival);
	else
	  fprintf(tracefile, "  uint16_t %s = %u\n", varname, uival);
	break;
      case trace_uint32:
	uival = va_arg(args, unsigned int); /* unsigned integer types <= unsigned int are promoted to unsigned int in unprototyped argument lists */
	fprintf(tracefile, "  uint32_t %s = %u\n", varname, uival);
	break;
      case trace_uint64:
	ui64val = va_arg(args, uint64_t);
	fprintf(tracefile, "  uint64_t %s = %" PRIu64 "\n", varname, ui64val);
	break;
      case trace_size:
	size = va_arg(args, size_t);
	fprintf(tracefile, "  size_t %s = %zu\n", varname, size);
	break;
      case trace_int8:
	ival = va_arg(args, int); /* signed integer types <= int are promoted to int in unprototyped argument lists */
	if ((ival > SCHAR_MAX) || (ival < SCHAR_MIN))
	  fprintf(tracefile, "  int8_t %s has impossible value %d\n", varname, ival);
	else
	  fprintf(tracefile, "  int8_t %s = %u\n", varname, ival);
	break;
      case trace_int16:
	ival = va_arg(args, int); /* signed integer types <= int are promoted to int in unprototyped argument lists */
	if ((ival > SHRT_MAX) || (ival < SHRT_MIN))
	  fprintf(tracefile, "  int16_t %s has impossible value %d\n", varname, ival);
	else
	  fprintf(tracefile, "  int16_t %s = %u\n", varname, ival);
	break;
      case trace_int32:
	ival = va_arg(args, int); /* signed integer types <= int are promoted to int in unprototyped argument lists */
	fprintf(tracefile, "  int32_t %s = %u\n", varname, ival);
	break;
      case trace_int64:
	i64val = va_arg(args, int64_t);
	fprintf(tracefile, "  int64_t %s = %" PRId64 "\n", varname, i64val);
	break;
      case trace_string:
	strval = va_arg(args, const char *);
	fprintf(tracefile, "  char *%s = (char *)%" PRIxPTR " :\n", varname, (uintptr_t)strval);
	if (strval)
	{
	  fflush(tracefile); /* In case we are about to look at memory we don't own */
	  fprintf(tracefile, "    \"%s\"\n", strval);
	}
	break;
      case trace_ptr:
	ptrval = va_arg(args, const void *);
	fprintf(tracefile, "  void *%s = (void *)%" PRIxPTR "\n", varname, (uintptr_t)ptrval);
	break;
      case trace_buffer:
	bufval = va_arg(args, const unsigned char *);
	size = va_arg(args, size_t);
	fprintf(tracefile, "  unsigned char *%s /* %zu bytes */ = (unsigned char *)%" PRIxPTR " :\n", varname, size, (uintptr_t)bufval);
	if (bufval)
	{
	  fflush(tracefile); /* In case we are about to look at memory we don't own */
	  for ( ; size >= 16 ; size -= 16, bufval += 16)
	  {
	    fprintf(tracefile, "    %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx %2.2hhx\n",
		bufval[0],
		bufval[1],
		bufval[2],
		bufval[3],
		bufval[4],
		bufval[5],
		bufval[6],
		bufval[7],
		bufval[8],
		bufval[9],
		bufval[10],
		bufval[11],
		bufval[12],
		bufval[13],
		bufval[14],
		bufval[15]
		);
	    fflush(tracefile); /* In case we are about to look at memory we don't own */
	  }
	  if (size)
	  {
	    fputs("   ", tracefile);
	    for (; size ; --size, ++bufval)
	      fprintf(tracefile, " %2.2hhx", *bufval);
	    fputs("\n", tracefile);
	  }
	}
	break;
      case trace_float:
	dblval = va_arg(args, double); /* floats are promoted to double in unprototyped argument lists */
	fprintf(tracefile, "  float *%s = %.6g\n", varname, dblval);
	break;
      case trace_double:
	dblval = va_arg(args, double);
	fprintf(tracefile, "  double *%s = %.12g\n", varname, dblval);
	break;
      case trace_longdbl:
	longdblval = va_arg(args, long double);
	fprintf(tracefile, "  double *%s = %.18Lg\n", varname, longdblval);
	break;
    }
  }
  va_end(args);
}

void trace_init()
{
  tracefile = stderr;
  const char *traceenv;
  const char *tracefileenv;
  traceenv = getenv("Y_TRACE");
  if (!traceenv) return;
  tracing = 1;
  tracefileenv = getenv("Y_TRACE_FILE");
  if (tracefileenv)
  {
    tracefile = fopen (tracefileenv, "w");
    if (!tracefile)
    {
      tracing = 0;
      perror ("Could not open requested trace file for writing, tracing will be disabled");
    }
  }
}
