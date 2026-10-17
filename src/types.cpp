module liunicode;
using namespace rstd::prelude;
namespace liunicode
{
auto decode_utf8(ref<str> text) -> Result<Vec<unsigned>, Error> {
    if (rstd::str_::from_utf8(text->as_bytes()).is_err()) return Err(Error::InvalidUtf8);
    Vec<unsigned> output;
    auto          chars = text->chars();
    for (auto cp = chars.next(); cp.is_some(); cp = chars.next())
        output.push(unsigned(cp->to_primitive()));
    return Ok(rstd::move(output));
}
} // namespace liunicode
