/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


struct CustomVolumeSlider : juce::Slider
{
    CustomVolumeSlider() : juce::Slider(Slider::SliderStyle::LinearBarVertical,
        Slider::TextEntryBoxPosition::NoTextBox)
    {
    }
};
//==============================================================================

class MidiRTCAudioProcessorEditor  :    public juce::AudioProcessorEditor,
                                        private juce::Slider::Listener
{
public:
    MidiRTCAudioProcessorEditor (MidiRTCAudioProcessor&);
    ~MidiRTCAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void sliderValueChanged(juce::Slider* slider) override;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MidiRTCAudioProcessor& audioProcessor;
    CustomVolumeSlider midiInputVolumeSlider, midiOutputVolumeSlider;
    juce::Label localIdLabel, partnerIdLabel;
    juce::TextEditor localIdText, partnerIdText;
    
    juce::Rectangle<int> bounds, headerArea, settingsArea, localIdArea, partnerIdArea, inputVolumeArea, outputVolumeArea;

    //LookAndFeel lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiRTCAudioProcessorEditor)
};

//struct LookAndFeel : LookAndFeel_V4
//{
//    void drawLinearSlider(Graphics&,
//        int x, int y, int width, int height,
//        float sliderPos,
//        float minSliderPos,
//        float maxSliderPos,
//        const Slider::SliderStyle,
//        Slider&) override;
//};