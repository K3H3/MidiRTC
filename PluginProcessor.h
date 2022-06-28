/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
//==============================================================================
#include <rtc/rtc.hpp>
#include <parse_cl.h>
#include <nlohmann/json.hpp>

//standard bibs c
#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <thread>
#include <unordered_map>

class MidiRTCAudioProcessor  : public juce::AudioProcessor
{
public:
    float noteOnVel;
    
    //==============================================================================
    MidiRTCAudioProcessor();
    ~MidiRTCAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    std::string getLocalId();
    std::string getPartnerId();
    void setPartnerId(std::string partnerId);
    void connectToPartner();
    
    bool isConnected() {
        return connected;
    };
    
private:
    //binary byteBuffer();
    //juce::MemoryOutputStream byteStream{7};
    //std::promise<void> wsPromise;
    //std::future<void> wsFuture;

    std::uint8_t expRunNum = 0;
    std::uint8_t runningNum = 0;
    bool connected = false;
    bool sending = false;
    rtc::Configuration config;
    std::weak_ptr<rtc::WebSocket> wws;
    std::shared_ptr<rtc::WebSocket> ws;
    std::shared_ptr<rtc::DataChannel> dc;
    std::shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration& config, 
        std::weak_ptr<rtc::WebSocket> wws, std::string id);
    std::string localId;
    std::string partnerId;
    juce::MidiMessage midiDummy;

    juce::MidiMessage recreateMidiMessage(rtc::binary messageData);
    void setLocalId(std::string localId);
    void generateLocalId(size_t length);
    void resetRunningNum() {
        runningNum = 0;
    };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiRTCAudioProcessor)
};