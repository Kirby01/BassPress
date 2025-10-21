#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleDriveAudioProcessor::SimpleDriveAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::mono(), true)
                        .withOutput ("Output", juce::AudioChannelSet::mono(), true)),
      parameters (*this, nullptr, "PARAMS",
                  {
                      std::make_unique<juce::AudioParameterFloat>(
                          "slider", "Drive",
                          juce::NormalisableRange<float> (1.0f, 400.0f, 0.01f),
                          1.0f),

                      std::make_unique<juce::AudioParameterChoice>(
                          "osMode", "Oversampling",
                          juce::StringArray { "Off", "2x", "3x", "6x", "12x" },
                          0)
                  })
{
}

int SimpleDriveAudioProcessor::getOversampleFactor() const
{
    const int mode = (int) *parameters.getRawParameterValue ("osMode");
    switch (mode)
    {
        case 1:  return 2;
        case 2:  return 3;
        case 3:  return 6;
        case 4:  return 12;
        default: return 1;
    }
}

void SimpleDriveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    a = 1.0;
    vo.prepare (sampleRate, samplesPerBlock, getOversampleFactor());
    vo.reset();
}

void SimpleDriveAudioProcessor::releaseResources() {}

bool SimpleDriveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        && (layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono() || !isNonRealtime());
}

void SimpleDriveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    static int lastFactor = 1;
    const int wanted = getOversampleFactor();
    if (wanted != lastFactor)
    {
        vo.prepare (getSampleRate(), buffer.getNumSamples(), wanted);
        vo.reset();
        lastFactor = wanted;
    }

    juce::dsp::AudioBlock<float> baseBlock (buffer.getArrayOfWritePointers(), 1, buffer.getNumSamples());

    auto osBlock = vo.processUp (baseBlock);

    constexpr double eps = 1.0e-9;
    const float slider = *parameters.getRawParameterValue ("slider");
    auto* d = osBlock.getChannelPointer (0);
    const int m = (int) osBlock.getNumSamples();

    for (int i = 0; i < m; ++i)
    {
        const float l = d[i];
        a = (1.0 - 0.001) * std::sqrt (std::max (a, eps))
          + 0.001 * (std::pow (std::abs (1.0 - (double)l * (double)slider), 2.0)
                     / std::max (a, eps));

        if (!std::isfinite (a)) a = 1.0;
        else a = juce::jlimit (eps, 1.0e9, a);

        d[i] = (float)((double)l / std::max (a, eps));
    }

    vo.processDown (baseBlock, osBlock);
}

//==============================================================================
void SimpleDriveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SimpleDriveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessorEditor* SimpleDriveAudioProcessor::createEditor()
{
    return new SimpleDriveAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleDriveAudioProcessor();
}
