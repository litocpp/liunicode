#include <rstd/test/gtest.hpp>
import liunicode;
import rstd.test;
using namespace rstd::prelude;
using namespace rstd::literals;
using namespace liunicode;
namespace
{
auto data(ref<str> name) -> String {
    auto source = rstd::path::PathBuf::from(__FILE__ ""_str);
    auto path   = rstd::path::PathBuf::from(source.as_path().parent().unwrap());
    auto dir    = rstd::path::PathBuf::from(
        rstd::format("../build/unicode/{}.{}.{}", version_major, version_minor, version_patch)
            .as_str());
    path.push(dir.as_path());
    auto file = rstd::path::PathBuf::from(name);
    path.push(file.as_path());
    return rstd::fs::read_to_string(path.as_path()).unwrap();
}
auto digit(unsigned cp) -> unsigned {
    if (cp >= '0' && cp <= '9') return cp - '0';
    if (cp >= 'A' && cp <= 'F') return cp - 'A' + 10;
    return 16;
}
void boundaries(bool words) {
    auto          source = data(words ? "WordBreakTest.txt"_str : "GraphemeBreakTest.txt"_str);
    auto          chars  = source.as_str()->chars();
    Vec<unsigned> points;
    Vec<usize>    expected;
    unsigned      hex {}, digits {}, cases {}, line_number { 1 };
    bool          comment {};
    auto          number = [&] {
        if (digits) points.push(unsigned(hex));
        hex = digits = 0;
    };
    auto line = [&] {
        number();
        if (! expected.is_empty()) {
            auto       result = segment(points.as_slice()).unwrap();
            Vec<usize> actual;
            if (words) {
                actual.push(usize());
                for (const auto& word : result.words) actual.push(usize(word.end));
            } else
                actual = rstd::move(result.graphemes);
            if (actual.len() != expected.len())
                rstd::io::println(
                    "boundary line {}: {} != {}", line_number, actual.len(), expected.len());
            EXPECT_EQ(actual.len(), expected.len());
            for (usize i {}; i < expected.len() && i < actual.len(); ++i) {
                if (actual[i] != expected[i])
                    rstd::io::println("boundary line {} index {}", line_number, i);
                EXPECT_EQ(actual[i], expected[i]);
            }
            ++cases;
        }
        points.clear();
        expected.clear();
        comment = false;
        ++line_number;
    };
    for (auto item = chars.next(); item.is_some(); item = chars.next()) {
        auto cp = item->to_primitive();
        if (cp == '\n') {
            line();
            continue;
        }
        if (comment) continue;
        if (cp == '#') {
            number();
            comment = true;
            continue;
        }
        if (cp == 0xf7) {
            number();
            expected.push(points.len());
        } else if (digit(cp) < 16) {
            hex = hex * 16 + digit(cp);
            ++digits;
        } else
            number();
    }
    line();
    EXPECT_GT(cases, words ? 1900u : 700u);
    rstd::io::println(
        "Unicode {} {} break cases={}", version_major, words ? "word"_str : "grapheme"_str, cases);
}
} // namespace
TEST(Unicode, GraphemeBreakConformance) {
    boundaries(false);
}
TEST(Unicode, WordBreakConformance) {
    boundaries(true);
}
TEST(Unicode, NormalizationConformance) {
    auto          source = data("NormalizationTest.txt"_str);
    auto          chars  = source.as_str()->chars();
    Vec<unsigned> columns[5];
    unsigned      column {}, hex {}, digits {}, cases {};
    bool          comment {};
    auto          number = [&] {
        if (digits && column < 5) columns[column].push(unsigned(hex));
        hex = digits = 0;
    };
    auto line = [&] {
        number();
        if (column == 5) {
            for (unsigned input {}; input < 5; ++input)
                for (unsigned form {}; form < 4; ++form) {
                    auto result =
                        normalize(columns[input].as_slice(), Normalization(form)).unwrap();
                    auto& expected = columns[form >= 2   ? form + 1
                                             : input < 3 ? form + 1
                                                         : form + 3];
                    ASSERT_EQ(result.len(), expected.len());
                    for (usize i {}; i < result.len(); ++i) EXPECT_EQ(result[i], expected[i]);
                }
            ++cases;
        }
        for (auto& values : columns) values.clear();
        column  = 0;
        comment = false;
    };
    for (auto item = chars.next(); item.is_some(); item = chars.next()) {
        auto cp = item->to_primitive();
        if (cp == '\n') {
            line();
            continue;
        }
        if (comment) continue;
        if (cp == '#' || cp == '@') {
            number();
            comment = true;
            continue;
        }
        if (cp == ';') {
            number();
            ++column;
        } else if (digit(cp) < 16) {
            hex = hex * 16 + digit(cp);
            ++digits;
        } else
            number();
    }
    line();
    EXPECT_GT(cases, 19000u);
    rstd::io::println("Unicode {} normalization cases={}", version_major, cases);
}
TEST(Unicode, CaseContextsAndInvalidInputs) {
    EXPECT_EQ(convert_case_utf8("ΑΣ\u0301 ΣΑ Σ İ ß"_str, Case::Lower).unwrap().as_str(),
              "ας\u0301 σα σ i\u0307 ß"_str);
    EXPECT_EQ(convert_case_utf8("straße"_str, Case::Upper).unwrap().as_str(), "STRASSE"_str);
    EXPECT_FALSE(id_start('$'));
    EXPECT_FALSE(id_start('_'));
    EXPECT_TRUE(id_continue('_'));
    EXPECT_TRUE(id_start(0x16ea0));
    EXPECT_FALSE(id_start(0xd800));
    unsigned values[] { 0xd800, 0x110000 };
    auto     lone = slice<unsigned>::from_raw_parts(values, usize(1));
    EXPECT_EQ(convert_case(lone, Case::Upper).unwrap()[usize()], 0xd800u);
    EXPECT_EQ(normalize(lone, Normalization::NFKC).unwrap()[usize()], 0xd800u);
    auto invalid = slice<unsigned>::from_raw_parts(values, usize(2));
    EXPECT_TRUE(segment(invalid).is_err());
    EXPECT_TRUE(normalize(invalid, Normalization::NFC).is_err());
    EXPECT_TRUE(convert_case(invalid, Case::Lower).is_err());
    auto bad = rstd::str_::from_utf8_unchecked("\xff"_bytes);
    EXPECT_TRUE(segment_utf8(bad).is_err());
    EXPECT_TRUE(convert_case_utf8(bad, Case::Lower).is_err());
    EXPECT_EQ(segment_utf8(""_str).unwrap().graphemes.len(), usize(1));
    EXPECT_TRUE(segment_utf8(""_str).unwrap().words.is_empty());
}
TEST(Unicode, Utf8WordClassificationAndLongContexts) {
    auto text = segment_utf8("中カa 12👩‍🚀"_str).unwrap();
    EXPECT_EQ(text.words.len(), usize(6));
    EXPECT_EQ(text.words[usize()].kind, WordKind::Ideograph);
    EXPECT_EQ(text.words[usize(1)].kind, WordKind::Kana);
    EXPECT_EQ(text.words[usize(2)].kind, WordKind::Letter);
    EXPECT_EQ(text.words[usize(3)].kind, WordKind::Whitespace);
    EXPECT_EQ(text.words[usize(4)].kind, WordKind::Number);
    EXPECT_EQ(text.words[usize(5)].kind, WordKind::Emoji);
    EXPECT_FALSE(text.words[usize(5)].is_word());
    EXPECT_EQ(text.graphemes[text.graphemes.len() - usize(1)], usize(21));
    Vec<unsigned> points;
    points.push(unsigned('A'));
    for (unsigned i {}; i < 20000; ++i) points.push(unsigned(0x301));
    auto breaks = segment(points.as_slice()).unwrap();
    EXPECT_EQ(breaks.graphemes.len(), usize(2));
    points.push(unsigned(0x3a3));
    auto lower = convert_case(points.as_slice(), Case::Lower).unwrap();
    EXPECT_EQ(lower[lower.len() - usize(1)], 0x3c2u);
    for (unsigned i {}; i < 20000; ++i) points.push(unsigned(0x3a3));
    lower = convert_case(points.as_slice(), Case::Lower).unwrap();
    EXPECT_EQ(lower[lower.len() - usize(1)], 0x3c2u);
}
TEST(Unicode, PropertySetsAndSequences) {
    u16  name[] { u16('L'), u16('u') };
    auto found =
        property(PropertyGroup::GeneralCategory, slice<u16>::from_raw_parts(name, usize(2)))
            .unwrap();
    EXPECT_TRUE(found.contains('A'));
    EXPECT_FALSE(found.contains('a'));
    u16  emoji[] { u16('R'), u16('G'), u16('I'), u16('_'), u16('E'),
                   u16('m'), u16('o'), u16('j'), u16('i') };
    auto sequences =
        property(PropertyGroup::Strings, slice<u16>::from_raw_parts(emoji, usize(9))).unwrap();
    u32 points[] { u32(0x1f469), u32(0x200d), u32(0x1f680) };
    EXPECT_TRUE(sequences.contains_sequence(slice<u32>::from_raw_parts(points, usize(3))));
    EXPECT_FALSE(sequences.contains(0x1f469));
}

namespace
{
template<typename F>
void records(ref<str> name, F callback) {
    auto        source = data(name);
    auto        chars  = source.as_str()->chars();
    Vec<String> fields;
    String      field;
    bool        comment {};
    auto        finish = [&] {
        fields.push(rstd::move(field));
        field = String {};
        if (fields.len() > usize(1)) callback(fields.as_slice());
        fields.clear();
        comment = false;
    };
    for (auto item = chars.next(); item.is_some(); item = chars.next()) {
        auto cp = item->to_primitive();
        if (cp == '\n') {
            finish();
            continue;
        }
        if (comment) continue;
        if (cp == '#') {
            comment = true;
            continue;
        }
        if (cp == ';') {
            fields.push(rstd::move(field));
            field = String {};
        } else
            field.push(char32_t(cp));
    }
    finish();
}
auto hexadecimal(ref<str> text) -> Vec<unsigned> {
    Vec<unsigned> output;
    unsigned      hex {}, digits {};
    auto          chars  = text->chars();
    auto          finish = [&] {
        if (digits) output.push(unsigned(hex));
        digits = hex = 0;
    };
    for (auto item = chars.next(); item.is_some(); item = chars.next()) {
        auto value = digit(item->to_primitive());
        if (value < 16) {
            hex = hex * 16 + value;
            ++digits;
        } else
            finish();
    }
    finish();
    return output;
}
} // namespace
TEST(Unicode, CaseMappingSourceData) {
    unsigned folds {}, specials {};
    records("CaseFolding.txt"_str, [&](slice<String> fields) {
        auto status = fields[usize(1)].as_str();
        if (! status->contains("C"_str) && ! status->contains("S"_str)) return;
        auto source = hexadecimal(fields[usize()].as_str());
        auto target = hexadecimal(fields[usize(2)].as_str());
        ASSERT_EQ(source.len(), usize(1));
        ASSERT_EQ(target.len(), usize(1));
        EXPECT_EQ(simple_case_fold(source[usize()]), target[usize()]);
        ++folds;
    });
    records("SpecialCasing.txt"_str, [&](slice<String> fields) {
        if (fields.len() < usize(5)) return;
        for (auto byte : fields[usize(4)].as_str()->as_bytes())
            if (byte != u8(' ')) return;
        auto cp = hexadecimal(fields[usize()].as_str())[usize()];
        for (unsigned i {}; i < 2; ++i) {
            auto expected = hexadecimal(fields[usize(i ? 3 : 1)].as_str());
            auto actual   = i ? uppercase(cp) : lowercase(cp);
            ASSERT_EQ(usize(actual.size), expected.len());
            unsigned points[] { actual.first, actual.second, actual.third };
            for (usize j {}; j < expected.len(); ++j)
                EXPECT_EQ(points[j.to_primitive()], expected[j]);
        }
        ++specials;
    });
    EXPECT_GT(folds, 1500u);
    EXPECT_GT(specials, 100u);
    rstd::io::println(
        "Unicode {} case mappings: {} folds, {} special rows", version_major, folds, specials);
    Property empty;
    EXPECT_FALSE(empty.contains('A'));
    EXPECT_EQ(empty.maximum_length(), 0u);
}
