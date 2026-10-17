export module liunicode:types;
export import rstd;
using namespace rstd::prelude;
export namespace liunicode
{
inline constexpr unsigned version_major = 18, version_minor = 0, version_patch = 0;
enum class Error
{
    InvalidUtf8,
    InvalidCodePoint
};
// Code point APIs preserve surrogate values; UTF-8 APIs accept only valid
// scalar encodings.
auto decode_utf8(rstd::ref<rstd::str> text) -> rstd::Result<Vec<unsigned>, Error>;
} // namespace liunicode
