/*
  ==============================================================================

    HlacEncoder.h
    Created: 16 Apr 2017 10:18:36am
    Author:  Christoph

  ==============================================================================
*/

#ifndef HLACENCODER_H_INCLUDED
#define HLACENCODER_H_INCLUDED

#include "JuceHeader.h"

#include "CompressionHelpers.h"

class HlacEncoder
{
public:

	HlacEncoder():
		currentCycle(0),
		workBuffer(0)
	{};

	struct CompressorOptions
	{
		bool useDeltaEncoding = true;
		int fixedBlockWidth = -1;
		bool reuseFirstCycleLengthForBlock = true;
		bool removeDcOffset = true;
		float deltaCycleThreshhold = 0.2f;
		int bitRateForWholeBlock = 6;
		bool useDiffEncodingWithFixedBlocks = false;
	};


	void compress(AudioSampleBuffer& source, OutputStream& output);

	void reset();

	void setOptions(CompressorOptions& newOptions)
	{
		options = newOptions;
	}

	float getCompressionRatio() const { return ratio; }

private:

	bool encodeBlock(AudioSampleBuffer& block, OutputStream& output);

	uint8 getBitReductionAmountForMSEncoding(AudioSampleBuffer& block);

	bool isBlockExhausted() const
	{
		return indexInBlock >= COMPRESSION_BLOCK_SIZE;
	}



	bool encodeCycle(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output);
	bool encodeDiff(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output);
	bool encodeCycleDelta(CompressionHelpers::AudioBufferInt16& nextCycle, OutputStream& output);
	void  writeUncompressed(AudioSampleBuffer& block, OutputStream& output);

	bool writeCycleHeader(bool isTemplate, uint8 bitDepth, uint16 numSamples, OutputStream& output);
	bool writeDiffHeader(uint8 fullBitRate, uint8 errorBitRate, uint16 blockSize, OutputStream& output);

	uint16 getCycleLength(CompressionHelpers::AudioBufferInt16& block);
	uint16 getCycleLengthFromTemplate(CompressionHelpers::AudioBufferInt16& newCycle, CompressionHelpers::AudioBufferInt16& rest);

	BitCompressors::Collection collection;

	CompressionHelpers::AudioBufferInt16 currentCycle;

	CompressionHelpers::AudioBufferInt16 workBuffer;

	uint16 indexInBlock;

	uint32 numBytesWritten = 0;

	uint32 numTemplates = 0;
	uint32 numDeltas = 0;

	uint32 blockOffset = 0;

	uint8 bitRateForCurrentCycle;

	int16 firstCycleLength = -1;

	MemoryBlock readBuffer;

	CompressorOptions options;

	float ratio = 0.0f;

	uint64 readIndex = 0;

	double decompressionSpeed = 0.0;
};


#endif  // HLACENCODER_H_INCLUDED