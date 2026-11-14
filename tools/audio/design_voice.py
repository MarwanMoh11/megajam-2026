"""Voice Design candidates for a character, and keeping the one Marwan picks.

  python tools/audio/design_voice.py CHIEF               previews into C:\Jam\Audio\voice_design\CHIEF_<n>.mp3
  python tools/audio/design_voice.py CHIEF --keep <n>    saves candidate n as a voice and prints its voice_id
                                                         (put it in manifest.VOICES, then regenerate the lines)
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import el_api  # noqa: E402

DIR = r"C:\Jam\Audio\voice_design"

BRIEFS = {
    "CHIEF": dict(
        name="Print It - Chief",
        description=("A gruff, older, working-class American man in his late fifties, head of security at a 1950s "
                     "auction house, a noir doorman type. Low, gravelly smoker's voice, unhurried and contemptuous, "
                     "clipped New York vowels, tired of every trick. Close, dry recording, no reverb. Not an "
                     "announcer, not a narrator."),
        text=("You. That carpet is for invited guests. No, I don't care who your tailor is, pal. Off the carpet, "
              "and back in line with the rest of them."),
    ),
    "VANCE": dict(
        name="Print It - Vance",
        description=("A smooth, older English auctioneer and showman in his sixties, at the mic of a black-tie 1950s "
                     "auction. Velvet, theatrical, silky charm with a hint of menace, savouring every word, master of "
                     "the room. Warm baritone. Close, dry recording, no reverb. Not an announcer, not a narrator."),
        text=("Ladies and gentlemen: the Director's Cut. Lot forty-one has never been screened. One reel, one "
              "print, and tonight it finds a home."),
    ),
    "DOORMAN": dict(
        name="Print It - Doorman",
        description=("A tall, stiff, middle-aged doorman on the red carpet of a grand 1950s auction house. Polite "
                     "on the surface, icily disdainful underneath, deep and measured, old-fashioned East Coast "
                     "accent. Close, dry recording, no reverb. Not an announcer, not a narrator."),
        text=("Invitation, sir? I'm afraid not. Guests only on the carpet, sir. The queue is to your left, behind "
              "the gentleman in the grey coat."),
    ),
    "BROADCAST": dict(
        name="Print It - Broadcast",
        description=("A harried 1950s live-television producer in his forties, talking to his camera crew on a "
                     "talkback headset. Fast, clipped, urgent, mid-Atlantic accent, slightly nasal, under pressure. "
                     "Close, dry recording. Not an announcer, not a narrator."),
        text=("Who is the gentleman in white? Camera one, find me that gentleman in white. Stay on the man in "
              "white!"),
    ),
    "SECURITY": dict(
        name="Print It - Security",
        description=("A young, burly security guard in his twenties at a 1950s auction house, Brooklyn accent, "
                     "breathless and shouting, rough and excited, a little out of his depth. Close, dry recording, "
                     "no reverb. Not an announcer, not a narrator."),
        text=("Nobody leaves! Seal the doors at both ends of the hall. There! The man in white! Stop that man, he "
              "has the reel!"),
    ),
}

MAX_CANDIDATES = 2   # Voice Design returns three previews; Marwan asked for two a character after the chief


def main(argv):
    who = argv[0]
    brief = BRIEFS[who]
    os.makedirs(DIR, exist_ok=True)
    state_path = os.path.join(DIR, who + ".json")
    if "--keep" in argv:
        n = int(argv[argv.index("--keep") + 1])
        state = json.load(open(state_path))
        vid = el_api.save_designed_voice(brief["name"], brief["description"], state[str(n)])
        print(f"{who} candidate {n} saved as voice_id {vid}")
        return
    state = {}
    previews = el_api.design_voice(brief["description"], brief["text"])
    for i, (gid, audio) in enumerate(previews[: (3 if who == "CHIEF" else MAX_CANDIDATES)], start=1):
        path = os.path.join(DIR, f"{who}_{i}.mp3")
        open(path, "wb").write(audio)
        state[str(i)] = gid
        print(f"candidate {i}: {path}")
    json.dump(state, open(state_path, "w"), indent=1)


if __name__ == "__main__":
    main(sys.argv[1:])
