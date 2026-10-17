import rstd;
import liunicode;
using namespace rstd::prelude;
using namespace rstd::literals;
int main(int argc, char** argv) {
    auto     input  = argc > 1 ? rstd::ffi::CStr::from_ptr(argv[1]).to_str().unwrap() : "AΣ中"_str;
    unsigned result = unsigned(input->len().to_primitive());
    result += unsigned(
        liunicode::convert_case_utf8(input, liunicode::Case::Lower).unwrap().len().to_primitive());
    result += unsigned(liunicode::segment_utf8(input).unwrap().graphemes.len().to_primitive());
    auto points = liunicode::decode_utf8(input).unwrap();
    result += unsigned(liunicode::normalize(points.as_slice(), liunicode::Normalization::NFC)
                           .unwrap()
                           .len()
                           .to_primitive());
    u16 name[] { u16('L') };
    result += liunicode::property(liunicode::PropertyGroup::GeneralCategory,
                                  slice<u16>::from_raw_parts(name, usize(1)))
                  .unwrap()
                  .contains(unsigned(input->len().to_primitive()));
    rstd::io::println("{}", result);
    return 0;
}
