#include <CoreMIDI/CoreMIDI.h>
#include <set>
#include "c74_min.h"

using namespace c74::min;

void midiNotification(const MIDINotification *message, void *refCon);
void midiIn(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon);

class global_midi;

static MIDIClientRef g_client;
static std::set<global_midi *> g_objects;

class global_midi : public object<global_midi> {
private:
    MIDIPortRef m_inPort;
    fifo<int> m_packets;

public:
    global_midi(const atoms &args = {})
    {
        g_objects.insert(this);

        if(!g_client)
        {
            auto status = MIDIClientCreate(CFSTR("GlobalMidi"), &midiNotification,
                                           nullptr, &g_client);
            if(status != noErr)
            {
                cout << "Couldn't create midi client: " << status << endl;
            }
        }

        if(g_client)
        {
            MIDIInputPortCreate(g_client, CFSTR("In"), midiIn, this, &m_inPort);
        }
    }

    ~global_midi()
    {
        g_objects.erase(this);

        MIDIPortDispose(m_inPort);
    }

    MIN_DESCRIPTION	{"Get MIDI messages inside M4L."};
    MIN_TAGS		{"utilities"};
    MIN_AUTHOR		{"Seequence"};
    MIN_RELATED		{"midiparse midiin ctlin"};

//    inlet<>  input	{ this, "(bang) post greeting to the max console" };
    outlet<> output	{ this, "(int) midi byte" };


    argument<symbol> device_arg { this, "device", "Device to connect to.",
        MIN_ARGUMENT_FUNCTION {
            device = arg;
        }
    };

    attribute<symbol> device { this, "device", "",
        description {
            "Device to connect to"
        },
        setter { MIN_FUNCTION {
            connectInput(args[0]);
            return args;
        } }
    };

    timer<timer_options::deliver_on_scheduler> deliverer { this,
        MIN_FUNCTION {
            int message;
            while(m_packets.try_dequeue(message))
            {
                output.send(message);
            }

            return {};
        }
    };

    void connectInput(const std::string &deviceString)
    {
        if(!m_inPort)
        {
            return;
        }

        auto numSources = MIDIGetNumberOfSources();

        for(auto i = 0; i < numSources; i++)
        {
            auto endPoint = MIDIGetSource(i);
            CFStringRef fname;
            MIDIObjectGetStringProperty(endPoint, kMIDIPropertyDisplayName, &fname);

            char name[64];
            CFStringGetCString(fname, name, 64, 0);

            if(deviceString == name)
            {
                auto status = MIDIPortConnectSource(m_inPort, endPoint, nullptr);
                if(status != noErr)
                {
                    cout << "Couldn't connect to device" << endl;
                }
            }
            else
            {
                MIDIPortDisconnectSource(m_inPort, endPoint);
            }
        }
    }

    void devicesChanged()
    {
        symbol the_device = device;
        connectInput(the_device);
    }

    void scheduleMessage(const MIDIPacketList *pktlist)
    {
        for(auto i = 0; i < pktlist->numPackets; i++)
        {
            const auto &packet = pktlist->packet[i];

            for(auto j = 0; j < packet.length; j++)
            {
                m_packets.try_enqueue(packet.data[j]);
            }
        }

        deliverer.delay(0);
    }
};

void midiNotification(const MIDINotification *message, void *refCon)
{
    switch(message->messageID)
    {
        case kMIDIMsgSetupChanged:
        case kMIDIMsgObjectAdded:
        case kMIDIMsgObjectRemoved:
            for(auto object : g_objects)
            {
                object->devicesChanged();
            }

        default:
            return;
    }
}

void midiIn(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon)
{
    auto klass = static_cast<global_midi *>(readProcRefCon);
    klass->scheduleMessage(pktlist);
}

MIN_EXTERNAL(global_midi);
