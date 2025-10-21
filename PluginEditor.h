#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class SimpleDriveAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SimpleDriveAudioProcessorEditor (SimpleDriveAudioProcessor&);
    ~SimpleDriveAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SimpleDriveAudioProcessor& processor;

    juce::Slider driveSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

    juce::ComboBox osMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDriveAudioProcessorEditor)
};
