#include <libconfig.h>

int main(int argc, char *argv[]){

    //Read config file
        config_t cfg;
        config_init(&cfg);

        if (!config_read_file(&cfg, "./trash-ship-client/client_config.conf"))
        {
            fprintf(stderr, "error\n");
            config_destroy(&cfg);
            return 1;
        }

        int parametro_1_int;
        config_lookup_int(  &cfg, 
                            "universe.size", 
                            &parametro_1_int);

        printf("Max parametro_1_int: %d\n", parametro_1_int);

        config_destroy(&cfg);
        
    return 0;
}
