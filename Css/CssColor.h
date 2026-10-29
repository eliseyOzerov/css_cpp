// CSS colours (CSS Color Module Level 3): the hex forms, rgb(), hsl(), `transparent` and the named
// colours, as SVG, HTML and every CSS-styled format write them. Read in the shape of the SwiftSVG
// package's colour reader, rewritten to these rules.
#pragma once

#include <optional>
#include <string_view>

namespace editor {

/// A colour as CSS gives it: red, green, blue and alpha, each from 0 to 1, the channels
/// sRGB-encoded as written (no conversion to linear light).
///
/// Use when: holding a colour read from CSS before turning it into a host's own colour type
/// Tags: css colour, rgba, color value
/// Node: none (file format)
struct CssColor {
    float red = 0;
    float green = 0;
    float blue = 0;
    float alpha = 1;

    bool operator==(const CssColor&) const = default;
};

/// The colour a CSS colour value spells, or none when it is not one: `#rgb`, `#rgba`, `#rrggbb`,
/// `#rrggbbaa`, `rgb()` / `rgba()` with numbers 0–255 or percentages (commas, or spaces with an
/// optional `/ alpha`), `hsl()` / `hsla()`, `transparent`, and the 147 CSS named colours, in any
/// letter case.
///
/// How: `hsl(h, s, l)` is converted with chroma C = (1 − |2l − 1|)·s, X = C·(1 − |(h / 60) mod 2 − 1|)
/// and m = l − C/2: the sextant of h picks which of red, green and blue gets C, X and 0, and m is
/// added to each (CSS Color Module Level 3, section 4.2.4). Channels outside their range are clamped.
///
///     cssColor("#f80") = (1, 0.533, 0, 1);  cssColor("hsl(120, 100%, 25%)") = (0, 0.5, 0, 1)
///
/// Use when: reading an SVG `fill`, `stroke`, `stop-color` or `color`; a colour in any CSS
/// Tags: css colour, parse color, named colour, rgb, hsl, hex colour
/// Node: none (file format)
std::optional<CssColor> cssColor(std::string_view text);

/// A CSS `<alpha-value>`: a number, or a percentage as a hundredth, held in 0…1; none for anything else.
///
///     cssAlphaValue("0.5") = 0.5;  cssAlphaValue("25%") = 0.25;  cssAlphaValue("2") = 1
///
/// Use when: reading `opacity`, `fill-opacity` or the alpha of `rgba()`
/// Tags: alpha value, opacity, percentage
/// Node: none (file format)
std::optional<float> cssAlphaValue(std::string_view text);

} // namespace editor
