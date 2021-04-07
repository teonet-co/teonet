/**
 * Read command line options
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#ifndef HAVE_MINGW
#include <syslog.h>
#endif
#include <libgen.h>
#include <sys/stat.h>
#ifdef HAVE_MINGW
#include <rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include "config/config.h"
#include "utils/utils.h"
#include "config/conf.h"
#include "daemon.h"
#include "ev_mgr.h"

void opt_usage(char *app_name, int app_argc, char** app_argv);

/**
 * Read configuration and command line options
 *
 * This function should be called to read configuration and command line 
 * options
 *
 * @param argc Number of command line arguments
 * @param argv String array with command line arguments
 * @param conf Pointer to teonet_cfg structure to read configuration and
 *             command line parameters to
 * @param app_argc Number of application arguments
 * @param app_argv String array with application arguments names
 * @param app_argv_descr String array with application arguments descriptions
 * @param show_arg Show arguments 
 *
 * @return Entered application arguments
 */
char ** ksnet_optRead(int argc, char **argv, teonet_cfg *conf,
        int app_argc, char **app_argv, char **app_argv_descr, int show_arg) {

    int option_index = 0, opt;
    struct option loptions[] = {
        { "help",                   no_argument,       0, 'h' },
        { "version",                no_argument,       0, 'v' },
        { "app_name",               no_argument,       0,  0  },
        { "app_description",        no_argument,       0,  0  },
        { "uuid",                   no_argument,       0,  0  },
        { "port",                   required_argument, 0, 'p' },
        { "port_increment_disable", no_argument,       &conf->port_inc_f, 0 },
        { "r_port",                 required_argument, 0, 'r' },
        { "r_tcp_port",             required_argument, 0, 't' },
        { "r_address",              required_argument, 0, 'a' },
        { "r_tcp",                  no_argument,       &conf->r_tcp_f, 1 },
        { "network",                required_argument, 0, 'n' },
        { "key",                    required_argument, 0, 'e' },
        { "auth_secret",            required_argument, 0, 'u' },
        { "tcp_allow",              no_argument,       &conf->tcp_allow_f, 1 },
        { "tcp_port",               required_argument, 0, 'o' },
        { "l0_allow",               no_argument,       &conf->l0_allow_f, 1 },
        { "l0_tcp_port",            required_argument, 0, 'l' },
        { "l0_tcp_ip_remote",       required_argument, 0, 'I' },
        { "filter",                 required_argument, 0, 'f' },
        { "hot_keys",               no_argument,       &conf->hot_keys_f, 1 },
        { "show_debug",             no_argument,       &conf->show_debug_f, 1 },
        { "show_debug_vv",          no_argument,       &conf->show_debug_vv_f, 1 },
        { "show_debug_vvv",         no_argument,       &conf->show_debug_vvv_f, 1 },
        { "show_connect",           no_argument,       &conf->show_connect_f, 1 },
        { "show_peers",             no_argument,       &conf->show_peers_f, SHOW_PEER_CONTINUOSLY },
        { "show_trudp",             no_argument,       &conf->show_tr_udp_f, SHOW_PEER_CONTINUOSLY },
        #if M_ENAMBE_VPN
        { "vpn_start",      no_argument,       &conf->vpn_connect_f, 1 },
        { "vpn_ip",         required_argument, 0, 'i' },
        { "vpn_mtu",        required_argument, 0, 'm' },
        #endif
        #if M_ENAMBE_LOGGING_SERVER
        { "logging",        no_argument,       &conf->logging_f, 1 },
        #endif
        #if M_ENAMBE_LOGGING_CLIENT
        { "log_disable",    no_argument,       &conf->log_disable_f, 1 },
        { "send_all_logs",  no_argument,       &conf->send_all_logs_f, 1 },
        #endif

        { "l0_public_ipv4",   required_argument, 0, '4' },
        { "l0_public_ipv6",   required_argument, 0, '6' },

        { "statsd_ip",      required_argument, 0, 's' },
        { "statsd_port",    required_argument, 0, 'S' },
        { "statsd_peers",   no_argument,       &conf->statsd_peers_f, 1 },

        { "sig_segv",       no_argument,       &conf->sig_segv_f, 1 },
        { "log_priority",   required_argument, 0, 'L' }, 
        { "color_output_disable", no_argument, &conf->color_output_disable_f, 1 },
        { "extended_l0_log", no_argument,      &conf->extended_l0_log_f, 1 },
        { "block_cli_input", no_argument,      &conf->block_cli_input_f, 1 },        
        { "no_multi_thread", no_argument,      &conf->no_multi_thread_f, 1 },
        { "send_ack_event", no_argument,       &conf->send_ack_event_f, 1 },
        
        { "daemon",         no_argument,       &conf->dflag, 1 },
        { "kill",           no_argument,       &conf->kflag, 1 },
        { 0, 0, 0, 0 }
    };


    char *data_path = getDataPath();
    #ifdef DEBUG_KSNET
    printf("Current data path: %s\n", data_path);
    #endif

    // Create data folder (if absent)
    #ifdef HAVE_MINGW
    mkdir(data_path);
    #else
    mkdir(data_path, 0755);
    #endif

    free(data_path);

    // initialize random seed
    srand(time(NULL));

    // Read options

    // Initialize getopt()
    optind = 0;

    // Check online parameters
    for(;;) {

      opt = getopt_long (argc, argv, "?hva:I:dkl:n:o:p:P:r:t:f:", loptions,
                         &option_index);
      if (opt==-1)
      {
          //usage();
          //exit(EXIT_SUCCESS);
          break;
      }
      switch (opt) {

        case 0:
          if(!strcmp(loptions[option_index].name, "app_name")) {
              printf("Name of this application is: %s\n\n", conf->app_name);
              exit(EXIT_SUCCESS);
          }

          else if(!strcmp(loptions[option_index].name, "app_description")) {
              printf("Application description:\n%s\n\n",
                     conf->app_description);
              exit(EXIT_SUCCESS);
          }

          else if(!strcmp(loptions[option_index].name, "uuid")) {

              printf("UUID: ");
              #ifndef HAVE_MINGW
              size_t i;
              char pl = 4;
              uuid_t uuid;
              uuid_generate(uuid);
              for(i = 0; i < sizeof uuid; i ++) {
                  if(i && i <= 10 && !(i % pl)) {
                      printf("-");
                      pl = 2;
                  }
                  printf("%02x", uuid[i]);
              }
              #else
              UUID uuid;
              unsigned char *uuid_str;
              UuidCreateNil(&uuid);
              UuidCreate(&uuid);
              UuidToString(&uuid, &uuid_str);
              printf("%s", uuid_str);
              RpcStringFree(&uuid_str);
              #endif
              printf("\n\n");
              exit(EXIT_SUCCESS);
          }

          break;

        case '0': break;

        case '?':
        case 'h':
          opt_usage(argv[0], app_argc, app_argv);
          exit(EXIT_SUCCESS);
          break;

        case 'v':
          printf("The current version of the %s is %s\n\n",
                 PACKAGE_NAME, VERSION);
          exit(EXIT_SUCCESS);
          break;

        case 'p':
          conf->port = atoi(optarg);
          break;

        case 'P':
        case 'r':
          conf->r_port = atoi(optarg);
          break;
          
        case 't':
          conf->r_tcp_port = atoi(optarg);
          break;

        case 'a': {
          strncpy((char*)conf->r_host_addr_opt, optarg, KSN_BUFFER_SM_SIZE/2);
          resolveDnsName(conf);
        } break;

        case '4':
        {
          strncpy((char*)conf->l0_public_ipv4, optarg, KSN_BUFFER_SM_SIZE/2);
        }
        break;
        case '6':
        {
          strncpy((char*)conf->l0_public_ipv6, optarg, KSN_BUFFER_SM_SIZE/2);
        }
        break;
        case 'i':
          strncpy((char*)conf->vpn_ip, optarg, KSN_BUFFER_SM_SIZE/2);
          break;

        case 'm':
          conf->vpn_mtu = atoi(optarg);
          break;

        case 's':
          strncpy((char*)conf->statsd_ip, optarg, KSN_BUFFER_SM_SIZE/2);
          break;

        case 'S':
          conf->statsd_port = atoi(optarg);
          break;
          
        case 'L':
          conf->log_priority = atoi(optarg);
          break;

        case 'n':
          strncpy((char*)conf->network, optarg, KSN_BUFFER_SM_SIZE/2);
          break;
          
        case 'e':
          strncpy((char*)conf->net_key, optarg, KSN_BUFFER_SM_SIZE/2);
          break;
          
        case 'o':
          conf->tcp_port = atoi(optarg);
          break;

        case 'l':
          conf->l0_tcp_port = atoi(optarg);
          break;
          
        case 'I':
          strncpy((char*)conf->l0_tcp_ip_remote, optarg, KSN_BUFFER_SM_SIZE/2);
          break;
          
        case 'f':
          strncpy((char*)conf->filter, optarg, KSN_BUFFER_SM_SIZE/2);
          break;
         
        case 'd':
          // Start this application in Daemon mode
          conf->dflag = 1;
          break;

        case 'k':
          // Kill application started in Daemon mode
          conf->kflag = 1;
          break;

        case 'u'://NOTE: both a and s are already used, so aUth_secret
          strncpy((char*)conf->auth_secret, optarg, KSN_BUFFER_SM_SIZE/2);
          break;
      }
    }

    // Check for arguments
    if((argc - optind) < app_argc) {

        int i;

        fprintf(stderr, "\nExpected argument%s:",
                ((app_argc - (argc - optind)) > 1 ? "s" : ""));
        // Show expected arguments
        for(i = argc - optind; i < app_argc; i++) {
            printf(" %s", app_argv[i]);
        }
        printf("\n\n");
        
        // Parameters description
        if(app_argv_descr != NULL) {
            
            int number_of_parameters = 0;
            char *descr_str = ksnet_formatMessage("where:\n\n");
            for(i = argc - optind; i < app_argc; i++) {
                if(app_argv_descr[i] != NULL) {
                    descr_str = ksnet_sformatMessage(descr_str, 
                            "%s  %s - %s\n", descr_str, app_argv[i], app_argv_descr[i]);
                    number_of_parameters++;
                }
            }
            descr_str = ksnet_sformatMessage(descr_str, "%s\n", descr_str);
            if(number_of_parameters) /*printf*/ puts(descr_str);
            free(descr_str);
        }
        opt_usage(argv[0], app_argc, app_argv);
        exit(EXIT_FAILURE);
    }
    else if((argc - optind) > app_argc) {

        int i, extraind = optind + app_argc;
        fprintf(stderr, "Extra argument%s:", (argc - extraind > 1 ? "s" : ""));
        // Show extra arguments
        for(i = extraind; i < argc; i++) {
            printf(" %s", argv[i]);
        }
        printf("\n\n");
        opt_usage(argv[0], app_argc, app_argv);
        exit(EXIT_FAILURE);
    }

    // Set Host name
    strncpy(conf->host_name, argv[optind], KSN_MAX_HOST_NAME);
    
    // Show arguments
    if(show_arg) {

        // Show network
        printf("Network: %s\n", conf->network);

        int i;
        for(i = 0; i < app_argc; i++) {
            printf("%s: %s\n", app_argv[i], argv[optind+i]);
        }
        printf("\n");
    }      

    #ifndef HAVE_MINGW
    // Start or stop application in daemon mode
    start_stop_daemon(argv, conf);
    #endif

    // Change configuration for daemon mode
    if(conf->dflag) {
        conf->show_connect_f = 0;
        conf->show_debug_f = 0;
        conf->hot_keys_f = 0;
    }    

    return &argv[optind];
}

/**
 * Show online parameters usage help
 */
void opt_usage(char *app_name, int app_argc, char** app_argv) {

    int i;

    // Create arguments string
    char app_argv_str[KSN_BUFFER_SIZE], 
         *app_name_cpy = strdup(app_name);
    
    // Remove "lt-" from application name
    char *ptr;
    if( (ptr = strstr(app_name_cpy,"lt-")) != NULL) {
        
        int len = strlen(app_name_cpy) + 1;
        memmove(ptr, ptr + 3, len - ((void*)ptr - (void*)app_name_cpy));
    }
    
    app_argv_str[0] = 0;
    for(i = 0; i < app_argc; i++) {
        strncat(app_argv_str, " ", KSN_BUFFER_SIZE - strlen(app_argv_str) - 1);
        strncat(app_argv_str, app_argv[i], KSN_BUFFER_SIZE - strlen(app_argv_str) - 1);
    }

    printf("Usage: %s [OPTIONS]%s\n"
    "\n"
    "Options:\n"
    "\n"
    "  -h,  --help               SHow this help message\n"
    "  -v,  --version            Show current version\n"
    "       --uuid               Generate new uuid\n"
    "       --app_name           Show this application name\n"
    "       --app_description    Show this application description\n"
    "  -n,  --network=value      Set network name\n"
    "       --key=value          Set network key\n"
    "       --auth_secret=value  Set auth key\n"
    "  -p,  --port=value         Set port number (default "KSNET_PORT_DEFAULT")\n"
    "       --port_increment     Increment port if busy\n"
    "  -a,  --r_address=value    Set remote server address (default localhost)\n"
    "  -P|r --r_port=value       Set remote server port number (default "KSNET_PORT_DEFAULT")\n"
    "       --r_tcp              Connect to remote TCP Proxy port\n"            
    "  -t,  --r_tcp_port=value   Set remote server TCP port number (default "KSNET_PORT_DEFAULT")\n"
    "       --tcp_allow          Allow TCP Proxy connection to this server\n"
    "  -o,  --tcp_port=value     TCP Proxy port number (default "KSNET_PORT_DEFAULT")\n"
    "       --l0_allow           Allow L0 Server and l0 clients connection\n"
    "  -l,  --l0_tcp_port=value  L0 Server TCP port number (default "KSNET_PORT_DEFAULT")\n"
    "  -I,  --l0_tcp_ip=value    L0 Server remote IP address (send to clients)\n"
    "  -f,  --filter=value       Set display log filter\n"
    "       --hot_keys           Switch on the hot keys monitor after starts\n"
    #ifdef DEBUG_KSNET
    "       --show_debug         Show debug messages\n"
    "       --show_debug_vv      Show debug additional messages\n"
    #endif
    "       --show_connect       Show connection messages\n"
    "       --show_peers         Show peers screen after connection\n"
    "       --show_trudp         Show TR-UDP statistic after connection\n"
    #if M_ENAMBE_VPN
    "       --vpn_start          Start VPN\n"
    "       --vpn_ip             VPN IP\n"
    "       --vpn_mtu            VPN MTU\n"
    #endif
    #if M_ENAMBE_LOGGING_SERVER
    "       --logging            Start logging server\n"
    #endif
    #if M_ENAMBE_LOGGING_CLIENT
    "       --log_disable        Disable send logs to logging servers\n"
    "       --send_all_logs      Send all logs (by default send only metrics)\n"
    #endif
    "\n"
    "       --l0_public_ipv4     Set public ipv4 (to use in CMD_GET_PUBLIC_IP)\n"
    "       --l0_public_ipv6     Set public ipv6 (to use in CMD_GET_PUBLIC_IP)\n"
    "\n"
    "       --statsd_ip          Metric exporter IP address\n"
    "       --statsd_port        Metric exporter Port number\n"
    "       --statsd_peers       Send preers metrics\n"
    "\n"
    "       --sig_segv           Segmentation fault error processing by library\n"
    "       --log_priority       Syslog priority (Default: 4):\n"
    "                            DEBUG: 4, MESSAGE: 3, CONNECT: 2, ERROR_M: 1,\n"
    "                            NO_LOG: 0\n"
    "\n"
    "       --color_output       Disable color output in stdout terminal logs,\n"
    "                            (full flag name is: --color_output_disable)\n"
    "\n"
    "       --extended_l0_log    Extends L0 server DEBUG log\n"
    "\n"
    "       --no_multi_thread    Don't check multi thread mode in async calls\n"   
    "       --send_ack_event     Send ACK event when cmd delivered\n"
    "\n"
    "  -d, --daemon              Start this application in daemon mode\n"
    "  -k, --kill                Kill the application running in daemon mode\n"
    "\n",
    basename(app_name_cpy), app_argv_str);

    free(app_name_cpy);
}

/**
 * Set application name
 *
 * Set application name, prompt and description (use it before ksnet_optRead)
 */
void ksnet_optSetApp(teonet_cfg *conf,
                     const char* app_name,
                     const char* app_prompt,
                     const char* app_description) {

    const char *lt = "lt-"; 
    const size_t lt_len = 3, ptr = !strncmp(app_name, lt, lt_len) ? lt_len : 0;
    
    strncpy(conf->app_name, app_name + ptr, KSN_BUFFER_SM_SIZE/2);
    strncpy(conf->app_prompt, app_prompt, KSN_BUFFER_SM_SIZE/2);
    strncpy(conf->app_description, app_description, KSN_BUFFER_SM_SIZE/2);
}
