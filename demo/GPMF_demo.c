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

#define GPMF_HAS_SAMPLES 0

extern void PrintGPMF(GPMF_stream *ms);

void printSampleRate(size_t mp4, GPMF_stream *ms)
{
	// Find all the available Streams and compute they sample rates
	while (GPMF_OK == GPMF_FindNext(ms, GPMF_KEY_STREAM, GPMF_RECURSE_LEVELS))
	{
		if (GPMF_OK == GPMF_SeekToSamples(ms)) //find the last FOURCC within the stream
		{
			uint32_t fourcc = GPMF_Key(ms);
			double rate = GetGPMFSampleRate(mp4, fourcc, GPMF_SAMPLE_RATE_PRECISE);// GPMF_SAMPLE_RATE_FAST);
			printf("%c%c%c%c sampling rate = %f Hz\n", PRINTF_4CC(fourcc), rate);
		}
	}	
}

int printSamples(GPMF_stream *ms, char* fourCC, float in, float out) 
{
	int ret = GPMF_HAS_SAMPLES;
	if (GPMF_OK == GPMF_FindNext(ms, STR2FOURCC(fourCC), GPMF_RECURSE_LEVELS))
	{
		uint32_t key = GPMF_Key(ms);
		uint32_t samples = GPMF_Repeat(ms);
		uint32_t elements = GPMF_ElementsInStruct(ms);
		uint32_t buffersize = samples * elements * sizeof(double);
		GPMF_stream find_stream;
		double *ptr, *tmpbuffer = malloc(buffersize);

		printf("TIME, %.3f, %.3f\n", in, out);

		if (tmpbuffer && samples)
		{
			uint32_t i, j;
			GPMF_ScaledData(ms, tmpbuffer, buffersize, 0, samples, GPMF_TYPE_DOUBLE);  //Output scaled data as floats

			ptr = tmpbuffer;
			for (i = 0; i < samples; i++)
			{
				printf("%c%c%c%c, ", PRINTF_4CC(key));
				for (j = 0; j < elements; j++)
					printf("%.8f, ", *ptr++);
				printf("\n");
			}
			free(tmpbuffer);
		}
	}
	else 
		ret = -1;
	
	GPMF_ResetState(ms);
	return ret;
}

void printUTCDate(GPMF_stream *ms)
{
	if (GPMF_OK == GPMF_FindNext(ms, STR2FOURCC("GPSU"), GPMF_RECURSE_LEVELS))
	{
		uint32_t key = GPMF_Key(ms);
		char GSPU[17];
		GPMF_FormattedData(ms, GSPU, 16, 0, 1);
		GSPU[16] = 0;
		printf("%c%c%c%c, %s\n", PRINTF_4CC(key), GSPU);
	}	
}

int main(int argc, char *argv[])
{
	int32_t ret = GPMF_OK;
	GPMF_stream metadata_stream, *ms = &metadata_stream;
	double metadatalength;
	uint32_t *payload = NULL; //buffer to store GPMF samples from the MP4.

	if (argc != 2)
	{
		printf("usage: %s <file_with_GPMF>\n", argv[0]);
		return -1;
	}

	size_t mp4 = OpenMP4Source(argv[1], MOV_GPMF_TRAK_TYPE, MOV_GPMF_TRAK_SUBTYPE);
	metadatalength = GetDuration(mp4);

	if (metadatalength > 0.0)
	{
		uint32_t index, payloads = GetNumberPayloads(mp4);
		for (index = 0; index < payloads; index++)
		{
			uint32_t payloadsize = GetPayloadSize(mp4, index);
			float in = 0.0, out = 0.0; //times
			payload = GetPayload(mp4, payload, index);
			if (payload == NULL)
				goto cleanup;

			ret = GetPayloadTime(mp4, index, &in, &out);
			if (ret != GPMF_OK)
				goto cleanup;

			ret = GPMF_Init(ms, payload, payloadsize);
			if (ret != GPMF_OK)
				goto cleanup;
			
			printUTCDate(ms);

			// Don't print unless you have GPS data.
			if (printSamples(ms, "GPS5", in, out) == GPMF_HAS_SAMPLES)
			{
				printSamples(ms, "GPSP", in, out);
				printSamples(ms, "GYRO", in, out);		
			}
		}

	cleanup:
		if (payload) FreePayload(payload); payload = NULL;
		CloseSource(mp4);
	}

	return ret;
}
