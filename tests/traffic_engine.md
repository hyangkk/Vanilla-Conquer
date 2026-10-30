# Campaign local traffic recovery diagnostic

Run `traffic_engine.cpp` only as an isolated temporary mission-start entry point,
using SDL dummy video/audio, private assets/save copies and a private UserPath.
Restore the normal `missionstarts.cpp` immediately afterwards. Never launch the
live application or alter the user's saved games for this diagnostic.

The diagnostic builds a controlled one-cell road with a passing bay several cells
behind the point where opposing vehicles meet. It executes native Unit/Infantry AI
and the once-per-second traffic coordinator; it does not teleport units or relax
collision checks. Covered journeys:

- Opposing tanks back into an actually reachable parking bay, pass one another,
  resume their original targets, and reach opposite ends of the road. No vehicle
  cell overlap and no travel through concrete walls is allowed.
- Save/load during a real temporary yield restores both original missions/goals
  through native SuspendedMission/SuspendedNavCom/SuspendedTarCom fields.
- Friendly blockage near a destination no longer discards the move command;
  vehicles and infantry resume after the obstruction is removed.
- An idle friendly blocker clears the lane and retains its guard behavior.
- Explicit stop discards the suspended old destination. Later native scattering
  or a new yield may reposition an idle object, but cannot revive that cancelled
  destination.
- Scripted teams/triggers, loaners, radio/tether/transport/repair/harvester/MCV
  states, enemy obstruction and skirmish retain their existing semantics.

The temporary MOVE override is unique to traffic; existing engine obstacle attack
overrides use ATTACK. New explicit destinations and non-MOVE queued orders clear
traffic's suspended intent. Observations are runtime-only and reset on loading;
native saved fields carry the order that must survive a yield.

Traffic observations run once per game second. Only stalled actors perform a
local six-cell flood search for clear parking space, with deterministic ID-based
right of way and fallback to the other actor if the first cannot reach a bay.
Same-direction columns queue rather than shoving one another. Repeated path
failures retain intent only when a friendly obstruction is nearby; permanently
impassable terrain and enemy obstacles still use native failure/attack behavior.

The negative control disables both the coordinator and friendly-blockage retention
in `campaigntraffic.cpp` alone. It must fail the corridor, saved-yield and wake-up
journeys. Restore that file, rerun the positive diagnostic, then build the release
entry point before installation. UI appearance and long-session frame rate are
outside this engine diagnostic. A completely sealed one-cell passage with no
reachable passing space cannot physically support vehicles passing each other.
