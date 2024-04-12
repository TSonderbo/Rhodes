/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EPianoAudioProcessor::EPianoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ), apvts(*this, nullptr, "Parameters", createParams())
#endif
{
    for (int i = 0; i < config::mpe::numVoices; i++)
    {
        synth.addVoice(new Tine());
    }

    synth.setVoiceStealingEnabled(true);

    synth.enableLegacyMode(24);
}

EPianoAudioProcessor::~EPianoAudioProcessor()
{

}

//==============================================================================
const juce::String EPianoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EPianoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EPianoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EPianoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EPianoAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EPianoAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EPianoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EPianoAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EPianoAudioProcessor::getProgramName (int index)
{
    return {};
}

void EPianoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EPianoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < synth.getNumVoices(); i++)
    {
        if (auto voice = dynamic_cast<Tine*>(synth.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate);
        }
    }
}

void EPianoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EPianoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EPianoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    auto numSamples = buffer.getNumSamples();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    synth.renderNextBlock(buffer, midiMessages, 0, numSamples);
    
    scopeDataCollector.process(buffer.getReadPointer(0), (size_t)numSamples);
}

//==============================================================================
bool EPianoAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EPianoAudioProcessor::createEditor()
{
    return new EPianoAudioProcessorEditor (*this);
}

//==============================================================================
void EPianoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void EPianoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EPianoAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout EPianoAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //Inserts params here...
    params.push_back(std::make_unique<juce::AudioParameterFloat>(config::parameter::id_velocity, config::parameter::name_velocity, juce::NormalisableRange<float>(0.1f, 20.0f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(config::parameter::id_amplitude, config::parameter::name_amplitude , juce::NormalisableRange<float>(0.1f, 100.0f), 50.0f));

    return { params.begin(), params.end() };
}

AudioBufferQueue& EPianoAudioProcessor::getAudioBufferQueue()
{
    return audioBufferQueue;
}