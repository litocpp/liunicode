module liunicode;
import :data.properties;
import :tables;
using namespace rstd::prelude;
namespace liunicode
{
using namespace data_properties;
using namespace tables;
namespace
{
auto compare(slice<u16> text, const char* ascii) -> int {
    rstd::size_t index {};
    while (index < text.len().to_primitive() && ascii[index] &&
           text[usize(index)].to_primitive() == static_cast<unsigned char>(ascii[index]))
        ++index;
    if (index == text.len().to_primitive()) return ascii[index] ? -1 : 0;
    if (! ascii[index]) return 1;
    return text[usize(index)].to_primitive() < static_cast<unsigned char>(ascii[index]) ? -1 : 1;
}

template<rstd::size_t N>
auto alias(const Alias (&aliases)[N], slice<u16> name) -> Option<unsigned> {
    rstd::size_t first {}, last = N;
    while (first < last) {
        auto middle = first + (last - first) / 2;
        auto order  = compare(name, Names + aliases[middle].name);
        if (order < 0)
            last = middle;
        else if (order > 0)
            first = middle + 1;
        else
            return Some(unsigned(aliases[middle].property));
    }
    return None<unsigned>();
}
} // namespace
auto property(PropertyGroup group, slice<u16> name) -> Option<Property> {
    Option<unsigned> index;
    switch (group) {
    case PropertyGroup::Binary: index = alias(BinaryAliases, name); break;
    case PropertyGroup::GeneralCategory: index = alias(CategoryAliases, name); break;
    case PropertyGroup::Script: index = alias(ScriptAliases, name); break;
    case PropertyGroup::ScriptExtensions: index = alias(ExtensionAliases, name); break;
    case PropertyGroup::Strings: index = alias(StringAliases, name); break;
    }
    return index.is_some() ? Some(Property(*index, group == PropertyGroup::Strings))
                           : None<Property>();
}
auto Property::contains(unsigned cp) const -> bool {
    return valid_ && ! strings_ && tables::contains(storage, index_, cp);
}
auto Property::maximum_length() const -> unsigned {
    return valid_ ? (strings_ ? StringLengths[index_] : 1) : 0;
}
auto Property::contains_sequence(slice<u32> codepoints, bool ignore_case) const -> bool {
    if (! valid_ || ! strings_) return false;
    auto node = StringRoots[ignore_case];
    for (auto point : codepoints) {
        auto cp = point.to_primitive();
        if (cp > 0x10ffff) return false;
        if (ignore_case) cp = simple_case_fold(cp);
        auto child =
            mapping(storage, StringEdgesTable, (static_cast<unsigned long long>(node) << 21) | cp);
        if (! child.size) return false;
        node = child.values[0];
    }
    return (StringMasks[node] & (1u << index_)) != 0;
}
} // namespace liunicode
