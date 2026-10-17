module liunicode;
import :data.segmentation;
import :tables;
using namespace rstd::prelude;
namespace liunicode
{
namespace
{
using namespace data_segmentation;
using G = Grapheme;
using W = Word;
using I = Indic;
auto value(unsigned table, unsigned cp) -> unsigned {
    auto row = tables::mapping(storage, table, cp, false);
    return row.size && cp - row.key <= row.values[0] ? row.values[1] : 0;
}
auto has(unsigned table, unsigned cp) -> bool {
    return tables::contains(storage, table, cp);
}
struct Character {
    G        grapheme {};
    W        word {};
    I        indic {};
    bool     pictographic {};
    WordKind kind {};
    usize    before {}, after {};
};
auto character(unsigned cp) -> Character {
    Character result { G(value(GraphemeTable, cp)),
                       W(value(WordTable, cp)),
                       I(value(IndicTable, cp)),
                       has(PictographicTable, cp) };
    if (has(IdeographicTable, cp))
        result.kind = WordKind::Ideograph;
    else if (result.word == W::Katakana)
        result.kind = WordKind::Kana;
    else if (has(AlphabeticTable, cp))
        result.kind = WordKind::Letter;
    else if (result.word == W::Numeric)
        result.kind = WordKind::Number;
    else if (result.pictographic || result.word == W::Regional_Indicator)
        result.kind = WordKind::Emoji;
    else if (has(WhitespaceTable, cp))
        result.kind = WordKind::Whitespace;
    return result;
}
auto control(G value) -> bool {
    return value == G::Control || value == G::CR || value == G::LF;
}
auto newline(W value) -> bool {
    return value == W::Newline || value == W::CR || value == W::LF;
}
auto ignored(W value) -> bool {
    return value == W::Extend || value == W::Format || value == W::ZWJ;
}
auto letter(W value) -> bool {
    return value == W::ALetter || value == W::Hebrew_Letter;
}
auto midletter(W value) -> bool {
    return value == W::MidLetter || value == W::MidNumLet || value == W::Single_Quote;
}
auto midnumber(W value) -> bool {
    return value == W::MidNum || value == W::MidNumLet || value == W::Single_Quote;
}
auto extended(W value) -> bool {
    return letter(value) || value == W::Numeric || value == W::Katakana;
}
struct GraphemeState {
    bool     pictographic {}, zwj {}, linker {};
    unsigned regional {};
    void     push(const Character& cp) {
        zwj = pictographic && cp.grapheme == G::ZWJ;
        if (cp.pictographic)
            pictographic = true;
        else if (cp.grapheme != G::Extend)
            pictographic = false;
        if (cp.indic == I::Linker)
            linker = true;
        else if (cp.indic != I::Extend)
            linker = false;
        regional = cp.grapheme == G::Regional_Indicator ? (regional ^ 1u) : 0;
    }
    auto boundary(const Character& left, const Character& right) const -> bool {
        auto a = left.grapheme, b = right.grapheme;
        if (a == G::CR && b == G::LF) return false;
        if (control(a) || control(b)) return true;
        if (a == G::L && (b == G::L || b == G::V || b == G::LV || b == G::LVT)) return false;
        if ((a == G::LV || a == G::V) && (b == G::V || b == G::T)) return false;
        if ((a == G::LVT || a == G::T) && b == G::T) return false;
        if (b == G::Extend || b == G::ZWJ || b == G::SpacingMark || a == G::Prepend) return false;
        if (linker && right.indic == I::Consonant) return false;
        if (zwj && right.pictographic) return false;
        if (regional && b == G::Regional_Indicator) return false;
        return true;
    }
};
auto word_boundary(slice<Character> chars, usize index, unsigned regional) -> bool {
    const auto& right  = chars[index];
    auto        direct = chars[index - usize(1)].word;
    auto        b      = right.word;
    if (direct == W::CR && b == W::LF) return false;
    if (newline(direct) || newline(b)) return true;
    if (direct == W::ZWJ && right.pictographic) return false;
    if (direct == W::WSegSpace && b == W::WSegSpace) return false;
    if (ignored(b)) return false;
    auto previous = right.before;
    if (previous == chars.len()) return true;
    auto a      = chars[previous].word;
    auto before = chars[previous].before;
    auto c      = before == chars.len() ? W::Other : chars[before].word;
    auto d      = right.after == chars.len() ? W::Other : chars[right.after].word;
    if (letter(a) && letter(b)) return false;
    if (letter(a) && midletter(b) && letter(d)) return false;
    if (letter(c) && midletter(a) && letter(b)) return false;
    if (a == W::Hebrew_Letter && b == W::Single_Quote) return false;
    if (a == W::Hebrew_Letter && b == W::Double_Quote && d == W::Hebrew_Letter) return false;
    if (c == W::Hebrew_Letter && a == W::Double_Quote && b == W::Hebrew_Letter) return false;
    if (a == W::Numeric && (b == W::Numeric || letter(b))) return false;
    if (letter(a) && b == W::Numeric) return false;
    if (c == W::Numeric && midnumber(a) && b == W::Numeric) return false;
    if (a == W::Numeric && midnumber(b) && d == W::Numeric) return false;
    if (a == W::Katakana && b == W::Katakana) return false;
    if ((extended(a) || a == W::ExtendNumLet) && b == W::ExtendNumLet) return false;
    if (a == W::ExtendNumLet && extended(b)) return false;
    if (a == W::Regional_Indicator && b == W::Regional_Indicator && regional) return false;
    return true;
}
} // namespace
auto segment(slice<unsigned> points) -> Result<Segmentation, Error> {
    Vec<Character> chars;
    auto           previous = points.len();
    for (usize index {}; index < points.len(); ++index) {
        auto cp = points[index];
        if (cp > 0x10ffff) return Err(Error::InvalidCodePoint);
        auto item   = character(cp);
        item.before = previous;
        if (! ignored(item.word)) previous = index;
        chars.push(rstd::move(item));
    }
    auto following = points.len();
    for (auto index = points.len(); index > usize();) {
        auto& item = chars[--index];
        item.after = following;
        if (! ignored(item.word)) following = index;
    }
    Segmentation result;
    result.graphemes.push(usize());
    GraphemeState state;
    unsigned      regional {};
    WordSegment   word;
    for (usize index {}; index < chars.len(); ++index) {
        const auto& cp = chars[index];
        if (index != usize()) {
            if (state.boundary(chars[index - usize(1)], cp)) result.graphemes.push(usize(index));
            if (word_boundary(chars.as_slice(), index, regional)) {
                word.end = index;
                result.words.push(rstd::move(word));
                word = WordSegment { index, {}, WordKind::None };
            }
        }
        if (unsigned(cp.kind) > unsigned(word.kind)) word.kind = cp.kind;
        state.push(cp);
        if (! ignored(cp.word)) regional = cp.word == W::Regional_Indicator ? regional ^ 1u : 0;
    }
    if (! points.is_empty()) {
        result.graphemes.push(points.len());
        word.end = points.len();
        result.words.push(rstd::move(word));
    }
    return Ok(rstd::move(result));
}
auto segment_utf8(ref<str> text) -> Result<Segmentation, Error> {
    auto points = decode_utf8(text);
    if (points.is_err()) return Err(points.unwrap_err());
    auto result = segment(points->as_slice());
    if (result.is_err()) return Err(result.unwrap_err());
    Vec<usize> offsets;
    usize      offset;
    for (auto cp : points.unwrap()) {
        offsets.push(usize(offset));
        offset += usize(cp < 0x80 ? 1 : cp < 0x800 ? 2 : cp < 0x10000 ? 3 : 4);
    }
    offsets.push(usize(offset));
    for (auto& boundary : result->graphemes) boundary = offsets[boundary];
    for (auto& word : result->words) {
        word.start = offsets[word.start];
        word.end   = offsets[word.end];
    }
    return result;
}
} // namespace liunicode
