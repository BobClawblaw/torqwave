#!/usr/bin/env python3
"""Writes the site's Sounds pages (docs/sounds/*.html), its Quality page's clips (the part of
docs/quality/index.html between the "demos" marks) and the clips on the front page (the part of
docs/index.html between the "clips" marks) from what Torqwave's tests rendered:

    TorqwaveTests --sound-demos docs/sounds/audio      (the clips and docs/sounds/audio/index.json)
    TorqwaveTests --quality-demos docs/quality         (the clips, pictures and docs/quality/facts.json)
    python3 docs/build_pages.py

Every name, description, length and number on those pages comes from the two JSON files, apart from
the sentences below written for the clips chosen by ear for the first Sounds page."""

import html
import json
import re
from collections import OrderedDict
from pathlib import Path

DOCS = Path(__file__).resolve().parent
CLIPS = json.loads((DOCS / "sounds/audio/index.json").read_text())
FACTS = json.loads((DOCS / "quality/facts.json").read_text())

# What each clip chosen by ear is, written with the patch or preset open.
BY_EAR = {
    ("swarm", "Tremolo Swarm Wall"): "Twelve strings over three octaves of one note, 30 cents apart, four guitars picked 14 times a second through the fuzz.",
    ("swarm", "Every String One Note"): "Twelve strings in unison on four guitars: the plainest wall there is.",
    ("swarm", "Low Octave Roar"): "The lowest strings an octave down, three guitars driven hard.",
    ("swarm", "Palm Muted Chug"): "Short, damped sixteenths through the fuzz.",
    ("swarm", "Crunch Riff"): "One stage of crunch, a low octave under the riff.",
    ("swarm", "Fifths Riff"): "Root, fifth and octave from one key: a power chord on every note.",
    ("swarm", "Tremolo Strum"): "Strummed again and again while a chord is held.",
    ("swarm", "Clean Shimmer"): "No amp: the strings alone, bright and ringing.",
    ("swarm", "Long Low Drone"): "Twelve strings that ring for twenty seconds.",
    ("swarm", "Eight Voice Swarm Pulse"): "Eight oscillators, a resonant filter snapping open on each sixteenth of a sequence.",
    ("swarm", "Racing Swarm Sixteenths"): "A tight, crunchy swarm for fast sequences.",
    ("swarm", "Swarm Pulse Engine"): "One held note, pulsing in sixteenths synced to the tempo.",
    ("swarm", "Fuzzed Swarm Motor"): "A held note pulsing in sixteenth triplets through the fuzz.",
    ("swarm", "Converging Swarm"): "Oscillators spread wide that close to one pitch over three seconds.",
    ("swarm", "Opening Swarm"): "The other way: one pitch that opens into a cloud.",
    ("swarm", "Bruised Swarm"): "A slow, dark swarm under minor chords.",
    ("swarm", "Gliding Swarm"): "Every oscillator sliding to the next note at its own speed.",
    ("swarm", "Smeared Swarm Lead"): "Long glides and a wide fan: the notes smear into each other.",
    ("swarm", "Swarm Bass"): "A tight swarm an octave down, its filter plucked on every note.",
    ("swarm", "Swarm Opening Out"): "Twelve oscillators opening over three octaves for a rise.",
    ("swarm", "Swarm Through Fuzz"): "The swarm through the guitar's fuzz and cabinet.",
    ("swarm", "Wailing Swarm"): "A gliding, fuzzed swarm lead.",
    ("synth", "Target Lock Bass"): "A sequenced bass: a sawtooth through a resonant low-pass that rings on its third harmonic.",
    ("synth", "EBM Sequence Bass"): "A saw and a square 7 cents apart, the filter plucked on every note.",
    ("synth", "Warehouse Sub"): "A round, mono sub, its Grit macro left down.",
    ("synth", "Abyss Reese"): "Two pairs of detuned saws beating, under a slow filter sway.",
    ("synth", "Acid Corrosion"): "A squelching acid line with a dotted echo.",
    ("synth", "Siren Burn"): "Three saws through the fuzz, a vibrato fading in, and a dotted echo.",
    ("synth", "Mercury Lead"): "A hard-synced wave swept by its envelope, with an echo.",
    ("synth", "Wailing Fuzz Lead"): "Strings ringing on over a sustained wave, fuzz, long slides and a late vibrato.",
    ("synth", "Body Riff Power Chord"): "A synth guitar for EBM: a power chord from one key, clipped hard.",
    ("synth", "Swarm Drone"): "Strings all on one note, strummed, drifting and fuzzed.",
    ("synth", "Minor Stab"): "A minor triad from one key, three saws plucked by the filter.",
    ("synth", "Glass Shard"): "A bright FM pluck with a dotted echo.",
    ("synth", "Cold Bell"): "A metallic bell ringing out under a reverb.",
    ("synth", "Gated Machine"): "Three detuned saws chopped by a 16-step gate, with an echo.",
    ("synth", "Concrete Pad"): "Eight detuned saws, a slow filter sway, chorus and a long reverb.",
    ("synth", "Reactor Drone"): "A growling wave over a saw an octave down, slowly drifting.",
    ("synth", "Tension Riser"): "Noise and saws swept up through a band-pass over six seconds.",
    ("synth", "Low Boom"): "A sine dropping in pitch over a sub, into a long reverb.",
}

# The clips on the front page, in this order.
FRONT = [("synth", "Concrete Pad"), ("synth", "Furnace Halo"), ("synth", "Cold Bell"), ("synth", "Glass Shard"), ("synth", "Iron Verdict"),
         ("synth", "Ghost Radio Choir"), ("synth", "Abyss Reese"), ("synth", "Siren Burn"), ("swarm", "Tremolo Swarm Wall")]

# The Sequence Library's genres and styles in the order the pages show them: techno and industrial first.
GENRES = ["Techno", "Industrial"]
FIRST_STYLES = ["Techno/Dark", "Techno/Industrial", "Techno/Hard", "Techno/Acid", "Industrial/EBM", "Industrial/Electro-industrial",
                "Industrial/Dark electro", "Industrial/Aggrotech", "Industrial/Techno-body"]
ROLES = ["Drums", "Bass", "Harmony", "Pads", "Arps", "Melody", "SFX"]

GROUPS = OrderedDict([
    ("synth", ("Torqwave Synth", "synth.html")),
    ("swarm", ("StringSwarm", "stringswarm.html")),
    ("drums", ("Basic Drums", "drums-and-sampler.html")),
    ("sampler", ("Torqwave Sampler", "drums-and-sampler.html")),
    ("kits", ("Sequence Library: kits", "kits.html")),
    ("sequences", ("Sequence Library: sequences", "sequences.html")),
])


def esc(text):
    return html.escape(str(text), quote=True)


def anchor(text):
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def of(group):
    return [c for c in CLIPS if c["group"] == group]


def megabytes(clips):
    return f"{sum(c['bytes'] for c in clips) / 1048576:.1f} MB"


def number(n):
    return f"{n:g}"


def player(clip, prefix):
    return f'<audio controls preload="none" src="{esc(prefix + clip["file"])}"></audio>'


def instrument_card(clip, prefix, heading="h3", kind=None):
    """A patch, preset or kit of a built-in instrument."""
    written = BY_EAR.get((clip["group"], clip["name"]))
    lines = [f'<div class="clip"><div class="kind">{esc(kind or clip["category"])}</div><{heading}>{esc(clip["name"])}</{heading}>']

    if written:
        lines.append(f'<p class="what">{esc(written)}</p>')

    if clip.get("about"):
        lines.append(f'<p{"" if written else " class=\"what\""}>{esc(clip["about"])}</p>')

    if clip.get("groove"):
        plays = groove_line(clip)
    elif clip.get("featured"):
        plays = "Chosen by ear. Plays " + clip["phrase"] + "."
    else:
        plays = f'Plays the audition phrase “{clip["phrase"]}” at 125 BPM.'

        if "parts" in clip:
            plays = f'{clip["parts"]} parts in use; no. {clip["place"]} of {clip["of"]} in the synth\'s {clip["category"]} list. ' + plays
        elif "place" in clip:
            plays = f'No. {clip["place"]} of {clip["of"]} in {clip["category"]}. ' + plays

    lines.append(f'<p>{esc(plays)}</p><div class="fill"></div>{player(clip, prefix)}</div>')
    return "".join(lines)


def groove_line(clip):
    """What a Basic Drums clip plays: a factory groove of the Sequence Library."""
    return (f'Plays the factory groove “{clip["name"]}” ({clip["style"].replace("/", " › ")}, {number(clip["tempo"])} BPM, energy {clip["energy"]} of 5): '
            f'{clip["groove"][0].lower() + clip["groove"][1:]}.')


def loop_card(clip, prefix, heading="h3", kind=None):
    """A factory kit or sequence of the Sequence Library."""
    facts = [f'{number(clip["tempo"])} BPM']

    if clip.get("key"):
        facts.append(clip["key"])

    facts.append(f'{number(clip["bars"])} bar{"" if clip["bars"] == 1 else "s"}' + (f', {clip["rounds"]} times round' if clip["rounds"] > 1 else ""))

    if clip.get("energy"):
        facts.append(f'energy {clip["energy"]} of 5')

    lines = [f'<div class="clip"><div class="kind">{esc(kind or clip["category"].replace("/", " › "))}</div><{heading}>{esc(clip["name"])}</{heading}>']

    if clip["group"] == "kits":
        lines.append(f'<p>{esc(" · ".join(facts))}</p><ul>')
        lines += [f'<li>{esc(part["name"])} <span>({esc(part["role"])}, on {esc(part["voice"])})</span></li>' for part in clip["parts"]]
        lines.append("</ul>")
    else:
        lines.append(f'<p class="what">{esc(clip["about"])}</p>')
        lines.append(f'<p>{esc(clip["role"].replace(" > ", " › "))} · {esc(" · ".join(facts))} · on {esc(clip["parts"][0]["voice"])}</p>')

    lines.append(f'<div class="fill"></div>{player(clip, prefix)}</div>')
    return "".join(lines)


def card(clip, prefix, heading="h3", kind=None):
    return (loop_card if clip["group"] in ("kits", "sequences") else instrument_card)(clip, prefix, heading, kind)


def page(path, title, description, body, section="sounds"):
    front = f'---\nlayout: default\ntitle: "{title}"\ndescription: "{description}"\nsection: {section}\nwide: true\n---\n'
    (DOCS / path).write_text(front + "<!-- Written by docs/build_pages.py: change that, not this. -->\n" + body + "\n")
    print(path)


def crumbs(name):
    return f'<p class="crumbs"><a href="./">Sounds</a> › {esc(name)}</p>'


def chips(names, counts=None):
    return '<div class="chips">' + "".join(
        f'<a href="#{anchor(n)}">{esc(n)}{f" <b>{counts[n]}</b>" if counts else ""}</a>' for n in names) + "</div>"


def grouped(clips, key):
    groups = OrderedDict()

    for c in clips:
        groups.setdefault(key(c), []).append(c)

    return groups


HOW = ("Each clip was rendered by Torqwave's own code (<code>TorqwaveTests --sound-demos</code>, in the repository) and written as an MP3. "
       "Nothing was added: no effects, no levelling, no editing, except that a clip still sounding where it stops is faded over its last 30 ms so the cut does not click.")


RULE = ("A point for each oscillator on, each of those with unison voices, each with a warp, the sub, the noise, each filter, each effect on, "
        "each modulation routing in use and each named macro.")


def synth_page():
    clips = of("synth")
    by_ear = [c for c in clips if c.get("featured")]
    by_category = grouped(clips, lambda c: c["category"])
    cards = []

    for category, found in by_category.items():
        found.sort(key=lambda c: (-c["parts"], c.get("place", 0)))
        cards.append(f'<a class="card" href="synth/{anchor(category)}.html"><div class="count">{len(found)} patches · {megabytes(found)}</div>'
                     f'<h3>{esc(category)}</h3><p>{esc(", ".join(c["name"] for c in found[:3]))} and {len(found) - 3} more.</p></a>')

    body = [f'<div class="title">{crumbs("Torqwave Synth")}<h1>Torqwave Synth: {len(clips)} of its patches</h1>',
            f'<p>A wavetable synth with 3,743 factory patches in 29 categories. Here are {len(clips)} of them: the ones that use the most of the synth, from every category, '
            f'and {len(by_ear)} chosen by ear. Under each name is what the patch is made of, read off the synth with the patch loaded.</p>',
            f'<p class="meta">{HOW} Each patch plays one of the audition phrases Torqwave itself plays when you choose a sound, at 125 BPM; the {len(by_ear)} chosen by ear play a part that suits them. '
            f'{megabytes(clips)} in all; a clip loads only when you play it.</p>',
            chips(["Chosen by ear", "By category", "How they were chosen"]), "</div>",
            '<h2 id="chosen-by-ear">Chosen by ear</h2><p class="lede">Picked for the first version of this page, each on four bars at 120 BPM.</p>'
            '<div class="clips">' + "".join(card(c, "audio/") for c in by_ear) + "</div>",
            '<h2 id="by-category">By category</h2><p class="lede">In each category, the patches with the most going on.</p>'
            '<div class="grid">' + "".join(cards) + "</div>",
            '<h2 id="how-they-were-chosen">How they were chosen</h2><ul class="plain">'
            f'<li><b>By count, not by ear.</b> Every patch was loaded and its parts in use counted. {RULE} Each card gives its count.</li>'
            f'<li><b>From every category.</b> Each category has its share of the {len(clips)} (about a fifth of its patches, ten at least), and within it the patches with the highest count come first, '
            'the earlier in the synth\'s list among equals.</li>'
            '<li><b>A high count is not a promise of a better sound.</b> It finds the patches with layers, movement and effects; a plain sub with one oscillator is a good sub and counts low. '
            f'Apart from the {len(by_ear)} chosen by ear, these were not listened to one by one.</li></ul>']
    page("sounds/synth.html", "Torqwave Synth: hear its patches",
         f"{len(clips)} Torqwave Synth factory patches from all 29 categories, the ones that use the most of the synth, rendered by Torqwave with nothing added.", "\n".join(body))
    (DOCS / "sounds/synth").mkdir(exist_ok=True)
    names = list(by_category)

    for at, (category, found) in enumerate(by_category.items()):
        before, after = names[at - 1] if at else None, names[at + 1] if at + 1 < len(names) else None
        pager = '<p class="meta">' + " · ".join(
            f'<a href="{anchor(n)}.html">{arrow[0]}{esc(n)}{arrow[1]}</a>' for n, arrow in ((before, ("← ", "")), (after, ("", " →"))) if n) + "</p>"
        body = [f'<div class="title"><p class="crumbs"><a href="../">Sounds</a> › <a href="../synth.html">Torqwave Synth</a> › {esc(category)}</p>'
                f'<h1>Torqwave Synth: {esc(category)}</h1>',
                f'<p>{len(found)} of the {max(c.get("of", 0) for c in found)} factory patches in {esc(category)}: the ones that use the most of the synth, the highest count first.</p>',
                f'<p class="meta">{HOW} {RULE} {megabytes(found)} in all; a clip loads only when you play it.</p>{pager}</div>',
                '<div class="clips">' + "".join(card(c, "../audio/") for c in found) + "</div>", pager]
        page(f"sounds/synth/{anchor(category)}.html", f"Torqwave Synth: {category} patches",
             f"{len(found)} Torqwave Synth factory patches in {category}, rendered by Torqwave with nothing added.", "\n".join(body))


def swarm_page():
    clips = of("swarm")
    by_category = grouped(clips, lambda c: c["category"])
    order = [n for n in by_category if not n.startswith("Swarm")] + [n for n in by_category if n.startswith("Swarm")]
    counts = {name: len(by_category[name]) for name in order}
    body = [f'<div class="title">{crumbs("StringSwarm")}<h1>StringSwarm: {len(clips)} of its presets</h1>',
            '<p>A guitar with every string tuned to the note you play, through a fuzz or a crunch amp: one note becomes a wall of strings beating against each other. '
            'With its source switched to Swarm, an oscillator for each string takes the strings\' place, fanned out, converging, gliding and drifting. '
            f'It has 296 factory presets in 11 categories; here are {len(clips)}: 22 chosen by ear, and from each category the first preset and two more spread through the list.</p>',
            f'<p class="meta">{HOW} {megabytes(clips)} in all.</p>', chips(order, counts), "</div>"]

    for category in order:
        found = sorted(by_category[category], key=lambda c: (0 if c.get("featured") else 1, c.get("place", 0)))
        body.append(f'<h2 id="{anchor(category)}">{esc(category)}</h2><div class="clips">' + "".join(card(c, "audio/") for c in found) + "</div>")

    page("sounds/stringswarm.html", "StringSwarm: hear its presets",
         f"{len(clips)} StringSwarm factory presets from all 11 categories, rendered by Torqwave with nothing added.", "\n".join(body))


def drums_page():
    drums, sampler = of("drums"), of("sampler")
    kits = grouped(drums, lambda c: c["category"])
    body = [f'<div class="title">{crumbs("Basic Drums and Torqwave Sampler")}<h1>Basic Drums and Torqwave Sampler</h1>',
            f'<p>Basic Drums\' {len(kits)} kits, each playing factory grooves written for it, and Torqwave Sampler\'s {len(sampler)} factory presets.</p>',
            f'<p class="meta">{HOW} Each kit plays the Sequence Library\'s factory grooves written for it (of its Drums › Full Grooves, the three with the most energy, or all of them where a kit has fewer), as the library\'s preview plays them, at the tempo they were written at. '
            f'The sampler\'s presets play the audition phrases Torqwave plays when you choose a sound, at 125 BPM. {megabytes(drums + sampler)} in all.</p>',
            chips(["Basic Drums", "Torqwave Sampler"]), "</div>",
            '<h2 id="basic-drums">Basic Drums</h2>',
            '<p class="lede">A drum machine with 30 sounds that cover every General MIDI drum note, 21 of them with a second, Modern type beside the Classic one, each with its own tune, decay, tone, level, pan and drive, '
            'and a filter, crush, reverb and delay on every pad. A kit sets all of it at once.</p>']

    for kit, found in kits.items():
        body.append(f'<h3 class="sub" id="{anchor(kit)}">{esc(kit)}</h3><p class="lede">{esc(found[0]["about"])}</p><div class="clips">')

        for c in found:
            what = (f'{c["groove"]}. {c["style"].replace("/", " › ")}, {number(c["tempo"])} BPM, energy {c["energy"]} of 5.' if c.get("groove")
                    else f'The audition beat “{c["phrase"]}” at 125 BPM.')
            body.append(f'<div class="clip"><div class="kind">{esc(kit)}{" · one of " + str(c["of"]) + " factory grooves" if c.get("groove") else ""}</div>'
                        f'<h4>{esc(c["name"] if c.get("groove") else c["phrase"])}</h4>'
                        f'<p>{esc(what)}</p><div class="fill"></div>{player(c, "audio/")}</div>')

        body.append("</div>")

    body.append('<h2 id="torqwave-sampler">Torqwave Sampler</h2>'
                '<p class="lede">The sampler plays your own sounds across the keys, or cut into slices at their hits. '
                'It ships with a small set of factory presets rendered from Torqwave\'s own instruments, so there is something on the keys before you add a sound of yours: these are all of them.</p>'
                '<div class="clips">' + "".join(card(c, "audio/") for c in sampler) + "</div>")
    page("sounds/drums-and-sampler.html", "Basic Drums and Torqwave Sampler: hear them",
         f"Basic Drums' {len(kits)} kits and Torqwave Sampler's {len(sampler)} factory presets, rendered by Torqwave with nothing added.", "\n".join(body))


def style_order(styles):
    def rank(style):
        genre = style.split("/")[0]
        return (GENRES.index(genre) if genre in GENRES else len(GENRES), genre,
                FIRST_STYLES.index(style) if style in FIRST_STYLES else len(FIRST_STYLES), style)

    return sorted(styles, key=rank)


def library_page(group, path, title, heading, intro, description):
    clips = of(group)
    by_style = grouped(clips, lambda c: c["category"])
    styles = style_order(by_style)
    genres = grouped(styles, lambda s: s.split("/")[0])
    body = [f'<div class="title">{crumbs(title)}<h1>{heading}</h1>', intro,
            f'<p class="meta">{HOW} Each plays on the voices it comes with, Torqwave Synth patches and Basic Drums kits, at the tempo it was written at, '
            f'as the Sequence Library\'s own preview plays it; a loop under five seconds goes round again. {megabytes(clips)} in all.</p>',
            chips(list(genres), {g: sum(len(by_style[s]) for s in found) for g, found in genres.items()}), "</div>"]

    for genre, found in genres.items():
        body.append(f'<h2 id="{anchor(genre)}">{esc(genre)}</h2>')

        for style in found:
            in_style = by_style[style]

            if group == "sequences":
                in_style.sort(key=lambda c: ROLES.index(c["role"].split(" > ")[0]))
            else:
                in_style.sort(key=lambda c: c["name"])

            body.append(f'<h3 class="sub" id="{anchor(style)}">{esc(style.split("/")[1])} <small>{len(in_style)} {"kit" if group == "kits" else "sequence"}{"" if len(in_style) == 1 else "s"}</small></h3>'
                        '<div class="clips">' + "".join(card(c, "audio/", "h4", style.split("/")[1]) for c in in_style) + "</div>")

    page(path, title, description, "\n".join(body))


def kits_page():
    clips = of("kits")
    library_page("kits", "sounds/kits.html", "Sequence Library: every factory kit", f"The Sequence Library: all {len(clips)} factory kits",
                 f'<p>A kit is a set of sequences written to play together: a groove, a bass line, chords or stabs, an arp or a hook, a fill and a transition. '
                 f'Dropped into a session it makes a track for each part, with its instrument. The Sequence Library ships {len(clips)} of them in '
                 f'{len(set(c["category"] for c in clips))} styles, techno and industrial first. This is every one, each part named with the voice it plays on.</p>',
                 f"All {len(clips)} factory kits of Torqwave's Sequence Library, by style, rendered by Torqwave with nothing added.")


def sequences_page():
    clips = of("sequences")
    library_page("sequences", "sounds/sequences.html", "Sequence Library: factory sequences", f"The Sequence Library: {len(clips)} of its 1,032 factory sequences",
                 f'<p>The Sequence Library ships 1,032 sequence presets in 37 styles: drums, bass lines, harmony, pads, arps, melodies and effects, each with a line saying what it does. '
                 f'Here are {len(clips)} on their own: one of each role from nine techno and industrial styles, and from each of the others a bass line and a melody, an arp or a harmony part. '
                 'Of each, the one that does the most by measure was taken: its notes to a bar, its different pitches and how varied its rhythm is, on a voice the style\'s other clips do not play. '
                 'A part with two notes a bar or fewer, or that repeats one note, is passed over, and where a style\'s bass lines all do that, another role stands in. '
                 'A sub on one note is shown nowhere. Club tools has only one-sound parts, and shows its kick and its hats. Chosen by that rule, not by ear: nobody has listened to them one by one. '
                 'The <a href="kits.html">kits</a> play six sequences together.</p>',
                 f"{len(clips)} of the 1,032 factory sequences in Torqwave's Sequence Library, from all 37 styles, rendered by Torqwave with nothing added.")


def find(group, name):
    for c in CLIPS:
        if c["group"] == group and c["name"] == name:
            return c

    raise SystemExit(f"no clip {group}: {name}")


def front_clips(prefix):
    cards = []

    for group, name in FRONT:
        clip = find(group, name)
        kind = GROUPS[group][0].replace("Sequence Library: kits", "Factory kit") + (" · " + clip["category"].replace("/", " › ") if group in ("kits", "synth", "sampler") else "")
        cards.append(card(clip, prefix, "h3", kind))

    return '<div class="clips">' + "".join(cards) + "</div>"


def sounds_index():
    total = len(CLIPS)
    rows = []
    pages = [("synth", "synth.html", "Torqwave Synth", "of 3,743 patches: the ones that use the most of the synth, from all 29 categories"),
             ("swarm", "stringswarm.html", "StringSwarm", "of 296 presets, from all 11 categories"),
             ("drums", "drums-and-sampler.html#basic-drums", "Basic Drums", f"all {len(set(c['category'] for c in of('drums')))} kits, on factory grooves from the Sequence Library"),
             ("sampler", "drums-and-sampler.html#torqwave-sampler", "Torqwave Sampler", "all 11 factory presets"),
             ("kits", "kits.html", "Sequence Library: kits", f"all 116 factory kits, in {len(set(c['category'] for c in of('kits')))} styles"),
             ("sequences", "sequences.html", "Sequence Library: sequences", "of 1,032 factory sequences, from all 37 styles")]
    cards = []

    for group, link, name, what in pages:
        clips = of(group)
        cards.append(f'<a class="card" href="{link}"><div class="count">{len(clips)} clips · {megabytes(clips)}</div><h3>{esc(name)}</h3><p>{len(clips)} {esc(what)}.</p></a>')
        rows.append(f'<tr><td>{esc(name)}</td><td class="n">{len(clips)}</td><td>{esc(what)}</td><td class="n">{megabytes(clips)}</td></tr>')

    body = [f'<div class="title"><h1>Hear Torqwave: {total} clips of what is built in</h1>',
            '<p>Torqwave comes with four instruments, thousands of patches and presets for them, and a library of sequences and kits to play on them. '
            'These pages let you hear them before you download anything.</p>',
            f'<p class="meta">{HOW}</p></div>',
            '<h2 id="start">Start here</h2><p class="lede">Pads, bells, a choir, a lead and a wall of strings from Torqwave Synth and StringSwarm.</p>', front_clips("audio/"),
            '<h2 id="all">Everything rendered</h2><div class="grid">' + "".join(cards) + "</div>",
            '<table class="facts"><tr><th>What</th><th>Clips</th><th>Which</th><th>Size</th></tr>' + "".join(rows)
            + f'<tr><th>All</th><th>{total}</th><th></th><th>{megabytes(CLIPS)}</th></tr></table>',
            '<h2 id="how">How they were chosen and made</h2>',
            '<ul class="plain">'
            '<li><b>Chosen by ear:</b> 18 Torqwave Synth patches and 22 StringSwarm presets, picked for the first version of this page and each given a part that suits it (four bars at 120 BPM). Their cards say so.</li>'
            '<li><b>Chosen by rule:</b> everything else, so the pages show what the lists are like and not only their best. Of the synth\'s patches, the ones that use the most of the synth, by a count of their parts in use, a share from every category; '
            'from each StringSwarm category, the first preset and two more; every Basic Drums kit, every sampler preset and every kit of the Sequence Library; '
            'and of the sequences, the one in each role of a style that does the most by measure (notes to a bar, different pitches, rhythm), on a voice the style\'s other clips do not play.</li>'
            '<li><b>Played as the app plays them:</b> patches and presets on the audition phrases Torqwave plays when you choose a sound; kits and sequences through the Sequence Library\'s own preview, at the tempo they were written at.</li>'
            '<li><b>Not listened to one by one.</b> The 40 chosen by ear were; the rest were rendered and checked by measurement only (each has sound, none reaches full scale). If one is wrong, <a href="https://github.com/BobClawblaw/torqwave/issues">say so</a>.</li>'
            '<li><b>MP3s</b> at a variable bit rate (LAME V4), stereo, 48 kHz, written by Torqwave\'s own exporter. The app plays the instruments themselves, not these files.</li>'
            '</ul>']
    page("sounds/index.html", "Hear Torqwave",
         f"{total} clips of Torqwave's built-in instruments, patches, presets, kits and sequences, rendered by Torqwave with nothing added.", "\n".join(body))


def between(path, mark, text):
    """Puts text between <!-- mark:start --> and <!-- mark:end --> in a hand-written page."""
    file = DOCS / path
    old = file.read_text()
    start, end = f"<!-- {mark}:start -->", f"<!-- {mark}:end -->"

    if start not in old or end not in old:
        raise SystemExit(f"{path} has no {mark} marks")

    file.write_text(old[:old.index(start) + len(start)] + "\n" + text + "\n" + old[old.index(end):])
    print(path, f"({mark})")


def db(v):
    return f"{v:.1f}".replace("-", "−")


def quality_demos():
    by_id = {d["id"]: d for d in FACTS}
    out = []

    def demo(d, heading, players, table, note=""):
        out.append(f'<div class="demo" id="{d["id"]}"><h3>{esc(heading)}</h3><dl>'
                   f'<dt>The clip</dt><dd>{esc(d["source"])}.</dd><dt>The damage</dt><dd>{esc(d["damage"][0].upper() + d["damage"][1:])}.</dd>'
                   f'<dt>The repair</dt><dd>{esc(d["repair"])}.</dd></dl><div class="shots">')

        for which, label in players:
            out.append(f'<figure><figcaption>{esc(label)}</figcaption><audio controls preload="none" src="{esc(d[which])}"></audio>'
                       f'<img loading="lazy" src="{esc(d[which + "Picture"])}" alt="Spectrogram of the clip: {esc(label.lower())}" width="900" height="220"></figure>')

        out.append('</div><table class="facts">' + table + "</table>" + note + "</div>")

    d = by_id["clicks"]
    between_marks = d["clicksAdded"] - d["clicksLeft"] - d["clicksGone"]
    demo(d, "Clicks", [("before", "With the clicks"), ("after", "Clicks repaired")],
         "<tr><th>Measured</th><th>With the clicks</th><th>Clicks repaired</th></tr>"
         f'<tr><td>Clicks</td><td class="n">{d["clicksAdded"]} put in</td><td class="n">{d["clicksRepaired"]} repaired, {d["skipped"]} skipped</td></tr>'
         f'<tr><td>Places still more than a tenth of full scale from the clean clip</td><td></td><td class="n">{d["clicksLeft"]}</td></tr>'
         f'<tr><td>Places within a hundredth of full scale of it</td><td></td><td class="n">{d["clicksGone"]}</td></tr>'
         f'<tr><td>Places in between</td><td></td><td class="n">{between_marks}</td></tr>'
         f'<tr><td>How far the clip is from the clean one</td><td class="n">{db(d["differenceBefore"])} dB</td><td class="n">{db(d["differenceAfter"])} dB</td></tr>',
         '<p class="meta">A place is the three samples from where a click was put in, in both channels. The repair rebuilds each click from the sound round it, so a repaired place is near the clean clip, not equal to it. '
         'These clicks are in a pad and a bell; the changelog records what the repair does with clicks in a drum loop, where it leaves some.</p>')

    d = by_id["hiss"]
    demo(d, "Hiss, with a noise print", [("before", "With the hiss"), ("after", "Reduced by 12 dB"), ("after24", "Reduced by 24 dB")],
         "<tr><th>Measured</th><th>With the hiss</th><th>Reduced by 12 dB</th><th>Reduced by 24 dB</th></tr>"
         f'<tr><td>The noise alone, in the first second (RMS)</td><td class="n">{db(d["noiseBefore"])} dBFS</td><td class="n">{db(d["noiseAfter"])} dBFS</td><td class="n">{db(d["noiseAfter24"])} dBFS</td></tr>'
         f'<tr><td>How far the music is from the clean clip</td><td class="n">{db(d["differenceBefore"])} dB</td><td class="n">{db(d["differenceAfter"])} dB</td><td class="n">{db(d["differenceAfter24"])} dB</td></tr>',
         '<p class="meta">The second row is everything that differs from the clean clip, against the clean clip\'s own level: the hiss before, what is left of it and what the reduction did to the music after. '
         'Lower is nearer the original. The noise falls by about what was asked for; the music ends about as near at 24 dB as at 12, so the deeper setting buys quieter gaps, not a truer clip.</p>')

    d = by_id["hum"]
    demo(d, "Mains hum", [("before", "With the hum"), ("after", "Hum removed")],
         "<tr><th>Level at</th><th>Clean clip</th><th>With the hum</th><th>Hum removed</th></tr>"
         + "".join(f'<tr><td>{hz} Hz</td><td class="n">{db(d[f"clean{hz}"])} dBFS</td><td class="n">{db(d[f"before{hz}"])} dBFS</td><td class="n">{db(d[f"after{hz}"])} dBFS</td></tr>' for hz in (50, 100, 150)),
         '<p class="meta">Each row is the level of that one frequency over the whole clip (the left channel). The clean clip has a little there of its own, which the notches take too.</p>')

    d = by_id["highs"]
    demo(d, "Missing highs", [("original", "The original"), ("before", "Nothing over 5 kHz"), ("after", "Highs rebuilt")],
         "<tr><th>Measured</th><th>The original</th><th>Nothing over 5 kHz</th><th>Highs rebuilt</th></tr>"
         f'<tr><td>Level from 6 to 16 kHz</td><td class="n">{db(d["highsOriginal"])} dB</td><td class="n">{db(d["highsBefore"])} dB</td><td class="n">{db(d["highsAfter"])} dB</td></tr>'
         f'<tr><td>The cutoff Torqwave found</td><td></td><td class="n">{d["cutoffFound"]:,} Hz</td><td></td></tr>',
         '<p class="meta">Rebuilt highs are made from the band below them: they are a likeness of what was lost, not the lost sound itself. Compare the rebuilt clip with the original by ear.</p>')

    d = by_id["loudness"]
    over = d["truePeakAfter"] + 1.0
    demo(d, "Loudness: a loop brought to −10 LUFS", [("before", "As rendered"), ("after", "Through the Maximizer")],
         "<tr><th>Measured by Torqwave's loudness meter</th><th>As rendered</th><th>Through the Maximizer</th></tr>"
         f'<tr><td>Integrated loudness</td><td class="n">{db(d["loudnessBefore"])} LUFS</td><td class="n">{db(d["loudnessAfter"])} LUFS</td></tr>'
         f'<tr><td>True peak</td><td class="n">{d["truePeakBefore"]:.2f} dBTP</td><td class="n">{d["truePeakAfter"]:.2f} dBTP</td></tr>'
         f'<tr><td>Loudness range</td><td class="n">{d["rangeBefore"]:.1f} LU</td><td class="n">{d["rangeAfter"]:.1f} LU</td></tr>'
         f'<tr><td>Gain into the ceiling</td><td></td><td class="n">{d["gainDb"]:.1f} dB</td></tr>',
         (f'<p class="note">The true peak reads {d["truePeakAfter"]:.2f} dBTP on Torqwave\'s own meter, {over:.2f} dB over the −1 dBTP ceiling that was set. No sample is over the ceiling; '
          'Leave that margin if a delivery spec is strict.</p>'
          if over > 0.005 else
          f'<p class="meta">The ceiling was set to −1 dBTP, and the clip reads {d["truePeakAfter"]:.2f} dBTP on Torqwave\'s own meter: under it. '
          'Before 0.21.0 the same clip read −0.79 dBTP, 0.21 dB over.</p>'))

    between("quality/index.html", "demos", "\n".join(out))
    return sum(v for d in FACTS if d["id"] != "clipping" for k, v in d.items() if k.endswith("Bytes"))


if __name__ == "__main__":
    sounds_index()
    synth_page()
    swarm_page()
    drums_page()
    kits_page()
    sequences_page()
    between("index.html", "clips", front_clips("sounds/audio/")
            + f'\n<div class="actions"><a class="button primary" href="sounds/">Hear all {len(CLIPS)} clips</a></div>'
              f'\n<p class="meta">By instrument: <a href="sounds/synth.html">Torqwave Synth</a> ({len(of("synth"))}), '
              f'<a href="sounds/stringswarm.html">StringSwarm</a> ({len(of("swarm"))}), <a href="sounds/drums-and-sampler.html">Basic Drums and Torqwave Sampler</a> ({len(of("drums")) + len(of("sampler"))}), '
              f'<a href="sounds/kits.html">every factory kit</a> ({len(of("kits"))}) and <a href="sounds/sequences.html">sequences from every style</a> ({len(of("sequences"))}).</p>')
    bytes_quality = quality_demos()
    print(f"{len(CLIPS)} sound clips, {megabytes(CLIPS)}; quality clips and pictures {bytes_quality / 1048576:.1f} MB")
