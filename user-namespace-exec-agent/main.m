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
	const char *session = argv[1];

    return 0;
}

