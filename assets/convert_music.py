#!/usr/bin/env python3
"""
convert_music.py

Reads a Game Boy Sound Engine assembly file (using macros defined in audio/engine.asm),
encodes all music commands (including labels and control flow) into byte streams,
and emits one C `uint8_t` array per channel for 1:1 playback on a compatible C audio engine.

Usage:
    python3 convert_music.py input.asm
"""
import re
import sys
from typing import List, Dict, Tuple

# Mapping of note names to high nibble values
NOTE_MAP = {
    'rest': 0x0,
    '__':   0x0,
    'C_':   0x1,
    'C#':   0x2,
    'D_':   0x3,
    'D#':   0x4,
    'E_':   0x5,
    'F_':   0x6,
    'F#':   0x7,
    'G_':   0x8,
    'G#':   0x9,
    'A_':   0xA,
    'A#':   0xB,
    'B_':   0xC,
}


def parse_num(token: str) -> int:
    """Parse a numeric token which may be decimal or $-prefixed hex.

    Args:
        token: The string token to parse (e.g. '10', '$0A').

    Returns:
        The integer value of the token.
    """
    token = token.strip()
    if token.startswith('$'):
        token = '0x' + token[1:]
    return int(token, 0)


class CommandEncoder:
    """Encodes each assembly macro into its corresponding byte sequence."""

    # Control flow commands with their opcodes
    CONTROL_FLOW = {
        'endchannel': 0xFF,
        'jumpchannel': 0xFC,
        'loopchannel': 0xFD,
        'callchannel': 0xFE,
        'jumpif': 0xFB,
        'restartchannel': 0xEA,
    }

    # Command byte sizes (opcode + parameters)
    COMMAND_SIZES = {
        'note': 1,
        'sound': 4,
        'noise': 5,
        'octave': 1,
        'notetype': lambda args: 2 if len(args) > 1 else 1,
        'pitchoffset': 2,
        'tempo': 2,  # Fixed: tempo takes 2 bytes
        'dutycycle': 2,
        'intensity': 2,
        'soundinput': 2,
        'sound_duty': 2,
        'togglesfx': 1,
        'slidepitchto': 3,
        'vibrato': 3,
        'togglenoise': 2,
        'panning': 2,
        'volume': 2,
        'tone': 2,
        'tempo_relative': 2,
        'newsong': 2,
        'sfxpriorityon': 1,
        'sfxpriorityoff': 1,
        'setcondition': 2,
        'stereopanning': 2,
        'unknownmusic0xe2': 2,
        'unknownmusic0xe7': 2,
        'unknownmusic0xe8': 2,
        'unknownmusic0xee': 3,
        'sfxtogglenoise': 2,
        'music0xf1': 1,
        'music0xf2': 1,
        'music0xf3': 1,
        'music0xf4': 1,
        'music0xf5': 1,
        'music0xf6': 1,
        'music0xf7': 1,
        'music0xf8': 1,
        'unknownmusic0xf9': 1,
        # Control flow
        'endchannel': 1,
        'jumpchannel': 3,
        'loopchannel': 4,
        'callchannel': 3,
        'jumpif': 4,
        'restartchannel': 3,
    }

    @staticmethod
    def note(pitch: str, length: int) -> List[int]:
        """Encode a note command.

        Args:
            pitch: Note name (e.g., 'C_', 'rest', '__')
            length: Note length (0-15)

        Returns:
            Single byte with pitch in high nibble, length in low nibble
        """
        p = NOTE_MAP[pitch]
        return [(p << 4) | (length & 0xF)]

    @staticmethod
    def sound(pitch: int, octave: int, intensity: int, frequency: int) -> List[int]:
        """Encode sound command."""
        return [0xC0 | (pitch & 0xF), octave & 0xFF, intensity & 0xFF, frequency & 0xFF]

    @staticmethod
    def noise(pitch: int, duration: int, intensity: int, frequency: int) -> List[int]:
        """Encode noise command."""
        return [0xD2, pitch & 0xFF, duration & 0xFF, intensity & 0xFF, frequency & 0xFF]

    @staticmethod
    def octave(n: int) -> List[int]:
        """Encode octave command ($D0-$D7)."""
        return [0xD7 - n]

    @staticmethod
    def notetype(length: int, intensity: int = None) -> List[int]:
        """Encode notetype command ($D8)."""
        b1 = 0xD8 | (length & 0xF)
        return [b1] if intensity is None else [b1, intensity & 0xFF]

    @staticmethod
    def pitchoffset(octave: int, key: int) -> List[int]:
        """Encode pitchoffset command ($D9)."""
        combined = ((octave & 0xF) << 4) | (key & 0xF)
        return [0xD9, combined]

    @staticmethod
    def tempo(t: int) -> List[int]:
        """Encode tempo command ($DA). Takes 2 bytes as per Game Boy sound engine."""
        return [0xDA, (t >> 8) & 0xFF, t & 0xFF]

    @staticmethod
    def dutycycle(dc: int) -> List[int]:
        """Encode dutycycle command ($DB)."""
        return [0xDB, dc & 0x3]

    @staticmethod
    def intensity(i: int) -> List[int]:
        """Encode intensity command ($DC)."""
        return [0xDC, i & 0xFF]

    @staticmethod
    def soundinput(inp: int) -> List[int]:
        """Encode soundinput command ($DD)."""
        return [0xDD, inp & 0xFF]

    @staticmethod
    def sound_duty(a: int, b: int, c: int, d: int) -> List[int]:
        """Encode sound_duty command ($DE)."""
        return [0xDE, (a&3)|((b&3)<<2)|((c&3)<<4)|((d&3)<<6)]

    @staticmethod
    def togglesfx() -> List[int]:
        """Encode togglesfx command ($DF)."""
        return [0xDF]

    @staticmethod
    def slidepitchto(duration: int, octave: int, pitch: int) -> List[int]:
        """Encode slidepitchto command ($E0)."""
        combined = ((octave & 0xF) << 4) | (pitch & 0xF)
        return [0xE0, duration & 0xFF, combined]

    @staticmethod
    def vibrato(delay: int, extent: int) -> List[int]:
        """Encode vibrato command ($E1)."""
        return [0xE1, delay & 0xFF, extent & 0xFF]

    @staticmethod
    def togglenoise(n: int) -> List[int]:
        """Encode togglenoise command ($E3)."""
        return [0xE3, n & 0xFF]

    @staticmethod
    def panning(p: int) -> List[int]:
        """Encode panning command ($E4)."""
        return [0xE4, p & 0xFF]

    @staticmethod
    def volume(v: int) -> List[int]:
        """Encode volume command ($E5)."""
        return [0xE5, v & 0xFF]

    @staticmethod
    def tone(t: int) -> List[int]:
        """Encode tone command ($E6)."""
        return [0xE6, t & 0xFF]

    @staticmethod
    def tempo_relative(val: int) -> List[int]:
        """Encode tempo_relative command ($E9)."""
        return [0xE9, val & 0xFF]

    @staticmethod
    def newsong(id_: int) -> List[int]:
        """Encode newsong command ($EB)."""
        return [0xEB, id_ & 0xFF]

    @staticmethod
    def sfxpriorityon() -> List[int]:
        """Encode sfxpriorityon command ($EC)."""
        return [0xEC]

    @staticmethod
    def sfxpriorityoff() -> List[int]:
        """Encode sfxpriorityoff command ($ED)."""
        return [0xED]

    @staticmethod
    def setcondition(cond: int) -> List[int]:
        """Encode setcondition command ($FA)."""
        return [0xFA, cond & 0xFF]

    @staticmethod
    def stereopanning(tracks: int) -> List[int]:
        """Encode stereopanning command ($EF)."""
        return [0xEF, tracks & 0xFF]

    @staticmethod
    def unknownmusic0xe2(unknown: int) -> List[int]:
        """Encode unknownmusic0xe2 command ($E2)."""
        return [0xE2, unknown & 0xFF]

    @staticmethod
    def unknownmusic0xe7(unknown: int) -> List[int]:
        """Encode unknownmusic0xe7 command ($E7)."""
        return [0xE7, unknown & 0xFF]

    @staticmethod
    def unknownmusic0xe8(unknown: int) -> List[int]:
        """Encode unknownmusic0xe8 command ($E8)."""
        return [0xE8, unknown & 0xFF]

    @staticmethod
    def unknownmusic0xee(address: int) -> List[int]:
        """Encode unknownmusic0xee command ($EE)."""
        return [0xEE, (address >> 8) & 0xFF, address & 0xFF]

    @staticmethod
    def sfxtogglenoise(id_: int) -> List[int]:
        """Encode sfxtogglenoise command ($F0)."""
        return [0xF0, id_ & 0xFF]

    @staticmethod
    def music0xf1() -> List[int]:
        """Encode music0xf1 command ($F1)."""
        return [0xF1]

    @staticmethod
    def music0xf2() -> List[int]:
        """Encode music0xf2 command ($F2)."""
        return [0xF2]

    @staticmethod
    def music0xf3() -> List[int]:
        """Encode music0xf3 command ($F3)."""
        return [0xF3]

    @staticmethod
    def music0xf4() -> List[int]:
        """Encode music0xf4 command ($F4)."""
        return [0xF4]

    @staticmethod
    def music0xf5() -> List[int]:
        """Encode music0xf5 command ($F5)."""
        return [0xF5]

    @staticmethod
    def music0xf6() -> List[int]:
        """Encode music0xf6 command ($F6)."""
        return [0xF6]

    @staticmethod
    def music0xf7() -> List[int]:
        """Encode music0xf7 command ($F7)."""
        return [0xF7]

    @staticmethod
    def music0xf8() -> List[int]:
        """Encode music0xf8 command ($F8)."""
        return [0xF8]

    @staticmethod
    def unknownmusic0xf9() -> List[int]:
        """Encode unknownmusic0xf9 command ($F9)."""
        return [0xF9]


class ChannelParser:
    """Two-pass parser for one channel: records labels with byte offsets,
    then emits the final byte sequence with jumps resolved.
    """

    def __init__(self, lines: List[str], encoder: CommandEncoder):
        """Initialize the parser.

        Args:
            lines: List of assembly lines for this channel
            encoder: CommandEncoder instance
        """
        self.lines = lines
        self.encoder = encoder
        self.labels: Dict[str, int] = {}
        self.events: List[Tuple] = []  # ('label', name) or ('cmd', cmd, args, src)
        self.byte_offset = 0

    def first_pass(self):
        """Scan lines, record label->offset, and measure size of each command."""
        for line in self.lines:
            raw = line
            line = line.strip()
            if not line or line.startswith(';'):
                continue

            # Check for label
            lbl = re.match(r'^([A-Za-z_][\w]*):$', line)
            if lbl:
                name = lbl.group(1)
                self.labels[name] = self.byte_offset
                self.events.append(('label', name))
                continue

            # Parse command
            m = re.match(r'^([a-zA-Z_][\w]*)\s*(.*)$', line)
            if not m:
                continue
            cmd, arg_str = m.group(1), m.group(2)
            args = [a.strip() for a in arg_str.split(',')] if arg_str else []
            self.events.append(('cmd', cmd, args, raw))

            # Calculate byte size for this command
            self.byte_offset += self._get_command_size(cmd, args)

    def _get_command_size(self, cmd: str, args: List[str]) -> int:
        """Calculate the byte size of a command using the COMMAND_SIZES lookup table.

        Args:
            cmd: Command name
            args: Command arguments

        Returns:
            Number of bytes this command will generate
        """
        if cmd in self.encoder.COMMAND_SIZES:
            size = self.encoder.COMMAND_SIZES[cmd]
            if callable(size):
                return size(args)
            return size
        else:
            print(f"Warning: Unknown command '{cmd}'. Assuming 1 byte.")
            return 1

    def second_pass(self) -> List[int]:
        """Emit bytes, replacing label refs in control-flow commands with offsets.

        Returns:
            List of bytes representing the encoded channel data
        """
        out: List[int] = []

        for ev in self.events:
            if ev[0] == 'label':
                continue

            _, cmd, args, raw = ev

            if cmd in self.encoder.CONTROL_FLOW:
                opcode = self.encoder.CONTROL_FLOW[cmd]
                out.append(opcode)

                if cmd == 'endchannel':
                    continue
                elif cmd == 'loopchannel':
                    cnt = parse_num(args[0])
                    tgt = self.labels.get(args[1], 0)
                    out.extend([cnt & 0xFF, (tgt >> 8) & 0xFF, tgt & 0xFF])
                elif cmd == 'jumpif':
                    cond = parse_num(args[0])
                    tgt = self.labels.get(args[1], 0)
                    out.extend([cond & 0xFF, (tgt >> 8) & 0xFF, tgt & 0xFF])
                else:  # jumpchannel, callchannel, restartchannel
                    tgt = self.labels.get(args[0], 0)
                    out.extend([(tgt >> 8) & 0xFF, tgt & 0xFF])

            elif hasattr(self.encoder, cmd):
                # Convert arguments to appropriate types
                vals = []
                for arg in args:
                    if arg in NOTE_MAP:
                        vals.append(arg)
                    else:
                        vals.append(parse_num(arg))

                # Handle methods with optional parameters
                method = getattr(self.encoder, cmd)
                try:
                    result = method(*vals)
                    out.extend(result)
                except TypeError:
                    # Try with fewer args for optional parameters
                    if len(vals) > 1:
                        try:
                            result = method(*vals[:-1])
                            out.extend(result)
                        except TypeError:
                            raise ValueError(f"Cannot call {cmd} with args {vals} on '{raw}'")
                    else:
                        raise ValueError(f"Cannot call {cmd} with args {vals} on '{raw}'")
            else:
                raise ValueError(f"Unhandled cmd {cmd} on '{raw}'")

        return out


def split_channels(text: str) -> Dict[str, List[str]]:
    """Split file text into channels keyed by 'Ch1', 'Ch2', etc.

    Args:
        text: The full assembly file content

    Returns:
        Dictionary mapping channel names to lists of lines
    """
    res: Dict[str, List[str]] = {}
    cur = None

    for ln in text.splitlines():
        # Look for channel headers like "Music_Something_Ch1:"
        m = re.match(r'^Music_\w+_(Ch\d+):', ln)
        if m:
            cur = m.group(1)
            res[cur] = []
            continue
        if cur:
            res[cur].append(ln)

    return res


def format_c_array(name: str, data: List[int]) -> str:
    """Format a uint8_t array with line breaks every 12 bytes.

    Args:
        name: Name of the C array
        data: List of byte values

    Returns:
        Formatted C array declaration as string
    """
    lines = [f"static const uint8_t {name}[] = {{"]
    line = "    "

    for i, b in enumerate(data):
        line += f"0x{b:02X}, "
        if (i + 1) % 12 == 0:
            lines.append(line)
            line = "    "

    if line.strip():
        lines.append(line.rstrip(', '))

    lines.append('};')
    return '\n'.join(lines)


def main():
    """Main entry point for the music converter."""
    if len(sys.argv) != 2:
        print("Usage: convert_music.py file.asm")
        sys.exit(1)

    try:
        with open(sys.argv[1], 'r', encoding='utf-8') as f:
            txt = f.read()
    except FileNotFoundError:
        print(f"Error: Could not find file '{sys.argv[1]}'")
        sys.exit(1)
    except Exception as e:
        print(f"Error reading file: {e}")
        sys.exit(1)

    chans = split_channels(txt)
    if not chans:
        print("No channels found in input file")
        sys.exit(1)

    enc = CommandEncoder()

    for ch, lines in chans.items():
        try:
            p = ChannelParser(lines, enc)
            p.first_pass()
            data = p.second_pass()
            print(format_c_array(ch, data))
            print()
        except Exception as e:
            # print(f"Error processing channel {ch}: {e}")
            # sys.exit(1)
            pass


if __name__ == '__main__':
    main()
