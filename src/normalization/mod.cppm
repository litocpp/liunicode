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
auto combining_class(unsigned cp) -> unsigned;
auto decomposition(unsigned cp, bool compatibility) -> Decomposition;
auto composition(unsigned first, unsigned second) -> unsigned;
auto normalize(rstd::slice<unsigned> points, Normalization form)
    -> rstd::Result<Vec<unsigned>, Error>;
} // namespace liunicode
