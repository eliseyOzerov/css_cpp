#include "CssStyleSheet.h"

#include <algorithm>
#include <cctype>
#include <optional>

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

/// `text` with every `/* … */` comment replaced by a space, so it still separates what was either side.
std::string withoutComments(std::string_view text) {
    std::string result;
    size_t position = 0;
    while (position < text.size()) {
        if (text.compare(position, 2, "/*") == 0) {
            const size_t end = text.find("*/", position + 2);
            result += ' ';
            position = end == std::string_view::npos ? text.size() : end + 2;
        } else {
            result += text[position];
            position += 1;
        }
    }
    return result;
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

/// The parts of one compound selector (`path.a#b`), or none when it uses anything else.
std::optional<CssStyleSheet::Selector> compoundSelector(std::string_view text) {
    CssStyleSheet::Selector selector;
    if (text.empty()) return std::nullopt;
    const auto isNameCharacter = [](char character) {
        return std::isalnum(static_cast<unsigned char>(character)) || character == '-' || character == '_'
               || static_cast<unsigned char>(character) >= 0x80;
    };
    size_t position = 0;
    const auto name = [&] {
        const size_t start = position;
        while (position < text.size() && isNameCharacter(text[position])) position += 1;
        return std::string(text.substr(start, position - start));
    };
    if (text[0] == '*') {
        position = 1;
    } else if (isNameCharacter(text[0])) {
        selector.type = name();
        selector.specificity[2] += 1;
    }
    while (position < text.size()) {
        const char marker = text[position];
        position += 1;
        const std::string part = name();
        if (part.empty()) return std::nullopt;
        if (marker == '.') {
            selector.classes.push_back(part);
            selector.specificity[1] += 1;
        } else if (marker == '#') {
            if (!selector.id.empty()) return std::nullopt;
            selector.id = part;
            selector.specificity[0] += 1;
        } else {
            return std::nullopt;
        }
    }
    return selector;
}

/// Whether the space-separated `classes` include `name`.
bool hasClass(std::string_view classes, std::string_view name) {
    for (std::string_view piece : splitTrimmed(classes, ' ')) {
        if (piece == name) return true;
    }
    return false;
}

} // namespace

std::vector<CssDeclaration> cssDeclarations(std::string_view text) {
    std::vector<CssDeclaration> declarations;
    const std::string uncommented = withoutComments(text);
    for (std::string_view piece : splitTrimmed(uncommented, ';')) {
        const size_t colon = piece.find(':');
        if (colon == std::string_view::npos) continue;
        const std::string_view name = trimmed(piece.substr(0, colon));
        std::string_view value = trimmed(piece.substr(colon + 1));
        // `!important` is read and dropped: the cascade here has no separate important layer.
        if (const size_t bang = value.find('!'); bang != std::string_view::npos) value = trimmed(value.substr(0, bang));
        if (!name.empty()) declarations.emplace_back(lowercased(name), std::string(value));
    }
    return declarations;
}

void CssStyleSheet::read(std::string_view css) {
    const std::string text = withoutComments(css);
    size_t position = 0;
    int order = static_cast<int>(rules.size());
    while (position < text.size()) {
        const size_t open = text.find('{', position);
        if (open == std::string::npos) break;
        const std::string_view prelude = trimmed(std::string_view(text).substr(position, open - position));
        // An at-rule's block (`@media … { rules }`) can hold nested braces: skip to its matching close.
        int depth = 1;
        size_t close = open + 1;
        while (close < text.size() && depth > 0) {
            if (text[close] == '{') depth += 1;
            if (text[close] == '}') depth -= 1;
            close += 1;
        }
        const std::string_view body = std::string_view(text).substr(open + 1, close - open - 2);
        position = close;
        if (prelude.empty() || prelude.front() == '@') continue;
        const auto declarations = cssDeclarations(body);
        for (std::string_view piece : splitTrimmed(prelude, ',')) {
            if (const std::optional<Selector> selector = compoundSelector(piece)) {
                rules.push_back(Rule{*selector, declarations, order});
                order += 1;
            }
        }
    }
}

std::vector<CssDeclaration> CssStyleSheet::declarationsFor(std::string_view type, std::string_view id,
                                                                                std::string_view classes) const {
    std::vector<const Rule*> matching;
    for (const Rule& rule : rules) {
        const Selector& selector = rule.selector;
        if (!selector.type.empty() && selector.type != type) continue;
        if (!selector.id.empty() && selector.id != id) continue;
        const bool hasEveryClass = std::all_of(selector.classes.begin(), selector.classes.end(),
                                               [&](const std::string& name) { return hasClass(classes, name); });
        if (hasEveryClass) matching.push_back(&rule);
    }
    std::stable_sort(matching.begin(), matching.end(), [](const Rule* first, const Rule* second) {
        if (first->selector.specificity != second->selector.specificity) return first->selector.specificity < second->selector.specificity;
        return first->order < second->order;
    });
    std::vector<CssDeclaration> declarations;
    for (const Rule* rule : matching) declarations.insert(declarations.end(), rule->declarations.begin(), rule->declarations.end());
    return declarations;
}

} // namespace editor
