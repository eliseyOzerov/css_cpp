#include "CssColor.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <string>
#include <cstdlib>
#include <vector>

namespace editor {

namespace {

/// Whether `character` is CSS whitespace.
bool isSpace(char character) { return character == ' ' || character == '\t' || character == '\n' || character == '\r' || character == '\f'; }

/// `text` without the whitespace at either end.
std::string_view trimmed(std::string_view text) {
    while (!text.empty() && isSpace(text.front())) text.remove_prefix(1);
    while (!text.empty() && isSpace(text.back())) text.remove_suffix(1);
    return text;
}

/// `text` in lower case (ASCII only, as CSS keywords are).
std::string lowercased(std::string_view text) {
    std::string result(text);
    for (char& character : result) {
        if (character >= 'A' && character <= 'Z') character = static_cast<char>(character - 'A' + 'a');
    }
    return result;
}

/// The value of one hexadecimal digit, or −1 when it is not one.
int hexDigitValue(char character) {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

/// `text` split at each `separator`, every piece trimmed.
std::vector<std::string_view> splitTrimmed(std::string_view text, char separator) {
    std::vector<std::string_view> pieces;
    size_t start = 0;
    while (true) {
        const size_t end = text.find(separator, start);
        pieces.push_back(trimmed(text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start)));
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return pieces;
}

/// The number `text` holds in full, or none. `strtof` rather than `std::from_chars`, whose float
/// form Apple's library has only from macOS and iOS 26; CSS numbers never need a locale, and the
/// C locale every program starts in reads them as written.
std::optional<float> wholeNumber(std::string_view text) {
    if (text.empty() || std::isspace(static_cast<unsigned char>(text.front()))) return std::nullopt;
    const std::string copy(text);
    char* end = nullptr;
    const float value = std::strtof(copy.c_str(), &end);
    if (end != copy.c_str() + copy.size()) return std::nullopt;
    return value;
}

/// `value` held in `low`…`high`.
float clamped(float value, float low, float high) { return std::min(std::max(value, low), high); }

/// A colour from 0xRRGGBB, opaque.
CssColor cssColorFromRGB(uint32_t rgb) {
    return CssColor{static_cast<float>((rgb >> 16) & 0xff) / 255, static_cast<float>((rgb >> 8) & 0xff) / 255,
                 static_cast<float>(rgb & 0xff) / 255, 1};
}

/// The CSS named colours (CSS Color Module Level 3, section 4.3), sorted by name for a binary search.
struct NamedColor {
    std::string_view name;
    uint32_t rgb;
};

constexpr NamedColor namedColors[] = {
    {"aliceblue", 0xf0f8ff}, {"antiquewhite", 0xfaebd7}, {"aqua", 0x00ffff}, {"aquamarine", 0x7fffd4},
    {"azure", 0xf0ffff}, {"beige", 0xf5f5dc}, {"bisque", 0xffe4c4}, {"black", 0x000000},
    {"blanchedalmond", 0xffebcd}, {"blue", 0x0000ff}, {"blueviolet", 0x8a2be2}, {"brown", 0xa52a2a},
    {"burlywood", 0xdeb887}, {"cadetblue", 0x5f9ea0}, {"chartreuse", 0x7fff00}, {"chocolate", 0xd2691e},
    {"coral", 0xff7f50}, {"cornflowerblue", 0x6495ed}, {"cornsilk", 0xfff8dc}, {"crimson", 0xdc143c},
    {"cyan", 0x00ffff}, {"darkblue", 0x00008b}, {"darkcyan", 0x008b8b}, {"darkgoldenrod", 0xb8860b},
    {"darkgray", 0xa9a9a9}, {"darkgreen", 0x006400}, {"darkgrey", 0xa9a9a9}, {"darkkhaki", 0xbdb76b},
    {"darkmagenta", 0x8b008b}, {"darkolivegreen", 0x556b2f}, {"darkorange", 0xff8c00}, {"darkorchid", 0x9932cc},
    {"darkred", 0x8b0000}, {"darksalmon", 0xe9967a}, {"darkseagreen", 0x8fbc8f}, {"darkslateblue", 0x483d8b},
    {"darkslategray", 0x2f4f4f}, {"darkslategrey", 0x2f4f4f}, {"darkturquoise", 0x00ced1}, {"darkviolet", 0x9400d3},
    {"deeppink", 0xff1493}, {"deepskyblue", 0x00bfff}, {"dimgray", 0x696969}, {"dimgrey", 0x696969},
    {"dodgerblue", 0x1e90ff}, {"firebrick", 0xb22222}, {"floralwhite", 0xfffaf0}, {"forestgreen", 0x228b22},
    {"fuchsia", 0xff00ff}, {"gainsboro", 0xdcdcdc}, {"ghostwhite", 0xf8f8ff}, {"gold", 0xffd700},
    {"goldenrod", 0xdaa520}, {"gray", 0x808080}, {"green", 0x008000}, {"greenyellow", 0xadff2f},
    {"grey", 0x808080}, {"honeydew", 0xf0fff0}, {"hotpink", 0xff69b4}, {"indianred", 0xcd5c5c},
    {"indigo", 0x4b0082}, {"ivory", 0xfffff0}, {"khaki", 0xf0e68c}, {"lavender", 0xe6e6fa},
    {"lavenderblush", 0xfff0f5}, {"lawngreen", 0x7cfc00}, {"lemonchiffon", 0xfffacd}, {"lightblue", 0xadd8e6},
    {"lightcoral", 0xf08080}, {"lightcyan", 0xe0ffff}, {"lightgoldenrodyellow", 0xfafad2}, {"lightgray", 0xd3d3d3},
    {"lightgreen", 0x90ee90}, {"lightgrey", 0xd3d3d3}, {"lightpink", 0xffb6c1}, {"lightsalmon", 0xffa07a},
    {"lightseagreen", 0x20b2aa}, {"lightskyblue", 0x87cefa}, {"lightslategray", 0x778899}, {"lightslategrey", 0x778899},
    {"lightsteelblue", 0xb0c4de}, {"lightyellow", 0xffffe0}, {"lime", 0x00ff00}, {"limegreen", 0x32cd32},
    {"linen", 0xfaf0e6}, {"magenta", 0xff00ff}, {"maroon", 0x800000}, {"mediumaquamarine", 0x66cdaa},
    {"mediumblue", 0x0000cd}, {"mediumorchid", 0xba55d3}, {"mediumpurple", 0x9370db}, {"mediumseagreen", 0x3cb371},
    {"mediumslateblue", 0x7b68ee}, {"mediumspringgreen", 0x00fa9a}, {"mediumturquoise", 0x48d1cc}, {"mediumvioletred", 0xc71585},
    {"midnightblue", 0x191970}, {"mintcream", 0xf5fffa}, {"mistyrose", 0xffe4e1}, {"moccasin", 0xffe4b5},
    {"navajowhite", 0xffdead}, {"navy", 0x000080}, {"oldlace", 0xfdf5e6}, {"olive", 0x808000},
    {"olivedrab", 0x6b8e23}, {"orange", 0xffa500}, {"orangered", 0xff4500}, {"orchid", 0xda70d6},
    {"palegoldenrod", 0xeee8aa}, {"palegreen", 0x98fb98}, {"paleturquoise", 0xafeeee}, {"palevioletred", 0xdb7093},
    {"papayawhip", 0xffefd5}, {"peachpuff", 0xffdab9}, {"peru", 0xcd853f}, {"pink", 0xffc0cb},
    {"plum", 0xdda0dd}, {"powderblue", 0xb0e0e6}, {"purple", 0x800080}, {"rebeccapurple", 0x663399},
    {"red", 0xff0000}, {"rosybrown", 0xbc8f8f}, {"royalblue", 0x4169e1}, {"saddlebrown", 0x8b4513},
    {"salmon", 0xfa8072}, {"sandybrown", 0xf4a460}, {"seagreen", 0x2e8b57}, {"seashell", 0xfff5ee},
    {"sienna", 0xa0522d}, {"silver", 0xc0c0c0}, {"skyblue", 0x87ceeb}, {"slateblue", 0x6a5acd},
    {"slategray", 0x708090}, {"slategrey", 0x708090}, {"snow", 0xfffafa}, {"springgreen", 0x00ff7f},
    {"steelblue", 0x4682b4}, {"tan", 0xd2b48c}, {"teal", 0x008080}, {"thistle", 0xd8bfd8},
    {"tomato", 0xff6347}, {"turquoise", 0x40e0d0}, {"violet", 0xee82ee}, {"wheat", 0xf5deb3},
    {"white", 0xffffff}, {"whitesmoke", 0xf5f5f5}, {"yellow", 0xffff00}, {"yellowgreen", 0x9acd32},
};

/// The named colour `name` (already lower case), or none.
std::optional<CssColor> namedColor(std::string_view name) {
    const auto found = std::lower_bound(std::begin(namedColors), std::end(namedColors), name,
                                        [](const NamedColor& entry, std::string_view key) { return entry.name < key; });
    if (found == std::end(namedColors) || found->name != name) return std::nullopt;
    return cssColorFromRGB(found->rgb);
}

/// The colour of `#` followed by 3, 4, 6 or 8 hex digits, or none.
std::optional<CssColor> hexColor(std::string_view digits) {
    if (digits.size() != 3 && digits.size() != 4 && digits.size() != 6 && digits.size() != 8) return std::nullopt;
    std::vector<int> values;
    for (char character : digits) {
        const int value = hexDigitValue(character);
        if (value < 0) return std::nullopt;
        values.push_back(value);
    }
    // In the short forms each digit stands for itself twice: #f80 is #ff8800, and 0xff = 15·17.
    const bool isShort = digits.size() <= 4;
    const auto channel = [&](int index) {
        const int byte = isShort ? values[index] * 17 : values[2 * index] * 16 + values[2 * index + 1];
        return static_cast<float>(byte) / 255;
    };
    const bool hasAlpha = digits.size() == 4 || digits.size() == 8;
    return CssColor{channel(0), channel(1), channel(2), hasAlpha ? channel(3) : 1};
}

/// The arguments inside `name(…)`, split at commas or, in the newer syntax, at spaces with an
/// optional `/ alpha`; none when `text` is not a call of `name`.
std::optional<std::vector<std::string_view>> functionArguments(std::string_view text, std::string_view name) {
    if (text.size() < name.size() + 2 || lowercased(text.substr(0, name.size())) != name) return std::nullopt;
    std::string_view rest = trimmed(text.substr(name.size()));
    if (rest.size() < 2 || rest.front() != '(' || rest.back() != ')') return std::nullopt;
    rest = rest.substr(1, rest.size() - 2);
    std::vector<std::string_view> arguments;
    if (rest.find(',') != std::string_view::npos) return splitTrimmed(rest, ',');
    size_t position = 0;
    while (position < rest.size()) {
        while (position < rest.size() && (isSpace(rest[position]) || rest[position] == '/')) position += 1;
        const size_t start = position;
        while (position < rest.size() && !isSpace(rest[position]) && rest[position] != '/') position += 1;
        if (position > start) arguments.push_back(rest.substr(start, position - start));
    }
    return arguments;
}

/// A percentage `text` as a fraction, or none when it is not one.
std::optional<float> percentage(std::string_view text) {
    if (text.empty() || text.back() != '%') return std::nullopt;
    const std::optional<float> number = wholeNumber(text.substr(0, text.size() - 1));
    if (!number) return std::nullopt;
    return *number / 100;
}

/// One `rgb()` channel as 0…1: a number out of 255 or a percentage.
std::optional<float> rgbChannel(std::string_view text) {
    if (const std::optional<float> fraction = percentage(text)) return clamped(*fraction, 0, 1);
    const std::optional<float> number = wholeNumber(text);
    if (!number) return std::nullopt;
    return clamped(*number / 255, 0, 1);
}

/// An alpha argument as 0…1: a number or a percentage.
std::optional<float> alphaChannel(std::string_view text) {
    if (const std::optional<float> fraction = percentage(text)) return clamped(*fraction, 0, 1);
    const std::optional<float> number = wholeNumber(text);
    if (!number) return std::nullopt;
    return clamped(*number, 0, 1);
}

/// The colour of `rgb(…)` / `rgba(…)`, or none.
std::optional<CssColor> rgbColor(const std::vector<std::string_view>& arguments) {
    if (arguments.size() != 3 && arguments.size() != 4) return std::nullopt;
    const std::optional<float> red = rgbChannel(arguments[0]), green = rgbChannel(arguments[1]), blue = rgbChannel(arguments[2]);
    const std::optional<float> alpha = arguments.size() == 4 ? alphaChannel(arguments[3]) : std::optional<float>(1);
    if (!red || !green || !blue || !alpha) return std::nullopt;
    return CssColor{*red, *green, *blue, *alpha};
}

/// The colour of hue (degrees), saturation and lightness (0…1); see `cssColor` for the formula.
CssColor cssColorFromHSL(float hue, float saturation, float lightness, float alpha) {
    const float wrappedHue = std::fmod(std::fmod(hue, 360.0f) + 360.0f, 360.0f);
    const float chroma = (1 - std::fabs(2 * lightness - 1)) * saturation;
    const float sextant = wrappedHue / 60;
    const float second = chroma * (1 - std::fabs(std::fmod(sextant, 2.0f) - 1));
    const float match = lightness - chroma / 2;
    float red = 0, green = 0, blue = 0;
    switch (static_cast<int>(sextant)) {
    case 0: red = chroma, green = second; break;
    case 1: red = second, green = chroma; break;
    case 2: green = chroma, blue = second; break;
    case 3: green = second, blue = chroma; break;
    case 4: red = second, blue = chroma; break;
    default: red = chroma, blue = second; break;
    }
    return CssColor{red + match, green + match, blue + match, alpha};
}

/// The colour of `hsl(…)` / `hsla(…)`, or none. The hue may carry `deg`.
std::optional<CssColor> hslColor(const std::vector<std::string_view>& arguments) {
    if (arguments.size() != 3 && arguments.size() != 4) return std::nullopt;
    std::string_view hueText = arguments[0];
    if (hueText.size() > 3 && lowercased(hueText.substr(hueText.size() - 3)) == "deg") hueText.remove_suffix(3);
    const std::optional<float> hue = wholeNumber(hueText);
    const std::optional<float> saturation = percentage(arguments[1]), lightness = percentage(arguments[2]);
    const std::optional<float> alpha = arguments.size() == 4 ? alphaChannel(arguments[3]) : std::optional<float>(1);
    if (!hue || !saturation || !lightness || !alpha) return std::nullopt;
    return cssColorFromHSL(*hue, clamped(*saturation, 0, 1), clamped(*lightness, 0, 1), *alpha);
}

} // namespace

std::optional<CssColor> cssColor(std::string_view text) {
    text = trimmed(text);
    if (text.empty()) return std::nullopt;
    if (text.front() == '#') return hexColor(text.substr(1));
    const std::string lower = lowercased(text);
    if (lower == "transparent") return CssColor{0, 0, 0, 0};
    for (std::string_view name : {"rgba", "rgb"}) {
        if (const auto arguments = functionArguments(text, name)) return rgbColor(*arguments);
    }
    for (std::string_view name : {"hsla", "hsl"}) {
        if (const auto arguments = functionArguments(text, name)) return hslColor(*arguments);
    }
    return namedColor(lower);
}

std::optional<float> cssAlphaValue(std::string_view text) { return alphaChannel(trimmed(text)); }

} // namespace editor
