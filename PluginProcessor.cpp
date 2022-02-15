/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

    Parts of this code were adapted and customized from Paul-Louis Ageneau (libdatachannel)
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


using namespace juce;
//==============================================================================

using namespace rtc;
using namespace std;
using namespace std::chrono_literals;
using chrono::milliseconds;
using chrono::steady_clock;
using json = nlohmann::json;

std::unordered_map<std::string, std::shared_ptr<rtc::PeerConnection>> peerConnectionMap;
std::unordered_map<std::string, std::shared_ptr<rtc::DataChannel>> dataChannelMap;

const size_t messageSize = 65535;
binary messageData(messageSize);
unordered_map<string, atomic<size_t>> receivedSizeMap;
unordered_map<string, atomic<size_t>> sentSizeMap;

bool noSend = false;
bool enableThroughputSet;
int throughtputSetAsKB;
int bufferSize;

const float STEP_COUNT_FOR_1_SEC = 100.0;
const int stepDurationInMs = int(1000 / STEP_COUNT_FOR_1_SEC);

template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

string MidiRTCAudioProcessor::getLocalId()
{
    return localId;
}

void MidiRTCAudioProcessor::setLocalId(string localId)
{
    this->localId = localId;
}

void MidiRTCAudioProcessor::setPartnerId(string partnerId)
{
    this->partnerId = partnerId;
}

void MidiRTCAudioProcessor::connectToPartner(string partnerId)
{
    auto pc = make_shared<PeerConnection>(config);
    DBG("Waiting for signaling to be connected...");

    if (partnerId.empty()) {
        DBG("no partnerId given");
        // Nothing to do
        return;
    }
    if (partnerId == localId) {
        DBG("Invalid remote ID (This is my local ID). Exiting...");
        return;
    }

    DBG( "Offering to " + partnerId );
    pc = createPeerConnection(config, ws, partnerId);

    // We are the offerer, so create a data channel to initiate the process
    const string label = "DC-" + std::to_string(1);
    cout << "Creating DataChannel with label \"" << label << "\"" << endl;
    DBG("Creating DataChannel with label \"" + label + "\"");
    auto dc = pc->createDataChannel(label);
    receivedSizeMap.emplace(label, 0);
    sentSizeMap.emplace(label, 0);

    // Set Buffer Size
    dc->setBufferedAmountLowThreshold(bufferSize);

    dc->onOpen([partnerId, wdc = make_weak_ptr(dc), label]() {
        cout << "DataChannel from " << partnerId << " open" << endl;
        DBG("DataChannel from " + partnerId + " open");
        if (noSend)
            return;

        if (enableThroughputSet)
            return;

        if (auto dcLocked = wdc.lock()) {
            try {
                while (dcLocked->bufferedAmount() <= bufferSize) {
                    dcLocked->send(messageData);
                    sentSizeMap.at(label) += messageData.size();
                }
            }
            catch (const std::exception& e) {
                std::cout << "Send failed: " << e.what() << std::endl;
                DBG("Send failed: " << e.what());
            }
        }
    });

    dc->onBufferedAmountLow([wdc = make_weak_ptr(dc), label]() {
        if (noSend)
            return;

        if (enableThroughputSet)
            return;

        auto dcLocked = wdc.lock();
        if (!dcLocked)
            return;

        // Continue sending
        try {
            while (dcLocked->isOpen() && dcLocked->bufferedAmount() <= bufferSize) {
                dcLocked->send(messageData);
                sentSizeMap.at(label) += messageData.size();
            }
        }
        catch (const std::exception& e) {
            std::cout << "Send failed: " << e.what() << std::endl;
            DBG("Send failed: " << e.what());
        }
    });

    dc->onClosed([partnerId]() { cout << "DataChannel from " << partnerId << " closed" << endl; });

    //a Data Channel, once opened, is bidirectional
    dc->onMessage([partnerId, wdc = make_weak_ptr(dc), label](variant<binary, string> data) {
        if (holds_alternative<binary>(data))
            receivedSizeMap.at(label) += get<binary>(data).size();
    });

    dataChannelMap.emplace(label, dc);

};

//function to create and setup PeerConnection
shared_ptr<PeerConnection> MidiRTCAudioProcessor::createPeerConnection(const Configuration& config,
    weak_ptr<WebSocket> wws, string id)
{
    auto pc = make_shared<PeerConnection>(config);

    DBG("peertest " + id);

    pc->onStateChange([](PeerConnection::State state) {
        cout << "State: " << state << endl;
        DBG("onStateChange");
        });

    pc->onGatheringStateChange(
        [](PeerConnection::GatheringState state) {
            DBG("onGatheringStateChange");
            cout << "Gathering State: " << state << endl;
        });

    pc->onLocalDescription([wws, id](Description description) {
        DBG("onLocalDescription");
        json message = {
            {"id", id}, {"type", description.typeString()}, {"description", string(description)} };

        if (auto ws = wws.lock())
            ws->send(message.dump());
        });

    pc->onLocalCandidate([wws, id](Candidate candidate) {
        DBG("onLocalCandidate");
        json message = { {"id", id},
                        {"type", "candidate"},
                        {"candidate", string(candidate)},
                        {"mid", candidate.mid()} };

        if (auto ws = wws.lock())
            ws->send(message.dump());
        });

    pc->onDataChannel([id](shared_ptr<DataChannel> dc) {
        DBG("onDatachannel");
        const string label = dc->label();
        DBG("DataChannel from " + id + " received with label \"" + label + "\"");

        DBG("###########################################");
        DBG("### Check other peer's screen for stats ###");
        DBG("###########################################");

        receivedSizeMap.emplace(dc->label(), 0);
        sentSizeMap.emplace(dc->label(), 0);

        // Set Buffer Size
        dc->setBufferedAmountLowThreshold(bufferSize);

        //Call paint to render partnerIdTextField

        if (!noSend && !enableThroughputSet) {
            try {
                while (dc->bufferedAmount() <= bufferSize) {
                    dc->send(messageData);
                    sentSizeMap.at(label) += messageData.size();
                }
            }
            catch (const std::exception& e) {
                std::cout << "Send failed: " << e.what() << std::endl;
                DBG("Send failed: " << e.what());
            }
        }

        if (!noSend && enableThroughputSet) {
            // Create Send Data Thread
            // Thread will join when data channel destroyed or closed
            std::thread([wdc = make_weak_ptr(dc), label]() {
                steady_clock::time_point stepTime = steady_clock::now();
                // Byte count to send for every loop
                int byteToSendOnEveryLoop = throughtputSetAsKB * stepDurationInMs;
                while (true) {
                    this_thread::sleep_for(milliseconds(stepDurationInMs));

                    auto dcLocked = wdc.lock();
                    if (!dcLocked)
                        break;

                    if (!dcLocked->isOpen())
                        break;

                    try {
                        const double elapsedTimeInSecs =
                            std::chrono::duration<double>(steady_clock::now() - stepTime).count();

                        stepTime = steady_clock::now();

                        int byteToSendThisLoop =
                            static_cast<int>(byteToSendOnEveryLoop *
                                ((elapsedTimeInSecs * 1000.0) / stepDurationInMs));

                        binary tempMessageData(byteToSendThisLoop);
                        fill(tempMessageData.begin(), tempMessageData.end(), std::byte(0xFF));

                        if (dcLocked->bufferedAmount() <= bufferSize) {
                            dcLocked->send(tempMessageData);
                            sentSizeMap.at(label) += tempMessageData.size();
                        }
                    }
                    catch (const std::exception& e) {
                        std::cout << "Send failed: " << e.what() << std::endl;
                        DBG("Send failed: " << e.what());
                    }
                }
                cout << "Send Data Thread exiting..." << endl;
                DBG("Send Data Thread exiting...");
            }).detach();
        }

        dc->onBufferedAmountLow([wdc = make_weak_ptr(dc), label]() {
            if (noSend)
                return;

            if (enableThroughputSet)
                return;

            auto dcLocked = wdc.lock();
            if (!dcLocked)
                return;

            // Continue sending
            try {
                while (dcLocked->isOpen() && dcLocked->bufferedAmount() <= bufferSize) {
                    dcLocked->send(messageData);
                    sentSizeMap.at(label) += messageData.size();
                }
            }
            catch (const std::exception& e) {
                std::cout << "Send failed: " << e.what() << std::endl;
                DBG("Send failed: "<< e.what());
            }
        });

        dc->onClosed([id]() {
            cout << "DataChannel from " << id << " closed" << endl;
            DBG("DataChannel from " << id << " closed");
            });

        dc->onMessage([id, wdc = make_weak_ptr(dc), label](variant<binary, string> data) {
            if (holds_alternative<binary>(data)) //erkennung von binären Daten --> true
                receivedSizeMap.at(label) += get<binary>(data).size(); //hier kommen binäre Daten an -> Midi auslesen und weiterverarbeiten
        });

        dataChannelMap.emplace(label, dc);
        });

    peerConnectionMap.emplace(id, pc);
    return pc;
}

//generate localID
void MidiRTCAudioProcessor::generateLocalId(size_t length) {
    static const string characters(
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    string id(length, '0');
    default_random_engine rng(random_device{}());
    uniform_int_distribution<int> dist(0, int(characters.size() - 1));
    generate(id.begin(), id.end(), [&]() { return characters.at(dist(rng)); });
    setLocalId(id);
}

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

    string stunServer = "";


    generateLocalId(4);


    ws = make_shared<WebSocket>();
    
    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();

    ws->onOpen([&wsPromise]() {
        DBG("WebSocket connected, signaling ready");
        wsPromise.set_value();
        });

    ws->onError([&wsPromise](string s) {
        DBG( "WebSocket error" );
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
        });

    ws->onClosed([]() { DBG( "WebSocket closed" ); });

    ws->onMessage([&](variant<binary, string> data) {
        if (!holds_alternative<string>(data))
            return;

        json message = json::parse(get<string>(data));

        auto it = message.find("id");
        if (it == message.end())
            return;
        //string id = it->get<string>();
        setPartnerId(it->get<string>());

        it = message.find("type");
        if (it == message.end())
            return;
        string type = it->get<string>();

        shared_ptr<PeerConnection> pc;

        // entweder connection wird nicht erstellt weil schon ID vorhanden (if), verbindung wird
        // erstellt, weil vom typ offer und noch nicht vorhanden(else if), oder Abbruch (else)

        if (auto jt = peerConnectionMap.find(partnerId); jt != peerConnectionMap.end()) {
            pc = jt->second; // peer connection wird nicht erstellt, weil mit der ID schon eine
                             // vorhanden ist
        }
        else if (type == "offer") {
                    DBG("Answering to " + partnerId);
                    //std::cout << "Answering to " + id<< endl;
                    // angegebener ID wird geantwortet -> verbindung aufgebaut
            pc = createPeerConnection(config, ws, partnerId); // peer connection wird erstellt
            }
        else {
            return;
        }

        // wird aufgerufen, wenn verbindung bereits besteht
        if (type == "offer" || type == "answer") {
            auto sdp = message["description"].get<string>();
            pc->setRemoteDescription(Description(sdp, type));
        }
        else if (type == "candidate") {
            auto sdp = message["candidate"].get<string>();
            auto mid = message["mid"].get<string>();
            pc->addRemoteCandidate(Candidate(sdp, mid));
        }
        });

    //create Websocket
    string wsPrefix = "ws://";

    //"127.0.0.1:8000" hardcoded
    const string url = wsPrefix + "127.0.0.1:8000" + "/" + localId;
    DBG( "Url is " + url);
    //own websocket is opened
    ws->open(url);
    wsFuture.get();
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