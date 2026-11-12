# Global working rules

I want the best result for the fewest tokens: quality first, then cost. Re-reading, over-polishing and waiting in loops add cost without adding quality, so treat them as waste.

## Context

Long contexts degrade your work and cost me money, so aim to stay under ~200k tokens.

- Locate before you read: grep/glob to the spot, then read just those lines. Whole files are fine when small or fully needed.
- Keep command output short — `head`/`tail`, quiet or summary flags, or redirect long logs to a file and read the failing part. Build output, minified code, lockfiles, big JSON and transcripts are almost never worth pulling in whole.
- Facts already established in the conversation (including files you just wrote) can be trusted without re-reading.
- Past ~150k tokens, finish the current step and suggest a `/compact`, noting what must survive.
- When compacting, preserve the modified-file list, the test/build commands in use, open decisions, and the current "done" definition.

## Scope

I review everything you change, so unrequested work costs me review time.

- Decide what "done" means before starting and stop there. Mention worthwhile extras in one line rather than doing them.
- Verify in proportion: targeted tests while iterating, the full suite once at the end.
- Before any disk cleanup, check active sessions and open files, and leave out what they use, because moving caches or build folders mid-run can break them.
- Pick an approach and commit to it; only lay out alternatives when the choice is genuinely mine.

## When things stall

- Wait for long commands via background runs or one blocking call with a timeout — sleep-polling burns turns.
- If the same fix fails twice, step back and rethink the cause or report the blocker; variations on a failed fix rarely work.
- Shrinking returns from repeated similar steps mean it's time to wrap up.

## Learning from corrections

When I correct a mistake that a future session could repeat, propose a one-line addition to the relevant project's CLAUDE.md (or a skill's Gotchas section) capturing the lesson and why. Show it and let me approve before writing.

## Subagents

Subagents multiply token spend, so I opt in per request: use agents or workflows only when I ask for them in that request.

- Opus advises, Sonnet builds, Haiku does side tasks. `opus-xhigh` plans and judges: architecture, specs, prompts, hard debugging, every review. `sonnet-high` is the default builder for features, UI, fixes and applying review findings, working from a plan. `opus-high` builds only the critical paths (security, credentials, concurrency, data integrity, migrations) and takes any fix a Sonnet builder failed twice. `sonnet-medium` does mechanical edits and ship steps; `haiku-low` and `Explore` do lookups, sweeps, summaries, git chores and bookkeeping. Don't use `max` effort or Fable.
- Each agent prompt should carry: scope and file ownership (no two agents on the same files), a "done" definition, a tool-call budget, a request for a concise report with a "Not done" list, and these same rules.
- Agents share this machine, so stagger full test suites and heavy jobs rather than running them at once in the same tree.
- Check on long runners and send a wrap-up as soon as the work turns into polish or loops.

## Communication

Be concise: lead with the result, skip recaps of what I've already seen, and don't narrate routine steps.
