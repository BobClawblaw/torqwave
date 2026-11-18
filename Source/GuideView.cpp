#include "GuideView.h"
#include "Theme.h"

namespace Guide
{
namespace
{
    constexpr float bodySize = 15.0f, captionSize = 13.0f, tableSize = 14.0f;
    constexpr float lineSpacing = 4.0f;
    constexpr float margin = 32.0f, maxWidth = 880.0f, listStep = 22.0f, cellPadX = 10.0f, cellPadY = 7.0f;

    float headingSize (int level)   { return level == 1 ? 28.0f : level == 2 ? 21.0f : 17.0f; }

    juce::Font fontFor (const Span& span, float size)
    {
        if (span.code)
            return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size - 1.0f, juce::Font::plain));

        juce::Font f { juce::FontOptions (size) };

        if (span.bold)
            f = f.boldened();

        if (span.italic)
            f = f.italicised();

        if (span.link.isNotEmpty())
            f.setUnderline (true);

        return f;
    }

    /** Text laid out once, where it sits on the page, and the links in it by their character ranges. */
    struct TextPiece
    {
        juce::TextLayout layout;
        juce::Point<float> at;
        std::vector<std::pair<juce::Range<int>, juce::String>> links;

        /** The link under a point of the page, if any. A link's text is a run of its own (its
            colour differs from the text around it), so a run's start tells which span it is. */
        juce::String linkAt (juce::Point<float> p) const
        {
            if (links.empty())
                return {};

            const auto local = p - at;

            for (const auto& line : layout)
            {
                const auto top = line.lineOrigin.y - line.ascent, bottom = line.lineOrigin.y + line.descent;

                if (local.y < top || local.y > bottom)
                    continue;

                for (const auto* run : line.runs)
                {
                    if (run->glyphs.isEmpty())
                        continue;

                    const auto left = line.lineOrigin.x + run->glyphs.getReference (0).anchor.x;
                    const auto right = line.lineOrigin.x + run->glyphs.getReference (run->glyphs.size() - 1).anchor.x + run->glyphs.getReference (run->glyphs.size() - 1).width;

                    if (local.x < left || local.x > right)
                        continue;

                    for (const auto& [range, target] : links)
                        if (range.contains (run->stringRange.getStart()))
                            return target;
                }
            }

            return {};
        }
    };

    TextPiece layOut (const Text& text, float size, juce::Colour colour, float width, juce::Justification justification = juce::Justification::left)
    {
        TextPiece piece;
        juce::AttributedString s;
        s.setJustification (justification);
        s.setLineSpacing (lineSpacing);
        s.setWordWrap (juce::AttributedString::byWord);

        for (const auto& span : text)
        {
            const auto start = s.getText().length();
            const auto c = span.link.isNotEmpty() ? Theme::accentLight : span.code ? Theme::textStrong : colour;
            s.append (span.text, fontFor (span, size), c);

            if (span.link.isNotEmpty())
                piece.links.push_back ({ { start, s.getText().length() }, span.link });
        }

        piece.layout.createLayout (s, juce::jmax (40.0f, width));
        return piece;
    }

    /** A column's natural width: its widest cell set on one line. */
    float naturalWidth (const Text& text, float size)
    {
        auto piece = layOut (text, size, Theme::text, 10000.0f);
        float w = 0.0f;

        for (const auto& line : piece.layout)
            w = juce::jmax (w, line.getLineBoundsX().getEnd());

        return w;
    }
}

//==============================================================================
/** One page, laid out for the width it is given. */
class Browser::PageView final : public juce::Component
{
public:
    std::function<void (const juce::String&)> onLink;

    void setPage (const juce::String& newPage)
    {
        page = newPage;
        blocks = parse (pageText (page), page);
        build();
    }

    /** Lays the page out for a width, at least as tall as the view it is seen in. */
    void setWidthAndBuild (int width, int minimumHeight)
    {
        shortest = minimumHeight;
        setSize (width, getHeight());
        build();
    }

    int anchorY (const juce::String& anchor) const
    {
        const auto found = anchors.find (anchor);
        return found != anchors.end() ? (int) found->second : 0;
    }

    void paint (juce::Graphics& g) override
    {
        if (builtFor != Theme::text || builtAccent != Theme::accentLight)
            build();   // the theme changed: the text's colours with it

        g.fillAll (Theme::surface);

        for (const auto& [r, fill] : boxes)
        {
            g.setColour (fill);
            g.fillRect (r);
        }

        g.setColour (Theme::line);

        for (const auto& r : outlines)
            g.drawRect (r, 1.0f);

        for (const auto& l : rules)
            g.drawLine (l, 1.0f);

        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

        for (const auto& [image, r, alt] : images)
        {
            if (image.isValid())
            {
                g.drawImage (image, r, juce::RectanglePlacement::stretchToFit);
                g.setColour (Theme::line);
                g.drawRect (r, 1.0f);
            }
            else
            {
                g.setColour (Theme::dimText);
                g.drawRect (r, 1.0f);
                g.drawFittedText (alt, r.reduced (8.0f).toNearestInt(), juce::Justification::centred, 4);
            }
        }

        for (const auto& [text, where, colour] : markers)
        {
            g.setColour (colour);
            g.setFont (juce::FontOptions (bodySize));
            g.drawText (text, juce::Rectangle<float> (where.x, where.y, listStep + 6.0f, bodySize * 1.4f), juce::Justification::topLeft, false);
        }

        for (const auto& t : texts)
            t.layout.draw (g, { t.at.x, t.at.y, t.layout.getWidth(), t.layout.getHeight() });
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (linkAt (e.position).isNotEmpty() ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
            return;

        if (const auto link = linkAt (e.position); link.isNotEmpty() && onLink != nullptr)
            onLink (link);
    }

private:
    juce::String page;
    std::vector<Block> blocks;
    std::vector<TextPiece> texts;
    std::vector<std::pair<juce::Rectangle<float>, juce::Colour>> boxes;
    std::vector<juce::Rectangle<float>> outlines;
    std::vector<juce::Line<float>> rules;
    std::vector<std::tuple<juce::Image, juce::Rectangle<float>, juce::String>> images;
    std::vector<std::tuple<juce::String, juce::Point<float>, juce::Colour>> markers;
    std::map<juce::String, float> anchors;
    juce::Colour builtFor, builtAccent;
    int shortest = 0;

    juce::String linkAt (juce::Point<float> p) const
    {
        for (const auto& t : texts)
            if (juce::Rectangle<float> (t.at.x, t.at.y, t.layout.getWidth(), t.layout.getHeight()).contains (p))
                if (auto link = t.linkAt (p); link.isNotEmpty())
                    return link;

        return {};
    }

    float place (TextPiece piece, float x, float y)
    {
        piece.at = { x, y };
        const auto h = piece.layout.getHeight();
        texts.push_back (std::move (piece));
        return h;
    }

    void build()
    {
        texts.clear();
        boxes.clear();
        outlines.clear();
        rules.clear();
        images.clear();
        markers.clear();
        anchors.clear();
        builtFor = Theme::text;
        builtAccent = Theme::accentLight;

        const auto width = juce::jmin (maxWidth, (float) getWidth() - 2.0f * margin);
        const auto x0 = margin;
        auto y = margin;
        auto previous = Block::Kind::heading;
        auto first = true;

        for (const auto& b : blocks)
        {
            const auto listy = b.kind == Block::Kind::bullet || b.kind == Block::Kind::numbered;
            const auto wasListy = previous == Block::Kind::bullet || previous == Block::Kind::numbered;

            if (! first)
            {
                if (b.kind == Block::Kind::heading)
                    y += b.level <= 2 ? 26.0f : 16.0f;
                else if (listy && wasListy)
                    y += 5.0f;
                else if (b.kind == Block::Kind::paragraph && b.level > 0)
                    y += 5.0f;
                else
                    y += 12.0f;
            }

            first = false;
            const auto indent = (float) b.level * listStep;

            switch (b.kind)
            {
                case Block::Kind::heading:
                {
                    anchors[b.anchor] = y;
                    auto bold = b.text;

                    for (auto& span : bold)
                        span.bold = true;

                    y += place (layOut (bold, headingSize (b.level), Theme::textStrong, width), x0, y);

                    if (b.level <= 2)
                    {
                        y += 6.0f;
                        rules.push_back ({ x0, y, x0 + width, y });
                    }

                    break;
                }

                case Block::Kind::paragraph:
                {
                    // Under a list item (a level in), it lines up with the item's text.
                    const auto inset = b.level > 0 ? indent + 6.0f : 0.0f;
                    y += place (layOut (b.text, bodySize, Theme::text, width - inset), x0 + inset, y);
                    break;
                }

                case Block::Kind::caption:
                    y += place (layOut (b.text, captionSize, Theme::dimText, width), x0, y);
                    break;

                case Block::Kind::bullet:
                case Block::Kind::numbered:
                {
                    const auto marker = b.kind == Block::Kind::bullet ? juce::String (juce::CharPointer_UTF8 ("\xe2\x80\xa2")) : b.number + ".";
                    markers.push_back ({ marker, { x0 + indent + 4.0f, y }, Theme::dimText });
                    y += place (layOut (b.text, bodySize, Theme::text, width - indent - listStep - 6.0f), x0 + indent + listStep + 6.0f, y);
                    break;
                }

                case Block::Kind::image:
                {
                    const auto img = image (b.image);
                    auto w = b.width > 0 ? (float) b.width : img.isValid() ? (float) img.getWidth() / 2.0f : width;
                    w = juce::jmin (w, width);
                    const auto h = img.isValid() ? w * (float) img.getHeight() / (float) img.getWidth() : 120.0f;
                    images.push_back ({ img, { x0, y + 4.0f, w, h }, b.alt });
                    y += h + 8.0f;
                    break;
                }

                case Block::Kind::table:
                    y += table (b, x0, y, width);
                    break;
            }

            previous = b.kind;
        }

        setSize (getWidth(), juce::jmax (shortest, (int) std::ceil (y + margin)));
    }

    float table (const Block& b, float x0, float y, float width)
    {
        if (b.rows.empty())
            return 0.0f;

        size_t columns = 0;

        for (const auto& row : b.rows)
            columns = juce::jmax (columns, row.size());

        std::vector<float> widths (columns, 0.0f);

        for (size_t r = 0; r < b.rows.size(); ++r)
            for (size_t c = 0; c < b.rows[r].size(); ++c)
            {
                auto text = b.rows[r][c];

                if (r == 0)
                    for (auto& span : text)
                        span.bold = true;

                widths[c] = juce::jmax (widths[c], naturalWidth (text, tableSize) + 2.0f * cellPadX + 2.0f);
            }

        // Too wide: the columns wider than an even share give up the excess, in proportion.
        if (auto total = std::accumulate (widths.begin(), widths.end(), 0.0f); total > width)
        {
            const auto share = width / (float) columns;
            float narrow = 0.0f, wide = 0.0f;

            for (auto w : widths)
                (w <= share ? narrow : wide) += w;

            const auto scale = (width - narrow) / juce::jmax (1.0f, wide);

            for (auto& w : widths)
                if (w > share)
                    w *= scale;
        }

        const auto tableWidth = std::accumulate (widths.begin(), widths.end(), 0.0f);
        auto top = y;

        for (size_t r = 0; r < b.rows.size(); ++r)
        {
            std::vector<TextPiece> cells;
            float rowHeight = 0.0f;

            for (size_t c = 0; c < columns; ++c)
            {
                auto text = c < b.rows[r].size() ? b.rows[r][c] : Text {};

                if (r == 0)
                    for (auto& span : text)
                        span.bold = true;

                cells.push_back (layOut (text, tableSize, r == 0 ? Theme::textStrong : Theme::text, widths[c] - 2.0f * cellPadX));
                rowHeight = juce::jmax (rowHeight, cells.back().layout.getHeight());
            }

            rowHeight += 2.0f * cellPadY;

            if (r == 0)
                boxes.push_back ({ { x0, top, tableWidth, rowHeight }, Theme::raised });
            else if (r % 2 == 0)
                boxes.push_back ({ { x0, top, tableWidth, rowHeight }, Theme::base.withAlpha (0.5f) });

            auto x = x0;

            for (size_t c = 0; c < columns; ++c)
            {
                outlines.push_back ({ x, top, widths[c], rowHeight });
                place (std::move (cells[c]), x + cellPadX, top + cellPadY);
                x += widths[c];
            }

            top += rowHeight;
        }

        return top - y;
    }
};

//==============================================================================
/** The list of pages down the left. */
class Browser::Contents final : public juce::ListBoxModel
{
public:
    Contents (const juce::StringArray& pagesToList, std::function<void (int)> chosen)
        : pages (pagesToList), onChosen (std::move (chosen))
    {
        for (const auto& p : pages)
            titles.add (titleOf (p));
    }

    int getNumRows() override   { return pages.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (selected)
        {
            g.setColour (Theme::accentDeep);
            g.fillRoundedRectangle (juce::Rectangle<float> (4.0f, 1.0f, (float) width - 8.0f, (float) height - 2.0f), Theme::radius);
        }

        g.setColour (selected ? Theme::textStrong : Theme::text);
        g.setFont (juce::FontOptions (Theme::labelText));
        g.drawText (titles[row], 14, 0, width - 20, height, juce::Justification::centredLeft, true);
    }

    // A click, or the arrow keys moving the selection, opens the page.
    void selectedRowsChanged (int row) override   { if (row >= 0) onChosen (row); }

private:
    juce::StringArray pages, titles;
    std::function<void (int)> onChosen;
};

//==============================================================================
Browser::Browser() : pageList (pages())
{
    contents = std::make_unique<Contents> (pageList, [this] (int row) { open (pageList[row], {}, true); });
    contentsList.setModel (contents.get());
    contentsList.setRowHeight (30);
    Theme::follow (contentsList, juce::ListBox::backgroundColourId, Theme::base);
    addAndMakeVisible (contentsList);

    back.onClick = [this] { step (-1); };
    forward.onClick = [this] { step (1); };
    addAndMakeVisible (back);
    addAndMakeVisible (forward);

    pageTitle.setFont (juce::FontOptions (Theme::headingText).withStyle ("Bold"));
    Theme::follow (pageTitle, juce::Label::textColourId, Theme::textStrong);
    addAndMakeVisible (pageTitle);

    pageView = std::make_unique<PageView>();
    pageView->onLink = [this] (const juce::String& link) { follow (link); };
    viewport.setViewedComponent (pageView.get(), false);
    viewport.setScrollBarsShown (true, false);
    viewport.setSingleStepSizes (0, 40);
    addAndMakeVisible (viewport);

    setSize (1100, 780);
}

Browser::~Browser()
{
    contentsList.setModel (nullptr);
}

void Browser::show (const juce::String& page, const juce::String& anchor)
{
    open (page, anchor, true);
}

void Browser::open (const juce::String& page, const juce::String& anchor, bool record)
{
    if (pageText (page).isEmpty())
        return;

    // The contents list selecting the page this shows (as open itself does) changes nothing.
    if (record && anchor.isEmpty() && ! history.empty() && history[at].page == page)
        return;

    if (record)
    {
        if (! history.empty())
        {
            history[at].scroll = viewport.getViewPositionY();
            history.resize (at + 1);
        }

        history.push_back ({ page, 0 });
        at = history.size() - 1;
    }

    pageView->setPage (page);
    pageView->setWidthAndBuild (viewport.getMaximumVisibleWidth(), viewport.getMaximumVisibleHeight());
    pageTitle.setText (titleOf (page), juce::dontSendNotification);
    contentsList.selectRow (pageList.indexOf (page), true, true);   // reenters open, which returns early

    const auto y = anchor.isNotEmpty() ? juce::jmax (0, pageView->anchorY (anchor) - 12) : record ? 0 : history[at].scroll;
    viewport.setViewPosition (0, y);
    refreshButtons();
}

void Browser::follow (const juce::String& link)
{
    const auto target = resolve (history.empty() ? juce::String() : history[at].page, link);

    if (target.page.isNotEmpty())
    {
        if (! history.empty() && target.page == history[at].page && target.anchor.isNotEmpty())
            viewport.setViewPosition (0, juce::jmax (0, pageView->anchorY (target.anchor) - 12));
        else
            open (target.page, target.anchor, true);
    }
    else if (target.url.isNotEmpty())
    {
        juce::URL (target.url).launchInDefaultBrowser();
    }
}

void Browser::step (int by)
{
    const auto next = (int) at + by;

    if (history.empty() || next < 0 || next >= (int) history.size())
        return;

    history[at].scroll = viewport.getViewPositionY();
    at = (size_t) next;
    open (history[at].page, {}, false);
}

void Browser::refreshButtons()
{
    back.setEnabled (at > 0);
    forward.setEnabled (at + 1 < history.size());
}

void Browser::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);
}

void Browser::resized()
{
    auto r = getLocalBounds();
    contentsList.setBounds (r.removeFromLeft (250).reduced (0, Theme::gap));

    auto bar = r.removeFromTop (44).reduced (Theme::gap);
    back.setBounds (bar.removeFromLeft (80));
    bar.removeFromLeft (Theme::gap);
    forward.setBounds (bar.removeFromLeft (80));
    bar.removeFromLeft (2 * Theme::gap);
    pageTitle.setBounds (bar);

    viewport.setBounds (r);
    const auto scroll = viewport.getViewPositionY();
    pageView->setWidthAndBuild (viewport.getMaximumVisibleWidth(), viewport.getMaximumVisibleHeight());
    viewport.setViewPosition (0, scroll);
}

//==============================================================================
Window::Window (std::function<void()> onClose)
    : DocumentWindow ("User Guide", Theme::base, DocumentWindow::allButtons),
      closeCallback (std::move (onClose))
{
    setUsingNativeTitleBar (true);
    browser = new Browser();
    setContentOwned (browser, true);
    setResizable (true, false);
    setResizeLimits (720, 480, 4000, 4000);
    centreWithSize (1180, 820);
    setVisible (true);
}
}
