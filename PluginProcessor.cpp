/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


using namespace juce;
//==============================================================================
//kopiert aus client benchmark

using namespace rtc;
using namespace std;
using namespace std::chrono_literals;

using chrono::milliseconds;
using chrono::steady_clock;

using json = nlohmann::json;

template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

unordered_map<string, shared_ptr<PeerConnection>> peerConnectionMap;
unordered_map<string, shared_ptr<DataChannel>> dataChannelMap;

shared_ptr<PeerConnection> createPeerConnection(const Configuration& config,
    weak_ptr<WebSocket> wws, string id); //peerConnection wird erstellt (implementation unten)
string randomId(size_t length); //Um id zu erstellen für PeerConnection

// Benchmark
const size_t messageSize = 65535; //eventuell später nach unten anpassen um pakete klein zu halten
binary messageData(messageSize); //erstellt messagedata (vektor) mit der größe messageSize
unordered_map<string, atomic<size_t>> receivedSizeMap;
unordered_map<string, atomic<size_t>> sentSizeMap;
bool noSend = false;

//==============================================================================
MidiRTCAudioProcessor::MidiRTCAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

MidiRTCAudioProcessor::~MidiRTCAudioProcessor()
{
}

//==============================================================================
const juce::String MidiRTCAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidiRTCAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MidiRTCAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MidiRTCAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MidiRTCAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidiRTCAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MidiRTCAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MidiRTCAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MidiRTCAudioProcessor::getProgramName (int index)
{
    return {};
}

void MidiRTCAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MidiRTCAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void MidiRTCAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MidiRTCAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void MidiRTCAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    juce::MidiBuffer processedMidi;

    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        const auto time = metadata.samplePosition;

        if (message.isNoteOn())
        {
            message = juce::MidiMessage::noteOn(message.getChannel(),
                message.getNoteNumber(),
                (juce::uint8)noteOnVel);
        }

        processedMidi.addEvent(message, time);
    }

    midiMessages.swapWith(processedMidi);
}

/*
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

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
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}
*/

//==============================================================================
bool MidiRTCAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MidiRTCAudioProcessor::createEditor()
{
    return new MidiRTCAudioProcessorEditor (*this);
}

//==============================================================================
void MidiRTCAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void MidiRTCAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiRTCAudioProcessor();
}
