#pragma once
#include "Processor.h"
class NeuroEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit NeuroEditor(NeuroProcessor &);
    ~NeuroEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    NeuroProcessor &processor;
    juce::LookAndFeel_V4 look;
    juce::TooltipWindow tips{this, 550};
    juce::TextButton generate{"GENERATE"}, mutate{"MUTATE"}, undo{"Undo"}, redo{"Redo"}, save{"Save"},
        load{"Load"}, newWT{"NEW WT"}, rhythm{"NEW RHYTHM"}, seedGo{"Use seed"};
    juce::ToggleButton color{"COLOR"}, spectral{"SPECTRAL"}, safe{"SAFE OUTPUT"}, clean{"CLEAN SUB"},
        audition{"AUDITION C"}, advanced{"ADVANCED"}, seedLock{"Lock seed"}, wtLock{"Lock WT"};
    juce::ComboBox family, factory;
    juce::TextEditor seed;
    juce::Label status, meters;
    juce::Viewport viewport;
    juce::Component advancedContent;
    std::array<juce::Slider, nj::Count> sliders;
    std::array<juce::Label, nj::Count> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, nj::Count>
        sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    std::array<juce::ToggleButton, 8> locks;
    std::unique_ptr<juce::FileChooser> chooser;
    int flash = 0;
    void timerCallback() override;
    void syncUI();
    void result(bool, const juce::String &);
    void choose(bool saving);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuroEditor)
};
