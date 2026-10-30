# Retired campaign reinforcements

`campaignwaves_engine.cpp` now verifies that the removed timed/escalating reinforcements never spawn, including a six-hour simulated wait and loading a save containing a pending legacy wave. Existing troops can still receive attack orders.

The economy, native factory release, triple-production, resource yield and player attack-bike weapon regressions are in `enemyeconomybike_engine.cpp`. These probes run with dummy SDL drivers and isolated saves; they never launch or modify the live user game.
