//
//  main.m
//  user-namespace-exec
//
//  Created by Conor Hughes on 2/19/14.
//
//

#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <err.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>

#include "user-namespace-exec-constants.h"
#include "une_client.h"

#define WAIT_TIME 2500

int main(int argc, const char * argv[])
{
	if (argc == 1) {
		errx(EX_USAGE, "no argv");
	}
	
	mach_port_t server = MACH_PORT_NULL;
	mach_port_t new_bsport = MACH_PORT_NULL;
	
	kern_return_t kr = bootstrap_look_up(bootstrap_port, USER_EXEC_SERVICE_NAME, &server);
	
	if (kr != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't look up service: %s", mach_error_string(kr));
	}
	
	if (strcmp(argv[1], "--dump") == 0) {
		kr = unedaemonclient_dump_state(server);
		if (kr != KERN_SUCCESS) {
			errx(EX_SOFTWARE, "Could not send dump state ping: %s", mach_error_string(kr));
		} else {
			exit(EXIT_SUCCESS);
		}
	}
	
	unedaemonclient_get_agent(server, USER_EXEC_SESSION_TYPE_BACKGROUND, &new_bsport, WAIT_TIME);
	
	if (kr != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't get agent: %s", mach_error_string(kr));
	} else if (new_bsport == MACH_PORT_NULL || new_bsport == MACH_PORT_DEAD) {
		errx(EX_SOFTWARE, "Got bogus bootstrap port 0x%08x", new_bsport);
	}
	
	kr = task_set_bootstrap_port(mach_task_self(), new_bsport);
	
	if (kr != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't set bootstrap port: %s", mach_error_string(kr));
	}
	
	execvp(argv[1], (char * const *)argv + 1);
	
	err(EXIT_FAILURE, "execvp");
}

