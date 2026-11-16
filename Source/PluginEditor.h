#pragma once
#include "PluginProcessor.h"
#include "ui/Panels.h"
#include "ui/Ear.h"

namespace halo
{
class TopBar;
class MeterView;

class HaloEditor : public juce::AudioProcessorEditor
{
public:
    static constexpr int BaseW = 1580, BaseH = 920;

    explicit HaloEditor (HaloProcessor&);
    ~HaloEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void frame();

    HaloProcessor& proc;
    ui::HaloLookAndFeel lnf;
    ui::SpectrumEngine engine;

    struct Content : juce::Component
    {
        void paint (juce::Graphics&) override;
        float t = 0;
        juce::Array<juce::Line<float>> dividers;
    } content;

    std::unique_ptr<TopBar> top;
    std::vector<std::unique_ptr<ui::ModulePanel>> panels;
    std::unique_ptr<ui::SpectrumView> spectrum;
    std::unique_ptr<MeterView> meters;
    std::unique_ptr<ui::EarView> ear;
    std::unique_ptr<ui::SplashScreen> splash;
    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<juce::VBlankAttachment> vblank;
    double lastTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HaloEditor)
};
} // namespace halo
