module liunicode;
import :data.normalization;
import :tables;
using namespace rstd::prelude;
namespace liunicode
{
using namespace data_normalization;
using namespace tables;
auto combining_class(unsigned cp) -> unsigned {
    return scalar(storage, CombiningTable, cp, 0);
}

auto decomposition(unsigned cp, bool compatibility) -> Decomposition {
    auto          entry = mapping(storage, DecompositionTable, cp);
    Decomposition result;
    if (entry.size && (! entry.values[0] || compatibility)) {
        result.length = entry.size - 1;
        for (unsigned i {}; i < result.length; ++i) result.points[i] = entry.values[i + 1];
    }
    return result;
}

auto composition(unsigned first, unsigned second) -> unsigned {
    if (first > 0x10ffff || second > 0x10ffff) return 0;
    auto key   = (static_cast<unsigned long long>(first) << 21) | second;
    auto entry = mapping(storage, CompositionTable, key);
    return entry.size ? entry.values[0] : 0;
}

} // namespace liunicode
namespace
{
auto point(unsigned value) -> unsigned {
    return value;
}
auto point(const liunicode::SourceScalar& value) -> unsigned {
    return value.code_point;
}
auto with_point(unsigned, unsigned value) -> unsigned {
    return value;
}
auto with_point(liunicode::SourceScalar value, unsigned cp) -> liunicode::SourceScalar {
    value.code_point = cp;
    return value;
}
void combine(unsigned& value, unsigned, unsigned cp) {
    value = cp;
}
void combine(liunicode::SourceScalar& value, const liunicode::SourceScalar& other, unsigned cp) {
    value.code_point = cp;
    if (other.begin < value.begin) value.begin = other.begin;
    if (other.end > value.end) value.end = other.end;
}
auto cancelled(liunicode::NormalizationControl control) -> bool {
    return control.cancelled && control.cancelled(control.context);
}
template<class T>
auto decompose(T                               source,
               bool                            compatibility,
               Vec<T>&                         output,
               liunicode::NormalizationControl control) -> bool {
    if (cancelled(control)) return false;
    auto cp = point(source);
    if (cp >= 0xac00 && cp <= 0xd7a3) {
        auto index = cp - 0xac00;
        output.push(with_point(source, 0x1100 + index / 588));
        output.push(with_point(source, 0x1161 + index % 588 / 28));
        if (index % 28) output.push(with_point(source, 0x11a7 + index % 28));
        return true;
    }
    auto mapping = liunicode::decomposition(cp, compatibility);
    if (mapping.length) {
        for (unsigned index = 0; index < mapping.length; ++index)
            if (! decompose(
                    with_point(source, mapping.points[index]), compatibility, output, control))
                return false;
        return true;
    }
    auto combining = liunicode::combining_class(cp);
    auto index     = output.len();
    output.push(T(source));
    if (! combining) return true;
    while (index > usize()) {
        if (cancelled(control)) return false;
        auto preceding = liunicode::combining_class(point(output[index - usize(1)]));
        if (! preceding || preceding <= combining) break;
        output[index] = output[index - usize(1)];
        --index;
    }
    output[index] = source;
    return true;
}

auto compose_pair(unsigned first, unsigned second) -> unsigned {
    if (first >= 0x1100 && first < 0x1100 + 19 && second >= 0x1161 && second < 0x1161 + 21)
        return 0xac00 + ((first - 0x1100) * 21 + second - 0x1161) * 28;
    if (first >= 0xac00 && first <= 0xd7a3 && (first - 0xac00) % 28 == 0 && second > 0x11a7 &&
        second < 0x11a7 + 28)
        return first + second - 0x11a7;
    return liunicode::composition(first, second);
}
} // namespace

namespace liunicode
{
template<class T>
auto normalize_units(slice<T> points, Normalization form, NormalizationControl control = {})
    -> Result<Vec<T>, Error> {
    if (cancelled(control)) return Err(Error::Cancelled);
    bool compatibility = form == Normalization::NFKC || form == Normalization::NFKD;
    bool composed      = form == Normalization::NFC || form == Normalization::NFKC;
    for (auto value : points) {
        if (cancelled(control)) return Err(Error::Cancelled);
        if (point(value) > 0x10ffff) return Err(Error::InvalidCodePoint);
    }
    Vec<T> decomposed;
    for (auto value : points)
        if (! decompose(value, compatibility, decomposed, control)) return Err(Error::Cancelled);
    if (! composed || decomposed.len() == usize()) return Ok(rstd::move(decomposed));

    Vec<T> result;
    result.push(T(decomposed[usize()]));
    usize    starter;
    unsigned last_combining = 0;
    for (usize index(1); index < decomposed.len(); ++index) {
        if (cancelled(control)) return Err(Error::Cancelled);
        auto unit      = decomposed[index];
        auto cp        = point(unit);
        auto combining = liunicode::combining_class(cp);
        auto composite = last_combining == 0 || last_combining < combining
                             ? compose_pair(point(result[starter]), cp)
                             : 0;
        if (composite) {
            combine(result[starter], unit, composite);
            continue;
        }
        if (! combining) starter = result.len();
        result.push(T(unit));
        last_combining = combining;
    }
    return Ok(rstd::move(result));
}
auto normalize(slice<unsigned> points, Normalization form) -> Result<Vec<unsigned>, Error> {
    return normalize_units(points, form);
}
auto normalize_with_source(slice<SourceScalar>  points,
                           Normalization        form,
                           NormalizationControl control) -> Result<Vec<SourceScalar>, Error> {
    if (cancelled(control)) return Err(Error::Cancelled);
    for (auto value : points) {
        if (cancelled(control)) return Err(Error::Cancelled);
        if (value.begin > value.end) return Err(Error::InvalidSourceRange);
    }
    return normalize_units(points, form, control);
}
auto normalize_utf8_with_source(ref<str> text, Normalization form)
    -> Result<Vec<SourceScalar>, Error> {
    if (rstd::str_::from_utf8(text->as_bytes()).is_err()) return Err(Error::InvalidUtf8);
    Vec<SourceScalar> points;
    auto              chars = text->chars();
    usize             offset;
    for (auto cp = chars.next(); cp.is_some(); cp = chars.next()) {
        auto end = offset + cp->len_utf8();
        points.push(SourceScalar { unsigned(cp->to_primitive()), offset, end });
        offset = end;
    }
    return normalize_with_source(points.as_slice(), form, {});
}
} // namespace liunicode
