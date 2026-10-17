import rstd;
import liunicode;
using namespace rstd::prelude;
using namespace rstd::literals;
int main(int argc, char** argv) {
    auto     input  = argc > 1 ? rstd::ffi::CStr::from_ptr(argv[1]).to_str().unwrap() : "AΣ中"_str;
    unsigned result = unsigned(input->len().to_primitive());

    rstd::io::println("{}", result);
    return 0;
}
