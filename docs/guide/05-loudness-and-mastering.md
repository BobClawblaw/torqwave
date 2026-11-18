# Loudness and mastering

[Guide](README.md) › Loudness and mastering

Broadcasters, podcast hosts and streaming services each ask for audio at a set **loudness**. Loudness is measured in LUFS, which follows how loud the audio sounds over its whole length rather than its highest peak. They also set a **true-peak ceiling**, so the audio doesn't clip when it is converted or encoded.

This page shows how to measure both, hit a target in one step, and finish a master.

## Measure first

| Tool | Where | What it tells you |
|---|---|---|
| Levels | Window > Levels | The level of what is playing, with peak and clip lights. Click it to reset them |
| Loudness Statistics... | Effects > Level and Loudness | The EBU R128 loudness and true peak of the selection or file |
| Amplitude Statistics | Window > Amplitude Statistics | Click **Scan**: peaks and where they are, true peaks, samples at full scale, DC offset, RMS, the bits the samples need, and loudness, per channel. **Copy** puts the table on the clipboard as text |
| Frequency Analysis | Window > Frequency Analysis | The spectrum of what is playing |
| Tonal Balance | Window > Tonal Balance | The long-term spectrum of what is playing against a target range for a style: Full mix, Bass-heavy, Bright, Speech or Orchestral |
| Phase Meter | Window > Phase Meter | The stereo image and the correlation between left and right |

The **Analysis** workspace (Window > Workspace > Analysis) lays out the analyzer, the phase meter and the levels together.

## Hit a loudness target

### Match Loudness

**Effects > Level and Loudness > Match Loudness...** sets the integrated loudness of the selection or file.

| Setting | What it does |
|---|---|
| Target (LUFS) | The loudness to reach. Common targets: EBU R128 -23, ATSC A/85 -24, podcasts -16, streaming -14 |
| True peak ceiling (dBTP) | The highest the true peak may go |
| When the ceiling stops the gain | **Stop short of the target** keeps the audio untouched apart from gain, so it ends up quieter than the target. **Limit to reach the target** adds a true-peak limiter that holds the peaks at the ceiling |

### Meet Delivery Spec

**Effects > Level and Loudness > Meet Delivery Spec...** does the whole job in one step and writes a report of the result. Choose a spec:

| Spec | Target | Tolerance | True peak |
|---|---|---|---|
| EBU R128 | -23 LUFS | ±0.5 LU | -1 dBTP |
| ATSC A/85 | -24 LKFS | ±2 dB | -2 dBTP |
| Online -16 LUFS | -16 LUFS | ±1 LU | -1 dBTP |
| Online -14 LUFS | -14 LUFS | ±1 LU | -1 dBTP |
| Your own | -70 to 0 LUFS | 0.1 to 10 LU | -30 to 0 dBTP |

Torqwave applies the gain to reach the target. Where that gain would push the true peak past the ceiling, it uses a true-peak limiter instead.

The report lists, each with PASS or FAIL and the measurement before for comparison:

- the integrated loudness and the true peak, against the spec;
- for information: the loudness range, the loudest momentary and short-term loudness, and the sample peak.

A session's mixdown and its stems can be delivered to a spec in the same way. For stems, one gain is used for all of them, so they still add up to the delivered mix.

## Speech

**Level Speech...** evens out a voice. It raises quiet passages and lowers loud ones, towards a target loudness, with limits on how far it may boost or cut. Use it on interviews and podcasts before matching the loudness, so that a quiet guest and a loud host end up close together.

For the rest of a dialogue cleanup (noise, breaths, plosives, room), see [Spectral editing and repair](03-spectral-editing-and-repair.md).

## Finishing a master

A typical order for finishing a music master:

1. **Tone:** shape the balance with the Parametric EQ, or with **Match EQ** to take on the tone of a reference track. Capture the reference first with **Capture Match Reference** (from a selection) or **Capture Match Reference from File...**. Check the result against Tonal Balance.
2. **Dynamics:** use a gentle Compressor or the Multiband Compressor if needed. The Transient Shaper brings attacks forward or back.
3. **Width:** use **Multiband Stereo Width**. A narrow or mono low end and slightly wider highs is a common choice. Watch the Phase Meter.
4. **Loudness:** finish with the **Maximizer**:
   - **Gain** drives the sound into the ceiling. Nothing comes out above the **Ceiling**, the true peaks between samples included.
   - **Character** sets how the limiter moves. **Transparent** looks 10 ms ahead and recovers slowly, so it pumps and distorts little. **Loud** looks 2 ms ahead and recovers fast, louder and rougher. **Balanced** sits between.
   - **Transient emphasis** lifts the attacks before the limiter, by up to 6 dB, so drums keep their edge.
5. **Check:** use **Loudness Statistics...**, or **Meet Delivery Spec...** for a pass or fail.
6. **Lossy formats:** **Codec Preview...** plays the selection as it will come back from an MP3 or AAC file at a chosen bit rate, so you can hear what encoding does before you export.
7. **Bit depth:** **Dither...** is the very last step before saving at 16 or 24 bits. Choose the depth, and noise shaping of None, Light or Strong.

The dynamics and EQ steps can all go in the [effects rack](04-effects-and-the-rack.md#the-effects-rack), so you can adjust them together while you listen, then apply them in one go.

Next: [Multitrack sessions](06-multitrack-sessions.md).
