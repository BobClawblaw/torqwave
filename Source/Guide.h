#pragma once

#include <JuceHeader.h>

/** The user guide inside the app: its Markdown pages and screenshots, built into the app from
    docs/, and a reader for the Markdown they use. Pages and images are named by their path in the
    repository ("docs/guide/02-editing-audio.md", "docs/screenshots/editor.png"). */
namespace Guide
{
    /** A run of text with one look. A link's target is as written in the Markdown. */
    struct Span
    {
        juce::String text;
        bool bold = false, italic = false, code = false;
        juce::String link;

        bool operator== (const Span&) const = default;
    };

    using Text = std::vector<Span>;

    struct Block
    {
        enum class Kind { heading, paragraph, caption, bullet, numbered, table, image };

        Kind kind = Kind::paragraph;
        int level = 0;                         // a heading's level (1 to 3); a list item's or a list paragraph's depth (0 for none)
        juce::String number;                   // a numbered item's number
        Text text;
        std::vector<std::vector<Text>> rows;   // a table's rows of cells, the header first
        juce::String image, alt;               // an image's path in the repository, and its description
        int width = 0;                         // an image's width in points when the page sets one, else 0
        juce::String anchor;                   // a heading's anchor, as GitHub makes it
    };

    /** The inline Markdown of one line or paragraph: **bold**, *italic*, `code` and [links](target). */
    Text parseInline (const juce::String&);

    /** The blocks of a page. page is its path in the repository, for the images it shows. */
    std::vector<Block> parse (const juce::String& markdown, const juce::String& page);

    /** A heading's anchor as GitHub makes it: lower case, punctuation dropped, spaces as hyphens. */
    juce::String anchorFor (const juce::String& headingText);

    /** The pages in reading order: the guide's contents, its twelve pages, then the shortcut list. */
    juce::StringArray pages();

    /** A page's title: its first heading. */
    juce::String titleOf (const juce::String& page);

    /** A built-in page's Markdown, or empty when there is no such page. */
    juce::String pageText (const juce::String& page);

    /** A built-in image, or an invalid one. */
    juce::Image image (const juce::String& path);

    /** Where a link goes from a page: another built-in page (with the anchor after # if any), or an
        address to open in the browser. */
    struct Target
    {
        juce::String page, anchor, url;
    };

    Target resolve (const juce::String& fromPage, const juce::String& link);

    /** The address of a file of the repository on GitHub: at this version's tag for a release build,
        so it matches the app; on main for a test build, whose commit may not be published. */
    juce::URL onGitHub (const juce::String& path);
}
