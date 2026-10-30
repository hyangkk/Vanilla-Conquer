# Crew escape at full infantry capacity

The October 30 crash report for the previously installed game showed a null `this`
write in Abstract/Object/Infantry constructors, called from Unit.Take_Damage via
an ion explosion, with an infantry-pool count of 500 in the registers.

The crew branch for an unarmed vehicle creates an INFANTRY_C1 and immediately
writes IsTechnician before checking allocation success. Although allocation is
noexcept and the build uses -fcheck-new, that unconditional dereference permits
optimization under a non-null assumption. A full pool therefore reproduces a
SIGSEGV in the constructor path. Guard the IsTechnician write with `if (i)`; keep
normal crew creation and the following existing null handling unchanged.

The private native diagnostic fills the infantry heap, confirms a separate new
expression returns null safely, then destroys crewed unarmed harvesters using
fixed random seeds. The original source crashes at seed three. The guarded source
survives all twenty destructions without adding infantry. After freeing one slot,
normal crew escape again produces one live technician. No capacity increase or
loss of existing infantry is required.

Use dummy SDL and a private user directory; restore missionstarts.cpp before the
release build. User savegame.026 is preserved separately before UI restore attempts.
