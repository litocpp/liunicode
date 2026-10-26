#include <rstd/test/gtest.hpp>
import liunicode;
using namespace rstd::prelude;
using namespace rstd::literals;

TEST(CaseFold, FullDefaultMappingDoesNotUseLocaleOrLowercase) {
    EXPECT_EQ(liunicode::case_fold_utf8("Straße STRASSE Σςσ İ ﬃ"_str).unwrap().as_str(),
              "strasse strasse σσσ i\u0307 ffi"_str);
    EXPECT_EQ(liunicode::case_fold_utf8("\u13a0\uab70"_str).unwrap().as_str(), "\u13a0\u13a0"_str);
    auto sharp = liunicode::case_fold(0xdf);
    EXPECT_EQ(sharp.size, 2u);
    EXPECT_EQ(sharp.first, unsigned('s'));
    EXPECT_EQ(sharp.second, unsigned('s'));
    EXPECT_EQ(liunicode::simple_case_fold(0xdf), 0xdfu);
    EXPECT_TRUE(liunicode::case_fold_utf8(rstd::str_::from_utf8_unchecked("\xff"_bytes)).is_err());
}
