#include "PluginEditor.h"

//==============================================================================
SimpleDriveAudioProcessorEditor::SimpleDriveAudioProcessorEditor (SimpleDriveAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setSize (360, 240);

    // === Drive Knob ===
    driveSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
    driveSlider.setRange (1.0, 400.0, 0.01);
    driveSlider.setSkewFactorFromMidPoint (20.0);
    addAndMakeVisible (driveSlider);

    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "slider", driveSlider);

    // === Oversampling Menu ===
    osMenu.addItemList (juce::StringArray { "Off", "2x", "3x", "6x", "12x" }, 1);
    osMenu.setJustificationType (juce::Justification::centred);
    osMenu.setTooltip ("Select oversampling factor");
    addAndMakeVisible (osMenu);

    osAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.parameters, "osMode", osMenu);
}

void SimpleDriveAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawFittedText ("SimpleDrive v1.0 - Harmonious Records",
                      getLocalBounds().removeFromTop (30),
                      juce::Justification::centred, 1);

    g.setFont (14.0f);
    g.drawFittedText ("Drive", juce::Rectangle<int> (0, 170, getWidth(), 20),
                      juce::Justification::centred, 1);
    g.drawFittedText ("Oversampling", juce::Rectangle<int> (0, 200, getWidth(), 20),
                      juce::Justification::centred, 1);
}

void SimpleDriveAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    driveSlider.setBounds (area.removeFromTop (160).withSizeKeepingCentre (140, 140));
    auto comboArea = getLocalBounds().removeFromBottom (40).reduced (100, 10);
    osMenu.setBounds (comboArea);
}
