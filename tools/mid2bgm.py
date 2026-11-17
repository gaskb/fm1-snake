#!/usr/bin/env python3
"""Convert OutRun MIDI files to compact 4-byte event streams for FM-1 Arcade."""
import sys
import mido


def convert_midi(path, ch_map, max_sec=115.0):
    mid = mido.MidiFile(path)
    # Collect (time_sec, voice_idx, note, vel)
    raw = []
    cur_t = 0.0
    for msg in mid:
        cur_t += msg.time
        if cur_t > max_sec:
            break
        if msg.type == "note_on" and msg.velocity > 0:
            if msg.channel == 9:
                # GM Drum mapping -> 1=kick, 2=snare, 3=hat
                n = msg.note
                if n in (35, 36):
                    d = 1
                elif n in (38, 40, 37, 39):
                    d = 2
                elif n in (42, 44, 46, 49, 51, 53, 55, 57, 59):
                    d = 3
                else:
                    continue
                raw.append((cur_t, 4, d, min(127, msg.velocity)))
            elif msg.channel in ch_map:
                v = ch_map[msg.channel]
                raw.append((cur_t, v, msg.note, min(127, msg.velocity)))
        elif msg.type == "note_off" or (msg.type == "note_on" and msg.velocity == 0):
            if msg.channel in ch_map and msg.channel != 9:
                v = ch_map[msg.channel]
                raw.append((cur_t, v, 0, 0))

    # Deduplicate simultaneous events on same melodic voice (keep latest) and convert to 10ms ticks
    events = []
    last_tick = 0
    active_note = [0, 0, 0, 0]
    for t_sec, ch, note, vel in raw:
        tick = int(round(t_sec * 100.0))
        if ch < 4:
            if note == 0:
                if active_note[ch] == 0:
                    continue
                active_note[ch] = 0
            else:
                active_note[ch] = note
        dt = max(0, tick - last_tick)
        last_tick = tick
        while dt > 255:
            events.append((255, 0xFF, 0, 0))
            dt -= 255
        events.append((dt, ch, note, vel))
    # End with short silence & note-off
    for ch in range(4):
        events.append((0, ch, 0, 0))
    events.append((50, 0xFF, 0, 0))
    return events


def main():
    songs = [
        (
            "BGM_MAGICAL",
            sys.argv[1],
            {1: 0, 3: 1, 0: 2, 2: 3},  # Lead1=ch1, Lead2=ch3, Bass=ch0, Chord=ch2
        ),
        (
            "BGM_BREEZE",
            sys.argv[2],
            {3: 0, 4: 1, 0: 2, 1: 3},  # Lead1=ch3, Lead2=ch4, Bass=ch0, Chord=ch1
        ),
        (
            "BGM_SPLASH",
            sys.argv[3],
            {2: 0, 1: 1, 0: 2, 4: 3},  # Lead1=ch2, Lead2=ch1, Bass=ch0, Chord=ch4
        ),
    ]

    out = ["/* Auto-generated from OutRun MIDI files by tools/mid2bgm.py */", "#pragma once", "#include <stdint.h>", ""]
    out.append("typedef struct { uint8_t dt; uint8_t ch; uint8_t note; uint8_t vel; } bgm_ev_t;")
    out.append("")

    for sym, path, ch_map in songs:
        evs = convert_midi(path, ch_map)
        print(f"{sym}: {len(evs)} events ({len(evs)*4} bytes)", file=sys.stderr)
        out.append(f"static const bgm_ev_t {sym}[] = {{")
        line = []
        for dt, ch, note, vel in evs:
            line.append(f"{{{dt},{ch},{note},{vel}}}")
            if len(line) >= 12:
                out.append("    " + ",".join(line) + ",")
                line = []
        if line:
            out.append("    " + ",".join(line) + ",")
        out.append("};")
        out.append(f"#define {sym}_LEN (sizeof({sym}) / sizeof({sym}[0]))")
        out.append("")

    sys.stdout.write("\n".join(out))


if __name__ == "__main__":
    main()
