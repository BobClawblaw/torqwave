#include "WaveSynthFavourites.h"
#include "WaveSynth.h"

namespace WaveSynth
{
namespace
{
    // Factory patches taken out of the bank as near-copies of another in their category (by the
    // sweep in Scripts/patch_sweep.py, at a distance under 2.25), and the patch each was a copy of:
    // a favourite marked under the old name is the one that stayed.
    const std::pair<const char*, const char*> mergedPatches[] {
        { "Abandoned Clank", "Pressed Clave" },
        { "Acid Rivet", "Strobe Slam" },
        { "Anvil Floor", "Seismic Tick" },
        { "Basalt Clink", "Brutal Tom" },
        { "Basalt Furnace", "Riveted Furnace" },
        { "Basalt Pulse", "Foundry Pump" },
        { "Basement Pulse", "Sodium Drive" },
        { "Basement Pump", "Cellar Ballast" },
        { "Bitter Field", "Tar Glow" },
        { "Bitter Smash", "Steel Smash" },
        { "Bitter Snare", "Hollow Zap" },
        { "Black Reactor", "Endless Silo" },
        { "Black Rumble", "Scorched Slab" },
        { "Bleak Hum", "Endless Silo" },
        { "Bleak Mass", "Sunken Pump" },
        { "Brutal Barb", "Dry Pin" },
        { "Brutal Cry", "Abandoned Cutter" },
        { "Brutal Detonation", "Steel Smash" },
        { "Bunker Drive", "Carbon Bass" },
        { "Buried Dynamo", "Substation" },
        { "Burnt Drum", "Brutal Tom" },
        { "Caldera Thump", "Seismic Tick" },
        { "Carbon Cutter", "Industrial Cry" },
        { "Carbon Zap", "Cobalt Block" },
        { "Cellar Engine", "Rolling Offbeat" },
        { "Cellar Turbine", "Bleak Girder" },
        { "Choir Clap", "Crowd Clap" },
        { "Choked Bass", "Iron Sub" },
        { "Choked Pipe", "Static Shaker" },
        { "Choked Squeal", "Carbon Spill" },
        { "Chrome Barb", "Sullen Pin" },
        { "Chrome Clamp", "Sharp Crack" },
        { "Chrome Groan", "FM Basement" },
        { "Chrome Stamp", "Riveted Jab" },
        { "Cinder Can", "Brutal Tom" },
        { "Cinder Etch", "Acid Choir" },
        { "Clanging Hat", "Brutal Tom" },
        { "Cold Kick", "Nickel Pipe" },
        { "Cold Spring", "Shadow Hook" },
        { "Concrete Surge", "Ash Countdown" },
        { "Concrete Voice", "Brutal Arc" },
        { "Corroded Kick", "Brutal Tom" },
        { "Corroded Mass", "Iron Sub" },
        { "Dead Arc", "Frozen Probe" },
        { "Dead Drive", "Obsidian Crank" },
        { "Dead Floor", "Feral Anvil" },
        { "Dead Hat", "Pressed Kick" },
        { "Dead Pebble", "Dry Pin" },
        { "Dead Throb", "Warehouse Sub" },
        { "Deep Bass", "Body Music" },
        { "Derelict Chop", "Abandoned Crack" },
        { "Derelict Drip", "Abandoned Drip" },
        { "Derelict Engine", "Foundry Pump" },
        { "Derelict Line", "Concrete Bath" },
        { "Derelict Weight", "Obsidian Crank" },
        { "Derelict Wire", "Welded Wire" },
        { "Distant Horizon", "Burnt Horizon" },
        { "Dry Shard", "Bit Pluck" },
        { "Electric Torch", "Frozen Cry" },
        { "Factory Clave", "Cobalt Block" },
        { "Feral Drive", "Scorched Sub" },
        { "Feral Fizz", "Acid Scream" },
        { "Feral Pin", "Brutal Pebble" },
        { "Feral Torch", "Oily Alarm" },
        { "Foundry Throb", "Foundry Pump" },
        { "Foundry Thud", "Void Floor" },
        { "Frozen Knock", "Minor Stab" },
        { "Frozen Scrap", "Static Texture" },
        { "Frozen Tine", "Pinged Shard" },
        { "Grey Kick", "Static Shaker" },
        { "Grey Pick", "Toxic Prong" },
        { "Grey Probe", "Industrial Cry" },
        { "Grey Tom", "Snapping Hat" },
        { "Grey Wail", "Abandoned Drip" },
        { "Ground Floor", "Foundry Pump" },
        { "Heavy Block", "Factory Tom" },
        { "Heavy Gate", "Cold Line" },
        { "Hollow Kick", "Snapping Hat" },
        { "Hollow Rain", "Rubber Acid" },
        { "Hollow Siren", "Brutal Tension" },
        { "Hollow Speech", "Riveted Vox" },
        { "Hollow Tom", "Chrome Can" },
        { "Ignition Manual", "Cracked Manual" },
        { "Industrial Loop", "Acid Corrosion" },
        { "Industrial Sky", "Void Cloud" },
        { "Industrial Spring", "Wire Pluck" },
        { "Infinite Plant", "Throbbing Boiler" },
        { "Iron Clank", "Feral Hat" },
        { "Iron Countdown", "Draining Ignition" },
        { "Iron Girder", "Bleak Thud" },
        { "Iron Lance", "Burning Spark" },
        { "Iron Sting", "Cinder Vat" },
        { "Iron Veil", "Steel Horizon" },
        { "Leaden Bath", "Carbon Burn" },
        { "Leaden Riser", "Brutal Tension" },
        { "Leaden Signal", "Sullen Filament" },
        { "Leaking Bite", "Corrosive Loop" },
        { "Live Beam", "Burning Spark" },
        { "Live Needle", "Sullen Filament" },
        { "Low Girder", "Bleak Girder" },
        { "Molten Anvil", "Industrial Weight" },
        { "Molten Tension", "Null Ignition" },
        { "Molten Turbine", "FM Basement" },
        { "Mute Voice", "Sullen Filament" },
        { "Mute Wind", "Tar Tension" },
        { "Mute Zap", "Null Knock" },
        { "Nickel Chain", "Nickel Pipe" },
        { "Night Block", "Snapping Hat" },
        { "Night Ignition", "Brutal Tension" },
        { "Night Laser", "Burning Spark" },
        { "Night Scrap", "Corroded Dust" },
        { "Null Lead", "Burning Spark" },
        { "Null Punch", "Nickel Slam" },
        { "Obsidian Spill", "Acid Corrosion" },
        { "Oily Block", "Pressed Clave" },
        { "Oily Dusk", "Fading Veil" },
        { "Percussion Relay", "Scorched Tonewheel" },
        { "Piercing Spark", "Rotten Probe" },
        { "Pressed Ballast", "Distorted 909 Bass" },
        { "Pressed Knot", "Soot Vortex" },
        { "Ratchet Console", "Spark Plug Organ" },
        { "Riveted Drip", "Corrosive Loop" },
        { "Riveted Signal", "Rotten Probe" },
        { "Riveted Squelch", "Acid Corrosion" },
        { "Riveted Thud", "Warehouse Sub" },
        { "Rotten Cutter", "Sullen Filament" },
        { "Rotten Gnaw", "Acid Corrosion" },
        { "Ruined Bolt", "Sharp Chord" },
        { "Ruined Hammer", "Bleak Girder" },
        { "Ruined Hull", "Oily Boiler" },
        { "Ruined Nail", "Sodium Twang" },
        { "Ruined Siren", "Rotten Probe" },
        { "Rust Hiss", "Iron Etch" },
        { "Scorched Anvil", "FM Basement" },
        { "Scorched Bolt", "Snapping Hat" },
        { "Severed Spring", "Bit Pluck" },
        { "Severed Squeal", "Night Drip" },
        { "Shadow Anvil", "Rust Drive" },
        { "Shadow Furnace", "Endless Silo" },
        { "Shadow Probe", "Brutal Arc" },
        { "Short Stab", "Minor Stab" },
        { "Singing Bows", "Bowed Girder" },
        { "Soot Wire", "Wire Pluck" },
        { "Static Blow", "Vacant Blow" },
        { "Static Etch", "Night Drip" },
        { "Static Grind", "Concrete Ballast" },
        { "Static Tendril", "Iron Wire" },
        { "Steel Kick", "Pressed Kick" },
        { "Steel Rivet", "Sharp Crack" },
        { "Steel Slab", "Cellar Ballast" },
        { "Sterile Dusk", "Cobalt Cloud" },
        { "Sterile Loop", "Abandoned Drip" },
        { "Sterile Probe", "Rotten Probe" },
        { "Sterile Undertow", "Obsidian Tendril" },
        { "Subsonic Dynamo", "Choked Shaft" },
        { "Sullen Bath", "Acid Corrosion" },
        { "Sullen Laser", "Burning Spark" },
        { "Sunken Ascent", "Shadow Pressure" },
        { "Sunken Hat", "Snapping Hat" },
        { "Tar Flare", "Sullen Filament" },
        { "Tar Press", "Scorched Slab" },
        { "Tar Shot", "Minor Stab" },
        { "Tar Sting", "Abandoned Drip" },
        { "Tar Weight", "Vacant Bass" },
        { "Toxic Block", "Pressed Clave" },
        { "Toxic Chop", "Riveted Jab" },
        { "Tungsten Bell Lead", "Frozen Cry" },
        { "Tunnel Sub", "Warehouse Sub" },
        { "Vacant Pump", "Concrete Ballast" },
        { "Void Press", "Foundry Pump" },
        { "Void Squeal", "Acid Choir" },
        { "Vowel Kick", "Ion Kick" },
        { "Wailing Cutter", "Industrial Cry" },
        { "Welded Spin", "Brutal Tension" },
    };

    int indexOfPatch (const juce::String& name)
    {
        const auto& all = factoryPatches();

        for (size_t i = 0; i < all.size(); ++i)
            if (all[i].name == name)
                return (int) i;

        return -1;
    }

    /** The name itself if it is a factory patch, the patch it was merged into if it was taken out,
        or nothing. */
    juce::String stillAFactoryPatch (const juce::String& name)
    {
        if (indexOfPatch (name) >= 0)
            return name;

        for (auto& [old, kept] : mergedPatches)
            if (name == old)
                return indexOfPatch (kept) >= 0 ? juce::String (kept) : juce::String();

        return {};
    }

    std::unique_ptr<Favourites>& sharedOnes()
    {
        static std::unique_ptr<Favourites> f;
        return f;
    }
}

Favourites::Favourites (juce::File f) : file (std::move (f))
{
    read();
}

Favourites& Favourites::shared()
{
    if (sharedOnes() == nullptr)
        sharedOnes() = std::make_unique<Favourites> (defaultFile());

    return *sharedOnes();
}

juce::File Favourites::defaultFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Torqwave").getChildFile ("Synth Favourites.txt");
}

void Favourites::useFile (const juce::File& f)
{
    sharedOnes() = std::make_unique<Favourites> (f);
}

void Favourites::read()
{
    marked.clear();

    for (auto& line : juce::StringArray::fromLines (file.loadFileAsString()))
        if (const auto name = stillAFactoryPatch (line.trim()); name.isNotEmpty())
            marked.addIfNotAlreadyThere (name);

    marked.sort (true);
}

void Favourites::write() const
{
    file.getParentDirectory().createDirectory();
    file.replaceWithText (marked.joinIntoString ("\n") + (marked.isEmpty() ? "" : "\n"));
}

bool Favourites::contains (const juce::String& name) const
{
    return marked.contains (name);
}

void Favourites::set (const juce::String& name, bool favourite)
{
    if (favourite == contains (name) || (favourite && indexOfPatch (name) < 0))
        return;

    if (favourite)
    {
        marked.add (name);
        marked.sort (true);
    }
    else
        marked.removeString (name);

    write();
}

bool Favourites::toggle (const juce::String& name)
{
    set (name, ! contains (name));
    return contains (name);
}

std::vector<int> Favourites::indices() const
{
    std::vector<int> found;

    for (auto& name : marked)
        if (const auto i = indexOfPatch (name); i >= 0)
            found.push_back (i);

    return found;
}

std::vector<int> Favourites::inTreeOrder() const
{
    auto found = indices();
    const auto& all = factoryPatches();
    const auto& categories = patchCategories();
    std::sort (found.begin(), found.end(), [&] (int a, int b)
    {
        const auto ca = categories.indexOf (all[(size_t) a].category), cb = categories.indexOf (all[(size_t) b].category);
        return ca != cb ? ca < cb : a < b;
    });
    return found;
}
}
