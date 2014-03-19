/**
 * Rapid-prototyping protection schemes with IEC 61850
 *
 * Copyright (c) 2014 Steven Blair
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define WPCAP
#define HAVE_REMOTE
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif


#include <pcap.h>
#include <math.h>
#include "iec61850.h"

#if JSON_INTERFACE == 1
#include "json\mongoose.h"
#include "json\json.h"
#endif


#define BUFFER_LENGTH	2048

pcap_t *fp;
char errbuf[PCAP_ERRBUF_SIZE];
unsigned char buf[BUFFER_LENGTH] = {0};
int len = 0;

void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data) {
	gse_sv_packet_filter((unsigned char *) pkt_data, header->len);
}

pcap_t *init_pcap() {
	pcap_t *fpl;
    pcap_if_t *alldevs;
    pcap_if_t *used_if;

    // Retrieve the device list from the local machine
#ifdef _WIN32
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL /* auth is not needed */, &alldevs, errbuf) == -1) {
		fprintf(stderr, "Error in pcap_findalldevs_ex: %s\n", errbuf);
		exit(1);
	}
#else
	if (pcap_findalldevs(&alldevs, errbuf) == -1) {
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		exit(1);
	}
#endif

    used_if = alldevs;

#ifdef _WIN32
    fprintf(stdout, "%s\n", used_if->description);
#else
    fprintf(stdout, "%s\n", used_if->name);
#endif
    fflush(stdout);

	if ((fpl = pcap_open_live(used_if->name,	// name of the device
							 65536,				// portion of the packet to capture. It doesn't matter in this case
							 1,					// promiscuous mode (nonzero means promiscuous)
							 1000,				// read timeout
							 errbuf				// error buffer
							 )) == NULL)
	{
		fprintf(stderr, "\nUnable to open the adapter. %s is not supported by WinPcap\n", alldevs->name);
		exit(2);
	}

//    pcap_freealldevs(alldevs);

	return fpl;
}


int main() {
	initialise_iec61850();
	fp = init_pcap();

#if JSON_INTERFACE == 1
	start_json_interface();

#ifdef USE_SSL
	// generate and print out hash, to be copied into password file
	// WARNING: for convenience only; do not store passwords in plain text in production code
	char hash[33];
	mg_md5(hash, "admin", ":", "localhost", ":", "admin", NULL);
	printf("%s\n", hash);
	fflush(stdout);
#endif

	srand(time(NULL));

	struct exampleJSON *relays[] = {&JSON.S1.C1.exampleJSON_1, &JSON.S1.C1.exampleJSON_2, &JSON.S1.C1.exampleJSON_3, &JSON.S1.C1.exampleJSON_4};
	int numberOfRelays = sizeof relays / sizeof relays[0];

	int timer = 0;
	int alarmTimer = 0;
	int lampTestMonitor = relays[0]->Ind.LEDTest;
	int resetTripMonitor = relays[0]->Ind.Trip;

	float phaseVoltageMag = (float) (11000.0 / sqrt(3));

	while (1) {
		if (++timer == 5) {
			timer = 0;

			int i = 0;
			for (i = 0; i < numberOfRelays; i++) {
				float phaseCurrentMag = 100.0 + 500.0 * (float) rand() / (float) RAND_MAX;
				float phaseCurrentAng = -30.0 * (float) rand() / (float) RAND_MAX;

				relays[i]->Hz.mag = 50.0;

				relays[i]->SeqV.phsA.cVal.mag.f = phaseVoltageMag;
				relays[i]->SeqV.phsA.cVal.ang.f = 0.0;
				relays[i]->SeqV.phsB.cVal.mag.f = 0.0;
				relays[i]->SeqV.phsB.cVal.ang.f = -120.0;
				relays[i]->SeqV.phsC.cVal.mag.f = 0.0;
				relays[i]->SeqV.phsC.cVal.ang.f = 120.0;
				relays[i]->SeqV.neut.cVal.mag.f = 0.0;
				relays[i]->SeqV.phsA.cVal.ang.f = 0.0;

				relays[i]->PhV.phsA.cVal.mag.f = phaseVoltageMag;
				relays[i]->PhV.phsA.cVal.ang.f = 0.0;
				relays[i]->PhV.phsB.cVal.mag.f = phaseVoltageMag;
				relays[i]->PhV.phsB.cVal.ang.f = -120.0;
				relays[i]->PhV.phsC.cVal.mag.f = phaseVoltageMag;
				relays[i]->PhV.phsC.cVal.ang.f = 120.0;
				relays[i]->PhV.neut.cVal.mag.f = 0.0;
				relays[i]->PhV.phsA.cVal.ang.f = 0.0;

				relays[i]->V1.phsA.cVal.mag.f = phaseVoltageMag;
				relays[i]->V1.phsA.cVal.ang.f = 0.0;
				relays[i]->V1.phsB.cVal.mag.f = 0.0;
				relays[i]->V1.phsB.cVal.ang.f = -120.0;
				relays[i]->V1.phsC.cVal.mag.f = 0.0;
				relays[i]->V1.phsC.cVal.ang.f = 120.0;
				relays[i]->V1.neut.cVal.mag.f = 0.0;
				relays[i]->V1.phsA.cVal.ang.f = 0.0;

				relays[i]->SeqA.phsA.cVal.mag.f = phaseCurrentMag;
				relays[i]->SeqA.phsA.cVal.ang.f = phaseCurrentAng;

				relays[i]->A1.phsA.cVal.mag.f = phaseCurrentMag;
				relays[i]->A1.phsA.cVal.ang.f = phaseCurrentAng;
				relays[i]->A1.phsB.cVal.mag.f = phaseCurrentMag;
				relays[i]->A1.phsB.cVal.ang.f = -120.0 + phaseCurrentAng;
				relays[i]->A1.phsC.cVal.mag.f = phaseCurrentMag;
				relays[i]->A1.phsC.cVal.ang.f = 120.0 + phaseCurrentAng;
				relays[i]->A1.neut.cVal.mag.f = 0.0;
				relays[i]->A1.neut.cVal.ang.f = 0.0;

				relays[i]->Ind.Trip = 1;
			}
		}

		if (++alarmTimer == 100) {
			alarmTimer = 0;
			relays[0]->Ind.NumOfAlarms++;
		}

		if (lampTestMonitor != relays[0]->Ind.LEDTest) {
			lampTestMonitor = relays[0]->Ind.LEDTest;
			printf("LED lamp test: %i\n", relays[0]->Ind.LEDTest);
			fflush(stdout);
		}

		if (resetTripMonitor != relays[0]->Ind.Trip) {
			resetTripMonitor = relays[0]->Ind.Trip;
			relays[0]->Ind.NumOfAlarms = 0;
			alarmTimer = 0;
		}

//		printf("trip: %i, SG: %i\n", relays[i]->Ind.Trip, relays[i]->Attr.ActiveSettingGroup);
//		fflush(stdout);


#ifndef USE_SSL
//		int port;
//		int reply_len;
//		char *reply;
//
//		// test get values
//		for (port = 8001; port <= 8012; port++) {
//			reply = send_http_request_get(port, &reply_len, "/");
//			free(reply);
//			Sleep(1);
//		}
//
//		// test get definition
//		for (port = 8001; port <= 8012; port++) {
//			reply = send_http_request_get(port, &reply_len, "/definition/");
//			free(reply);
//			Sleep(1);
//		}
//
//		// test get directory
//		for (port = 8001; port <= 8012; port++) {
//			char *reply = send_http_request_get(port, &reply_len, "/directory/");
//			free(reply);
//			Sleep(1);
//		}
//
//		// test setting values
//		{
//			float x = (float) rand() / (float) (RAND_MAX / 10000.0);
//			char value[32] = {0};
//			sprintf(value, "%f", x);
//			reply = send_http_request_post(8012, &reply_len, "/C1/exampleMMXU_1.A.phsA.cVal.mag.f", value);
//			free(reply);
//			Sleep(1);
//
//			sprintf(value, "%d", (int) x);
//			reply = send_http_request_post(8012, &reply_len, "/C1/exampleMMXU_1.A/phsA.testInteger", value);
//			free(reply);
//			Sleep(1);
//
//			reply = send_http_request_post(8001, &reply_len, "/C1/LN0.NamPlt.configRev", "abcdefgh");
//			free(reply);
//			Sleep(1);
//
//			reply = send_http_request_post(8012, &reply_len, "/C1/LN0/NamPlt/configRev/", "xyz");
//			free(reply);
//			Sleep(1);
//
//			reply = send_http_request_post(8012, &reply_len, "/C1/LN0/NamPlt/configRev", "1234567890");
//			free(reply);
//			Sleep(1);
//		}
		Sleep(100);
#else
		Sleep(100);
#endif
	}
#endif

	pcap_close(fp);

	return 0;
}
