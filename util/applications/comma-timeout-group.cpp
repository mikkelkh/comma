// This file is part of comma, a generic and flexible library
// Copyright (c) 2011 The University of Sydney
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Sydney nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


/// @author dmitry mikhin

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_PROCPS_DEV
#include <proc/readproc.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>

#include <boost/lexical_cast.hpp>
#include <boost/assign/list_of.hpp>

#include "../../application/contact_info.h"
#include "../../application/command_line_options.h"
#include "../../base/exception.h"
#include "../../base/types.h"

namespace {

void usage( bool )
{
    static char const * const msg_general =
        "\nRun a given command with its arguments. Kill the command if it is still running after the given"
        "\ntime duration."
        "\n"
        "\nUsage:"
        "\n    comma-timeout-group <options> duration command <args>"
        "\n"
        "\nA drop-in replacement of the standard timeout(1) utility for most common usage patterns."
        "\nThe capability to wait for all processes in a process group added."
#ifndef HAVE_PROCPS_DEV
        "\nWARNING: this new capability is not available in this build. Install procps (or procps-ng)"
        "\ndevelopment package (containing headers and library) and recompile comma-timeout-group."
#endif
        "\nSome options of timeout (1) are not supported, and, if given, this utility exits in error."
        "\n"
        "\nOptions:"
        "\n    -h,--help, print this help and exit"
        "\n    -v,--verbose, chat more"
        "\n    --report-timeout, run silently but print a message if a command times out"
        "\n    --verbose-signal-handler, print messages on stderr when sending signals within signal handler;"
        "\n        WARNING: generally output routines are not re-entrant and shall not be invoked in signal"
        "\n        handlers; use for debugging but not in production code"
        "\n    --preserve-status, exit with the same status as <command>, even when the command timed out"
        "\n    --foreground, not supported"
        "\n    -k, --kill-after=duration, if the command is still running this long after the initial"
        "\n        signal was sent, send the KILL signal to finish it off"
        "\n    --wait-for-process-group=duration, after the initial signal, wait this time for all the processes"
        "\n        in the current process group to finish; if some processes are still left, send the KILL signal"
        "\n        to finish them off (same as -k duration); if both this option and '-k' is given, the duration"
        "\n        specified by '-k' takes precedence"
#ifndef HAVE_PROCPS_DEV
        "\n        WARNING: your version of comma-timeout-group is built without procps support, the capability to"
        "\n        wait for process group is not available, and this options is a synonym to '-k'"
#endif
        "\n    --enforce-group, enforce waiting for process groups; if comma-timeout-group is built without procps"
        "\n        support, '--wait-for-process-group' would exit in error rather then become a synonym to '-k';"
        "\n        this option does nothing if procps support is built in"
        "\n    --wait-for-process-group-delay=value, when waiting for all processes in the group to finish, a delay"
        "\n        is inserted between each parsing of the process tree; the value in microseconds is passed to"
        "\n        usleep (2), default is 100000 (0.1 s); note that low delay values make the program more"
        "\n        responsive at the cost of higher CPU load when parsing the process tree"
#ifndef HAVE_PROCPS_DEV
        "\n        WARNING: your version of comma-timeout-group is built without procps support, this option"
        "\n        has no effect"
#endif
        "\n    -s, --signal=signal, the signal to be sent on timeout, given as a name (HUP, SIGHUP) or number;"
        "\n        only a sub-set of all available signal names is supported, use '--list-known-signals' to list;"
        "\n        arbitrary signal to use can be specified as a number, see 'kill -l' for the values;"
        "\n        by default, use SIGTERM"
        "\n    --list-known-signals, list the supported signals, one per line, and exit"
        "\n    --can-wait-for-process-group, if built with procps library and can wait for process groups, exit"
        "\n        with status success, otherwise, exit with failure"
        "\n"
        "\nAll the timeout durations are specified as floating point numbers with optional suffixes 's' for seconds,"
        "\n(default), 'm' for minutes, 'h' for hours, and 'd' for days. The '--wait-for-process-group' option also"
        "\n accepts special duration 'forever' (equal to max double) given as a literal string (no quotes). If both"
        "\n'--kill-after and --wait-for-process-group' durations are specified, the former takes precedence."
        "\n"
        "\nReturn value:"
        "\n    - if the command times out, exit with status 124"
        "\n    - if the command does not exit on the first signal, and the KILL signal is sent, exit with"
        "\n      status 128+9"
        "\n    - otherwise, exit with the status of command"
        "\n"
        "\nExamples:"
        "\n    Run an application:"
        "\n        comma-timeout-group 10 sleep 1  # exits after 1 second with status 0"
        "\n        comma-timeout-group 1 sleep 10  # exits after 1 second with status 124"
        "\n"
        "\n    Run a bash function with arguments:"
        "\n        comma-timeout-group 10 bash -c \"bash_function 1 2\""
        "\n            watch the quotes; the function must be 'export -f'-ed"
        "\n"
        "\n    Run an application, send KILL signal in 5 seconds if does not die:"
        "\n        comma-timeout-group -k 5 10 sleep 3"
        "\n            if the application does not exit within 5 s in response to the first signal (TERM,"
        "\n            default), send KILL signal"
        "\n"
        "\n    Pass custom signal:"
        "\n        comma-timeout-group --signal=USR1 10 sleep 3"
        "\n            send USR1 after 10 s timeout"
        "\n            send the bash process the USR1 signal if it is still running in 10 s after start"
        "\n            send the entire process group the KILL signal if it is still running after another 5 s"
        "\n"
        "\n    Wait for all processes in the group:"
        "\n        comma-timeout-group --wait-for-process-group=5 10 application"
        "\n            or"
        "\n        comma-timeout-group --wait-for-process-group=5 10 application"
        "\n            wait for 5 s for all processes in the group to exit; if some are left, send KILL"
        "\n            if your version is built without procps support, however, the call above"
        "\n"
        "\n";
    std::cerr << msg_general << comma::contact_info << std::endl << std::endl;
    exit( 0 );
}

// many values are used in the signal handler, no way to pass via arguments, hence, global
// the rest just moved here to keep all in one place
sig_atomic_t timed_out = 0;
int signal_to_use = SIGTERM;  // same default as kill and timeout commands
int child_pid = 0;
bool verbose = false;
bool verbose_signal_handler = false;
bool report_timeout = false;
bool preserve_status = false;
double timeout = 0.0;
double kill_after = 0.0;
#ifdef HAVE_PROCPS_DEV
bool wait_for_process_group = false;
const bool can_wait_for_process_group = true;
unsigned int wait_for_process_group_delay = 100000;
#else
const bool can_wait_for_process_group = false;
#endif

double seconds_from_string( const std::string& s, bool allow_forever = false )
{
    struct impl_ {
        static double from_string( const std::string & s )
        {
            const char & w = *s.rbegin();
            try {
                switch( w )
                {
                    case 'd':
                        return 60 * 60 * 24 * boost::lexical_cast< double >( static_cast< const char * >( &s[0] ), s.size() - 1 );
                    case 'h':
                        return 60 * 60 * boost::lexical_cast< double >( static_cast< const char * >( &s[0] ), s.size() - 1 );
                    case 'm':
                        return 60 * boost::lexical_cast< double >( static_cast< const char * >( &s[0] ), s.size() - 1 );
                    case 's':
                        return boost::lexical_cast< double >( static_cast< const char * >( &s[0] ), s.size() - 1 );
                    default:
                        return boost::lexical_cast< double >( s );
                }
            } catch ( boost::bad_lexical_cast & e ) {
                COMMA_THROW( comma::exception, "cannot convert '" << s << "' to seconds, " << e.what() );
            }
        }
    };
    if ( s.empty() )
    {
        COMMA_THROW( comma::exception, "expected non-empty string" );
    }
    if ( s == "forever" ) {
        if ( !allow_forever ) { COMMA_THROW( comma::exception, "the 'forever' duration is not allowed in this context" ); }
        return std::numeric_limits< double >::max();
    }
    double result = impl_::from_string( s );
    if ( result <= 0.0 )
    {
        COMMA_THROW( comma::exception, "negative time interval '" << s << "'" );
    }
    return result;
}

struct sig2str {
    // use vector and not map to keep things sorted by signal value (int)
    // size is tiny, map probably gives little speed-up in lookup
    typedef std::vector< std::pair< std::string, int > > V;

    static int list_all()
    {
        for ( V::const_iterator i = known_signals.begin(); i != known_signals.end(); ++i ) { std::cout << i->first << std::endl; }
        return 0;
    }

    static int from_string( const std::string & s ) {
        try {
            return boost::lexical_cast< int >( s );
        }
        catch ( boost::bad_lexical_cast & ) {
            // assume is a name
            return from_named_string( s, true );
        }
    }

    private:
        static V known_signals;

        static int from_named_string( const std::string & s, bool recurse ) {
            if ( recurse && 0 == s.compare( 0, 3, "SIG" ) ) {
                // no recursion to disable SIGSIGTERM
                return from_named_string( s.substr( 3 ), false );
            }
            for ( V::const_iterator i = known_signals.begin(); i != known_signals.end(); ++i ) { if ( i->first == s ) return i->second; }
            COMMA_THROW( comma::exception, "unknown signal '" << ( recurse ? "" : "SIG" ) << s << "'" );
        }
};

sig2str::V sig2str::known_signals = boost::assign::list_of< std::pair< std::string, int > > \
           (  "HUP",  SIGHUP ) \
           (  "INT",  SIGINT ) \
           ( "QUIT", SIGQUIT ) \
           ( "KILL", SIGKILL ) \
           ( "USR1", SIGUSR1 ) \
           ( "USR2", SIGUSR2 ) \
           ( "ALRM", SIGALRM ) \
           ( "TERM", SIGTERM ) \
           ( "CONT", SIGCONT ) \
           ( "STOP", SIGSTOP ) ;

#ifdef HAVE_PROCPS_DEV
int parse_process_tree( bool verbose = false )
{
    int ownpid;
    ownpid = getpid();
    int flags = PROC_FILLSTAT;
    if ( verbose ) flags = flags | PROC_FILLCOM;
    PROCTAB* proc = openproc(flags);
    proc_t proc_info;
    memset( &proc_info, 0, sizeof( proc_info ) );
    int first = 1;
    int count = 0;
    while ( readproc( proc, &proc_info ) != NULL ) {
        if ( proc_info.pgrp == ownpid ) {
            if ( first && verbose ) { std::cerr << "extant processes in group " << ownpid << std::endl; first = 0; }
            ++count;
            if ( verbose ) { std::cerr << "    " << proc_info.cmd << ":\t" << proc_info.tid << "\t" << proc_info.pgrp << "\t" << proc_info.state << "\t" << proc_info.start_time << std::endl; }
        }
    }
    closeproc(proc);
    return count;
}

int parse_process_tree_until_empty( bool verbose = false )
{
    int count = 0;
    while ( 1 ) {
        if ( ( count = parse_process_tree( verbose ) ) <= 1 ) break;
        usleep( wait_for_process_group_delay );
    }
    /* shouldn't happen: timeout itself in this process group. */
    if ( !count ) { COMMA_THROW( comma::exception, "error counting processes in the group, none left" ); }
    return count;
}
#endif

void set_alarm( double duration )
{
    sigset_t signal_set;
    sigemptyset( &signal_set );
    sigaddset( &signal_set, SIGALRM );
    if ( sigprocmask( SIG_UNBLOCK, &signal_set, NULL ) != 0 ) { COMMA_THROW( comma::exception, "cannot unblock SIGALRM" ); }

    struct itimerspec its = { { 0, 0 }, { 0, 0 } };
    static const long NSEC_PER_SEC = 1000000000L;
    double sec = std::floor( duration );
    if( sec < std::numeric_limits< time_t >::max() )
    {
        its.it_value.tv_sec = sec;
        double nsec = ( duration - its.it_value.tv_sec ) * 1e9;
        if( nsec < NSEC_PER_SEC ) { its.it_value.tv_nsec = nsec; }
        else if( verbose ) { std::cerr << "comma-timeout-group: warning: nsec duration '" << nsec << "' out of range, using 0" << std::endl; }
    }
    else
    {
        its.it_value.tv_sec = std::numeric_limits< time_t >::max();
        if ( verbose ) { std::cerr << "comma-timeout-group: warning: duration '" << duration << "' out of range, using max duration = " << its.it_value.tv_sec << std::endl; }
    }
    timer_t timerid;
    if ( 0 != timer_create( CLOCK_REALTIME, NULL, &timerid ) ) { COMMA_THROW( comma::exception, "cannot create timer id" ); }
    if ( 0 != timer_settime( timerid, 0, &its, NULL ) ) {
        timer_delete( timerid );
        COMMA_THROW( comma::exception, "cannot set timer" );
    }
}

void signal_handler( int received_signal )
{
    if ( !child_pid ) _exit( 128 + received_signal ); // per shell rules

    if ( received_signal == SIGALRM ) { timed_out = 1; }

    int signal_to_send = signal_to_use;
    if ( kill_after )
    {
        signal_to_use = SIGKILL;
        set_alarm( kill_after );
        kill_after = 0.0;
    }

    // generally would be nice to give more information but very few functions shall be used inside handlers
    // see man 7 signal; can define to 1 for debugging but not for production
    if ( verbose_signal_handler ) { fprintf( stderr, "comma-timeout-group: send signal %d to PID %d\n", signal_to_send, child_pid ); }
    kill( child_pid, signal_to_send ); // child could have created its own group
    // unset first, do not go into a loop
    signal( signal_to_send, SIG_IGN );
    if ( verbose_signal_handler ) { fprintf( stderr, "comma-timeout-group: send signal %d to own group %d\n", signal_to_send, getpid() ); }
    kill( 0, signal_to_send );
}

void initialize_signal_handling( int signal_to_use )
{
     struct sigaction sa;
     sigemptyset( &sa.sa_mask );
     sa.sa_handler = signal_handler;
     sa.sa_flags = SA_RESTART;  // on advice from man 7 signal and man 2 signaction

     sigaction( SIGHUP,  &sa, NULL );
     sigaction( SIGINT,  &sa, NULL );
     sigaction( SIGQUIT, &sa, NULL );
     sigaction( SIGALRM, &sa, NULL );
     sigaction( SIGTERM, &sa, NULL );

     sigaction( signal_to_use, &sa, NULL );

     signal( SIGTTIN, SIG_IGN );
     signal( SIGTTOU, SIG_IGN );
     signal( SIGCHLD, SIG_DFL );
}

int can_wait_group()
{
    if ( verbose ) { std::cerr << "comma-timeout-group: can " << ( can_wait_for_process_group ? "" : "NOT " ) << "wait for process group" << std::endl; }
    return !can_wait_for_process_group;
}

} // anonymous

int main( int ac, char** av ) try
{
    // cannot use comma::command_line_options on the entire command line;
    // the methods options.unnamed, options.exists, etc. will not parse correctly a generic case
    // an example:
    //     comma-timeout-group 10 my-command --verbose --signal=foo
    // the '--verbose' option will be interpreted as given to comma-timeout-group, not to my-command
    // same with '--signal=foo', which will cause comma-timeout-group to fail, but was intended to my-command and possibly can take 'foo' as argument

    // first non-option must be the timeout duration, and second the command to run
    comma::command_line_options all_options( ac, av );
    std::vector< std::string > non_options = all_options.unnamed( "-h,--help,-v,--verbose,--report-timeout,--verbose-signal-handler,--list-known-signals,--foreground,--preserve-status,--enforce-group,--can-wait-for-process-group", "-s,--signal,-k,--kill-after,--wait-for-process-group,--wait-for-process-group-delay" );
    if ( non_options.size() < 2 )
    {
        // user did not give all the arguments; OK in special cases
        verbose = all_options.exists( "-v,--verbose" );
        if ( all_options.exists( "-h,--help" ) ) { usage( true ); }
        if ( all_options.exists( "--list-known-signals" ) ) { return sig2str::list_all(); }
        if ( all_options.exists( "--can-wait-for-process-group" ) ) { return can_wait_group(); }
        std::string timeout = non_options.size() > 0 ? non_options[0] : std::string();
        std::string command = non_options.size() > 1 ? non_options[1] : std::string();
        COMMA_THROW( comma::exception, "please specify timeout and command to run (timeout: '" << timeout << "', command-to-run: '" << command << "', all command line: " << all_options.string() << ")" );
    }

    // split the command line into two: comma-timeout-group itself and the command-to-run
    std::string command_to_run = non_options[1];
    std::vector< std::string >::const_iterator command_to_run_start = std::find( all_options.argv().begin(), all_options.argv().end(), command_to_run );

    unsigned int command_to_run_pos = std::distance( all_options.argv().begin(), command_to_run_start );
    comma::command_line_options options( command_to_run_pos, av, usage );
    // idiosyncratic case when the user first gave sufficient input and then stuck in '--help' or '-list-known-signals'
    if ( options.exists( "--list-known-signals" ) ) { return sig2str::list_all(); }
    preserve_status = options.exists( "--preserve-status" );
    if ( options.exists( "--foreground" ) ) { COMMA_THROW( comma::exception, "--foreground: unsupported option of the original timeout" ); }

    verbose = options.exists( "-v,--verbose" );
    report_timeout = options.exists( "--report-timeout" ) || verbose;
    verbose_signal_handler = options.exists( "--verbose-signal-handler" );

    if ( options.exists( "-s,--signal" ) ) { signal_to_use = sig2str::from_string( options.values< std::string >( "-s,--signal" ).back() ); }

    if ( options.exists( "--can-wait-for-process-group" ) ) { return can_wait_group(); }

    if ( options.exists( "--wait-for-process-group" ) ) {
#ifdef HAVE_PROCPS_DEV
        wait_for_process_group = true;
#else
        if ( options.exists( "--enforce-group" ) ) {
            COMMA_THROW( comma::exception, "built without procps support, cannot wait for process groups" );
        } else {
            if ( verbose ) { std::cerr << "comma-timeout-group: built without procps support, '--wait-for-process-group' is a synonym to '-k'" << std::endl; }
        }
#endif
        kill_after = seconds_from_string( options.values< std::string >( "--wait-for-process-group" ).back(), true );
    }

    if ( options.exists( "-k,--kill-after" ) ) { kill_after = seconds_from_string( options.values< std::string >( "-k,--kill-after" ).back() ); }
    if ( options.exists( "--wait-for-process-group-delay" ) ) {
#ifdef HAVE_PROCPS_DEV
        wait_for_process_group_delay = options.value< unsigned int >( "--wait-for-process-group-delay" );
#else
        if ( verbose ) { std::cerr << "comma-timeout-group: built without procps support, '--wait-for-process-group-delay' is ignored" << std::endl; }
#endif
    }

    timeout = seconds_from_string( non_options[0] );

    // string intended not to run, just to use in error or trace messages
    std::ostringstream oss;
    for ( char **i = av + command_to_run_pos; i < av + ac; ++i ) { oss << " \"" << *i << "\""; };
    std::string command_to_run_with_args = oss.str().substr(1);

    if ( verbose ) {
        std::cerr << "comma-timeout-group:" << std::endl;
        std::cerr << "    running as process: " << getpid() << std::endl;
        std::cerr << "    command-line: " << options.string() << std::endl;
        std::cerr << "    will execute: " << command_to_run_with_args << std::endl;
        std::cerr << "    will time-out this command after " << timeout << " s" << std::endl;
        std::cerr << "    will use signal " << signal_to_use << " to interrupt the command by timeout" << std::endl;
        std::cerr << "    exit status of command: " << ( preserve_status ? "" : "NOT " ) << "preserved" << std::endl;
        if ( verbose_signal_handler ) { std::cerr << "    output messages when sending signals" << std::endl; }
#ifdef HAVE_PROCPS_DEV
        if ( wait_for_process_group ) {
            std::cerr << "    will wait" << ( kill_after < std::numeric_limits< double >::max() ? "" : " forever" ) << " for all processes in the group to finish" << std::endl;
            std::cerr << "    will use " << wait_for_process_group_delay << " microsecond delay between each parsing of the process tree" << std::endl;
        }
#endif
        if ( kill_after != 0 && kill_after < std::numeric_limits< double >::max() ) { std::cerr << "    will send KILL signal " << kill_after << " s after the timeout" << std::endl; }
        std::cerr << std::endl;
    }

    // become a group of our own
    setpgid( 0, 0 );

    // handle signals
    initialize_signal_handling( signal_to_use );

    child_pid = fork();
    if ( child_pid == -1 )
    {
        COMMA_THROW( comma::exception, "fork system call failed" );
    }
    else if ( child_pid == 0 )
    {
        // in the child

        // used to ignore
        signal( SIGTTIN, SIG_DFL );
        signal( SIGTTOU, SIG_DFL );

        int error = execvp( av[command_to_run_pos], av + command_to_run_pos );
        if ( error == -1 ) {
            if ( errno == ENOENT ) {
                std::cerr << "comma-timeout-group: command '" << av[command_to_run_pos] << "' not found" << std::endl;
                _exit(127);
            } else {
                std::cerr << "comma-timeout-group: cannot execute '" << av[command_to_run_pos] << "'" << std::endl;
                _exit(126);
            }
        }
    }

    // in the parent; all management occurs here
    set_alarm( timeout );

    int status;
    do {
        pid_t outcome = waitpid( child_pid, &status, 0 );
        if ( outcome < 0 ) { COMMA_THROW( comma::exception, "waitpid failed, status " << outcome ); }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    if ( verbose ) { std::cerr << "comma-timeout-group: out of waitpid call, child process terminated" << std::endl; }

#ifdef HAVE_PROCPS_DEV
    if ( wait_for_process_group ) {
        if ( verbose ) { std::cerr << "comma-timeout-group: parse the process tree waiting for all processes in the group to finish" << std::endl; }
        int count = parse_process_tree_until_empty( verbose );
        if ( count < 0 ) { COMMA_THROW( comma::exception, "expected at least one process in the group (at least itself), got none" ); }
    }
#endif

    if ( timed_out )
    {
        if ( report_timeout ) { std::cerr << "comma-timeout-group: command " << command_to_run_with_args << " timed-out" << std::endl; }
        if ( preserve_status ) {
            if ( WIFEXITED( status ) ) { return WEXITSTATUS(status); }
            if ( WIFSIGNALED( status ) ) { return 128 + WTERMSIG( status ); } // per shell rules
        }
        return 124;
    }

    if ( WIFEXITED( status ) )
    {
        if ( verbose ) { std::cerr << "comma-timeout-group: application exited" << std::endl; }
        return WEXITSTATUS(status);
    }

    if ( WIFSIGNALED( status ) ) // timed-out also signalled, consider first
    {
        int signal_caught = WTERMSIG( status );
        if ( verbose ) { std::cerr << "comma-timeout-group: application caught signal " << signal_caught << std::endl; }
        return signal_caught + 128; // per shell rules
    }

    std::cerr << "comma-timeout-group: command exited with unknown status " << status << std::endl;
    return 1;
}
catch( std::exception& ex ) { std::cerr << "comma-timeout-group: " << ex.what() << std::endl; exit(1); }
catch( ... ) { std::cerr << "comma-timeout-group: unknown exception" << std::endl; exit(1); }
