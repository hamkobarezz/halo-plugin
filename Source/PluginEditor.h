#pragma once
#include "PluginProcessor.h"
#include "ui/Departments.h"

namespace halo
{
class TopBar;
class FooterBar;

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
        juce::Array<juce::Line<float>> dividers;
    } content;

    std::unique_ptr<TopBar> top;
    std::unique_ptr<FooterBar> footer;
    std::vector<std::unique_ptr<ui::DeptCard>> cards;
    std::vector<std::unique_ptr<ui::DetailPanel>> details;
    std::unique_ptr<ui::EarView> ear;
    std::unique_ptr<ui::SplashScreen> splash;
    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<juce::VBlankAttachment> vblank;
    double lastTime = 0;
    float toneSmooth[7] {};
    float detailAlpha[5] { 1, 1, 1, 1, 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HaloEditor)
};
} // namespace halo
