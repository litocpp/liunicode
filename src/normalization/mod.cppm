export module liunicode:normalization;
export import :types;
using namespace rstd::prelude;
export namespace liunicode
{
struct Decomposition {
    unsigned points[18] {};
    unsigned length {};
};
enum class Normalization
{
    NFC,
    NFD,
    NFKC,
    NFKD
};
// A half-open source range enclosing the inputs that produced this scalar.
// It can also cover unrelated input after canonical reordering.
struct SourceScalar {
    unsigned    code_point {};
    rstd::usize begin {}, end {};
};
// The synchronous callback may be invoked during validation and normalization.
struct NormalizationControl {
    const void* context {};
    bool (*cancelled)(const void*) noexcept {};
};
auto combining_class(unsigned cp) -> unsigned;
auto decomposition(unsigned cp, bool compatibility) -> Decomposition;
auto composition(unsigned first, unsigned second) -> unsigned;
auto normalize(rstd::slice<unsigned> points, Normalization form)
    -> rstd::Result<Vec<unsigned>, Error>;
// Ranges use the caller's coordinates; begin must not exceed end.
auto normalize_with_source(rstd::slice<SourceScalar> points,
                           Normalization             form,
                           NormalizationControl      control = {})
    -> rstd::Result<Vec<SourceScalar>, Error>;
// Ranges are byte offsets in the validated UTF-8 input, including NUL bytes.
auto normalize_utf8_with_source(rstd::ref<rstd::str> text, Normalization form)
    -> rstd::Result<Vec<SourceScalar>, Error>;
} // namespace liunicode
