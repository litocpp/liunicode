export module liunicode:properties;
export import :types;
using namespace rstd::prelude;
export namespace liunicode
{
enum class PropertyGroup
{
    Binary,
    GeneralCategory,
    Script,
    ScriptExtensions,
    Strings
};
class Property {
    unsigned index_ {};
    bool     strings_ {};
    bool     valid_ {};
    Property(unsigned index, bool strings): index_(index), strings_(strings), valid_(true) {}
    friend auto property(PropertyGroup, rstd::slice<rstd::u16>) -> rstd::Option<Property>;

public:
    Property() = default;
    auto is_string() const -> bool { return strings_; }
    auto contains(unsigned cp) const -> bool;
    auto contains_sequence(rstd::slice<rstd::u32> points, bool folded = false) const -> bool;
    auto maximum_length() const -> unsigned;
};
// Names are exact UCD aliases; consumers own syntax and language-specific
// restrictions.
auto property(PropertyGroup group, rstd::slice<rstd::u16> name) -> rstd::Option<Property>;
} // namespace liunicode
