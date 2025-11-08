#ifndef DESCALE_HPP
#define DESCALE_HPP

#include <AC/Core/Image.hpp>

extern "C" {
#include <descale.h>
}

class Descale
{
public:
    Descale(int srcW, int srcH, int dstW, int dstH, int mode) noexcept;
    ~Descale() noexcept;

    void process(const ac::core::Image& src, ac::core::Image& dst) noexcept;

private:
    DescaleParams dspara{};
    DescaleCore* dscoreH{};
    DescaleCore* dscoreV{};
};

#endif
