export module liunicode:segmentation;
export import :types;
using namespace rstd::prelude;
export namespace liunicode
{
enum class WordKind
{
    None,
    Whitespace,
    Emoji,
    Number,
    Letter,
    Kana,
    Ideograph
};
struct WordSegment {
    rstd::usize start {}, end {};
    WordKind    kind {};
    auto        is_word() const -> bool {
        return kind == WordKind::Number || kind == WordKind::Letter || kind == WordKind::Kana ||
               kind == WordKind::Ideograph;
    }
};
struct Segmentation {
    Vec<rstd::usize> graphemes;
    Vec<WordSegment> words;
};
// Code point input returns code point offsets; UTF-8 input returns byte
// offsets.
auto segment(rstd::slice<unsigned> points) -> rstd::Result<Segmentation, Error>;
auto segment_utf8(rstd::ref<rstd::str> text) -> rstd::Result<Segmentation, Error>;
} // namespace liunicode
