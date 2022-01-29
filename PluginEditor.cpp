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

    //addAndMakeVisible(&localIdLabel);
    //addAndMakeVisible(&partnerIdLabel);
    addAndMakeVisible(&midiInputVolumeSlider);
    addAndMakeVisible(&midiOutputVolumeSlider);

    //partnerIdText.

    addAndMakeVisible(&localIdText);
    addAndMakeVisible(&partnerIdText);
    //partnerIdText.setEditable(true);
    //partnerIdText.setColour(juce::Label::backgroundColourId, juce::Colours::darkblue);
   // partnerIdText.onTextChange = [this] { uppercaseText.setText(partnerIdText.getText().toUpperCase(), juce::dontSendNotification); };

    midiInputVolumeSlider.addListener(this);
    midiOutputVolumeSlider.addListener(this);
}

MidiRTCAudioProcessorEditor::~MidiRTCAudioProcessorEditor()
{
    localIdLabel.setLookAndFeel(nullptr);
    partnerIdLabel.setLookAndFeel(nullptr);
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
    String localIdLabelDescription{ "Local ID: " };
    String partnerIdLabelDescription{ "Partner ID: " };

    //draw background
    g.fillAll(Colours::myYellow);

    //draw header field
    g.setColour(Colours::black);
    g.setFont(Font("Verdana", 20, 0));
    g.setFont(Font(17.f, Font::plain));
    g.drawFittedText(generalDescription, 0, 0, getWidth(), getHeight(), Justification::centredTop, 1);
    
    //draw info and ID field
    localIdLabel.attachToComponent(&localIdText, true);
    localIdLabel.setColour(Label::textColourId, Colours::black);
    //localIdLabel.setColour(Label::backgroundColourId, Colours::cornflowerblue);
    localIdLabel.setText(localIdLabelDescription, dontSendNotification);
    localIdLabel.setJustificationType(Justification::centred);

    localIdText.setColour(TextEditor::highlightedTextColourId, Colours::black);
    localIdText.setColour(TextEditor::backgroundColourId, Colours::cornflowerblue);
    localIdText.setColour(TextEditor::highlightColourId, Colours::transparentBlack);
    localIdText.setFont(Font(17.f, Font::plain));
    localIdText.setText(localId, dontSendNotification);
    localIdText.setReadOnly(true);
    localIdText.setCaretVisible(false);
    localIdText.selectAll();
    localIdText.copy();
    
    partnerIdLabel.attachToComponent(&partnerIdText, true);
    partnerIdLabel.setColour(Label::textColourId, Colours::black);
    //partnerIdLabel.setColour(Label::backgroundColourId, Colours::cornflowerblue);
    partnerIdLabel.setText(partnerIdLabelDescription, dontSendNotification);
    partnerIdLabel.setJustificationType(Justification::centred);

    //partnerIdText.setEditable(true);
    //partnerIdText.setColour(Label::backgroundColourId, Colours::cornflowerblue);
    partnerIdText.setColour(TextEditor::textColourId, Colours::black);
    partnerIdText.setColour(TextEditor::backgroundColourId, Colours::cornflowerblue);
    partnerIdText.setFont(Font(17.f, Font::plain));
    partnerIdText.setInputRestrictions(4);

    g.setColour(Colours::cornflowerblue);
    g.fillRect(localIdArea);
    g.fillRect(partnerIdArea);

    //draw input field
    g.setColour(Colours::myBlue);
    g.fillRect(inputVolumeArea);

    
    //draw external field
    g.setColour(Colours::myPink);
    g.fillRect(outputVolumeArea);
}

void MidiRTCAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    using namespace juce;
    bounds = getLocalBounds();
    headerArea = bounds.removeFromTop(bounds.getHeight() * 0.1);

    settingsArea = bounds.removeFromTop(bounds.getHeight() * 0.25);

    //localIdLabel.setBounds(settingsArea.withTrimmedBottom(settingsArea.getHeight() * 0.5));
    //partnerIdText.setBounds(settingsArea.withTrimmedTop(settingsArea.getHeight() * 0.5).withTrimmedLeft(settingsArea.getWidth()*0.5));
    localIdArea = settingsArea.withTrimmedBottom(settingsArea.getHeight() * 0.5);
    localIdText.setBounds(localIdArea.withTrimmedLeft(localIdArea.getWidth() * 0.5));
    partnerIdArea = settingsArea.withTrimmedTop(settingsArea.getHeight() * 0.5);
    partnerIdText.setBounds(partnerIdArea.withTrimmedLeft(partnerIdArea.getWidth() * 0.5));
    //partnerIdText.setBounds(100, 50, getWidth() - 110, 20);
    //partnerIdText.setBounds

    inputVolumeArea = bounds.removeFromLeft(bounds.getWidth() * 0.5);
    midiInputVolumeSlider.setBounds((inputVolumeArea.withTrimmedRight(inputVolumeArea.getWidth() * 0.8)).reduced(3));

    outputVolumeArea = bounds;
    midiOutputVolumeSlider.setBounds(outputVolumeArea.withTrimmedRight(outputVolumeArea.getWidth() * 0.8).reduced(3));
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
