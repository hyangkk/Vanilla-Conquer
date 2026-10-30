#ifndef TD_SAVE_DESCRIPTION_H
#define TD_SAVE_DESCRIPTION_H
#include <cstddef>
#include <cstring>
#include <cctype>
namespace SaveDescription {
// List labels include a faction prefix and may exceed the editable save name.
inline void Copy(char* destination, std::size_t capacity, const char* label)
{
    if (!capacity) return;
    if (!label) label = "";
    if (*label == '(') {
        const char* closing = std::strchr(label, ')');
        if (closing) {
            label = closing + 1;
            while (*label && std::isspace(static_cast<unsigned char>(*label))) ++label;
        }
    }
    std::size_t count = std::strlen(label);
    if (count >= capacity) count = capacity - 1;
    std::memmove(destination, label, count);
    while (count && std::isspace(static_cast<unsigned char>(destination[count - 1]))) --count;
    destination[count] = '\0';
}
}
#endif
