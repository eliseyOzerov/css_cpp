#include <cmath>

#include "../Css/CssColor.h"
#include "../Css/CssStyleSheet.h"
#include "Testing.h"

using namespace editor;

namespace {

/// Whether two colours agree to within 1/1000 in every channel.
bool sameColor(std::optional<CssColor> color, CssColor expected) {
    if (!color) return false;
    const auto near = [](float first, float second) { return std::fabs(first - second) < 0.001f; };
    return near(color->red, expected.red) && near(color->green, expected.green) && near(color->blue, expected.blue)
           && near(color->alpha, expected.alpha);
}

} // namespace

TEST(cssColorReadsHexInEveryLength) {
    // #f80 is #ff8800: 0x88 = 136, 136 / 255 = 0.5333.
    EXPECT(sameColor(cssColor("#f80"), CssColor{1, 0.5333f, 0, 1}));
    EXPECT(sameColor(cssColor("#F808"), CssColor{1, 0.5333f, 0, 0.5333f}));
    EXPECT(sameColor(cssColor("#ff8800"), CssColor{1, 0.5333f, 0, 1}));
    EXPECT(sameColor(cssColor("#ff880080"), CssColor{1, 0.5333f, 0, 128.0f / 255}));
    EXPECT(!cssColor("#ff880"));
    EXPECT(!cssColor("#ggg"));
}

TEST(cssColorReadsFunctionsNamesAndKeywords) {
    EXPECT(sameColor(cssColor("rgb(255, 0, 51)"), CssColor{1, 0, 0.2f, 1}));
    EXPECT(sameColor(cssColor("rgba(0,0,0,0.5)"), CssColor{0, 0, 0, 0.5f}));
    EXPECT(sameColor(cssColor("rgb(100%, 50%, 0%)"), CssColor{1, 0.5f, 0, 1}));
    EXPECT(sameColor(cssColor("rgb(255 0 0 / 25%)"), CssColor{1, 0, 0, 0.25f}));
    EXPECT(sameColor(cssColor("rgb(300, -5, 0)"), CssColor{1, 0, 0, 1}));
    // hsl(120, 100%, 25%): C = (1 − |0.5 − 1|)·1 = 0.5, sextant 2 puts C in green, m = 0.25 − 0.25 = 0.
    EXPECT(sameColor(cssColor("hsl(120, 100%, 25%)"), CssColor{0, 0.5f, 0, 1}));
    // hsl(0, 100%, 50%) is pure red; hsl(-120deg …) wraps to 240, pure blue.
    EXPECT(sameColor(cssColor("hsla(0, 100%, 50%, 0.5)"), CssColor{1, 0, 0, 0.5f}));
    EXPECT(sameColor(cssColor("hsl(-120deg, 100%, 50%)"), CssColor{0, 0, 1, 1}));
    EXPECT(sameColor(cssColor("CornflowerBlue"), CssColor{100.0f / 255, 149.0f / 255, 237.0f / 255, 1}));
    EXPECT(sameColor(cssColor(" yellowgreen "), CssColor{154.0f / 255, 205.0f / 255, 50.0f / 255, 1}));
    EXPECT(sameColor(cssColor("transparent"), CssColor{0, 0, 0, 0}));
    EXPECT(!cssColor("notacolour"));
    EXPECT(!cssColor("rgb(1, 2)"));
}

TEST(cssDeclarationsSplitsAnInlineStyle) {
    const auto declarations = cssDeclarations("fill: red; /* note */ Stroke-Width:2 !important;;bad");
    EXPECT_EQUAL(declarations.size(), size_t(2));
    EXPECT_EQUAL(declarations[0].first, std::string("fill"));
    EXPECT_EQUAL(declarations[0].second, std::string("red"));
    EXPECT_EQUAL(declarations[1].first, std::string("stroke-width"));
    EXPECT_EQUAL(declarations[1].second, std::string("2"));
}

TEST(cssStyleSheetAppliesRulesBySpecificityThenOrder) {
    CssStyleSheet sheet;
    sheet.read(R"(
        #logo { fill: blue }
        .cls-1, .cls-2 { fill: red; stroke: black }
        path { fill: green }
        @media print { path { fill: white } }
        path.cls-1 { stroke: gray }
        g > path { fill: purple }
        .cls-1 { fill: orange }
    )");
    // path.cls-1#logo matches (types, classes, ids): path (0,0,1), .cls-1 (0,1,0) twice in order,
    // path.cls-1 (0,1,1), #logo (1,0,0). The descendant rule and the @media block match nothing.
    const auto declarations = sheet.declarationsFor("path", "logo", "cls-1 extra");
    std::vector<std::string> fills;
    for (const auto& [name, value] : declarations) {
        if (name == "fill") fills.push_back(value);
    }
    EXPECT(fills == (std::vector<std::string>{"green", "red", "orange", "blue"}));
    EXPECT_EQUAL(declarations.back().second, std::string("blue"));
    EXPECT(sheet.declarationsFor("rect", "", "").empty());
    EXPECT_EQUAL(sheet.declarationsFor("rect", "", "cls-2").size(), size_t(2));
}

TEST(cssAlphaValueReadsNumbersAndPercentages) {
    EXPECT_EQUAL(*cssAlphaValue("0.5"), 0.5f);
    EXPECT_EQUAL(*cssAlphaValue(" 25% "), 0.25f);
    EXPECT_EQUAL(*cssAlphaValue("2"), 1.0f);
    EXPECT(!cssAlphaValue("half"));
}
