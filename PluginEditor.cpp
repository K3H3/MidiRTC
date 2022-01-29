/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rtc;
using namespace std;

string localId;


void generateLocalId(size_t length) {
    static const string characters(
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    string id(length, '0');
    default_random_engine rng(random_device{}());
    uniform_int_distribution<int> dist(0, int(characters.size() - 1));
    generate(id.begin(), id.end(), [&]() { return characters.at(dist(rng)); });
    localId = id;
}

//==============================================================================
MidiRTCAudioProcessorEditor::MidiRTCAudioProcessorEditor (MidiRTCAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (340, 300);
    
    Configuration config;

    string stunServer = "";

    generateLocalId(4);
      
    //=============================================
    //config.iceServers.emplace_back("mystunserver.org:3478");

    //PeerConnection pc(config);

    //pc.onLocalDescription([](Description sdp) {
        // Send the SDP to the remote peer
        //MY_SEND_DESCRIPTION_TO_REMOTE oder kopiere SDP an Clipboard (std::string(sdp));
        //cout << string(sdp) << endl;
        //});
    //versuch ende
 
    midiInputVolumeSlider.setRange(0.0, 127.0, 1.0);
    midiInputVolumeSlider.setPopupDisplayEnabled(true, false, this);
    midiInputVolumeSlider.setTextValueSuffix(" My Midi Volume");
    midiInputVolumeSlider.setValue(1.0);

    midiOutputVolumeSlider.setRange(0.0, 127.0, 1.0);
    midiOutputVolumeSlider.setPopupDisplayEnabled(true, false, this);
    midiOutputVolumeSlider.setTextValueSuffix(" Received Midi");
    midiOutputVolumeSlider.setValue(1.0);

    addAndMakeVisible(&ownIdLabel);
    addAndMakeVisible(&midiInputVolumeSlider);
    addAndMakeVisible(&midiOutputVolumeSlider);

    midiInputVolumeSlider.addListener(this);
    midiOutputVolumeSlider.addListener(this);
}

MidiRTCAudioProcessorEditor::~MidiRTCAudioProcessorEditor()
{
    ownIdLabel.setLookAndFeel(nullptr);
}

//==============================================================================
void MidiRTCAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    audioProcessor.noteOnVel = midiInputVolumeSlider.getValue();
}

//==============================================================================
void MidiRTCAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    int topPadding = 14;

    String generalDescription{ "Real time midi connector" };
    String localIdDescription{ "Your local ID is: " + localId };
    String otherIdDescription{ "Put partner ID here to connect: " };

    auto bounds = getLocalBounds();
    auto headerArea = bounds.removeFromTop(bounds.getHeight() * 0.1);
    auto settingsArea = bounds.removeFromTop(bounds.getHeight() * 0.25);
    auto inputVolumeArea = bounds.removeFromLeft(bounds.getWidth() * 0.5);

    //draw background
    g.fillAll(Colours::myYellow);

    //draw header field
    g.setColour(Colours::black);
    g.setFont(Font("Verdana", 20, 0));
    g.setFont(Font(17.f, Font::plain));
    g.drawFittedText(generalDescription, 0, 0, getWidth(), getHeight(), Justification::centredTop, 1);
    
    //draw info and ID field
    ownIdLabel.setColour(Label::textColourId, Colours::black);
    ownIdLabel.setColour(Label::backgroundColourId, Colours::cornflowerblue);
    ownIdLabel.setText(localIdDescription, dontSendNotification);
    
    otherIdLabel.setColour(Label::textColourId, Colours::black);
    otherIdLabel.setColour(Label::backgroundColourId, Colours::cornflowerblue);
    otherIdLabel.setText(otherIdDescription, dontSendNotification);

    //ownInfoLabel.attachToComponent(&ownIdLabel, false);


    /*idInputLabel.setText(otherIdDescription, dontSendNotification);
    idInputLabel.attachToComponent(&ownIdLabel, false);
    idInputLabel.setColour(Label::textColourId, Colours::black);
    idInputLabel.setFont(Font(17.f, Font::plain));
    idInputLabel.setJustificationType(Justification::centred);
    idInputLabel.setEditable(true);
    */
    //draw input field
    juce::Rectangle <int> inputArea(inputVolumeArea);
    g.setColour(Colours::myBlue);
    g.fillRect(inputArea);
    
    //draw external field
    g.setColour(Colours::myPink);
    g.fillRect(bounds);
}

void MidiRTCAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    using namespace juce;
    auto bounds = getLocalBounds();
    auto headerArea = bounds.removeFromTop(bounds.getHeight() * 0.1);

    //to be set!
    auto settingsArea = bounds.removeFromTop(bounds.getHeight() * 0.25);
    ownIdLabel.setBounds(settingsArea.removeFromTop(settingsArea.getHeight()*0.25));

    otherIdLabel.setBounds(settingsArea.removeFromTop(settingsArea.getHeight() * 0.5));
    ownInfoLabel.setBounds(settingsArea);
    //asdasd
    auto inputVolumeArea = bounds.removeFromLeft(bounds.getWidth() * 0.5);

    //auto midiInputVolumeSliderPos = inputVolumeArea.setPosition(juce::Justification::centred, 1);
    midiInputVolumeSlider.setBounds(inputVolumeArea.removeFromLeft(inputVolumeArea.getWidth() * 0.2).reduced(3));
    //midiInputVolumeSlider.setBounds(inputVolumeArea.getWidth() * 0.2);

    auto outputVolumeArea = bounds;
    midiOutputVolumeSlider.setBounds(outputVolumeArea.removeFromLeft(outputVolumeArea.getWidth() * 0.2).reduced(3));
}

//13.12 HALBFERTIG
//void LookAndFeel::drawLinearSlider(juce::Graphics& g,
//    int x, int y, int width, int height,
//    float sliderPos,
//    float minSliderPos,
//    float maxSliderPos,
//    const juce::Slider::SliderStyle,
//    juce::Slider& slider)
//{
//    using namespace juce;
//
//    auto bounds = Rectangle<float>(x, y, width, height);
//
//    auto enabled = slider.isEnabled();
//
//    g.setColour(enabled ? Colour(0u, 95u, 106u) : Colours::petrol);
//    g.fillEllipse(bounds);
//
//}
