#ifndef Y_CONST_H
#define Y_CONST_H

/* Y constants */

enum YMessageOperation
{
  YMO_ERROR,
  YMO_QUIT,
  YMO_EVENT,
  YMO_FIND_CLASS,
  YMO_INVOKE_CLASS_METHOD,
  YMO_INVOKE_INSTANCE_METHOD,
};

enum YUnixControlMessageType
  {
    ucmtNewChannel,
    ucmtAuthenticate
  };

#endif /* header guard */

/* arch-tag: 018dc291-69c6-4595-aa83-ba1a12ab326c
 */
