/*! @file GPMF_demo.c
 *
 *  @brief Demo to extract GPMF from an MP4
 *
 *  @version 1.0.1
 *
 *  (C) Copyright 2017 GoPro Inc (http://gopro.com/).
 *	
 *  Licensed under either:
 *  - Apache License, Version 2.0, http://www.apache.org/licenses/LICENSE-2.0  
 *  - MIT license, http://opensource.org/licenses/MIT
 *  at your option.
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../GPMF_parser.h"
#include "GPMF_mp4reader.h"
#define BUFFER_SIZE 10240

typedef struct {
	double lat, lon, speed, z;
} measurement;

extern void PrintGPMF(GPMF_stream *ms);

double GetAverageZ(double* buffer) {
	const int SAMPLE_SIZE = 22;  // 399 / 18
	double avg, total = 0.0;
	double* ptr = buffer;
	int i = 0;
	for (; i < SAMPLE_SIZE; i++) {
		total += *ptr;
		ptr+=3; // advance to the next sample.
	}
	avg = total / SAMPLE_SIZE;
	return avg;
}

double GetSeconds(uint32_t sample, double startTime) {
	const double SAMPLES = 18.0;
	return startTime + ((1.0/SAMPLES)*sample);
}

uint32_t GetGPMField(const char* field, GPMF_stream* ms, measurement* m) {
	if (GPMF_OK == GPMF_FindNext(ms, STR2FOURCC(field), GPMF_RECURSE_LEVELS))  
	{
		uint32_t key = GPMF_Key(ms);
		uint32_t samples = GPMF_Repeat(ms);
		uint32_t elements = GPMF_ElementsInStruct(ms);
		uint32_t buffersize = samples * elements * sizeof(double);
		double tmpbuffer[BUFFER_SIZE];
		
		if (buffersize > BUFFER_SIZE) 
		{
			printf("Buffer is too big: %i\n", buffersize);
			return 0;
		}
		else
		{
			memset(tmpbuffer, 0, BUFFER_SIZE);
		}

		if (samples)
		{
			uint32_t i, j;
			
			GPMF_ScaledData(ms, tmpbuffer, buffersize, 0, samples, GPMF_TYPE_DOUBLE);  //Output scaled data as floats
			//ptr = tmpbuffer;
			for (i = 0; i < samples; i++)
			{
				measurement* mptr = (m+i);
				if (key == STR2FOURCC("GPS5")) {
					mptr->lat = *tmpbuffer;
					mptr->lon = *(tmpbuffer+1);
					mptr->speed = *(tmpbuffer+3);
					//ptr += elements; // advance to the next sample.
				}
				else if (key == STR2FOURCC("GYRO")) {
					mptr->z = GetAverageZ(tmpbuffer);
					//ptr += elements * 22; // advance 22 records
				}
			}
		}
		
		return samples;
	}	
	else
		return 0;
}

int main(int argc, char *argv[])
{
	// Open GPMF stream from file.

	// More GYRO samples available than GPS
	// Average GYRO samples for each GPS entry (GYRO / GPS Sample Rate) 
	// Get the sampling rate for GPS & GYRO to calculate

	// Foreach payload (~1 second of data)
		// Get GPS samples
		// Get GYRO samples
		// Print GPS
		// Get average GYRO for GPS sample time frame
		// Print Gyro

	int32_t ret = GPMF_OK;
	GPMF_stream metadata_stream, *ms = &metadata_stream;
	double metadatalength;
	uint32_t *payload = NULL; //buffer to store GPMF samples from the MP4.

	// get file return data
	if (argc != 2)
	{
		printf("usage: %s <file_with_GPMF>\n", argv[0]);
		return -1;
	}

	metadatalength = OpenGPMFSource(argv[1]);

	if (metadatalength > 0.0)
	{
		uint32_t index, payloads = GetNumberGPMFPayloads();
		printf("found %.2fs of metadata, from %d payloads, within %s\n", metadatalength, payloads, argv[1]);

		measurement* measurements = malloc(sizeof(measurement) * payloads);

		for (index = 0; index < payloads; index++)
		{
			uint32_t payloadsize = GetGPMFPayloadSize(index);
			double in = 0.0, out = 0.0; //times
			payload = GetGPMFPayload(payload, index);
			if (payload == NULL)
				goto cleanup;

			ret = GetGPMFPayloadTime(index, &in, &out);
			if (ret != GPMF_OK)
				goto cleanup;

			ret = GPMF_Init(ms, payload, payloadsize);
			if (ret != GPMF_OK)
				goto cleanup;
	
			const char* gyro = "GYRO"; //ACCL
			const char* gps = "GPS5"; //ACCL
			uint32_t i, samples = GetGPMField(gps, ms, measurements);
			GPMF_ResetState(ms);
			GetGPMField(gyro, ms, measurements);
			measurement* ptr = measurements;
			for (i = 0; i < samples; i++) {
				double seconds = GetSeconds(i, in);
				printf("%.2f, %.6f, %.6f, %.2f, %.5f\n", seconds, ptr->lat, ptr->lon, ptr->speed, ptr->z);
				ptr++;
			}
			GPMF_ResetState(ms);
		}
		
		/*
		// Find all the available Streams and compute they sample rates
		while (GPMF_OK == GPMF_FindNext(ms, GPMF_KEY_STREAM, GPMF_RECURSE_LEVELS))
		{
			if (GPMF_OK == GPMF_SeekToSamples(ms)) //find the last FOURCC within the stream
			{
				uint32_t fourcc = GPMF_Key(ms);
				double rate = GetGPMFSampleRate(fourcc, GPMF_SAMPLE_RATE_PRECISE);// GPMF_SAMPLE_RATE_FAST);
				printf("%c%c%c%c sampling rate = %f Hz\n", PRINTF_4CC(fourcc), rate);
			}
		}
		*/

		//printf("Biggest buffer was: %i\n", biggestbuffer);

	cleanup:
		if (payload) FreeGPMFPayload(payload); payload = NULL;
		CloseGPMFSource();
		free(measurements); 
	}

	return ret;
}
