/**
 * @file   adin_mic_darwin_coreaudio.c
 * 
 * <JA>
 * @brief  adin microphone library for CoreAudio API
 *
 * by Masatomo Hashimoto <m.hashimoto@aist.go.jp>
 *
 * Tested on Mac OS X v10.3.9 and v10.4.1
 *
 * このプログラムは，
 * 独立行政法人 産業技術総合研究所 情報技術研究部門
 * ユビキタスソフトウェアグループ
 * より提供されました．
 * </JA>
 * 
 * <EN>
 * @brief  adin microphone library for CoreAudio API
 *
 * by Masatomo Hashimoto <m.hashimoto@aist.go.jp>
 *
 * Tested on Mac OS X v10.3.9 and v10.4.1
 *
 * This file has been contributed from the Ubiquitous Software Group,
 * Information Technology Research Institute, AIST.
 * 
 * </EN>
 * 
 * @author Masatomo Hashimoto
 * @date   Wed Oct 12 11:31:27 2005
 *
 * $Revision: 1.9 $
 * 
 */

/*
 * adin_mic_darwin_coreaudio.c
 *
 * adin microphone library for CoreAudio API
 *
 * by Masatomo Hashimoto <m.hashimoto@aist.go.jp>
 *
 * Tested on Mac OS X v10.3.9 and v10.4.1
 *
 */

/* $Id: adin_mic_darwin_coreaudio.c,v 1.9 2014/01/05 07:01:01 sumomo Exp $ */

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioOutputUnit.h>
#include <AudioToolbox/AudioConverter.h>
#include <CoreServices/CoreServices.h>
#include <pthread.h>
#include <stdio.h>

#define DEVICE_NAME_LEN 128
#define BUF_SAMPLES 4096

static UInt32 ConvQuality = kAudioConverterQuality_Medium;

typedef SInt16 Sample;
static UInt32 BytesPerSample = sizeof(Sample);

#define BITS_PER_BYTE 8

static AudioDeviceID InputDeviceID;
static AudioUnit InputUnit;
static AudioConverterRef Converter;

static pthread_mutex_t MutexInput;
static pthread_cond_t CondInput;

static bool CoreAudioRecordStarted = FALSE;
static bool CoreAudioHasInputDevice = FALSE;
static bool CoreAudioInit = FALSE;

static UInt32 NumSamplesAvailable = 0;

static UInt32 InputDeviceBufferSamples = 0;
static UInt32 InputBytesPerPacket = 0;
static UInt32 InputFramesPerPacket = 0;
static UInt32 InputSamplesPerPacket = 0;
static UInt32 OutputBitsPerChannel = 0;
static UInt32 OutputBytesPerPacket = 0;
static UInt32 OutputSamplesPerPacket = 0;

static AudioBufferList* BufList;
static AudioBufferList BufListBackup;
static AudioBufferList* BufListConverted;

static char deviceName[DEVICE_NAME_LEN];

#ifndef boolean
typedef unsigned char boolean;
#endif

static void printStreamInfo(AudioStreamBasicDescription* desc) {
  jlog("Stat: adin_darwin: ----- details of stream -----\n");
  jlog("Stat: adin_darwin: sample rate: %f\n", desc->mSampleRate);
  jlog("Stat: adin_darwin: format flags: %s%s%s%s%s%s%s\n", 
	   desc->mFormatFlags & kAudioFormatFlagIsFloat ? 
	   "[float]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsBigEndian ? 
	   "[big endian]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsSignedInteger ? 
	   "[signed integer]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsPacked ? 
	   "[packed]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsAlignedHigh ? 
	   "[aligned high]" : "",
	   desc->mFormatFlags & kAudioFormatFlagIsNonInterleaved ? 
	   "[non interleaved]" : "",
	   desc->mFormatFlags & kAudioFormatFlagsAreAllClear ? 
	   "[all clear]" : ""
	   );
  jlog("Stat: adin_darwin: bytes per packet: %d\n", desc->mBytesPerPacket);
  jlog("Stat: adin_darwin: frames per packet: %d\n", desc->mFramesPerPacket);
  jlog("Stat: adin_darwin: bytes per frame: %d\n", desc->mBytesPerFrame);
  jlog("Stat: adin_darwin: channels per frame: %d\n", desc->mChannelsPerFrame);
  jlog("Stat: adin_darwin: bits per channel: %d\n", desc->mBitsPerChannel);
  jlog("Stat: adin_darwin: -----------------------------------\n");
}

static void printAudioBuffer(AudioBuffer* buf) {
  int sz = buf->mDataByteSize / BytesPerSample;
  int i;
  Sample* p = (Sample*)(buf->mData);
  for (i = 0; i < sz; i++) {
    printf("%d ", p[i]);
  }
}

static AudioBufferList* 
allocateAudioBufferList(UInt32 data_bytes, UInt32 nsamples, UInt32 nchan) {

  AudioBufferList* bufl;

#ifdef DEBUG
  jlog("Stat: adin_darwin: allocateAudioBufferList: data_bytes:%d nsamples:%d nchan:%d\n",
	   data_bytes, nsamples, nchan);
#endif

  bufl = (AudioBufferList*)(malloc(sizeof(AudioBufferList)));

  if(bufl == NULL) {
    jlog("Erorr: adin_darwin: allocateAudioBufferList: failed\n");
    return NULL;
  }

  bufl->mNumberBuffers = nchan;

  int i;
  for (i = 0; i < nchan; i++) {
    bufl->mBuffers[i].mNumberChannels = nchan;
    bufl->mBuffers[i].mDataByteSize = data_bytes * nsamples;
    bufl->mBuffers[i].mData = malloc(data_bytes * nsamples);
    
    if(bufl->mBuffers[i].mData == NULL) {
      jlog("Erorr: adin_darwin: allocateAudioBufferList: malloc for mBuffers[%d] failed\n", i);
      return NULL;
    }
  }
  return bufl;
}

/* gives input data for Converter */
static OSStatus 
ConvInputProc(AudioConverterRef inConv,
	      UInt32* ioNumDataPackets,
	      AudioBufferList* ioData, // to be filled
	      AudioStreamPacketDescription** outDataPacketDesc,
	      void* inUserData)
{
  int i;
  UInt32 nPacketsRequired = *ioNumDataPackets;
  UInt32 nBytesProvided = 0;
  UInt32 nBytesRequired;
  UInt32 n;
  
  pthread_mutex_lock(&MutexInput);

#ifdef DEBUG
  jlog("Stat: adin_darwin: ConvInputProc: required %d packets\n", nPacketsRequired);
#endif

  while(NumSamplesAvailable == 0){
    pthread_cond_wait(&CondInput, &MutexInput);
  }

  for(i = 0; i < BufList->mNumberBuffers; i++) {
    n = BufList->mBuffers[i].mDataByteSize;
    if (nBytesProvided != 0 && nBytesProvided != n) {
      jlog("Warning: adin_darwin: buffer size mismatch\n");
    }
    nBytesProvided = n;
  }

#ifdef DEBUG
  jlog("Stat: adin_darwin: ConvInputProc: %d bytes in buffer\n", nBytesProvided);
#endif

  for(i = 0; i < BufList->mNumberBuffers; i++) {
    ioData->mBuffers[i].mNumberChannels = 
      BufList->mBuffers[i].mNumberChannels;

    nBytesRequired = nPacketsRequired * InputBytesPerPacket;

    if(nBytesRequired < nBytesProvided) {
      ioData->mBuffers[i].mData = BufList->mBuffers[i].mData;
      ioData->mBuffers[i].mDataByteSize = nBytesRequired;
      BufList->mBuffers[i].mData += nBytesRequired;
      BufList->mBuffers[i].mDataByteSize = nBytesProvided - nBytesRequired;
    } else {
      ioData->mBuffers[i].mData = BufList->mBuffers[i].mData;
      ioData->mBuffers[i].mDataByteSize = nBytesProvided;
      
      BufList->mBuffers[i].mData = BufListBackup.mBuffers[i].mData;
      BufList->mBuffers[i].mDataByteSize = BufListBackup.mBuffers[i].mDataByteSize;
    }

  }

  *ioNumDataPackets = ioData->mBuffers[0].mDataByteSize / InputBytesPerPacket;

#ifdef DEBUG
  jlog("Stat: adin_darwin: ConvInputProc: provided %d packets\n", *ioNumDataPackets);
#endif

  NumSamplesAvailable = 
    nBytesProvided / BytesPerSample - *ioNumDataPackets * InputSamplesPerPacket;

#ifdef DEBUG
  jlog("Stat: adin_darwin: ConvInputProc: %d samples available\n", NumSamplesAvailable);
#endif

  pthread_mutex_unlock(&MutexInput);

  return noErr;
}


/* called when input data are available (an AURenderCallback) */
static OSStatus 
InputProc(void* inRefCon,
	  AudioUnitRenderActionFlags* ioActionFlags,
	  const AudioTimeStamp* inTimeStamp,
	  UInt32 inBusNumber,
	  UInt32 inNumberFrames,
	  AudioBufferList* ioData // null
	  )
{
  OSStatus status = noErr;
  int i;

  pthread_mutex_lock(&MutexInput);

  if (NumSamplesAvailable == 0) {

    status = AudioUnitRender(InputUnit,
			     ioActionFlags,
			     inTimeStamp,
			     inBusNumber,
			     inNumberFrames,
			     BufList);
    NumSamplesAvailable = 
      BufList->mBuffers[0].mDataByteSize / InputBytesPerPacket;

#ifdef DEBUG
    printAudioBuffer(BufList->mBuffers);
#endif
  }

  pthread_mutex_unlock(&MutexInput);
  
  pthread_cond_signal(&CondInput);

  /*
  jlog("Stat: adin_darwin: InputProc: %d bytes filled (BufList)\n", 
	  BufList->mBuffers[0].mDataByteSize);
  */

  return status;
}


/* initialize default sound device */
boolean adin_mic_standby(int sfreq, void* dummy) {
  OSStatus status;
  UInt32 propertySize;
  struct AudioStreamBasicDescription inDesc;
  int err;

  jlog("Stat: adin_darwin: sample rate = %d\n", sfreq);

  if (CoreAudioInit) 
    return TRUE;

#ifdef MAC_OS_X_VERSION_10_6
  AudioComponent halout;
  AudioComponentDescription haloutDesc;
#else
  Component halout;
  ComponentDescription haloutDesc;
#endif
   
  haloutDesc.componentType = kAudioUnitType_Output;
  haloutDesc.componentSubType = kAudioUnitSubType_HALOutput;
  haloutDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
  haloutDesc.componentFlags = 0;
  haloutDesc.componentFlagsMask = 0;
  halout = FindNextComponent(NULL, &haloutDesc);

  if(halout == NULL) {
    jlog("Error: adin_darwin: no HALOutput component found\n");
    return FALSE;
  }

  OpenAComponent(halout, &InputUnit);

  UInt32 enableIO;
  
  enableIO = 1;
  status = AudioUnitSetProperty(InputUnit, 
				kAudioOutputUnitProperty_EnableIO,
				kAudioUnitScope_Input,
				1,
				&enableIO,
				sizeof(enableIO));
    if (status != noErr) {
      jlog("Error: adin_darwin: cannot set InputUnit's EnableIO(Input)\n");
      return FALSE;
    }

  enableIO = 0;
  status = AudioUnitSetProperty(InputUnit, 
				kAudioOutputUnitProperty_EnableIO,
				kAudioUnitScope_Output,
				0,
				&enableIO,
				sizeof(enableIO));
    if (status != noErr) {
      jlog("Error: adin_darwin: cannot set InputUnit's EnableIO(Output)\n");
      return FALSE;
    }


  /* get default input device */
  propertySize = sizeof(InputDeviceID);
  status = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice,
	                            &propertySize,
				    &InputDeviceID);
  if (status != noErr) {
    jlog("Error: adin_darwin: cannot get default input device\n");
    return FALSE;
  }

  if (InputDeviceID == kAudioDeviceUnknown) {
    jlog("Error: adin_darwin: no available input device found\n");
    return FALSE;

  } else {

    CoreAudioHasInputDevice = TRUE;

    /* get input device name */
    propertySize = sizeof(char) * DEVICE_NAME_LEN;
    status = AudioDeviceGetProperty(InputDeviceID,
				    1,
				    1,
				    kAudioDevicePropertyDeviceName,
				    &propertySize,
				    deviceName);
    if (status != noErr) {
      jlog("Error: adin_darwin: cannot get device name\n");
      return FALSE;
    }

    status = AudioUnitSetProperty(InputUnit,
				  kAudioOutputUnitProperty_CurrentDevice,
				  kAudioUnitScope_Global,
				  0,
				  &InputDeviceID,
				  sizeof(InputDeviceID));

    if (status != noErr) {
      jlog("Error: adin_darwin: cannot bind default input device to AudioUnit\n");
      return FALSE;
    }

    /* get input device's format */
    propertySize = sizeof(inDesc);
    status = AudioDeviceGetProperty(InputDeviceID,
				    1,
				    1,
				    kAudioDevicePropertyStreamFormat,
				    &propertySize,
				    &inDesc);
    if (status != noErr) {
      jlog("Error: adin_darwin: cannot get input device's stream format\n");
      return FALSE;
    }

    /* get input device's buffer frame size */
    UInt32 bufferFrameSize;
    propertySize = sizeof(bufferFrameSize);
    status = AudioDeviceGetProperty(InputDeviceID,
				    1,
				    1,
				    kAudioDevicePropertyBufferFrameSize,
				    &propertySize,
				    &bufferFrameSize);
    if (status != noErr) {
      jlog("Error: adin_darwin: cannot get input device's buffer frame size\n");
      return FALSE;
    }

    jlog("Stat: adin_darwin: using device \"%s\" for input\n", deviceName);
    jlog("Stat: adin_darwin: sample rate %f\n\t%ld channels\n\t%ld-bit sample\n",
	    inDesc.mSampleRate,
	    inDesc.mChannelsPerFrame,
	    inDesc.mBitsPerChannel);

    jlog("Stat: adin_darwin: %d buffer frames\n", bufferFrameSize);


    printStreamInfo(&inDesc);

    UInt32 formatFlagEndian = 
      inDesc.mFormatFlags & kAudioFormatFlagIsBigEndian;

    inDesc.mFormatFlags = 
      kAudioFormatFlagIsSignedInteger | 
      kAudioFormatFlagIsPacked | 
      formatFlagEndian;

    inDesc.mBytesPerPacket = BytesPerSample;
    inDesc.mFramesPerPacket = 1;
    inDesc.mBytesPerFrame = BytesPerSample;
    inDesc.mChannelsPerFrame = 1;
    inDesc.mBitsPerChannel = BytesPerSample * BITS_PER_BYTE;

    printStreamInfo(&inDesc);

    propertySize = sizeof(inDesc);
    status = AudioUnitSetProperty(InputUnit, 
				  kAudioUnitProperty_StreamFormat,
				  kAudioUnitScope_Output,
				  1,
				  &inDesc,
				  propertySize
				  );
    if (status != noErr) {
      jlog("Error: adin_darwin: cannot set InputUnit's stream format\n");
      return FALSE;
    }

    InputBytesPerPacket = inDesc.mBytesPerPacket;
    InputFramesPerPacket = inDesc.mFramesPerPacket;
    InputSamplesPerPacket = InputBytesPerPacket / BytesPerSample;

    InputDeviceBufferSamples = 
      bufferFrameSize * InputSamplesPerPacket * InputFramesPerPacket;

    jlog("Stat: adin_darwin: input device's buffer size (# of samples): %d\n", 
	     InputDeviceBufferSamples);

    AudioStreamBasicDescription outDesc;
    outDesc.mSampleRate = sfreq;
    outDesc.mFormatID = kAudioFormatLinearPCM;
    outDesc.mFormatFlags = 
      kAudioFormatFlagIsSignedInteger | 
      kAudioFormatFlagIsPacked | 
      formatFlagEndian;
    outDesc.mBytesPerPacket = BytesPerSample;
    outDesc.mFramesPerPacket = 1;
    outDesc.mBytesPerFrame = BytesPerSample;
    outDesc.mChannelsPerFrame = 1;
    outDesc.mBitsPerChannel = BytesPerSample * BITS_PER_BYTE;

    printStreamInfo(&outDesc);

    OutputBitsPerChannel = outDesc.mBitsPerChannel;
    OutputBytesPerPacket = outDesc.mBytesPerPacket;

    OutputSamplesPerPacket = (OutputBitsPerChannel / BITS_PER_BYTE) / OutputBytesPerPacket;

    status = AudioConverterNew(&inDesc, &outDesc, &Converter);
    if (status != noErr){
      jlog("Error: adin_darwin: cannot create audio converter\n");
      return FALSE;
    }

    /*
    UInt32 nChan = inDesc.mChannelsPerFrame;
    int i;

    if (inDesc.mFormatFlags & kAudioFormatFlagIsNonInterleaved && nChan > 1) {
      UInt32 chmap[nChan];
      for (i = 0; i < nChan; i++)
	chmap[i] = 0;

      status = AudioConverterSetProperty(Converter, 
					 kAudioConverterChannelMap,
					 sizeof(chmap), chmap);
      if (status != noErr){
	jlog("cannot set audio converter's channel map\n");
	return FALSE;
      }
    }
    */

    status = 
      AudioConverterSetProperty(Converter, 
				kAudioConverterSampleRateConverterQuality,
				sizeof(ConvQuality), &ConvQuality);
    if (status != noErr){
      jlog("Error: adin_darwin: cannot set audio converter quality\n");
      return FALSE;
    }


    //jlog("Stat: adin_darwin: audio converter generated\n");

    /* allocate buffers */
    BufList = allocateAudioBufferList(inDesc.mBitsPerChannel / BITS_PER_BYTE, 
				      InputDeviceBufferSamples, 1);
    if (BufList == NULL) return FALSE;

    BufListBackup.mNumberBuffers = BufList->mNumberBuffers;

    BufListBackup.mBuffers[0].mNumberChannels = 1;
    BufListBackup.mBuffers[0].mDataByteSize = 
      BufList->mBuffers[0].mDataByteSize;
    BufListBackup.mBuffers[0].mData = BufList->mBuffers[0].mData;

    BufListConverted = allocateAudioBufferList(BytesPerSample, BUF_SAMPLES, 1);
    if (BufListConverted == NULL) return FALSE;
    //jlog("Stat: adin_darwin: buffers allocated\n");

    err = pthread_mutex_init(&MutexInput, NULL);
    if (err) {
      jlog("Error: adin_darwin: cannot init mutex\n");
      return FALSE;
    }
    err = pthread_cond_init(&CondInput, NULL);
    if (err) {
      jlog("Error: adin_darwin: cannot init condition variable\n");
      return FALSE;
    }

    /* register the callback */
    AURenderCallbackStruct input;
    input.inputProc = InputProc; // an AURenderCallback
    input.inputProcRefCon = 0;
    AudioUnitSetProperty(InputUnit,
			 kAudioOutputUnitProperty_SetInputCallback,
			 kAudioUnitScope_Global,
			 0,
			 &input,
			 sizeof(input));

    status = AudioUnitInitialize(InputUnit);
    if (status != noErr){
      jlog("Error: adin_darwin: InputUnit initialize failed\n");
      return FALSE;
    }

  }

  CoreAudioInit = TRUE;

  jlog("Stat: adin_darwin: CoreAudio: initialized\n");

  return TRUE;
}

boolean adin_mic_begin(char *pathname){ return TRUE; }
boolean adin_mic_end(){ return TRUE; }

int adin_mic_read(void *buffer, int nsamples) {
  OSStatus status;

#ifdef DEBUG
  jlog("Stat: adin_darwin: read: %d samples required\n", nsamples);
#endif

  if (!CoreAudioHasInputDevice) 
    return -1;

  if (!CoreAudioRecordStarted) {
    status = AudioOutputUnitStart(InputUnit);
    CoreAudioRecordStarted = TRUE;
  }
  
  UInt32 capacity = BUF_SAMPLES * OutputSamplesPerPacket;
  UInt32 npackets = nsamples * OutputSamplesPerPacket;

  UInt32 numDataPacketsNeeded;

  Sample* inputDataBuf = (Sample*)(BufListConverted->mBuffers[0].mData);

  numDataPacketsNeeded = npackets < capacity ? npackets : capacity;

#ifdef DEBUG
  jlog("Stat: adin_darwin: numDataPacketsNeeded=%d\n", numDataPacketsNeeded);
#endif

  status = AudioConverterFillComplexBuffer(Converter, 
					   ConvInputProc, 
					   NULL, // user data
					   &numDataPacketsNeeded, 
					   BufListConverted, 
					   NULL // packet description
					   );
  if (status != noErr) {
    jlog("Error: adin_darwin: AudioConverterFillComplexBuffer: failed\n");
    return -1;
  }

#ifdef DEBUG
  jlog("Stat: adin_darwin: %d bytes filled (BufListConverted)\n", 
	   BufListConverted->mBuffers[0].mDataByteSize);
#endif

  int providedSamples = numDataPacketsNeeded / OutputSamplesPerPacket;

  pthread_mutex_lock(&MutexInput);

#ifdef DEBUG
  jlog("Stat: adin_darwin: provided samples: %d\n", providedSamples);
#endif

  Sample* dst_data = (Sample*)buffer;

  int i;

  int count = 0;

  for (i = 0; i < providedSamples; i++) {
    dst_data[i] = inputDataBuf[i];
    if (dst_data[i] == 0) count++;
  }

  //jlog("Stat: adin_darwin: %d zero samples\n", count);


  pthread_mutex_unlock(&MutexInput);

#ifdef DEBUG
  jlog("Stat: adindarwin: EXIT: %d samples provided\n", providedSamples);
#endif

  return providedSamples;
}

/** 
 * Function to pause audio input (wait for buffer flush)
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_pause()
{
  OSStatus status = 0;

  if (CoreAudioHasInputDevice && CoreAudioRecordStarted) {
    status = AudioOutputUnitStop(InputUnit);
    CoreAudioRecordStarted = FALSE;
  }
  return (status == 0) ? TRUE : FALSE;
}

/** 
 * Function to terminate audio input (disgard buffer)
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_terminate()
{
  return TRUE;
}
/** 
 * Function to resume the paused / terminated audio input
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_resume()
{
  return TRUE;
}

/** 
 * 
 * Function to return current input source device name
 * 
 * @return string of current input device name.
 * 
 */
char *
adin_mic_input_name()
{
  return(deviceName);
}
