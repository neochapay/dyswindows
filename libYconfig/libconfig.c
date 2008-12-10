#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "libconfig.h"

_Y_config_value_t *Y_get_value(const char *config_data, const char *key, const char delimiter)
{   
    char *keyValue;

    int setlen = strlen(key);
    int valLen = 0;
 
    char *ptr = strstr(config_data, key);  
    if(ptr == NULL)
    #if Y_ERROR_STRINGS
        /* return Y_NOT_FOUND; */
    #else
        return NULL;
    #endif  

    ptr += setlen;
    if((ptr[0] != '=') && (ptr[0] != ' '))
    #if Y_ERROR_STRINGS
        /* return Y_NOT_FOUND; */
    #else
        return NULL;
    #endif

    
    
    setlen = 0;
    while(ptr[valLen] != '\r' && ptr[valLen] != '\n')
    {
        if(ptr[valLen] == delimiter)
        {
            setlen = 1;
            break;
        }
        valLen++;
    }     
    if(setlen < 1) 
    #if Y_ERROR_STRINGS
        /* return Y_NO_DELIMITER; */
    #else
        return NULL;
    #endif    

   
    /* I think this needs some rechecking */
    ptr = strchr(ptr, delimiter);
    if(ptr == NULL)
    #if Y_ERROR_STRINGS
        /* return Y_UNKNOWN_ERROR; */
    #else
        return NULL;
    #endif
    ptr++;


    valLen = 0;
    setlen = 0;
    while(ptr[valLen] != '\r' && ptr[valLen] != '\n')
    {
        if(ptr[valLen] == '#') break; /* comment line */

        if(ptr[valLen] == ' ' && setlen == 0) ptr++;
        else
        {
            if(ptr[valLen] == ' ' && ptr[valLen+1] == ' ') break;
            else
            {
                if(setlen == 0) setlen = 1;
                valLen++;
            }
        }
    }
    if(valLen < 1)
    #if Y_ERROR_STRINGS
        /* return Y_CFG_EMPTY; */
    #else
        return NULL;
    #endif


    keyValue = malloc ( valLen * sizeof(keyValue) + 1);
    if(keyValue == NULL)
    #if Y_ERROR_STRINGS
        /* return Y_ALLOC_FAILURE; */
    #else
        return NULL;
    #endif

    memcpy(keyValue, ptr, valLen);
    keyValue[valLen] = 0;

    return keyValue;
    /* return Y_OK; */
}

void Y_free_config_value(_Y_config_value_t *keyValue)
{
    if(keyValue) free(keyValue);
}

_Y_config_data_t *Y_load_config(char *cAppName)
{
    _Y_config_data_t *config_data;
    FILE *cFile;
    long int fsize;

    cFile = fopen(cAppName, "rb");
    if(cFile == NULL) return NULL; /* File reading failed */

    if(fseek(cFile, 0, SEEK_END) != 0) return NULL; /* fseek failed */
    
    fsize = ftell(cFile);
    if(fsize < 0) return NULL; /* ftell failed */
    rewind(cFile);

    config_data = malloc((size_t)fsize * sizeof(char) + 1);
    if(config_data == NULL) return NULL; /* Memory allocating failed */
    
    fread(config_data, (size_t)fsize, sizeof(char), cFile);
    if(ferror(cFile))
    {
        if(config_data) free(config_data);
        fclose(cFile);
        return NULL; /* fread failed */  
    }
    config_data[fsize] = '\0';
  
    if((fclose(cFile)) == EOF)
    {
        if(config_data) free(config_data);
        return NULL; /* fclose failed */
    }

    return config_data;
}

#if Y_ERROR_STRINGS
char *Y_error_string(int error)
{
    switch(error)
    {
        /* global errors */
        case Y_NO_ERROR:      return "Y Config: No error.";
        case Y_ERROR:         return "Y Config: Error occured.";
        case Y_UNKNOWN_ERROR: return "Y Config: Unknown error.";

        /* getConfig errors */
        case Y_NOT_FOUND:     return "Y Config: key not found.";
        case Y_NO_DELIMITER:  return "Y Config: Delimiter not found.";
        case Y_CFG_EMPTY:     return "Y Config: Key value empty.";        
        case Y_ALLOC_FAILURE: return "Y Config: Data allocation failed.";

        default:    return "This shouldnt happen.";
    }
}
#endif
