#pragma once

#include <JuceHeader.h>
#include "Guide.h"

namespace Guide
{
    /** The user guide in a window of its own: the pages down the left, the page on the right, with
        back and forward. Links to other pages open here; anything else opens in the browser. */
    class Browser final : public juce::Component
    {
    public:
        Browser();
        ~Browser() override;

        /** Shows a page, scrolled to the heading with the anchor if one is given. */
        void show (const juce::String& page, const juce::String& anchor = {});

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        class PageView;
        class Contents;

        juce::StringArray pageList;
        std::unique_ptr<Contents> contents;
        juce::ListBox contentsList;
        juce::TextButton back { "Back" }, forward { "Forward" };
        juce::Label pageTitle;
        juce::Viewport viewport;
        std::unique_ptr<PageView> pageView;

        struct Place { juce::String page; int scroll = 0; };
        std::vector<Place> history;
        size_t at = 0;

        void open (const juce::String& page, const juce::String& anchor, bool record);
        void follow (const juce::String& link);
        void step (int by);
        void refreshButtons();
    };

    class Window final : public juce::DocumentWindow
    {
    public:
        explicit Window (std::function<void()> onClose);

        Browser& getBrowser()                  { return *browser; }
        void closeButtonPressed() override     { closeCallback(); }

    private:
        Browser* browser = nullptr;
        std::function<void()> closeCallback;
    };
}
