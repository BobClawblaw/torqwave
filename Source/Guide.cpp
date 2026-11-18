#include "Guide.h"
#include "BuildStamp.h"
#include "GuideData.h"

namespace Guide
{
namespace
{
    const juce::String guideFolder ("docs/guide/"), shortcutsPage ("docs/SHORTCUTS.md"), imageFolder ("docs/screenshots/");

    /** Where a built-in file is in the repository: the guide's pages in docs/guide, the shortcut
        list in docs, the screenshots in docs/screenshots. */
    juce::String repositoryPath (const juce::String& filename)
    {
        if (filename == "SHORTCUTS.md")
            return shortcutsPage;

        return (filename.endsWithIgnoreCase (".md") ? guideFolder : imageFolder) + filename;
    }

    /** A built-in file's bytes, found by its path in the repository. */
    const char* resource (const juce::String& path, int& size)
    {
        size = 0;

        for (int i = 0; i < GuideData::namedResourceListSize; ++i)
        {
            const auto* name = GuideData::namedResourceList[i];

            if (repositoryPath (GuideData::getNamedResourceOriginalFilename (name)) == path)
                return GuideData::getNamedResource (name, size);
        }

        return nullptr;
    }

    /** a/b/../c as a/c; nothing for a path that climbs out of the repository. */
    juce::String normalise (const juce::String& path)
    {
        juce::StringArray parts;

        for (const auto& part : juce::StringArray::fromTokens (path, "/", {}))
        {
            if (part.isEmpty() || part == ".")
                continue;

            if (part == "..")
            {
                if (parts.isEmpty())
                    return {};

                parts.remove (parts.size() - 1);
                continue;
            }

            parts.add (part);
        }

        return parts.joinIntoString ("/");
    }

    juce::String folderOf (const juce::String& page)
    {
        return page.upToLastOccurrenceOf ("/", true, false);
    }

    void add (Text& out, const juce::String& text, const Span& style)
    {
        if (text.isEmpty())
            return;

        if (! out.empty())
        {
            auto& last = out.back();

            if (last.bold == style.bold && last.italic == style.italic && last.code == style.code && last.link == style.link)
            {
                last.text += text;
                return;
            }
        }

        auto span = style;
        span.text = text;
        out.push_back (std::move (span));
    }

    /** The closing single * of an italic run that opens at from (not part of a **). */
    int closingStar (const juce::String& s, int from)
    {
        for (int j = from + 1; j < s.length(); ++j)
            if (s[j] == '*' && s[j - 1] != '*' && (j + 1 >= s.length() || s[j + 1] != '*'))
                return j;

        return -1;
    }

    void parseInto (const juce::String& s, const Span& style, Text& out)
    {
        juce::String plain;
        const auto flush = [&] { add (out, plain, style); plain.clear(); };

        for (int i = 0; i < s.length();)
        {
            if (s.substring (i).startsWith ("**"))
            {
                if (const auto end = s.indexOf (i + 2, "**"); end > i + 2)
                {
                    flush();
                    auto bold = style;
                    bold.bold = true;
                    parseInto (s.substring (i + 2, end), bold, out);
                    i = end + 2;
                    continue;
                }
            }
            else if (s[i] == '*' && ! style.italic)
            {
                if (const auto end = closingStar (s, i); end > i + 1)
                {
                    flush();
                    auto italic = style;
                    italic.italic = true;
                    parseInto (s.substring (i + 1, end), italic, out);
                    i = end + 1;
                    continue;
                }
            }
            else if (s[i] == '`')
            {
                if (const auto end = s.indexOfChar (i + 1, '`'); end > i)
                {
                    flush();
                    auto code = style;
                    code.code = true;
                    add (out, s.substring (i + 1, end), code);
                    i = end + 1;
                    continue;
                }
            }
            else if (s[i] == '[' && style.link.isEmpty())
            {
                const auto middle = s.indexOf (i + 1, "](");
                const auto end = middle > i ? s.indexOfChar (middle + 2, ')') : -1;

                if (middle > i && end > middle && ! s.substring (i + 1, middle).containsChar ('['))
                {
                    flush();
                    auto link = style;
                    link.link = s.substring (middle + 2, end).trim();
                    parseInto (s.substring (i + 1, middle), link, out);
                    i = end + 1;
                    continue;
                }
            }

            plain += s[i];
            ++i;
        }

        flush();
    }

    juce::String attribute (const juce::String& tag, const juce::String& name)
    {
        const auto start = tag.indexOf (name + "=\"");

        if (start < 0)
            return {};

        const auto from = start + name.length() + 2;
        const auto end = tag.indexOfChar (from, '"');
        return end > from ? tag.substring (from, end) : juce::String();
    }

    juce::String plainText (const Text& text)
    {
        juce::String s;

        for (const auto& span : text)
            s += span.text;

        return s;
    }

    /** A list item's marker: "- " or "* " (number empty), or "12. "; the text after it in rest. */
    bool listItem (const juce::String& trimmed, juce::String& number, juce::String& rest)
    {
        if (trimmed.startsWith ("- ") || (trimmed.startsWith ("* ") && ! trimmed.startsWith ("**")))
        {
            number.clear();
            rest = trimmed.substring (2);
            return true;
        }

        const auto dot = trimmed.indexOf (". ");

        if (dot > 0 && dot <= 3 && trimmed.substring (0, dot).containsOnly ("0123456789"))
        {
            number = trimmed.substring (0, dot);
            rest = trimmed.substring (dot + 2);
            return true;
        }

        return false;
    }

    int indentOf (const juce::String& line)
    {
        int n = 0;

        while (n < line.length() && line[n] == ' ')
            ++n;

        return n;
    }

    std::vector<Text> cells (const juce::String& line)
    {
        auto inner = line.trim();

        if (inner.startsWithChar ('|'))
            inner = inner.substring (1);

        if (inner.endsWithChar ('|'))
            inner = inner.dropLastCharacters (1);

        std::vector<Text> result;

        for (const auto& cell : juce::StringArray::fromTokens (inner, "|", "`"))
            result.push_back (parseInline (cell.trim()));

        return result;
    }

    bool isSeparatorRow (const juce::String& line)
    {
        return line.trim().removeCharacters ("|:- ").isEmpty();
    }
}

Text parseInline (const juce::String& s)
{
    Text out;
    parseInto (s, {}, out);
    return out;
}

juce::String anchorFor (const juce::String& headingText)
{
    juce::String anchor;

    for (const auto c : headingText.toLowerCase())
    {
        if (juce::CharacterFunctions::isLetterOrDigit (c) || c == '-' || c == '_')
            anchor += juce::String::charToString (c);
        else if (c == ' ')
            anchor += "-";
    }

    return anchor;
}

std::vector<Block> parse (const juce::String& markdown, const juce::String& page)
{
    std::vector<Block> blocks;
    const auto lines = juce::StringArray::fromLines (markdown);
    std::vector<int> listIndents;   // the indents of the lists open around the line, outermost first

    const auto depthFor = [&listIndents] (int indent)
    {
        while (! listIndents.empty() && indent < listIndents.back())
            listIndents.pop_back();

        if (listIndents.empty() || indent > listIndents.back())
            listIndents.push_back (indent);

        return (int) listIndents.size() - 1;
    };

    for (int i = 0; i < lines.size(); ++i)
    {
        const auto& line = lines[i];
        const auto trimmed = line.trim();

        if (trimmed.isEmpty())
            continue;

        const auto indent = indentOf (line);
        juce::String number, rest;
        const auto isItem = listItem (trimmed, number, rest);

        if (indent == 0 && ! isItem)
            listIndents.clear();   // a line at the margin ends the lists

        Block b;

        if (indent == 0 && trimmed.startsWithChar ('#'))
        {
            const auto level = trimmed.length() - trimmed.trimCharactersAtStart ("#").length();
            b.kind = Block::Kind::heading;
            b.level = juce::jlimit (1, 3, level);
            b.text = parseInline (trimmed.substring (level).trim());
            b.anchor = anchorFor (plainText (b.text));
        }
        else if (trimmed.startsWithChar ('|'))
        {
            b.kind = Block::Kind::table;

            for (; i < lines.size() && lines[i].trim().startsWithChar ('|'); ++i)
                if (! isSeparatorRow (lines[i]))
                    b.rows.push_back (cells (lines[i]));

            --i;
        }
        else if (trimmed.startsWith ("![") && trimmed.endsWithChar (')') && trimmed.contains ("]("))
        {
            b.kind = Block::Kind::image;
            b.alt = trimmed.fromFirstOccurrenceOf ("![", false, false).upToLastOccurrenceOf ("](", false, false);
            b.image = normalise (folderOf (page) + trimmed.fromLastOccurrenceOf ("](", false, false).dropLastCharacters (1));
        }
        else if (trimmed.startsWith ("<img "))
        {
            b.kind = Block::Kind::image;
            b.alt = attribute (trimmed, "alt");
            b.image = normalise (folderOf (page) + attribute (trimmed, "src"));
            b.width = attribute (trimmed, "width").getIntValue();
        }
        else if (isItem)
        {
            b.kind = number.isEmpty() ? Block::Kind::bullet : Block::Kind::numbered;
            b.number = number;
            b.level = depthFor (indent);
            b.text = parseInline (rest);
        }
        else
        {
            b.kind = trimmed.startsWithChar ('*') && ! trimmed.startsWith ("**") && trimmed.endsWithChar ('*')
                         ? Block::Kind::caption : Block::Kind::paragraph;

            // A paragraph indented under a list item belongs to it, a level in.
            b.level = indent > 0 && ! listIndents.empty() ? depthFor (indent) : 0;
            b.text = parseInline (trimmed);
        }

        blocks.push_back (std::move (b));
    }

    return blocks;
}

juce::StringArray pages()
{
    juce::StringArray guide;

    for (int i = 0; i < GuideData::namedResourceListSize; ++i)
    {
        const juce::String filename (GuideData::getNamedResourceOriginalFilename (GuideData::namedResourceList[i]));

        if (filename.endsWithIgnoreCase (".md") && filename != "README.md" && filename != "SHORTCUTS.md")
            guide.add (guideFolder + filename);
    }

    guide.sort (false);
    guide.insert (0, guideFolder + "README.md");
    guide.add (shortcutsPage);
    return guide;
}

juce::String titleOf (const juce::String& page)
{
    for (const auto& b : parse (pageText (page), page))
        if (b.kind == Block::Kind::heading)
            return plainText (b.text);

    return page.fromLastOccurrenceOf ("/", false, false);
}

juce::String pageText (const juce::String& page)
{
    int size = 0;
    const auto* data = resource (page, size);
    return data != nullptr ? juce::String::fromUTF8 (data, size) : juce::String();
}

juce::Image image (const juce::String& path)
{
    int size = 0;
    const auto* data = resource (path, size);
    return data != nullptr ? juce::ImageCache::getFromMemory (data, size) : juce::Image();
}

Target resolve (const juce::String& fromPage, const juce::String& link)
{
    if (link.startsWithIgnoreCase ("http://") || link.startsWithIgnoreCase ("https://") || link.startsWithIgnoreCase ("mailto:"))
        return { {}, {}, link };

    const auto path = link.upToFirstOccurrenceOf ("#", false, false);
    const auto anchor = link.fromFirstOccurrenceOf ("#", false, false);
    const auto page = path.isEmpty() ? fromPage : normalise (folderOf (fromPage) + path);

    if (page.isEmpty())
        return {};

    if (pageText (page).isNotEmpty())
        return { page, anchor, {} };

    return { {}, {}, onGitHub (page).toString (false) + (anchor.isNotEmpty() ? "#" + anchor : juce::String()) };
}

juce::URL onGitHub (const juce::String& path)
{
    auto* app = juce::JUCEApplication::getInstance();
    const auto version = app != nullptr ? app->getApplicationVersion() : juce::String();
    const auto ref = TORQWAVE_BUILD_RELEASE != 0 && version.isNotEmpty() ? "v" + version : juce::String ("main");
    return juce::URL ("https://github.com/BobClawblaw/torqwave/blob/" + ref + "/" + path);
}
}
