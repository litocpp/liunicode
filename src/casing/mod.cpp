module liunicode;
import :data.casing;
import :tables;
using namespace rstd::prelude;
namespace liunicode
{
using namespace data_casing;
using namespace tables;
auto id_start(unsigned cp) -> bool {
    return contains(storage, ID_StartTable, cp);
}
auto id_continue(unsigned cp) -> bool {
    return contains(storage, ID_ContinueTable, cp);
}
auto simple_case_fold(unsigned cp) -> unsigned {
    return scalar(storage, FoldTable, cp, cp);
}

auto cased(unsigned cp) -> bool {
    return contains(storage, CasedTable, cp);
}
auto case_ignorable(unsigned cp) -> bool {
    return contains(storage, Case_IgnorableTable, cp);
}

auto lowercase(unsigned cp) -> CaseMapping {
    auto entry = mapping(storage, LowercaseTable, cp);
    return entry.size
               ? CaseMapping { entry.values[0], entry.values[1], entry.values[2], entry.size }
               : CaseMapping { cp, 0, 0, 1 };
}
auto uppercase(unsigned cp) -> CaseMapping {
    auto entry = mapping(storage, UppercaseTable, cp);
    return entry.size
               ? CaseMapping { entry.values[0], entry.values[1], entry.values[2], entry.size }
               : CaseMapping { cp, 0, 0, 1 };
}
auto fold_candidate(unsigned cp, unsigned index) -> Option<unsigned> {
    auto target = simple_case_fold(cp);
    if (! index) return Some(target);
    auto sources = mapping(storage, FoldSourcesTable, target);
    return index <= sources.size ? Some(unsigned(sources.values[index - 1])) : None<unsigned>();
}
auto convert_case(slice<unsigned> points, Case mode) -> Result<Vec<unsigned>, Error> {
    Vec<unsigned> output;
    bool          preceded = false;
    usize         after;
    for (usize i {}; i < points.len(); ++i) {
        auto cp = points[i];
        if (cp > 0x10ffff) return Err(Error::InvalidCodePoint);
        auto mapped = mode == Case::Lower ? lowercase(cp) : uppercase(cp);
        if (mode == Case::Lower && cp == 0x03a3 && preceded) {
            if (after <= i) {
                after = i + usize(1);
                while (after < points.len() && case_ignorable(points[after])) ++after;
            }
            if (after == points.len() || ! cased(points[after])) mapped = { 0x03c2, 0, 0, 1 };
        }
        if (mapped.size) output.push(unsigned(mapped.first));
        if (mapped.size > 1) output.push(unsigned(mapped.second));
        if (mapped.size > 2) output.push(unsigned(mapped.third));
        if (! case_ignorable(cp)) preceded = cased(cp);
    }
    return Ok(rstd::move(output));
}
auto convert_case_utf8(ref<str> text, Case mode) -> Result<String, Error> {
    auto decoded = decode_utf8(text);
    if (decoded.is_err()) return Err(decoded.unwrap_err());
    auto converted = convert_case(decoded->as_slice(), mode);
    if (converted.is_err()) return Err(converted.unwrap_err());
    String result;
    for (auto cp : converted.unwrap()) result.push(char32_t(cp));
    return Ok(rstd::move(result));
}
} // namespace liunicode
