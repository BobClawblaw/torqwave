#include <JuceHeader.h>
#include "../Source/Guide.h"
#include "TestUtils.h"
#include "GuideTests.h"

namespace
{
    using TestUtils::check;

    void inlineMarkdown()
    {
        const auto t = Guide::parseInline ("Press **Space** to *play*, `F1` for [the guide](README.md#contents).");
        check (t.size() == 9, "the inline parser splits a line into its nine runs");

        if (t.size() == 9)
        {
            check (t[1].text == "Space" && t[1].bold && ! t[1].italic, "**bold** is bold");
            check (t[3].text == "play" && t[3].italic && ! t[3].bold, "*italic* is italic");
            check (t[5].text == "F1" && t[5].code, "`code` is code");
            check (t[7].text == "the guide" && t[7].link == "README.md#contents", "a link keeps its text and target");
            check (t[0].text == "Press " && t[6].text == " for " && t[8].text == ".", "plain text between them stays plain");
        }

        const auto nested = Guide::parseInline ("**[Help > User Guide](a.md)** and 2 * 3 = 6");
        check (nested.size() >= 2 && nested[0].bold && nested[0].link == "a.md", "a link inside bold is both");
        check (nested.back().text.endsWith ("2 * 3 = 6") && ! nested.back().italic, "a lone star is text");
    }

    void blocks()
    {
        const auto b = Guide::parse ("# Title\n\nSome text.\n\n- one\n  - two\n    more of two\n1. first\n\n| A | B |\n|---|---|\n| `x\\|y` | 2 |\n\n"
                                     "![Alt](../screenshots/editor.png)\n<img src=\"../screenshots/editor.png\" width=\"400\" alt=\"Editor\">\n*A caption.*\n",
                                     "docs/guide/01-getting-started.md");
        using K = Guide::Block::Kind;
        check (b.size() == 10, "the sample page has ten blocks");

        if (b.size() == 10)
        {
            check (b[0].kind == K::heading && b[0].level == 1 && b[0].anchor == "title", "a heading has its level and anchor");
            check (b[1].kind == K::paragraph && b[1].level == 0, "a paragraph");
            check (b[2].kind == K::bullet && b[2].level == 0 && b[3].kind == K::bullet && b[3].level == 1, "a nested bullet is a level in");
            check (b[4].kind == K::paragraph && b[4].level == 2, "a line indented under an item belongs to it");
            check (b[5].kind == K::numbered && b[5].number == "1" && b[5].level == 0, "a numbered item keeps its number");
            check (b[6].kind == K::table && b[6].rows.size() == 2 && b[6].rows[0].size() == 2, "a table skips its separator row");
            check (b[7].kind == K::image && b[7].image == "docs/screenshots/editor.png" && b[7].alt == "Alt", "an image's path is from the repository's top");
            check (b[8].kind == K::image && b[8].width == 400 && b[8].alt == "Editor", "an <img> keeps its width");
            check (b[9].kind == K::caption, "an italic line is a caption");
        }

        check (Guide::anchorFor ("Loudness & mastering: LUFS (2)") == "loudness--mastering-lufs-2", "anchors are made as GitHub makes them");
    }

    /** Every page the app carries parses, shows only images it carries, and links only to pages
        and headings that exist or to addresses on the web. */
    void everyPage()
    {
        const auto pages = Guide::pages();
        check (pages.size() == 16, "the guide has its contents, fourteen pages and the shortcut list");
        check (pages[0] == "docs/guide/README.md" && pages[pages.size() - 1] == "docs/SHORTCUTS.md", "the contents come first, the shortcuts last");

        juce::StringArray problems;

        for (const auto& page : pages)
        {
            const auto text = Guide::pageText (page);

            if (text.isEmpty())
            {
                problems.add (page + ": empty");
                continue;
            }

            if (Guide::titleOf (page) == page.fromLastOccurrenceOf ("/", false, false))
                problems.add (page + ": no title");

            for (const auto& b : Guide::parse (text, page))
            {
                if (b.kind == Guide::Block::Kind::image && ! Guide::image (b.image).isValid())
                    problems.add (page + ": image " + b.image);

                std::vector<const Guide::Text*> texts { &b.text };

                for (const auto& row : b.rows)
                    for (const auto& cell : row)
                        texts.push_back (&cell);

                for (const auto* t : texts)
                    for (const auto& span : *t)
                    {
                        if (span.link.isEmpty())
                            continue;

                        const auto target = Guide::resolve (page, span.link);

                        if (target.page.isNotEmpty())
                        {
                            if (target.anchor.isEmpty())
                                continue;

                            auto found = false;

                            for (const auto& other : Guide::parse (Guide::pageText (target.page), target.page))
                                found = found || (other.kind == Guide::Block::Kind::heading && other.anchor == target.anchor);

                            if (! found)
                                problems.add (page + ": no heading for " + span.link);
                        }
                        else if (! target.url.startsWith ("https://"))
                        {
                            problems.add (page + ": link " + span.link);
                        }
                    }
            }
        }

        check (problems.isEmpty(), "every page's images and links are good" + (problems.isEmpty() ? juce::String() : ": " + problems.joinIntoString ("; ")));

        const auto out = Guide::resolve ("docs/guide/README.md", "../../README.md#install");
        check (out.page.isEmpty() && out.url.startsWith ("https://github.com/BobClawblaw/torqwave/blob/") && out.url.endsWith ("/README.md#install"),
               "a link to a file the app doesn't carry opens it on GitHub");
        check (Guide::resolve ("docs/guide/README.md", "../SHORTCUTS.md").page == "docs/SHORTCUTS.md", "a link to the shortcut list opens it here");
    }
}

void runGuideTests()
{
    inlineMarkdown();
    blocks();
    everyPage();
}
