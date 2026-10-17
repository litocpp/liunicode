export module liunicode:casing;
export import :types;
using namespace rstd::prelude;
export namespace liunicode
{
struct CaseMapping {
    unsigned first, second, third, size;
};
enum class Case
{
    Lower,
    Upper
};
auto id_start(unsigned cp) -> bool;
auto id_continue(unsigned cp) -> bool;
auto simple_case_fold(unsigned cp) -> unsigned;
auto cased(unsigned cp) -> bool;
auto case_ignorable(unsigned cp) -> bool;
auto lowercase(unsigned cp) -> CaseMapping;
auto uppercase(unsigned cp) -> CaseMapping;
auto fold_candidate(unsigned cp, unsigned index) -> rstd::Option<unsigned>;
auto convert_case(rstd::slice<unsigned> points, Case mode) -> rstd::Result<Vec<unsigned>, Error>;
auto convert_case_utf8(rstd::ref<rstd::str> text, Case mode) -> rstd::Result<String, Error>;
} // namespace liunicode
