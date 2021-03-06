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
#include <bsm/libbsm.h>
#include <stdlib.h>
#include <assert.h>

#include "user-namespace-exec-constants.h"
#include "une_clientServer.h"
#include "une_agentServer.h"

struct user_bootstrap_registration {
	uid_t uid;
	mach_port_t bsports[USER_EXEC_SESSION_TYPE_MAX + 1];
};

static mach_port_t death_port = MACH_PORT_NULL;

static struct user_bootstrap_registration *bootstrap_registrations = NULL;
static size_t num_bootstrap_registrations = 0;
static size_t bootstrap_registrations_size = 0;

static struct user_bootstrap_registration *uid_to_bootstrap_registration(uid_t uid)
{
	for (size_t i=0; i < num_bootstrap_registrations; i++) {
		if (bootstrap_registrations[i].uid == uid) {
			return &bootstrap_registrations[i];
		}
	}
	
	if (num_bootstrap_registrations >= bootstrap_registrations_size) {
		size_t newsize = sizeof(*bootstrap_registrations) * (num_bootstrap_registrations ? num_bootstrap_registrations * 2 : 8);
		bootstrap_registrations = realloc(bootstrap_registrations, newsize);
		if (!bootstrap_registrations) { err(EX_OSERR, "realloc failed, allocation size %zu", newsize); }
		bootstrap_registrations_size = newsize;
	}
	
	struct user_bootstrap_registration *registration = &bootstrap_registrations[num_bootstrap_registrations++];
	
	registration->uid = uid;
	for (int i=0; i < sizeof(registration->bsports) / sizeof(registration->bsports[0]); i++) {
		registration->bsports[i] = MACH_PORT_NULL;
	}
	
	return registration;
}

static void dump_registrations(FILE *stream)
{
	fprintf(stream, "--BEGIN REGISTRATION DUMP");
	for (int i=0; i < num_bootstrap_registrations; i++) {
		fprintf(stream, "\n");
		fprintf(stream, "%d:\n", bootstrap_registrations[i].uid);
		fprintf(stream, "\tSystem:\t0x%08x\n", bootstrap_registrations[i].bsports[USER_EXEC_SESSION_TYPE_SYSTEM]);
		fprintf(stream, "\tBackground:\t0x%08x\n", bootstrap_registrations[i].bsports[USER_EXEC_SESSION_TYPE_BACKGROUND]);
		fprintf(stream, "\tAqua:\t0x%08x\n", bootstrap_registrations[i].bsports[USER_EXEC_SESSION_TYPE_AQUA]);
		fprintf(stream, "\tLoginwindow:\t0x%08x\n", bootstrap_registrations[i].bsports[USER_EXEC_SESSION_TYPE_LOGINWINDOW]);
		fprintf(stream, "\tStandardIO:\t0x%08x\n", bootstrap_registrations[i].bsports[USER_EXEC_SESSION_TYPE_STANDARDIO]);
	}
	fprintf(stream, "--END REGISTRATION DUMP\n");
}

static void add_bootstrap_registration(uid_t uid, enum user_exec_session_types session_type, mach_port_t bsport)
{
	struct user_bootstrap_registration *registration = uid_to_bootstrap_registration(uid);
	
	if (registration->bsports[session_type] != MACH_PORT_NULL) {
		kern_return_t kr = mach_port_deallocate(mach_task_self(), registration->bsports[session_type]);
		
		if (kr != KERN_SUCCESS) {
			warnx("Could not deallocate old bootstrap port send right: %s", mach_error_string(kr));
		}
	}
	
	registration->bsports[session_type] = bsport;
}

static mach_port_t lookup_bootstrap_registration(uid_t uid, enum user_exec_session_types session_type)
{
	return uid_to_bootstrap_registration(uid)->bsports[session_type];
}

static boolean_t server_demux(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{
	return agentipc_server(InHeadP, OutHeadP) || clientipc_server(InHeadP, OutHeadP);
}

static void ipc_loop(mach_port_t service_port)
{
	kern_return_t kr = KERN_SUCCESS;
	typedef union {
		union __RequestUnion__unedaemonserver_clientipc_subsystem creq;
		union __ReplyUnion__unedaemonserver_clientipc_subsystem crep;
		union __RequestUnion__unedaemonserver_agentipc_subsystem areq;
		union __ReplyUnion__unedaemonserver_agentipc_subsystem arep;
	} MAX_MSG_BUFFER;
	
	do {
		kr = mach_msg_server_once(server_demux,
								  sizeof(MAX_MSG_BUFFER)+ MAX_TRAILER_SIZE,
								  service_port,
								  MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AUDIT) | MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0));
		if (kr == MACH_RCV_INTERRUPTED) {
			kr = KERN_SUCCESS;
		}
	} while (kr == KERN_SUCCESS);
	
	errx(EX_OSERR, "mach_msg_server_once() failed: %d: %s", kr, mach_error_string(kr));
}

kern_return_t unedaemonserver_get_agent(mach_port_t sp, uint32_t session_type, 	mach_port_t *out_agent_port, mach_msg_type_name_t *out_agent_portPoly, audit_token_t ucreds)

{
	if (session_type > USER_EXEC_SESSION_TYPE_MAX) {
		warnx("Unknown session type for lookup %u", session_type);
		return KERN_INVALID_ARGUMENT;
	}
	mach_port_t bsport = lookup_bootstrap_registration(audit_token_to_ruid(ucreds), session_type);
	*out_agent_port = bsport;
	*out_agent_portPoly = MACH_MSG_TYPE_COPY_SEND;
	return KERN_SUCCESS;
}

kern_return_t unedaemonserver_register(mach_port_t server, uint32_t session_type,
									   mach_port_t agent_bootstrap_port, mach_port_t *out_death_port,
									   mach_msg_type_name_t *death_portPoly, audit_token_t ucreds)
{
	if (session_type > USER_EXEC_SESSION_TYPE_MAX) {
		warnx("Unknown session type for registration %u", session_type);
		return KERN_INVALID_ARGUMENT;
	}

	*out_death_port = death_port;
	*death_portPoly = MACH_MSG_TYPE_MAKE_SEND;
	
	add_bootstrap_registration(audit_token_to_ruid(ucreds), session_type, agent_bootstrap_port);
	
	return KERN_SUCCESS;
}

kern_return_t unedaemonserver_dump_state(mach_port_t server)
{
	dump_registrations(stderr);
	return KERN_SUCCESS;
}

int main(int argc, const char * argv[])
{
	mach_port_t service_port = MACH_PORT_NULL;
	
	kern_return_t kr = bootstrap_check_in(bootstrap_port, USER_EXEC_SERVICE_NAME, &service_port);
	if (kr != KERN_SUCCESS) {
		errx(EX_UNAVAILABLE, "bootstrap_check_in() failed for client service: %d: %s", kr, mach_error_string(kr));
	}

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &death_port);
	if (kr != KERN_SUCCESS) {
		errx(EX_OSERR, "mach_port_allocate() failed to create port: %d: %s", kr, mach_error_string(kr));
	}
	
	ipc_loop(service_port);
	errx(EX_SOFTWARE, "ipc loop returned?");
    return EX_OSERR;
}
