//
//  main.m
//  user-namespace-exec-agent
//
//  Created by Conor Hughes on 2/19/14.
//
//

#import <Foundation/Foundation.h>
#import <err.h>
#import <sysexits.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>

#include "user-namespace-exec-constants.h"
#include "une_agent.h"

#define WAIT_TIME 2500

void usage(void)
{
	warnx("usage: %s <session>", getprogname());
}

int main(int argc, const char * argv[])
{
	if (argc != 2) {
		usage();
		exit(EX_USAGE);
	}
	
	uint32_t session_id = ~0;
	const char *session = argv[1];
	
	if (!strcasecmp(session, "System")) {
		session_id = USER_EXEC_SESSION_TYPE_SYSTEM;
	} else if (!strcasecmp(session, "Aqua")) {
		session_id = USER_EXEC_SESSION_TYPE_AQUA;
	} else if (!strcasecmp(session, "Background")) {
		session_id = USER_EXEC_SESSION_TYPE_BACKGROUND;
	} else {
		errx(EX_USAGE, "invalid session type %s", session);
	}
	
	mach_port_t server_port = MACH_PORT_NULL;
	mach_port_t death_port = MACH_PORT_NULL;
	
	kern_return_t kr = bootstrap_look_up(bootstrap_port, USER_EXEC_AGENT_CONNECTION_NAME, &server_port);
	
	if (kr != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't look up service: %s", mach_error_string(kr));
	}
	
	kr = unedaemonagent_register(server_port, session_id, bootstrap_port, MACH_MSG_TYPE_COPY_SEND, &death_port, WAIT_TIME);
	
	if (kr != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't register bootstrap port: %s", mach_error_string(kr));
	}
	
	fprintf(stderr, "Cool got death port %d", death_port);
	
	while(1) {
		pause();
	}

    return 0;
}

