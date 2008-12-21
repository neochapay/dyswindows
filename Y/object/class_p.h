#ifndef Y_OBJECT_CLASS_P_H
#define Y_OBJECT_CLASS_P_H

#include <Y/y.h>
#include <Y/object/class.h>
#include <Y/object/object.h>
#include <Y/message/message.h>

struct Class;

#if 0
struct Method
{
  const char *name;
  struct Tuple *(*m)(struct Object *, const struct Tuple *);
};

struct Class
{
  const char *       name;
  int                id;
  struct Class *     super;
  struct Object *  (*instantiate)(const struct Tuple *);
  void             (*destroy)(struct Object *);
/*   void             (*despatch)(struct Object *, struct Message *); */
  int                methodCount;
#if __GNUC__ >= 3
  struct Method      methods[]; 
#else
# ifndef CLASS_MAX_METHODS
#  define CLASS_MAX_METHODS 64
# endif
  struct Method      methods[CLASS_MAX_METHODS];
#endif
};
#endif

#endif


