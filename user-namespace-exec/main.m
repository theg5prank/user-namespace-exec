//
//  main.m
//  user-namespace-exec
//
//  Created by Conor Hughes on 2/19/14.
//
//

#import <Foundation/Foundation.h>
#import <xpc/xpc.h>
#import <sysexits.h>
#import <err.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>

#include "user-namespace-exec-constants.h"
#include "une_daemon.h"

#define WAIT_TIME 2500

int main(int argc, const char * argv[])
{
	if (argc == 1) {
		errx(EX_USAGE, "no argv");
	}
	
	uint32_t pid;
	
	mach_port_t server = MACH_PORT_NULL;
	
	kern_return_t ok = bootstrap_look_up(bootstrap_port, USER_EXEC_CLIENT_CONNECTION_NAME, &server);
	
	if (ok != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't look up service: %s", mach_error_string(ok));
	}
	
	ok = unedaemonclient_get_agent(server, USER_EXEC_SESSION_TYPE_BACKGROUND, &pid, WAIT_TIME);
	
	if (ok != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't get agent: %s", mach_error_string(ok));
	}
	
	printf("%d\n", pid);
    return 0;
}

