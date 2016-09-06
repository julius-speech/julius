/* ----------------------------------------------------------------- */
/*           The Toolkit for Building Voice Interaction Systems      */
/*           "MMDAgent" developed by MMDAgent Project Team           */
/*           http://www.mmdagent.jp/                                 */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2015  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAgent project team nor the names of  */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

/* header */
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <stdlib.h>
#include "portaudio.h"

/* audio output setting (48kHz/16bit) */
#define PLAYER_BUFFERNUMSAMPLES 3200
#define PLAYER_WAITMS 10

/* audio input (16kHz/16bit) */
#define RECORDER_BUFFERNUMSAMPLES 800

/* OpenSLES engine */
static SLObjectItf engine = NULL;

/* audio output */
static SLObjectItf player = NULL;
static SLObjectItf mixer = NULL;
static SLAndroidSimpleBufferQueueItf playerBufferQueueInterface = NULL;
static char storingPlayerBufferName = 'A';
static short *playerBufferA = NULL;
static short *playerBufferB = NULL;
static size_t currentPlayerBufferSize = 0;
static size_t numQueuedPlayerBuffer = 0;

/* audio input */
static SLObjectItf recorder = NULL;
static SLAndroidSimpleBufferQueueItf recorderBufferQueueInterface = NULL;
static char storingRecorderBufferName = 'A';
static short *recorderBufferA = NULL;
static short *recorderBufferB = NULL;
static PaStreamCallback *sendRecordingBuffer = NULL;

/* Port Audio */
static PaHostApiInfo paHostApiInfo;
static PaDeviceInfo deviceInfo;
static PaStreamInfo paStreamInfo;

static void playerCallbackFunction(SLAndroidSimpleBufferQueueItf bufferQueueInterface, void *context)
{
   numQueuedPlayerBuffer--;
}

static void recorderCallbackFunction(SLAndroidSimpleBufferQueueItf bufferQueueInterface, void *context)
{
   PaStreamCallbackFlags flag;

   if(storingRecorderBufferName == 'A') {
      sendRecordingBuffer(recorderBufferA, NULL, RECORDER_BUFFERNUMSAMPLES, NULL, flag, NULL);
      (*recorderBufferQueueInterface)->Enqueue(recorderBufferQueueInterface, recorderBufferA, RECORDER_BUFFERNUMSAMPLES * sizeof(short));
      storingRecorderBufferName = 'B';
   } else {
      sendRecordingBuffer(recorderBufferB, NULL, RECORDER_BUFFERNUMSAMPLES, NULL, flag, NULL);
      (*recorderBufferQueueInterface)->Enqueue(recorderBufferQueueInterface, recorderBufferB, RECORDER_BUFFERNUMSAMPLES * sizeof(short));
      storingRecorderBufferName = 'A';
   }
}

PaError Pa_Initialize()
{
   SLresult result;

   /* reset */
   Pa_Terminate();

   /* initialize dummy information for PortAudio */
   memset(&paHostApiInfo, 0, sizeof(PaHostApiInfo));
   memset(&deviceInfo, 0, sizeof(PaDeviceInfo));
   memset(&paStreamInfo, 0, sizeof(PaStreamInfo));

   /* initialize OpenSLES engine */
   result = slCreateEngine(&engine, 0, NULL, 0, NULL, NULL);
   if(result != SL_RESULT_SUCCESS) {
      Pa_Terminate();
      return paInternalError;
   }
   result = (*engine)->Realize(engine, SL_BOOLEAN_FALSE);
   if(result != SL_RESULT_SUCCESS) {
      Pa_Terminate();
      return paInternalError;
   }

   return paNoError;
}

PaError Pa_Terminate()
{
   SLresult result1 = Pa_CloseStream((PaStream *) '\x01');
   SLresult result2 = Pa_CloseStream((PaStream *) '\x02');

   /* finalize OpenSLES engine */
   if (engine != NULL)
      (*engine)->Destroy(engine);
   engine = NULL;

   if(result1 != paNoError || result2 != paNoError)
      return paInternalError;

   return paNoError;
}

PaError Pa_OpenStream(PaStream** stream, const PaStreamParameters *inputParameters, const PaStreamParameters *outputParameters, double sampleRate, unsigned long framesPerBuffer, PaStreamFlags streamFlags, PaStreamCallback *streamCallback, void *userData)
{
   SLresult result;
   SLEngineItf engineInterface;

   /* get engine interface */
   result = (*engine)->GetInterface(engine, SL_IID_ENGINE, &engineInterface);
   if(result != SL_RESULT_SUCCESS) {
      Pa_Terminate();
      return paInternalError;
   }

   if(outputParameters != NULL) {
      /* reset */
      Pa_CloseStream((PaStream *) '\x01');

      /* prepare */
      {
         storingPlayerBufferName = 'A';
         playerBufferA = (short *) malloc(sizeof(short) * PLAYER_BUFFERNUMSAMPLES);
         playerBufferB = (short *) malloc(sizeof(short) * PLAYER_BUFFERNUMSAMPLES);
         currentPlayerBufferSize = 0;
         numQueuedPlayerBuffer = 0;
      }

      /* create mixer */
      {
         result = (*engineInterface)->CreateOutputMix(engineInterface, &mixer, 0, NULL, NULL);
         if(result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
         result = (*mixer)->Realize(mixer, SL_BOOLEAN_FALSE);
         if(result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
      }

      /* create player */
      {
         SLDataLocator_AndroidSimpleBufferQueue androidSimpleBufferQueue = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
         SLDataFormat_PCM dataFormat = { SL_DATAFORMAT_PCM, outputParameters->channelCount, 0, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, 0, SL_BYTEORDER_LITTLEENDIAN };
         if (outputParameters->channelCount == 1) {
            dataFormat.channelMask = SL_SPEAKER_FRONT_CENTER;
         } else {
            dataFormat.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
         }
         switch((int)sampleRate) {
         case 8000:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_8;
            break;
         case 11025:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_11_025;
            break;
         case 12000:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_12;
            break;
         case 16000:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_16;
            break;
         case 22050:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_22_05;
            break;
         case 24000:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_24;
            break;
         case 32000:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_32;
            break;
         case 44100:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_44_1;
            break;
         case 48000:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_48;
            break;
         case 64000:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_64;
            break;
         case 88200:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_88_2;
            break;
         case 96000:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_96;
            break;
         case 192000:
            dataFormat.samplesPerSec = SL_SAMPLINGRATE_192;
            break;
         default:
            Pa_Terminate();
            return paInternalError;
         }
         SLDataSource dataSource = { &androidSimpleBufferQueue, &dataFormat };
         const SLInterfaceID interfaceID[1] = { SL_IID_BUFFERQUEUE };
         const SLboolean interfaceRequired[1] = { SL_BOOLEAN_TRUE };
         SLDataLocator_OutputMix outputMix = { SL_DATALOCATOR_OUTPUTMIX, mixer };
         SLDataSink dataSink = { &outputMix, NULL };
         result = (*engineInterface)->CreateAudioPlayer(engineInterface, &player, &dataSource, &dataSink, 1, interfaceID, interfaceRequired);
         if(result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
         result = (*player)->Realize(player, SL_BOOLEAN_FALSE);
         if(result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
      }

      /* set callback function */
      {
         result = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &playerBufferQueueInterface);
         if(result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
         result = (*playerBufferQueueInterface)->RegisterCallback(playerBufferQueueInterface, playerCallbackFunction, NULL);
         if(result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
      }

      /* set dummy address */
      *stream = (PaStream *) '\x01';
   }

   if(inputParameters != NULL) {
      /* reset */
      Pa_CloseStream((PaStream *) '\x02');

      /* prepare */
      {
         storingRecorderBufferName = 'A';
         recorderBufferA = (short*) malloc(sizeof(short) * RECORDER_BUFFERNUMSAMPLES);
         recorderBufferB = (short*) malloc(sizeof(short) * RECORDER_BUFFERNUMSAMPLES);
         sendRecordingBuffer = streamCallback;
      }

      /* create recorder */
      {
         SLDataLocator_IODevice ioDevice = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
         SLDataSource dataSource = { &ioDevice, NULL };
         SLDataLocator_AndroidSimpleBufferQueue androidSimpleBufferQueue = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
         SLDataFormat_PCM dataFormat = { SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN };
         SLDataSink dataSink = { &androidSimpleBufferQueue, &dataFormat };
         const SLInterfaceID interfaceID[2] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };
         const SLboolean interfaceRequired[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
         result = (*engineInterface)->CreateAudioRecorder(engineInterface, &recorder, &dataSource, &dataSink, 2, interfaceID, interfaceRequired);
         if (result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
      }

      /* set recording configuration */
      {
         SLAndroidConfigurationItf config;
         SLint32 type = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
         result = (*recorder)->GetInterface(recorder, SL_IID_ANDROIDCONFIGURATION, &config);
         if (result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
         result = (*config)->SetConfiguration(config, SL_ANDROID_KEY_RECORDING_PRESET, &type, sizeof(SLint32));
         if (result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
         result = (*recorder)->Realize(recorder, SL_BOOLEAN_FALSE);
         if (result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
      }

      /* set callback function */
      {
         result = (*recorder)->GetInterface(recorder, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorderBufferQueueInterface);
         if (result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
         result = (*recorderBufferQueueInterface)->RegisterCallback(recorderBufferQueueInterface, recorderCallbackFunction, NULL);
         if (result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
         result = (*recorderBufferQueueInterface)->Enqueue(recorderBufferQueueInterface, recorderBufferA, RECORDER_BUFFERNUMSAMPLES * sizeof(short));
         if (result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
         result = (*recorderBufferQueueInterface)->Enqueue(recorderBufferQueueInterface, recorderBufferB, RECORDER_BUFFERNUMSAMPLES * sizeof(short));
         if (result != SL_RESULT_SUCCESS) {
            Pa_Terminate();
            return paInternalError;
         }
      }

      /* set dummy address */
      *stream = (PaStream *) '\x02';
   }

   return paNoError;
}

PaError Pa_WriteStream(PaStream *stream, const void *buffer, unsigned long frames)
{
   unsigned long i;
   PaError result;
   const short *tmp = (const short *) buffer;

   if(stream == (PaStream *) '\x01' && player != NULL) {
      for(i = 0; i < frames; i++) {
         if(storingPlayerBufferName == 'A')
            playerBufferA[currentPlayerBufferSize++] = tmp[i];
         else
            playerBufferB[currentPlayerBufferSize++] = tmp[i];
         if (currentPlayerBufferSize >= PLAYER_BUFFERNUMSAMPLES || ((i + 1 == frames) && (currentPlayerBufferSize != 0))) {
            /* enqueue */
            numQueuedPlayerBuffer++;
            if(storingPlayerBufferName == 'A') {
               result = (*playerBufferQueueInterface)->Enqueue(playerBufferQueueInterface, playerBufferA, sizeof(short) * currentPlayerBufferSize);
               if (result != SL_RESULT_SUCCESS) {
                  Pa_Terminate();
                  return paInternalError;
               }
               storingPlayerBufferName = 'B';
            } else {
               result = (*playerBufferQueueInterface)->Enqueue(playerBufferQueueInterface, playerBufferB, sizeof(short) * currentPlayerBufferSize);
               if (result != SL_RESULT_SUCCESS) {
                  Pa_Terminate();
                  return paInternalError;
               }
               storingPlayerBufferName = 'A';
            }
            /* wait */
            while (numQueuedPlayerBuffer >= 2)
               usleep(PLAYER_WAITMS);
            currentPlayerBufferSize = 0;
         }
      }
   }

   return paNoError;
}

void Pa_Sleep(long msec)
{
   usleep((unsigned int) (msec * 1000));
}

PaError Pa_CloseStream(PaStream *stream)
{
   if(stream == (PaStream *) '\x01') {
      if (player != NULL)
         (*player)->Destroy(player);
      if (mixer != NULL)
         (*mixer)->Destroy(mixer);
      if (playerBufferA != NULL)
         free(playerBufferA);
      if (playerBufferB != NULL)
         free(playerBufferB);
      player = NULL;
      mixer = NULL;
      playerBufferQueueInterface = NULL;
      storingPlayerBufferName = 'A';
      playerBufferA = NULL;
      playerBufferB = NULL;
      currentPlayerBufferSize = 0;
      numQueuedPlayerBuffer = 0;
   }

   if(stream == (PaStream *) '\x02') {
      if (recorder != NULL)
         (*recorder)->Destroy(recorder);
      if (recorderBufferA != NULL)
         free(recorderBufferA);
      if (recorderBufferB != NULL)
         free(recorderBufferB);
      recorder = NULL;
      recorderBufferQueueInterface = NULL;
      storingRecorderBufferName = 'A';
      recorderBufferA = NULL;
      recorderBufferB = NULL;
      sendRecordingBuffer = NULL;
   }

   return paNoError;
}

PaError Pa_StartStream(PaStream *stream)
{
   PaError result;

   if(stream == (PaStream *) '\x01' && player != NULL) {
      SLPlayItf playerInterface;
      result = (*player)->GetInterface(player, SL_IID_PLAY, &playerInterface);
      if(result != SL_RESULT_SUCCESS) {
         Pa_Terminate();
         return paInternalError;
      }
      result = (*playerInterface)->SetPlayState(playerInterface, SL_PLAYSTATE_PLAYING);
      if(result != SL_RESULT_SUCCESS) {
         Pa_Terminate();
         return paInternalError;
      }
   }

   if(stream == (PaStream *) '\x02' && recorder != NULL) {
      SLRecordItf recorderInterface;
      result = (*recorder)->GetInterface(recorder, SL_IID_RECORD, &recorderInterface);
      if (result != SL_RESULT_SUCCESS) {
         Pa_Terminate();
         return paInternalError;
      }
      result = (*recorderInterface)->SetRecordState(recorderInterface, SL_RECORDSTATE_RECORDING);
      if (result != SL_RESULT_SUCCESS) {
         Pa_Terminate();
         return paInternalError;
      }
   }

   return paNoError;
}

PaError Pa_StopStream(PaStream *stream)
{
   PaError result;

   if(stream == (PaStream *) '\x01' && player != NULL) {
      SLPlayItf playerInterface;
      result = (*player)->GetInterface(player, SL_IID_PLAY, &playerInterface);
      if (result != SL_RESULT_SUCCESS) {
         Pa_Terminate();
         return paInternalError;
      }
      result = (*playerInterface)->SetPlayState(playerInterface, SL_PLAYSTATE_STOPPED);
      if (result != SL_RESULT_SUCCESS) {
         Pa_Terminate();
         return paInternalError;
      }
   }

   if(stream == (PaStream *) '\x02' && recorder != NULL) {
      SLRecordItf recorderInterface;
      result = (*recorder)->GetInterface(recorder, SL_IID_RECORD, &recorderInterface);
      if (result != SL_RESULT_SUCCESS) {
         Pa_Terminate();
         return paInternalError;
      }
      result = (*recorderInterface)->SetRecordState(recorderInterface, SL_RECORDSTATE_STOPPED);
      if (result != SL_RESULT_SUCCESS) {
         Pa_Terminate();
         return paInternalError;
      }
   }

   return paNoError;
}

PaError Pa_AbortStream(PaStream *stream)
{
   return Pa_CloseStream(stream);
}

const PaHostApiInfo *Pa_GetHostApiInfo(PaHostApiIndex hostApi)
{
   return &paHostApiInfo;
}

const PaDeviceInfo* Pa_GetDeviceInfo(PaDeviceIndex device)
{
   return &deviceInfo;
}

const PaStreamInfo* Pa_GetStreamInfo(PaStream *stream)
{
   return &paStreamInfo;
}

const char *Pa_GetErrorText(PaError errorCode)
{
   return "";
}

PaDeviceIndex Pa_GetDeviceCount()
{
   return 0;
}

PaDeviceIndex Pa_GetDefaultInputDevice()
{
   return 0;
}

PaDeviceIndex Pa_GetDefaultOutputDevice()
{
   return 0;
}
