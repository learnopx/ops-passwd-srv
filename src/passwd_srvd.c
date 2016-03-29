/*
 * (c) Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

/************************************************************************//**
 * @ingroup passwd-srvd
 *
 * @file
 * Main source file for the Password Server daemon.
 *
 *    Password server serves other modules in OpenSwitch to perform password
 *     change for the user.
 *
 *    Its purpose in life is:
 *
 *       1. During start up, open UNIX domain socket to listen for password
 *           change request
 *       2. During operations, receive {username, old-password, new-password}
 *           to make password change for username.
 *       3. Manage /etc/shadow file to update password for a given user
 ***************************************************************************/
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <shadow.h>

#include <syslog.h>
#include <stdio.h>
#include <crypt.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <limits.h>

//#include "openvswitch/vlog.h"

#include <util.h>

#include <daemon.h>
#include <dirs.h>
#include <unixctl.h>
#include <fatal-signal.h>
#include <command-line.h>
#include <vswitch-idl.h>
#include <openvswitch/vlog.h>

//#include "eventlog.h"
#include "passwd_srv_pri.h"

//VLOG_DEFINE_THIS_MODULE(passwd-srv);

static char *
passwd_srv_parse_options(int argc, char *argv[], char **unixctl_pathp)
{
    enum {
        OPT_UNIXCTL = UCHAR_MAX + 1,
        VLOG_OPTION_ENUMS,
        DAEMON_OPTION_ENUMS,
        OVSDB_OPTIONS_END,
    };

    static const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"unixctl", required_argument, NULL, OPT_UNIXCTL},
        DAEMON_LONG_OPTIONS,
        VLOG_LONG_OPTIONS,
        {"ovsdb-options-end", optional_argument, NULL, OVSDB_OPTIONS_END},
        {NULL, 0, NULL, 0},
    };
    char *short_options = long_options_to_short_options(long_options);

    for (;;) {
        int c;
        int end_options = 0;

        c = getopt_long(argc, argv, short_options, long_options, NULL);
        if (c == -1) {
            break;
        }

        switch (c) {

        VLOG_OPTION_HANDLERS
        DAEMON_OPTION_HANDLERS

        default:
            end_options = 1;
            break;
        }
        if (end_options)
            break;
    }
    free(short_options);

    argc -= optind;
    argv += optind;

    return NULL;
} /* passwd_srv_parse_options */

/**
 * Setup directory in /var/run to store password server related files
 */
static void
create_directory()
{
    char *dir_name = "/var/run/ops-passwd-srv";
    struct stat f_stat = {0};

    if (0 > stat(dir_name, &f_stat))
    {
        /*/var/run/ops-passwd-srv doesn't exist, create one */
        mkdir(dir_name, S_IRWXU | S_IRWXG | S_IRWXO);
    }
    else if (!S_ISDIR(f_stat.st_mode))
    {
        /* ops-passwd-srv exists but not directory */
        if (0 != remove(dir_name))
        {
            /* cannot remove file, exit */
            /* TODO: logging for remove file failure */
            exit(-1);
        }
    }
}

/* password server main function */
int main(int argc, char **argv) {
    set_program_name(argv[0]);
    proctitle_init(argc, argv);
    fatal_ignore_sigpipe();

    /* assign program name */
    passwd_srv_parse_options(argc, argv, NULL);

    /* Fork and return in child process; but don't notify parent of
     * startup completion yet. */
    daemonize_start();

    create_directory();

    /* Notify parent of startup completion. */
    daemonize_complete();

    /* TODO: initialize event log */
    // event_log_init("PASSWD");
    /* TODO: initialize vlog */

    /* initialize socket connection */
    listen_socket();

    return 0;
}