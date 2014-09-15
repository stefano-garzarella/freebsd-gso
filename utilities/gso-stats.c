/*
 * Copyright (C) 2014, Stefano Garzarella - Universita` di Pisa.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/sysctl.h>
#include <net/gso.h>

static void
usage(void)
{
	const char *cmd = "gso-stats";
	fprintf(stderr,
			"Usage:\n"
			"%s arguments\n"
			"\t-r\treset GSO statistics\n"
			"",
			cmd);

	exit(0);
}

static void
print_gsostat_proto(struct gsostat_proto* gsp, char* type)
{
	printf("\t\t%lu \ttotal packets %sed\n", gsp->gsos_segmented, type);
	printf("\t\t%lu \toutput %ss created\n", gsp->gsos_osegments, type);
	printf("\t\t%Lf \tmean %ss per packet\n", gsp->gsos_osegments / (long double) gsp->gsos_segmented, type);
	printf("\t\t%lu \tmax size of packets\n", gsp->gsos_maxsegmented);
	if (gsp->gsos_segmented)
		printf("\t\t%lu \tmin size of packets\n", gsp->gsos_minsegmented);
	else
		printf("\t\t-- \tmin size of packets\n");
	printf("\t\t%lu \ttotal size of packets\n", gsp->gsos_totalbyteseg);
	printf("\t\t%Lf \tmean size of packets\n", gsp->gsos_totalbyteseg / (long double) gsp->gsos_segmented);
	printf("\t\t%Lf \tmean size of %ss\n", gsp->gsos_totalbyteseg / (long double) gsp->gsos_osegments, type);
	printf("\t\t%lu \tmax mss\n", gsp->gsos_max_mss);
	if (gsp->gsos_segmented)
		printf("\t\t%lu \tmin mss\n", gsp->gsos_min_mss);
	else
		printf("\t\t-- \tmin mss\n");
}

int 
main(int argc, char **argv) 
{
	int ch, reset = 0;
	struct gsostat gs;
	size_t gs_len = sizeof(struct gsostat);

	/* get options */
	while ((ch = getopt(argc, argv, "r")) != -1) {
		switch(ch) {
			default:
				usage();
				break;
			case 'r':
				reset = 1;
				break;
		}
	}

	/* get "struct gsostat" with sysctl */
	if (sysctlbyname(GSO_SYSCTL_STATS, &gs, &gs_len, NULL, 0) != 0) {
		printf("Error gathering statistics.\n");
		return -1;
	}

	/* print statistics */
	printf("GSO statistics \n");

	printf("\t TCP (IPv4/IPv6) packets\n");
	print_gsostat_proto(&(gs.tcp), "segment");

	printf("\t IPv4 packets (that need IP fragmentation)\n");
	print_gsostat_proto(&(gs.ipv4_frag), "fragment");

	printf("\t IPv6 packets (that need IP fragmentation)\n");
	print_gsostat_proto(&(gs.ipv6_frag), "fragment");

	/* if required, reset the statistics */
	if (reset) {
		gsostat_reset(&gs);
		if (sysctlbyname(GSO_SYSCTL_STATS, NULL, NULL, &gs, sizeof(gs)) < 0) {
			printf("Error resetting statistics.\n");
			return -1;
		}
		printf("\nGSO statistics cleared\n");
	}

	return 0;
}
