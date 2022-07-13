/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rtc;
using namespace std;


//==============================================================================
MidiRTCAudioProcessorEditor::MidiRTCAudioProcessorEditor(MidiRTCAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p)
{
	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.
	setSize(340, 300);

	midiInputVolumeSlider.setRange(0.0, 127.0, 1.0);
	midiInputVolumeSlider.setPopupDisplayEnabled(true, false, this);
	midiInputVolumeSlider.setTextValueSuffix(" My Midi Volume");
	midiInputVolumeSlider.setValue(1.0);

	midiOutputVolumeSlider.setRange(0.0, 127.0, 1.0);
	midiOutputVolumeSlider.setPopupDisplayEnabled(true, false, this);
	midiOutputVolumeSlider.setTextValueSuffix(" Received Midi");
	midiOutputVolumeSlider.setValue(1.0);

	addAndMakeVisible(&midiInputVolumeSlider);
	addAndMakeVisible(&midiOutputVolumeSlider);

	addAndMakeVisible(&localIdText);
	addAndMakeVisible(&partnerIdText);

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
void MidiRTCAudioProcessorEditor::paint(juce::Graphics& g)
{
	using namespace juce;
	int topPadding = 14;


	String generalDescription{ "Real time midi connector" };
	String localIdLabelDescription{ "Local ID: " };
	String partnerIdLabelDescription{ "Partner ID: " };

	//draw background
	//g.fillAll(Colours::lighyellow);

	//handle connection status with colours
	if (audioProcessor.isConnected())
	{
		repaint();
		g.setColour(Colours::palegreen);
	}
	else
	{
		g.setColour(Colours::lightpink);
	}

	g.fillRect(headerArea);

	//draw header field
	g.setColour(Colours::black);
	g.setFont(Font("Verdana", 20, 0));
	g.setFont(Font(17.f, Font::plain));
	g.drawFittedText(generalDescription, 0, 0, getWidth(), getHeight(), Justification::centredTop, 1);


	/*
	draw infoand ID field
	localIdLabelDescription is the field on the left
	localIdText is the localId
	*/
	localIdLabel.attachToComponent(&localIdText, true);
	localIdLabel.setColour(Label::textColourId, Colours::black);
	localIdLabel.setText(localIdLabelDescription, dontSendNotification);
	localIdLabel.setJustificationType(Justification::centred);

	localIdText.setColour(TextEditor::highlightedTextColourId, Colours::black);
	localIdText.setColour(TextEditor::backgroundColourId, Colours::lightyellow);
	localIdText.setColour(TextEditor::highlightColourId, Colours::transparentBlack);
	localIdText.setFont(Font(17.f, Font::plain));
	localIdText.setText(audioProcessor.getLocalId(), dontSendNotification);
	localIdText.setReadOnly(true);
	localIdText.setCaretVisible(false);
	localIdText.selectAll();

	//to do: add copy to clipboard function on click
	//void mouseDoubleClick (const MouseEvent&) override;
	//localIdText.copyToClipboard();

	partnerIdLabel.attachToComponent(&partnerIdText, true);
	partnerIdLabel.setColour(Label::textColourId, Colours::black);
	partnerIdLabel.setText(partnerIdLabelDescription, dontSendNotification);
	partnerIdLabel.setJustificationType(Justification::centred);

	partnerIdText.setColour(TextEditor::textColourId, Colours::black);
	partnerIdText.setColour(TextEditor::backgroundColourId, Colours::lightyellow);
	partnerIdText.setFont(Font(17.f, Font::plain));
	partnerIdText.setText(audioProcessor.getPartnerId(), dontSendNotification);
	partnerIdText.setInputRestrictions(4);
	partnerIdText.onTextChange = [this] { audioProcessor.setPartnerId(partnerIdText.getText().toStdString()); };
	partnerIdText.onReturnKey = [this] { audioProcessor.connectToPartner(); };


	//draw and fill IDAreas
	g.setColour(Colours::lightyellow);
	g.fillRect(localIdArea);
	g.fillRect(partnerIdArea);

	//draw input field
	g.setColour(Colours::cornflowerblue);
	g.fillRect(inputVolumeArea);

	//draw external field
	g.setColour(Colours::cornflowerblue);
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

	localIdArea = settingsArea.withTrimmedBottom(settingsArea.getHeight() * 0.5);
	localIdText.setBounds(localIdArea.withTrimmedLeft(localIdArea.getWidth() * 0.5));
	partnerIdArea = settingsArea.withTrimmedTop(settingsArea.getHeight() * 0.5);
	partnerIdText.setBounds(partnerIdArea.withTrimmedLeft(partnerIdArea.getWidth() * 0.5));

	inputVolumeArea = bounds.removeFromLeft(bounds.getWidth() * 0.5);
	midiInputVolumeSlider.setBounds((inputVolumeArea.withTrimmedRight(inputVolumeArea.getWidth() * 0.8)).reduced(3));

	outputVolumeArea = bounds;
	midiOutputVolumeSlider.setBounds(outputVolumeArea.withTrimmedRight(outputVolumeArea.getWidth() * 0.8).reduced(3));
}