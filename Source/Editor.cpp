#include "Editor.h"
using namespace nj;
NeuroEditor::NeuroEditor(NeuroProcessor &p) : AudioProcessorEditor(&p), processor(p)
{
    look.setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff101218));
    look.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff59efbe));
    look.setColour(juce::Slider::thumbColourId, juce::Colour(0xffc7fff0));
    look.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252a36));
    look.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff59efbe));
    setLookAndFeel(&look);
    setResizable(true, true);
    setResizeLimits(920, 650, 1600, 1100);
    setSize(1000, 720);
    for (auto *c : std::initializer_list<juce::Component *>{
             &generate, &mutate, &undo,     &redo, &save,   &load,     &newWT,    &rhythm,
             &seedGo,   &color,  &spectral, &safe, &clean,  &audition, &advanced, &seedLock,
             &wtLock,   &family, &factory,  &seed, &status, &meters,   &viewport})
        addAndMakeVisible(c);
    for (int i = 0; i < familyCount; ++i)
        family.addItem(families[i], i + 1);
    factory.addItem("Factory bank: choose a sound", 1);
    for (int i = 0; i < 90; ++i)
        factory.addItem(juce::String(i + 1).paddedLeft('0', 2) + " / " + families[1 + i % 29] + " / " +
                            juce::String(1 + i / 30),
                        i + 2);
    factory.setSelectedId(1);
    family.onChange = [this] { processor.setFamily(family.getSelectedId() - 1); };
    factory.onChange = [this]
    {
        if (factory.getSelectedId() > 1)
            result(processor.factory(factory.getSelectedId() - 2), "Factory patch loaded");
    };
    generate.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff327c66));
    generate.onClick = [this] { result(processor.makePatch(), "New patch generated"); };
    mutate.onClick = [this] { result(processor.makePatch(true), "Patch mutated"); };
    undo.onClick = [this] { result(processor.undo(), "Undo"); };
    redo.onClick = [this] { result(processor.redo(), "Redo"); };
    newWT.onClick = [this] { result(processor.newWavetable(), "New original wavetable"); };
    rhythm.onClick = [this] { result(processor.randomRhythm(), "New 16-step rhythm"); };
    save.onClick = [this] { choose(true); };
    load.onClick = [this] { choose(false); };
    seed.setInputRestrictions(10, "0123456789");
    seedGo.onClick = [this]
    {
        result(processor.recallSeed(uint32_t(seed.getText().getLargeIntValue())),
               "Seed regenerated with current settings");
    };
    audition.onClick = [this] { processor.audition(audition.getToggleState()); };
    advanced.onClick = [this] { resized(); };
    seedLock.onClick = [this] { processor.lockSeed(seedLock.getToggleState()); };
    wtLock.onClick = [this] { processor.lockWave(wtLock.getToggleState()); };
    auto attach = [&](int i, juce::ToggleButton &b)
    {
        buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.params, specs[i].id, b));
    };
    attach(ColorOn, color);
    attach(SpectralOn, spectral);
    attach(SafeOutput, safe);
    attach(CleanSub, clean);
    viewport.setViewedComponent(&advancedContent, false);
    viewport.setScrollBarsShown(true, false);
    for (int i = 0; i < Count; ++i)
    {
        if (i >= ColorOn && i <= SafeOutput)
            continue;
        auto &s = sliders[i];
        s.setSliderStyle(i < 8 ? juce::Slider::RotaryHorizontalVerticalDrag : juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 19);
        s.setTooltip(specs[i].name);
        labels[i].setText(specs[i].name, juce::dontSendNotification);
        labels[i].setFont(juce::Font(juce::FontOptions(12)));
        labels[i].setJustificationType(juce::Justification::centred);
        if (i < 12)
        {
            addAndMakeVisible(s);
            addAndMakeVisible(labels[i]);
        }
        else
        {
            advancedContent.addAndMakeVisible(s);
            advancedContent.addAndMakeVisible(labels[i]);
        }
        sliderAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.params, specs[i].id, s);
    }
    const char *names[] = {"Lock OSC",   "Lock FM",       "Lock FILTER", "Lock MOD / RHYTHM",
                           "Lock COLOR", "Lock SPECTRAL", "Lock FX",     "Lock SUB"};
    for (int i = 0; i < 8; ++i)
    {
        locks[i].setButtonText(names[i]);
        advancedContent.addAndMakeVisible(locks[i]);
        locks[i].onClick = [this, i] { processor.lockGroup(i, locks[i].getToggleState()); };
    }
    color.setTooltip("MIDI-tuned chord feedback resonators; clean sub bypasses them.");
    spectral.setTooltip(
        "Real 512-point FFT bin shift/stretch/blur/freeze. Host latency is always 512 samples.");
    newWT.setTooltip("Generate two original four-frame wavetable banks. Lock WT preserves them.");
    generate.setTooltip("Build a constrained patch in the selected family; section locks are respected.");
    status.setText("NEURO//J 0.2 / Development build / Start with your volume low",
                   juce::dontSendNotification);
    status.setFont(juce::Font(juce::FontOptions(12)));
    syncUI();
    startTimerHz(20);
}
NeuroEditor::~NeuroEditor()
{
    processor.audition(false);
    stopTimer();
    setLookAndFeel(nullptr);
}
void NeuroEditor::syncUI()
{
    auto p = processor.patchInfo();
    family.setSelectedId(p.selection + 1, juce::dontSendNotification);
    if (!seed.hasKeyboardFocus(false))
        seed.setText(juce::String(p.seed), false);
    seedLock.setToggleState(p.lockSeed, juce::dontSendNotification);
    wtLock.setToggleState(p.lockWave, juce::dontSendNotification);
    for (int i = 0; i < 8; ++i)
        locks[i].setToggleState(p.locks[i], juce::dontSendNotification);
}
void NeuroEditor::result(bool ok, const juce::String &text)
{
    status.setText(
        ok ? text
           : "Not changed: section locked, history empty, or queue busy. Try again after playback starts.",
        juce::dontSendNotification);
    if (ok)
    {
        flash = 8;
        syncUI();
    }
    repaint();
}
void NeuroEditor::choose(bool saving)
{
    auto initial = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                       .getChildFile("NEUROJ-" + juce::String(processor.patchInfo().seed) + ".neuroj");
    chooser = std::make_unique<juce::FileChooser>(saving ? "Save NEUROJ preset" : "Load NEUROJ preset",
                                                  initial, "*.neuroj");
    const int flags =
        saving ? (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                  juce::FileBrowserComponent::warnAboutOverwriting)
               : (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles);
    juce::Component::SafePointer<NeuroEditor> self(this);
    chooser->launchAsync(flags,
                         [self, saving](const juce::FileChooser &fc)
                         {
                             if (!self)
                                 return;
                             auto f = fc.getResult();
                             if (f == juce::File{})
                                 return;
                             self->result(saving ? self->processor.savePreset(f)
                                                 : self->processor.loadPreset(f),
                                          saving ? "Preset saved" : "Preset restored");
                         });
}
void NeuroEditor::resized()
{
    const int w = getWidth();
    factory.setBounds(220, 20, 300, 30);
    undo.setBounds(w - 350, 20, 65, 30);
    redo.setBounds(w - 280, 20, 65, 30);
    save.setBounds(w - 210, 20, 65, 30);
    load.setBounds(w - 140, 20, 65, 30);
    family.setBounds(25, 80, 230, 32);
    seed.setBounds(w - 320, 80, 150, 30);
    seedGo.setBounds(w - 160, 80, 95, 30);
    seedLock.setBounds(w - 320, 113, 110, 25);
    generate.setBounds(w / 2 - 175, 150, 350, 83);
    mutate.setBounds(w / 2 - 62, 245, 124, 32);
    color.setBounds(25, 155, 140, 30);
    spectral.setBounds(25, 194, 145, 30);
    audition.setBounds(w - 195, 160, 165, 30);
    clean.setBounds(w - 195, 198, 155, 25);
    safe.setBounds(w - 195, 230, 165, 25);
    for (int i = 0; i < 8; ++i)
    {
        int x = 20 + i * (w - 40) / 8;
        labels[i].setBounds(x, 295, (w - 40) / 8 - 4, 22);
        sliders[i].setBounds(x, 319, (w - 40) / 8 - 4, 108);
    }
    for (int i = 8; i < 12; ++i)
    {
        int x = 20 + (i - 8) * (w - 40) / 4;
        labels[i].setBounds(x, 435, (w - 40) / 4 - 8, 22);
        sliders[i].setBounds(x, 458, (w - 40) / 4 - 8, 50);
    }
    newWT.setBounds(25, 523, 100, 30);
    wtLock.setBounds(132, 523, 100, 30);
    rhythm.setBounds(240, 523, 125, 30);
    advanced.setBounds(w - 180, 523, 155, 30);
    bool expanded = advanced.getToggleState();
    viewport.setVisible(expanded);
    viewport.setBounds(20, 562, w - 40, std::max(1, getHeight() - 603));
    int contentW = w - 60;
    for (int i = 0; i < 8; ++i)
        locks[i].setBounds((i % 4) * contentW / 4, (i / 4) * 30, contentW / 4, 27);
    int n = 0;
    for (int i = WtA; i < Count; ++i)
    {
        int col = n % 3, row = n / 3;
        labels[i].setBounds(col * contentW / 3, 75 + row * 75, contentW / 3 - 12, 22);
        sliders[i].setBounds(col * contentW / 3 + 5, 98 + row * 75, contentW / 3 - 22, 46);
        ++n;
    }
    advancedContent.setSize(contentW, 80 + ((n + 2) / 3) * 75);
    status.setBounds(20, getHeight() - 32, w - 330, 24);
    meters.setBounds(w - 305, getHeight() - 32, 285, 24);
}
void NeuroEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff101218));
    g.setColour(juce::Colour(0xffc5fff0));
    g.setFont(juce::Font(juce::FontOptions(29).withStyle("Bold")));
    g.drawText("NEURO//J", 25, 18, 190, 36, juce::Justification::centredLeft);
    if (!advanced.getToggleState())
    {
        juce::Path path;
        auto y = double(560 + (getHeight() - 603) / 2);
        auto height = std::max(10, getHeight() - 615);
        for (int i = 0; i < 256; ++i)
        {
            float x = 25 + float(i) * (getWidth() - 50) / 255;
            float sy = float(y) - processor.scope[i].load() * height * .45f;
            if (i == 0)
                path.startNewSubPath(x, sy);
            else
                path.lineTo(x, sy);
        }
        g.setColour(juce::Colour(flash > 0 ? 0xffb2ffe2 : 0xff3e7e70));
        g.strokePath(path, juce::PathStrokeType(1.3f));
    }
}
void NeuroEditor::timerCallback()
{
    if (flash > 0)
        --flash;
    syncUI();
    float m = processor.meter.load();
    meters.setText("CPU " + juce::String(processor.cpu.load(), 1) + "%   PEAK " +
                       juce::String(m > 0 ? 20 * std::log10(m) : -100, 1) + " dB",
                   juce::dontSendNotification);
    repaint();
}
