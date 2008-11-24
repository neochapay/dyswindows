#ifndef Y_TRACE_TRACE_H
#define Y_TRACE_TRACE_H
enum tracetype {
  trace_uint8,  /* uint8_t                                                        */
  trace_uint16, /* uint16_t                                                       */
  trace_uint32, /* uint32_t                                                       */
  trace_uint64, /* uint64_t                                                       */
  trace_size,   /* size_t                                                         */
  trace_int8,   /* uint8_t                                                        */
  trace_int16,  /* uint16_t                                                       */
  trace_int32,  /* uint32_t                                                       */
  trace_int64,  /* uint64_t                                                       */
  trace_string, /* char *, printed as a NULL-terminated string unless ptr is NULL */
  trace_ptr,    /* void *, pointer printed as hex, not dereferenced               */
  trace_buffer, /* char *, contents printed as hex unless ptr is NULL             */
  trace_float,  /* float                                                          */
  trace_double, /* double                                                         */
  trace_longdbl /* long double                                                    */
};

#ifdef TRACE_ON
extern void trace_init(void);
extern void trace_
(
  const char *file,
  const int line,
  const char *function,
  const char *comment,
  ...
  /* Where ... is a sequence of these 3[4] parameters:
   * 
   * const char *variablename,
   * enum tracetype type,
   * const <type> variable,
   * [size_t size,] <- only for trace_buffer
   * 
   * You can pass-in as many parameters as you like.
   *
   * The final parameter must be a NULL, and it must appear in 
   * a position which would otherwise be a valid place to find
   * a variablename.
   *
   * Basically, save yourself some effort and use the macros.
   */
);
#  define trace_tostr(a) #a
#  define trace(comment,...) \
  trace_                     \
  (                          \
    __FILE__,                \
    __LINE__,                \
    __PRETTY_FUNCTION__,     \
    comment, ## __VA_ARGS__, \
    NULL                     \
  )
#  define trace_var(a,b) trace_tostr(a), b, a
#  define trace_buf(a,b) trace_tostr(a), trace_buffer, a, (size_t)b
#else
#  define trace_init()
#  define trace_()
#  define trace(a,...)
#  define trace_var(a,b)
#  define trace_buf(a,b)
#endif
#endif
