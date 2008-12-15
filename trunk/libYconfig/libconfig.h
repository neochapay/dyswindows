#ifndef LIB_Y_CONF_H
#define LIB_Y_CONF_H

#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"{
#endif

#define VER "0.0.1"
#define Y_ERROR_STRINGS 0

#define Y_config_value   _Y_config_value_t
#define Y_config_data    _Y_config_data_t

/* debug mode (under development) */
#if Y_ERROR_STRINGS
enum
{
    /* global errors */
    Y_CONFIG_UNKNOWN_ERROR  = -999,

    /* getConfig errors */
    Y_CONFIG_NOT_FOUND      = -10,
    Y_CONFIG_NO_DELIMITER   = -11,
    Y_CONFIG_CFG_EMPTY      = -12,
    Y_CONFIG_ALLOC_FAILURE  = -13
};
#endif

#define Y_CONFIG_NO_ERROR  0
#define Y_CONFIG_ERROR     1

/* cfg data struct */
typedef char _Y_config_data_t;

/* cfg item struct */
typedef char _Y_config_value_t;

/* get config data from file */
_Y_config_data_t *Y_load_config(const char *cAppName);

/* free config data */ 
void Y_free_config_data(_Y_config_data_t *config_data);

/* get key value from buffer */
_Y_config_value_t *Y_get_value(const char *config_data, const char *key, const char delimiter);

/* free key value*/
void Y_free_config_value(_Y_config_value_t *setdata);

/* debug mode (under development) */
#if Y_ERROR_STRINGS
char *Y_error_string(int error);
#endif

#ifdef __cplusplus
}
#endif

#endif
