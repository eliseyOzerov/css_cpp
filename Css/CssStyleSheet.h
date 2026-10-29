// CSS declarations and style sheets of simple selectors, as an SVG `<style>` element or a `style`
// attribute holds them. The reader follows the structure of the SwiftCSS package's parser
// (comments dropped, rules read prelude by prelude, at-rules skipped), rewritten to these rules.
#pragma once

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace editor {

/// One declaration, `name: value`: the name in lower case, the value trimmed.
using CssDeclaration = std::pair<std::string, std::string>;

/// The declarations of a CSS block such as an inline `style` attribute: `name: value` pairs
/// separated by semicolons, names in lower case, values trimmed; comments are dropped, a piece with
/// no colon is skipped, and `!important` is read and dropped.
///
///     "fill: red; stroke-width:2" → [("fill", "red"), ("stroke-width", "2")]
///
/// Use when: reading an SVG element's `style` attribute; the body of a stylesheet rule
/// Tags: inline style, css declarations, style attribute
/// Node: none (file format)
std::vector<CssDeclaration> cssDeclarations(std::string_view text);

/// A style sheet: rules of simple selectors and the declarations they set.
///
/// Conventions: a selector is a comma list of compound selectors made of an optional type (`path`
/// or `*`), `.class` and `#id` parts. A selector with a combinator (descendant, `>`, `+`, `~`), an
/// attribute test or a pseudo-class matches nothing. At-rules are skipped (an `@media` block's rules
/// included). Rules apply in order of specificity (ids, then classes, then types; CSS Selectors
/// Level 3, section 9), and in source order among equals; `!important` is not honoured.
///
/// Use when: reading the stylesheet of an SVG exported by Illustrator or Figma
/// Tags: css, stylesheet, selector, specificity, style element, class
/// Node: none (file format)
struct CssStyleSheet {
    struct Selector {
        std::string type;
        std::string id;
        std::vector<std::string> classes;
        /// Ids, classes and types, compared in that order.
        std::array<int, 3> specificity{};
    };

    struct Rule {
        Selector selector;
        std::vector<CssDeclaration> declarations;
        int order = 0;
    };

    std::vector<Rule> rules;

    /// Adds the rules the CSS text `css` holds after any already read.
    ///
    /// Use when: reading the text of an SVG `<style>` element
    /// Tags: parse css, stylesheet parser, css rules
    /// Node: none (file format)
    void read(std::string_view css);

    /// The declarations of every rule matching an element named `type` with the given `id` and
    /// space-separated `classes`, in the order they apply (the last one wins).
    ///
    /// Use when: running the cascade for one element
    /// Tags: matching rules, selector matching, cascade order
    /// Node: none (file format)
    std::vector<CssDeclaration> declarationsFor(std::string_view type, std::string_view id, std::string_view classes) const;
};

} // namespace editor
