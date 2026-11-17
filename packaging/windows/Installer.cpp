// HALO Setup — branded Windows installer / uninstaller.
// Payload (VST3 + standalone) is embedded at build time; see packaging/windows/build_installer.ps1
#include <juce_gui_extra/juce_gui_extra.h>
#include "SetupData.h"

using namespace juce;

namespace
{
const Colour bg (0xff0b0b0d), panel (0xff141414), border (0xff2a2a2a), text (0xffe9e7e2), label (0xffa7a49e), accent (0xffff5a1f);
const String version = "1.1.0";
const String regKey = "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HALO\\";

Typeface::Ptr tf (const char* data, int size) { return Typeface::createSystemTypefaceFor (data, (size_t) size); }
Font semi (float h) { static auto t = tf (SetupData::MontserratSemiBold_ttf, SetupData::MontserratSemiBold_ttfSize); return Font (FontOptions (t).withHeight (h)); }
Font med (float h)  { static auto t = tf (SetupData::MontserratMedium_ttf, SetupData::MontserratMedium_ttfSize); return Font (FontOptions (t).withHeight (h)); }

File commonFiles() { return File (SystemStats::getEnvironmentVariable ("CommonProgramW6432", "C:\\Program Files\\Common Files")); }
File programFiles() { return File (SystemStats::getEnvironmentVariable ("ProgramW6432", "C:\\Program Files")); }
File programData() { return File (SystemStats::getEnvironmentVariable ("ProgramData", "C:\\ProgramData")); }

File vst3Bundle()   { return commonFiles().getChildFile ("VST3").getChildFile ("HALO.vst3"); }
File oldBundle()    { return commonFiles().getChildFile ("VST3").getChildFile ("HB Halo.vst3"); }
File appDir()       { return programFiles().getChildFile ("HALO Studios").getChildFile ("HALO"); }
File startMenuDir() { return programData().getChildFile ("Microsoft\\Windows\\Start Menu\\Programs\\HALO Studios"); }
File desktopLink()  { return File (SystemStats::getEnvironmentVariable ("PUBLIC", "C:\\Users\\Public")).getChildFile ("Desktop\\HALO.lnk"); }

bool writeData (const File& f, const void* data, int size)
{
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    return f.replaceWithData (data, (size_t) size);
}
}

//==============================================================================
class SetupLnf : public LookAndFeel_V4
{
public:
    void drawTickBox (Graphics& g, Component&, float x, float y, float w, float h, bool ticked, bool, bool over, bool) override
    {
        auto r = Rectangle<float> (x, y, w, h).reduced (1.0f);
        g.setColour (ticked ? accent : (over ? Colour (0xff4a4a4a) : Colour (0xff3a3a3a)));
        g.drawRoundedRectangle (r, 3.0f, 1.2f);
        if (ticked) { g.setColour (accent); g.fillRoundedRectangle (r.reduced (3.5f), 1.5f); }
    }
    void drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool down) override
    {
        const float box = 16.0f;
        drawTickBox (g, b, 0, ((float) b.getHeight() - box) * 0.5f, box, box, b.getToggleState(), b.isEnabled(), over, down);
        g.setColour (text);
        g.setFont (med (13.0f));
        g.drawText (b.getButtonText(), b.getLocalBounds().withTrimmedLeft (28), Justification::centredLeft, false);
    }
    void drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool primary = (bool) b.getProperties().getWithDefault ("primary", false);
        if (primary)
        {
            g.setColour (down ? accent.darker (0.2f) : over ? accent.brighter (0.1f) : accent);
            g.fillRoundedRectangle (r, 4.0f);
        }
        else
        {
            g.setColour (over ? Colour (0xff1a1a1a) : panel);
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (border);
            g.drawRoundedRectangle (r, 4.0f, 1.0f);
        }
    }
    void drawButtonText (Graphics& g, TextButton& b, bool, bool) override
    {
        const bool primary = (bool) b.getProperties().getWithDefault ("primary", false);
        g.setColour (primary ? Colour (0xff0b0b0d) : text);
        g.setFont (semi (13.0f).withExtraKerningFactor (0.08f));
        g.drawText (b.getButtonText().toUpperCase(), b.getLocalBounds(), Justification::centred, false);
    }
};

//==============================================================================
class SetupComponent : public Component, private Timer
{
public:
    explicit SetupComponent (bool uninstallMode) : uninstall (uninstallMode)
    {
        logo = Drawable::createFromImageData (SetupData::halo_studios_logo_svg, SetupData::halo_studios_logo_svgSize);
        for (auto* t : { &optVst3, &optApp, &optDesktop }) { addAndMakeVisible (t); t->setToggleState (true, dontSendNotification); }
        optDesktop.setToggleState (false, dontSendNotification);
        optVst3.setEnabled (false); // the plugin is the point of the installer

        action.getProperties().set ("primary", true);
        action.setButtonText (uninstall ? "Uninstall" : "Install");
        action.onClick = [this] { if (state == State::Done) closeApp(); else start(); };
        cancel.setButtonText ("Cancel");
        cancel.onClick = [this] { closeApp(); };
        addAndMakeVisible (action);
        addAndMakeVisible (cancel);
        if (uninstall) { optVst3.setVisible (false); optApp.setVisible (false); optDesktop.setVisible (false); }
        setSize (640, 420);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (bg);
        auto r = getLocalBounds().reduced (36, 30);
        auto top = r.removeFromTop (78);
        if (logo != nullptr) logo->drawWithin (g, top.removeFromLeft (132).toFloat(), RectanglePlacement::xLeft | RectanglePlacement::yMid, 1.0f);
        top.removeFromLeft (24);
        g.setColour (text);
        g.setFont (semi (22.0f).withExtraKerningFactor (0.04f));
        g.drawText (uninstall ? "Uninstall HALO" : "Install HALO", top.removeFromTop (44), Justification::bottomLeft, false);
        g.setColour (label);
        g.setFont (med (12.5f));
        g.drawText ("Vocal, mix & mastering chain  -  v" + version, top, Justification::topLeft, false);

        // open divider
        Path d;
        d.startNewSubPath (36.0f, 130.0f);
        d.lineTo ((float) getWidth() * 0.55f, 130.0f);
        g.setColour (Colour (0xff232323));
        g.strokePath (d, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

        auto body = Rectangle<int> (36, 150, getWidth() - 72, 150);
        g.setFont (med (13.0f));
        if (state == State::Ready && ! uninstall)
        {
            g.setColour (label);
            g.setFont (semi (10.5f).withExtraKerningFactor (0.1f));
            g.drawText ("COMPONENTS", body.removeFromTop (18), Justification::centredLeft, false);
        }
        else
        {
            g.setColour (state == State::Error ? Colour (0xffff6b6b) : text);
            g.drawFittedText (message, body, Justification::topLeft, 6);
        }

        if (state == State::Working || state == State::Done)
        {
            auto bar = Rectangle<float> (36.0f, 300.0f, (float) getWidth() - 72.0f, 6.0f);
            g.setColour (Colour (0xff1e1e1e));
            g.fillRoundedRectangle (bar, 3.0f);
            g.setColour (accent);
            g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * progress), 3.0f);
        }
    }

    void resized() override
    {
        auto opts = Rectangle<int> (36, 176, getWidth() - 72, 26);
        optVst3.setBounds (opts); opts.translate (0, 32);
        optApp.setBounds (opts); opts.translate (0, 32);
        optDesktop.setBounds (opts);
        action.setBounds (getWidth() - 36 - 150, getHeight() - 30 - 42, 150, 42);
        cancel.setBounds (action.getX() - 12 - 110, action.getY(), 110, 42);
    }

private:
    enum class State { Ready, Working, Done, Error };

    void start()
    {
        state = State::Working;
        step = 0;
        progress = 0;
        action.setEnabled (false);
        cancel.setEnabled (false);
        for (auto* t : { &optVst3, &optApp, &optDesktop }) t->setVisible (false);
        message = uninstall ? "Removing HALO..." : "Installing HALO...";
        repaint();
        startTimer (120);
    }

    void timerCallback() override
    {
        const bool ok = uninstall ? uninstallStep (step) : installStep (step);
        if (! ok) { stopTimer(); fail(); return; }
        ++step;
        const int total = uninstall ? 4 : 6;
        progress = (float) step / (float) total;
        if (step >= total)
        {
            stopTimer();
            state = State::Done;
            message = uninstall
                ? "HALO has been removed from this computer."
                : "HALO is installed.\n\nVST3: " + vst3Bundle().getFullPathName()
                  + "\n\nIn FL Studio: Options > Manage plugins > Find installed plugins, then add HALO from the plugin list.";
            action.setButtonText ("Close");
            action.setEnabled (true);
            cancel.setVisible (false);
        }
        repaint();
    }

    bool installStep (int s)
    {
        switch (s)
        {
            case 0: // remove the pre-rename build so hosts don't list it twice
                if (oldBundle().isDirectory()) oldBundle().deleteRecursively();
                return true;
            case 1:
                return writeData (vst3Bundle().getChildFile ("Contents\\x86_64-win\\HALO.vst3"), SetupData::HALO_vst3, SetupData::HALO_vst3Size)
                    && writeData (vst3Bundle().getChildFile ("Contents\\Resources\\moduleinfo.json"), SetupData::moduleinfo_json, SetupData::moduleinfo_jsonSize);
            case 2:
                if (! optApp.getToggleState()) return true;
                return writeData (appDir().getChildFile ("HALO.exe"), SetupData::HALO_exe, SetupData::HALO_exeSize);
            case 3:
            {
                if (! optApp.getToggleState()) return true;
                startMenuDir().createDirectory();
                appDir().getChildFile ("HALO.exe").createShortcut ("HALO standalone", startMenuDir().getChildFile ("HALO.lnk"));
                if (optDesktop.getToggleState())
                    appDir().getChildFile ("HALO.exe").createShortcut ("HALO standalone", desktopLink());
                return true;
            }
            case 4:
            {
                auto self = File::getSpecialLocation (File::currentExecutableFile);
                auto un = appDir().getChildFile ("Uninstall HALO.exe");
                appDir().createDirectory();
                if (self != un) { un.deleteFile(); if (! self.copyFileTo (un)) return false; }
                return true;
            }
            case 5:
            {
                auto un = appDir().getChildFile ("Uninstall HALO.exe");
                WindowsRegistry::setValue (regKey + "DisplayName", "HALO");
                WindowsRegistry::setValue (regKey + "Publisher", "HALO Studios");
                WindowsRegistry::setValue (regKey + "DisplayVersion", version);
                WindowsRegistry::setValue (regKey + "InstallLocation", appDir().getFullPathName());
                WindowsRegistry::setValue (regKey + "UninstallString", "\"" + un.getFullPathName() + "\" --uninstall");
                WindowsRegistry::setValue (regKey + "DisplayIcon", optApp.getToggleState() ? appDir().getChildFile ("HALO.exe").getFullPathName() : un.getFullPathName());
                WindowsRegistry::setValue (regKey + "NoModify", (uint32) 1);
                WindowsRegistry::setValue (regKey + "NoRepair", (uint32) 1);
                return true;
            }
            default: return true;
        }
    }

    bool uninstallStep (int s)
    {
        switch (s)
        {
            case 0: vst3Bundle().deleteRecursively(); if (oldBundle().isDirectory()) oldBundle().deleteRecursively(); return ! vst3Bundle().exists();
            case 1: startMenuDir().deleteRecursively(); desktopLink().deleteFile(); return true;
            case 2: WindowsRegistry::deleteKey (regKey.dropLastCharacters (1)); return true;
            case 3:
            {
                // the uninstaller lives in appDir, so let cmd remove the folder after we exit
                appDir().getChildFile ("HALO.exe").deleteFile();
                const String cmd = "/c ping -n 3 127.0.0.1 >nul & rmdir /s /q \"" + appDir().getFullPathName() + "\"";
                File (SystemStats::getEnvironmentVariable ("ComSpec", "C:\\Windows\\System32\\cmd.exe")).startAsProcess (cmd);
                return true;
            }
            default: return true;
        }
    }

    void fail()
    {
        state = State::Error;
        message = "Something went wrong while writing the files.\n\nClose any DAW that has HALO loaded (FL Studio, Ableton...) and run the installer again.";
        action.setButtonText (uninstall ? "Retry" : "Retry");
        action.setEnabled (true);
        cancel.setEnabled (true);
        state = State::Error;
        repaint();
        // allow retry
        state = State::Error;
    }

    static void closeApp() { JUCEApplication::getInstance()->systemRequestedQuit(); }

    bool uninstall;
    State state = State::Ready;
    int step = 0;
    float progress = 0;
    String message;
    std::unique_ptr<Drawable> logo;
    ToggleButton optVst3 { "VST3 plugin  (FL Studio, Ableton, Cubase, Reaper, Studio One...)" },
                 optApp { "Standalone app + Start menu shortcut" },
                 optDesktop { "Desktop shortcut" };
    TextButton action, cancel;
};

//==============================================================================
class SetupApp : public JUCEApplication
{
public:
    const String getApplicationName() override { return "HALO Setup"; }
    const String getApplicationVersion() override { return version; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise (const String& cmd) override
    {
        LookAndFeel::setDefaultLookAndFeel (&lnf);
        const bool un = cmd.contains ("--uninstall");
        window = std::make_unique<Window> (un ? "Uninstall HALO" : "HALO Setup", new SetupComponent (un));
    }
    void shutdown() override { window.reset(); LookAndFeel::setDefaultLookAndFeel (nullptr); }
    void systemRequestedQuit() override { quit(); }

private:
    struct Window : DocumentWindow
    {
        Window (const String& name, Component* c) : DocumentWindow (name, bg, DocumentWindow::closeButton)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (c, true);
            centreWithSize (getWidth(), getHeight());
            setResizable (false, false);
            setVisible (true);
        }
        void closeButtonPressed() override { JUCEApplication::getInstance()->systemRequestedQuit(); }
    };
    SetupLnf lnf;
    std::unique_ptr<Window> window;
};

START_JUCE_APPLICATION (SetupApp)
