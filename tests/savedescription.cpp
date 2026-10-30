#include "tiberiandawn/savedescription.h"
#include <cstring>
#include <cstdio>
int main() {
    const char* labels[] = {"(NOD) 11", "(GDI) An extremely long mission description that exceeds the edit buffer", "", "(broken label", "(NOD)    padded name   ", nullptr};
    const char* expected[] = {"11", "An extremely long mission description t", "", "(broken label", "padded name", ""};
    for (int i = 0; i < 6; ++i) {
        char guarded[128]; std::memset(guarded, 0x5a, sizeof(guarded));
        SaveDescription::Copy(guarded, 40, labels[i]);
        if (std::strcmp(guarded, expected[i])) { std::printf("Mismatch %d: %s\n", i, guarded); return 1; }
        for (int j = 40; j < 128; ++j) if (guarded[j] != 0x5a) return 2;
    }
    char one[2] = {'x','y'};
    SaveDescription::Copy(one, 1, "(NOD) long name");
    if (one[0] || one[1] != 'y') return 3;
    SaveDescription::Copy(one, 0, "ignored");
    return 0;
}
