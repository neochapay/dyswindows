#include <stdio.h>
#include <string.h>

#include "../libconfig.h"

#define APPNAME "example"



int main(void)
{
    /* here goes the config data */
    Y_config_data *config_data;   

    /* 
     * here goes the values we get from the config file
     * basically these are just null terminated c-strings
     */
    Y_config_value  *cfg_value1;
    Y_config_value  *cfg_value2;
    Y_config_value  *cfg_value3;

    /* we convert two keyvalues to these datatypes later */
    int   cValue2;
    float cValue3;


    /* ================================================= * 
     *      read config file to buffer                   *
     * ================================================= */
    
    config_data = Y_load_config(APPNAME);
    if(config_data == NULL) /* something went wrong */ 
    {
        puts("Y Config Error: Cannot read the config file.\n");
         return -1;
    }


    /* ================================================= * 
     *      get config values from buffer                *
     * ================================================= */

    cfg_value1 = Y_get_value(config_data, "value1", '=');
    if(cfg_value1 == NULL) /* retrieving failed so lets bail out */
    {
        /* print something nice to inform about failure */
        puts("Retrieving keyvalue failed.\n");
        return -1;
    }

    cfg_value2 = Y_get_value(config_data, "value2", '=');
    if(cfg_value2 == NULL) /* same here */
    {
        puts("Retrieving keyvalue failed.\n");
        return -1;
    }

    /* lets try different delimiter */
    cfg_value3 = Y_get_value(config_data, "value3", ':'); 
    if(cfg_value3 == NULL) /* same here */
    {
        puts("Retrieving keyvalue failed.\n");
        return -1;
    }

    /* we dont need config data anymore so lets free it */
//    Y_free_config_data(config_data);

    /* Convert keyvalue strings to integer and float numbers */
    cValue2 = atoi(cfg_value2);
    cValue3 = atof(cfg_value3);

    /* No need for these anymore */
    Y_free_config_value( cfg_value2 );
    Y_free_config_value( cfg_value3 );

  
    /* ================================================= * 
     *      use the values as you please                 *
     * ================================================= */

    /* check the results */
    printf("\nResults:\n"
           "\n  (char)  cfg_value1\t->\t%s"      /* string  */
           "\n   (int)  cfg_value2\t->\t%d"      /* integer */
           "\n (float)  cfg_value3\t->\t%f\n\n", /* float   */
        cfg_value1,
        cValue2,
        cValue3
    );
 
    /* free last buffer */
    Y_free_config_value( cfg_value1 );
    
    return 0;
}
