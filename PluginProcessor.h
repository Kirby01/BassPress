#pragma once
#include <JuceHeader.h>

//==============================================================================
// Helper: supports oversampling factors 1, 2, 3, 6, and 12
struct VariableOversampler
{
    void prepare (double baseSampleRate, int maxBlockSize, int factorWanted)
    {
        factor = juce::jlimit (1, 12, factorWanted);
        baseFs = baseSampleRate;
        maxBlock = maxBlockSize;

        lagUp.reset();
        lagDown.reset();
        iirPreDown.reset();
        os2.reset();
        os4.reset();

        const int worst = maxBlock * 12;
        temp1.setSize (1, worst, false, false, true);
        temp2.setSize (1, worst, false, false, true);

        use3 = use2 = use4 = false;

        if (factor == 1)
            return;

        // Determine which oversampling stages are needed
        use3 = (factor == 3 || factor == 6 || factor == 12);
        use2 = (factor == 2 || factor == 6);
        use4 = (factor == 12);

        if (use2)
            os2 = std::make_unique<juce::dsp::Oversampling<float>>(
                1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

        if (use4)
            os4 = std::make_unique<juce::dsp::Oversampling<float>>(
                1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

        // Low-pass filter for downsampling (3× path)
        double fsAtLP = baseFs * (use4 ? 12.0 : use2 ? 6.0 : use3 ? 3.0 : 2.0);
        auto coeff = juce::dsp::IIR::Coefficients<float>::makeLowPass ((float) fsAtLP,
                                                                       (float) (0.45 * (baseFs * 0.5)));
        iirPreDown.coefficients = coeff;

        if (os2) os2->initProcessing (maxBlock * 6);
        if (os4) os4->initProcessing (maxBlock * 12);
    }

    void reset()
    {
        if (os2) os2->reset();
        if (os4) os4->reset();
        lagUp.reset();
        lagDown.reset();
        iirPreDown.reset();
    }

    juce::dsp::AudioBlock<float> processUp (juce::dsp::AudioBlock<float> inBlock)
    {
        using AB = juce::dsp::AudioBlock<float>;
        AB stage = inBlock;

        if (factor == 1)
            return stage;

        if (use3)
        {
            const int inN  = (int) stage.getNumSamples();
            const int outN = inN * 3;
            temp1.setSize (1, outN, false, false, true);
            lagUp.process (1.0 / 3.0,
                           stage.getChannelPointer (0),
                           temp1.getWritePointer (0),
                           outN);
            stage = AB (temp1);
        }

        if (use2)
            stage = os2->processSamplesUp (stage);
        if (use4)
            stage = os4->processSamplesUp (stage);

        return stage;
    }

    void processDown (juce::dsp::AudioBlock<float> baseRateBlock, juce::dsp::AudioBlock<float> oversampledBlock)
    {
        using AB = juce::dsp::AudioBlock<float>;
        AB stage = oversampledBlock;

        if (use4)
        {
            os4->processSamplesDown (stage);
            stage = AB (temp2).getSubBlock (0, stage.getNumSamples() / 4);
        }

        if (use2)
        {
            os2->processSamplesDown (stage);
            stage = AB (temp2).getSubBlock (0, stage.getNumSamples() / 2);
        }

        if (use3)
        {
            auto* d = stage.getChannelPointer (0);
            float* channels[] = { d };
juce::dsp::AudioBlock<float> lpb (channels, 1, stage.getNumSamples());
juce::dsp::ProcessContextReplacing<float> ctx (lpb);
iirPreDown.process (ctx);


            const int inN  = (int) stage.getNumSamples();
            const int outN = inN / 3;
            temp2.setSize (1, outN, false, false, true);

            lagDown.process (3.0, d, temp2.getWritePointer (0), outN);
            jassert (baseRateBlock.getNumSamples() == (size_t) outN);
            std::memcpy (baseRateBlock.getChannelPointer (0),
                         temp2.getReadPointer (0),
                         (size_t) outN * sizeof (float));
            return;
        }
    }

    int getFactor() const noexcept { return factor; }

    int factor { 1 };
    bool use3 { false }, use2 { false }, use4 { false };
    int maxBlock { 0 };
    double baseFs { 44100.0 };

    juce::AudioBuffer<float> temp1, temp2;
   juce::LagrangeInterpolator lagUp, lagDown;

    juce::dsp::IIR::Filter<float> iirPreDown;
    std::unique_ptr<juce::dsp::Oversampling<float>> os2, os4;
};

//==============================================================================

class SimpleDriveAudioProcessor : public juce::AudioProcessor
{
public:
    SimpleDriveAudioProcessor();
    ~SimpleDriveAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SimpleDrive"; }

    bool acceptsMidi() const override      { return false; }
    bool producesMidi() const override     { return false; }
    bool isMidiEffect() const override     { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override          { return 1; }
    int getCurrentProgram() override       { return 0; }
    void setCurrentProgram (int) override  {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    double a = 1.0;
    juce::AudioProcessorValueTreeState parameters;

    int getOversampleFactor() const;

private:
    VariableOversampler vo;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDriveAudioProcessor)
};
