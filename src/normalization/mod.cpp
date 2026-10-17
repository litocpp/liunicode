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
void decompose(unsigned cp, bool compatibility, Vec<unsigned>& output) {
    if (cp >= 0xac00 && cp <= 0xd7a3) {
        auto index = cp - 0xac00;
        output.push(unsigned(0x1100 + index / 588));
        output.push(unsigned(0x1161 + index % 588 / 28));
        if (index % 28) output.push(unsigned(0x11a7 + index % 28));
        return;
    }
    auto mapping = liunicode::decomposition(cp, compatibility);
    if (mapping.length) {
        for (unsigned index = 0; index < mapping.length; ++index)
            decompose(mapping.points[index], compatibility, output);
        return;
    }
    auto combining = liunicode::combining_class(cp);
    auto index     = output.len();
    output.push(unsigned(cp));
    if (! combining) return;
    while (index > usize()) {
        auto preceding = liunicode::combining_class(output[index - usize(1)]);
        if (! preceding || preceding <= combining) break;
        output[index] = output[index - usize(1)];
        --index;
    }
    output[index] = cp;
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
auto normalize(slice<unsigned> points, Normalization form) -> Result<Vec<unsigned>, Error> {
    bool compatibility = form == Normalization::NFKC || form == Normalization::NFKD;
    bool composed      = form == Normalization::NFC || form == Normalization::NFKC;
    for (auto cp : points)
        if (cp > 0x10ffff) return Err(Error::InvalidCodePoint);
    Vec<unsigned> decomposed;
    for (auto cp : points) decompose(cp, compatibility, decomposed);
    if (! composed || decomposed.len() == usize()) return Ok(rstd::move(decomposed));

    Vec<unsigned> result;
    result.push(unsigned(decomposed[usize()]));
    usize    starter;
    unsigned last_combining = 0;
    for (usize index(1); index < decomposed.len(); ++index) {
        auto cp        = decomposed[index];
        auto combining = liunicode::combining_class(cp);
        auto composite = last_combining == 0 || last_combining < combining
                             ? compose_pair(result[starter], cp)
                             : 0;
        if (composite) {
            result[starter] = composite;
            continue;
        }
        if (! combining) starter = result.len();
        result.push(unsigned(cp));
        last_combining = combining;
    }
    return Ok(rstd::move(result));
}
} // namespace liunicode
