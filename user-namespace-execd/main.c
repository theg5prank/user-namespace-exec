//
//  main.m
//  user-namespace-execd
//
//  Created by Conor Hughes on 2/19/14.
//
//

#include <stdio.h>

#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <err.h>
#include <sysexits.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>


#include "user-namespace-exec-constants.h"
#include "une_clientServer.h"
#include "une_agentServer.h"

static mach_port_t death_port = MACH_PORT_NULL;

boolean_t server_demux(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{
	return agentipc_server(InHeadP, OutHeadP) || clientipc_server(InHeadP, OutHeadP);
}

static void ipc_loop(mach_port_t port_set)
{
	kern_return_t kr = KERN_SUCCESS;
	typedef union {
		union __RequestUnion__unedaemonserver_clientipc_subsystem req;
		union __ReplyUnion__unedaemonserver_clientipc_subsystem rep;
	} MAX_MSG_BUFFER;
	
	do {
		kr = mach_msg_server_once(server_demux,
								  sizeof(MAX_MSG_BUFFER)+ MAX_TRAILER_SIZE,
								  port_set,
								  0);
		if (kr == MACH_RCV_INTERRUPTED) {
			kr = KERN_SUCCESS;
		}
	} while (kr == KERN_SUCCESS);
	
	errx(EX_OSERR, "mach_msg_server_once() failed: %d: %s", kr, mach_error_string(kr));
}

kern_return_t unedaemonserver_get_agent(mach_port_t sp, uint32_t session_type, 	mach_port_t *out_agent_port, mach_msg_type_name_t *out_agent_portPoly)

{
	*out_agent_port = bootstrap_port;
	*out_agent_portPoly = MACH_MSG_TYPE_COPY_SEND;
	return KERN_SUCCESS;
}

//kern_return_t unedaemonserver_register(mach_port_t server, uint32_t session_type,
//									   mach_port_t agent_bootstrap_port, mach_port_t *out_death_port,
//									   mach_msg_type_name_t *death_portPoly, audit_token_t ucreds)
//kern_return_t unedaemonserver_register(mach_port_t server, uint32_t session_type,
//									   mach_port_t agent_bootstrap_port)
kern_return_t unedaemonserver_register(mach_port_t server, uint32_t session_type,
									   mach_port_t agent_bootstrap_port, mach_port_t *out_death_port,
									   mach_msg_type_name_t *death_portPoly)
{
	*out_death_port = death_port;
	*death_portPoly = MACH_MSG_TYPE_MAKE_SEND;
	
	fprintf(stderr, "Cool sending a port over. We got %d %d", agent_bootstrap_port, session_type);
	
	return KERN_SUCCESS;
}

int main(int argc, const char * argv[])
{
	mach_port_t client_mp = MACH_PORT_NULL;
	mach_port_t agent_mp  = MACH_PORT_NULL;
	mach_port_t port_set = MACH_PORT_NULL;
	
	kern_return_t kr = bootstrap_check_in(bootstrap_port, USER_EXEC_CLIENT_CONNECTION_NAME, &client_mp);
	if (kr != KERN_SUCCESS) {
		errx(EX_UNAVAILABLE, "bootstrap_check_in() failed for client service: %d: %s", kr, mach_error_string(kr));
	}
	
	kr = bootstrap_check_in(bootstrap_port, USER_EXEC_AGENT_CONNECTION_NAME, &agent_mp);
	if (kr != KERN_SUCCESS) {
		errx(EX_UNAVAILABLE, "bootstrap_check_in() failed for agent service: %d: %s", kr, mach_error_string(kr));
	}
	
	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET, &port_set);
	if (kr != KERN_SUCCESS) {
		errx(EX_UNAVAILABLE, "mach_port_allocate() failed to create port set: %d: %s", kr, mach_error_string(kr));
	}
	
	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &death_port);
	if (kr != KERN_SUCCESS) {
		errx(EX_OSERR, "mach_port_allocate() failed to create port: %d: %s", kr, mach_error_string(kr));
	}
	
	kr = mach_port_move_member(mach_task_self(), client_mp, port_set);
	if (kr != KERN_SUCCESS) {
		errx(EX_OSERR, "could not move client port into port set: %d: %s", kr, mach_error_string(kr));
	}
	
	kr = mach_port_move_member(mach_task_self(), agent_mp, port_set);
	if (kr != KERN_SUCCESS) {
		errx(EX_OSERR, "could not move agent port into port set: %d: %s", kr, mach_error_string(kr));
	}
	
	ipc_loop(port_set);
	errx(EX_SOFTWARE, "ipc loop returned?");
    return EX_OSERR;
}
