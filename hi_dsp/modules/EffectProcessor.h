/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef HI_EFFECT_PROCESSOR_H_INCLUDED
#define HI_EFFECT_PROCESSOR_H_INCLUDED

#include <array>

namespace hise { using namespace juce;

#define EFFECT_PROCESSOR_COLOUR 0xff3a6666

/** Base class for all Processors that applies a audio effect on the audio data. 
*	@ingroup effect
*
*/
class EffectProcessor: public Processor
{
public:

	EffectProcessor(MainController *mc, const String &uid, int numVoices): 
		Processor(mc, uid, numVoices),	
		isTailing(false),
		tailCheck(2, 0)
	{
		
	};

	virtual ~EffectProcessor()
	{
		modChains.clear();
	};

	/** Renders all chains (envelopes & voicestart are rendered monophonically. */
	void renderAllChains(int startSample, int numSamples)
	{
		for (auto& mb : modChains)
		{
			if (!mb.getChain()->shouldBeProcessedAtAll())
				continue;

			mb.calculateMonophonicModulationValues(startSample, numSamples);
			mb.calculateModulationValuesForCurrentVoice(0, startSample, numSamples);

			if (mb.isAudioRateModulation())
				mb.expandVoiceValuesToAudioRate(0, startSample, numSamples);
		}
	}

	/** You have to override this method, since almost every effect needs the samplerate anyway. */
	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		Processor::prepareToPlay(sampleRate, samplesPerBlock);

		if (samplesPerBlock > 0 && hasTail())
		{
			ProcessorHelpers::increaseBufferIfNeeded(tailCheck, samplesPerBlock);	
		}

		for (auto& mc : modChains)
			mc.prepareToPlay(sampleRate, samplesPerBlock);
	};

	Colour getColour() const override 
	{
		return Colour(EFFECT_PROCESSOR_COLOUR);
	}

	/** Overwrite this method if the effect has a tail (produces sound if no input is active */
	virtual bool hasTail() const = 0;
	
	/** Checks if the effect is tailing off. This simply returns the calculated value, but the EffectChain overwrites this. */
	virtual bool isTailingOff() const {	return isTailing; };

	/** Renders the next block and applies the effect to the buffer. */
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) = 0;

	virtual void handleHiseEvent(const HiseEvent &m)
	{
		for (auto& mc : modChains)
			mc.handleHiseEvent(m);
	};

protected:

	/** Takes a copy of the buffer before it is processed to check if a tail was added after processing. */
	void saveBufferForTailCheck(AudioSampleBuffer &b, int startSample, int numSamples)
	{
		if (hasTail())
		{
			tailCheck.copyFrom(0, startSample, b.getReadPointer(0, startSample), numSamples);
			tailCheck.copyFrom(1, startSample, b.getReadPointer(1, startSample), numSamples);
		}
	}

	/** If your effect produces a tail, you have to call this method after your processing. */
	void checkTailing(AudioSampleBuffer &b, int startSample, int numSamples);

	ModulatorChain::Collection modChains;

private:

	AudioSampleBuffer tailCheck;

	bool isTailing = false;
};

/** A MasterEffectProcessor renders a effect on a block of audio samples. 
*	@ingroup effect
*
*	Derive all effects that are processed on the whole buffer from this class. For polyphonic effects, use VoiceEffectProcessor as baseclass.
*/
class MasterEffectProcessor: public EffectProcessor,
							 public RoutableProcessor
{
public:
	MasterEffectProcessor(MainController *mc, const String &uid): EffectProcessor(mc, uid, 1)
	{
		getMatrix().init();
		getMatrix().setOnlyEnablingAllowed(true);
		getMatrix().setNumAllowedConnections(2);
	};

	virtual ~MasterEffectProcessor() {};

	Path getSpecialSymbol() const override
	{
		Path path;

		path.loadPathFromData (HiBinaryData::SpecialSymbols::masterEffect, sizeof (HiBinaryData::SpecialSymbols::masterEffect));

		return path;
	}

	

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = Processor::exportAsValueTree();
		v.addChild(getMatrix().exportAsValueTree(), -1, nullptr);

		return v;
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		Processor::restoreFromValueTree(v);

		ValueTree r = v.getChildWithName("RoutingMatrix");

		if (r.isValid())
		{
			getMatrix().restoreFromValueTree(r);
		}
	}


	virtual void numDestinationChannelsChanged() override 
	{

	};
	virtual void numSourceChannelsChanged() override
	{
		
	};

	virtual void startMonophonicVoice()
	{
		for (auto& mb : modChains)
			mb.startVoice(0);
	}
	
	virtual void stopMonophonicVoice()
	{
		for (auto& mb : modChains)
			mb.stopVoice(0);
	}


	virtual void resetMonophonicVoice()
	{
		for (auto& mb : modChains)
			mb.resetVoice(0);
	}

	/** A wrapper function around the actual processing.
	*
	*	You can assume that all internal chains are processed and the numSample amount is set according to the stepsize calculated with
	*	calculateStepSize().
	*	That means you can grab the current modulation value using getCurrentModulationValue(), set the parameters and process the block
	*	with smooth parameter changes.
	*
	*	Also this effect grabs the whole buffer (it can be divided by incoming midi messages for VoiceEffectProcessors).
	*/
	virtual void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) = 0;

	/** This only renders the modulatorChains. */
	void renderNextBlock(AudioSampleBuffer &/*buffer*/, int startSample, int numSamples) final override
	{
		jassert(isOnAir());

		renderAllChains(startSample, numSamples);
	}

	/** This renders the whole buffer. 
	*
	*	You can still modulate the wet signal amount or pan effects using multiplications
	**/
	virtual void renderWholeBuffer(AudioSampleBuffer &buffer)
	{
		if (getLeftSourceChannel() != -1 && getRightSourceChannel() != -1 &&
            getLeftSourceChannel() < getMatrix().getNumDestinationChannels() &&
            getRightSourceChannel() < getMatrix().getNumDestinationChannels())
		{
			float *leftChannel = buffer.getWritePointer(getLeftSourceChannel());
			float *rightChannel = buffer.getWritePointer(getRightSourceChannel());

			float *samples[2] = { leftChannel, rightChannel };

			const int samplesToUse = buffer.getNumSamples();

			AudioSampleBuffer stereoBuffer(samples, 2, samplesToUse);

			applyEffect(stereoBuffer, 0, samplesToUse);

#if ENABLE_ALL_PEAK_METERS
			currentValues.outL = stereoBuffer.getMagnitude(0, 0, samplesToUse);
			currentValues.outR = stereoBuffer.getMagnitude(1, 0, samplesToUse);
#endif

			if (getMatrix().isEditorShown())
			{
				float gainValues[NUM_MAX_CHANNELS];

				jassert(getMatrix().getNumSourceChannels() == buffer.getNumChannels());

				for (int i = 0; i < buffer.getNumChannels(); i++)
				{
					gainValues[i] = buffer.getMagnitude(i, 0, samplesToUse);
				}

				getMatrix().setGainValues(gainValues, true);
				getMatrix().setGainValues(gainValues, false);

			}
		}
	};
    
};

/** A EffectProcessor which allows monophonic modulation of its parameters.
*	@ingroup effects
*
*	If your effect wants to do more than modulate the wet amount or anything else that can be achieved
*	with a trivial multiplication, you can subclass it from this class and it takes care of the following
*	things:
*
*	- calculation of all internal chains (polyphonic modulators only have one voice, so there can be some
*	  value jumps.
*	- analysis of the dynamic range within the modulation buffers to determine the update rate.
*	- sequentielly calling the virtual function applyEffect with the divided sections to allow the subclass
*	  to change the parameters before rendering.
*
*	Beware that this leads to multiple calls if the modulation buffers contain large dynamics / the incoming
*	midi messages are frequent, so if you need heavy processing, consider using MasterEffectProcessor class, which
*	also allows modulation of the basic parameters.
*/
class MonophonicEffectProcessor: public EffectProcessor
{
public:

	MonophonicEffectProcessor(MainController *mc, const String &uid): EffectProcessor(mc, uid, 1) {};
	
	virtual ~MonophonicEffectProcessor() {};

	Path getSpecialSymbol() const override
	{
		Path path;

		path.loadPathFromData (HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath, sizeof (HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath));

		return path;
	}

	virtual void startMonophonicVoice(int noteNumber=-1)
	{
		ignoreUnused(noteNumber);

		for (auto& mb : modChains)
			mb.startVoice(0);
	}
	
	virtual void stopMonophonicVoice()
	{
		for (auto& mb : modChains)
			mb.stopVoice(0);
	}

	virtual void resetMonophonicVoice() 
	{
		for (auto& mb : modChains)
			mb.resetVoice(0);
	}

	/** A wrapper function around the actual processing.
	*
	*	You can assume that all internal chains are processed and the numSample amount is set according to the stepsize calculated with
	*	calculateStepSize().
	*	That means you can grab the current modulation value using getCurrentModulationValue(), set the parameters and process the block
	*	with smooth parameter changes.
	*
	*	Also this effect grabs the whole buffer (it can be divided by incoming midi messages for VoiceEffectProcessors).
	*/
	virtual void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) = 0;

	/** Renders the next block and applies the effect to the buffer. */
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		jassert(isOnAir());

		renderAllChains(startSample, numSamples);

		constexpr int stepSize = 64;

		while(numSamples >= stepSize)
		{
			applyEffect(buffer, startSample, stepSize);

			startSample += stepSize;
			numSamples  -= stepSize;
		}

		if(numSamples != 0)
		{
			applyEffect(buffer, startSample, numSamples);
		}

#if ENABLE_ALL_PEAK_METERS
		currentValues.outL = buffer.getMagnitude(0, startSample, numSamples);
		currentValues.outR = buffer.getMagnitude(1, startSample, numSamples);
#endif
	}
};



/** A VoiceEffectProcessor has multiple states that allows polyphonic rendering of the audio effect. 
*	@ingroup effect
*/
class VoiceEffectProcessor: public EffectProcessor
{
public:

	VoiceEffectProcessor(MainController *mc, const String &uid, int numVoices_): 
		EffectProcessor(mc, uid, numVoices_)
	{};

	virtual ~VoiceEffectProcessor() {};

	Path getSpecialSymbol() const override;

	/** This is called before every voice is processed. Use this to calculate all non polyphonic modulators in your subclasses chains! */
	virtual void preRenderCallback(int startSample, int numSamples)
	{
		for (auto& mb : modChains)
			mb.calculateMonophonicModulationValues(startSample, numSamples);
	}

	/** A wrapper function around the actual processing.
	*
	*	You can assume that all internal chains are processed and the numSample amount is set according to the stepsize calculated with
	*	calculateStepSize().
	*	That means you can grab the current modulation value using getCurrentModulationValue(), set the parameters and process the block
	*	with smooth parameter changes.
	*/
	virtual void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSample) = 0;

	void preVoiceRendering(int voiceIndex, int startSample, int numSamples)
	{
		for (auto& mb : modChains)
		{
			mb.calculateModulationValuesForCurrentVoice(voiceIndex, startSample, numSamples);
			if (mb.isAudioRateModulation())
				mb.expandVoiceValuesToAudioRate(voiceIndex, startSample, numSamples);
		}
	}

	/** renders a voice and applies the effect on the voice. */
	virtual void renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
	{
		jassert(isOnAir());

		if(hasTail()) saveBufferForTailCheck(b, startSample, numSamples);

		const int startIndex = startSample;
		const int samplesToCheck = numSamples;

		preVoiceRendering(voiceIndex, startSample, numSamples);

		constexpr int stepSize = 64;

		while(numSamples >= stepSize)
		{
			applyEffect(voiceIndex, b, startSample, stepSize);

			startSample += stepSize;
			numSamples  -= stepSize;
		}

		if(numSamples != 0)
		{
			applyEffect(voiceIndex, b, startSample, numSamples);
		}

		if(hasTail()) checkTailing(b, startIndex, samplesToCheck);

		return;
	}

	virtual void startVoice(int voiceIndex, int /*noteNumber*/)
	{
		for (auto& mb : modChains)
			mb.startVoice(voiceIndex);
	}

	virtual void stopVoice(int voiceIndex)
	{
		for (auto& mb : modChains)
			mb.stopVoice(voiceIndex);
	}

	virtual void reset(int voiceIndex)
	{
		for (auto& mb : modChains)
			mb.resetVoice(voiceIndex);
	}

	void handleHiseEvent(const HiseEvent &m) final override
	{
		for (auto& mb : modChains)
			mb.handleHiseEvent(m);
	};
};

} // namespace hise

#endif
