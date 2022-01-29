/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

using namespace juce;
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

struct CustomVolumeSlider : Slider
{
    CustomVolumeSlider() : Slider(Slider::SliderStyle::LinearBarVertical,
        Slider::TextEntryBoxPosition::NoTextBox)
    {
    }
};
//==============================================================================

class MidiRTCAudioProcessorEditor  :    public AudioProcessorEditor,
                                        private Slider::Listener
{
public:
    MidiRTCAudioProcessorEditor (MidiRTCAudioProcessor&);
    ~MidiRTCAudioProcessorEditor() override;

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    void sliderValueChanged(Slider* slider) override;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MidiRTCAudioProcessor& audioProcessor;

    CustomVolumeSlider midiInputVolumeSlider, midiOutputVolumeSlider;

    Label ownIdLabel, otherIdLabel, ownInfoLabel;

    TextEditor labelEditor;

    //LookAndFeel lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiRTCAudioProcessorEditor)
};

