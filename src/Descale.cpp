#include <AC/Core/SIMD.hpp>

#include "Descale.hpp"

const auto dsapi = get_descale_api(ac::core::simd::supportAVX2() ? DESCALE_OPT_AVX2 : DESCALE_OPT_NONE);

// descale implementation comes from https://github.com/Irrational-Encoding-Wizardry/descale
Descale::Descale(int srcW, int srcH, int dstW, int dstH, int mode) noexcept
{
    dspara.mode = DESCALE_MODE_BILINEAR;
    switch (mode)
    {
    case ac::core::RESIZE_CATMULL_ROM: // b = 0, c = 1/2
        dspara.mode = DESCALE_MODE_BICUBIC;
        dspara.param1 = 0.0;
        dspara.param2 = 1.0 / 2.0;
        break;
    case ac::core::RESIZE_MITCHELL_NETRAVALI: // b = 1/3, c = 1/3
        dspara.mode = DESCALE_MODE_BICUBIC;
        dspara.param1 = 1.0 / 3.0;
        dspara.param2 = 1.0 / 3.0;
        break;
    case ac::core::RESIZE_BICUBIC_0_60: // b = 0, c = 0.6
        dspara.mode = DESCALE_MODE_BICUBIC;
        dspara.param1 = 0.0;
        dspara.param2 = 0.6;
        break;
    case ac::core::RESIZE_BICUBIC_0_75: // b = 0, c = 0.75
        dspara.mode = DESCALE_MODE_BICUBIC;
        dspara.param1 = 0.0;
        dspara.param2 = 0.75;
        break;
    case ac::core::RESIZE_BICUBIC_0_100: // b = 0, c = 1.0
        dspara.mode = DESCALE_MODE_BICUBIC;
        dspara.param1 = 0.0;
        dspara.param2 = 1.0;
        break;
    case ac::core::RESIZE_BICUBIC_20_50: // b = 0.2, c = 0.5
        dspara.mode = DESCALE_MODE_BICUBIC;
        dspara.param1 = 0.0;
        dspara.param2 = 0.5;
        break;
    case ac::core::RESIZE_SOFTCUBIC50: // b = 1/2, c = 1/2
        dspara.mode = DESCALE_MODE_BICUBIC;
        dspara.param1 = 1.0 / 2.0;
        dspara.param2 = 1.0 / 2.0;
        break;
    case ac::core::RESIZE_SOFTCUBIC75: // b = 3/4, c = 1/4
        dspara.mode = DESCALE_MODE_BICUBIC;
        dspara.param1 = 3.0 / 4.0;
        dspara.param2 = 1.0 / 4.0;
        break;
    case ac::core::RESIZE_SOFTCUBIC100: // b = 1, c = 0
        dspara.mode = DESCALE_MODE_BICUBIC;
        dspara.param1 = 1.0 / 1.0;
        dspara.param2 = 0.0;
        break;
    case ac::core::RESIZE_LANCZOS2:
        dspara.mode = DESCALE_MODE_LANCZOS;
        dspara.taps = 2;
        break;
    case ac::core::RESIZE_LANCZOS3:
        dspara.mode = DESCALE_MODE_LANCZOS;
        dspara.taps = 3;
        break;
    case ac::core::RESIZE_LANCZOS4:
        dspara.mode = DESCALE_MODE_LANCZOS;
        dspara.taps = 4;
        break;
    case ac::core::RESIZE_SPLINE16:
        dspara.mode = DESCALE_MODE_SPLINE16;
        break;
    case ac::core::RESIZE_SPLINE36:
        dspara.mode = DESCALE_MODE_SPLINE36;
        break;
    case ac::core::RESIZE_SPLINE64:
        dspara.mode = DESCALE_MODE_SPLINE64;
        break;
    case ac::core::RESIZE_BILINEAR:
    default:
        dspara.mode = DESCALE_MODE_BILINEAR;
        break;
    }
    dspara.border_handling = DESCALE_BORDER_REPEAT;
    dspara.shift = 0.0;
    dspara.active_dim = dstW;
    dscoreH = dsapi.create_core(srcW, dstW, &dspara);
    dspara.active_dim = dstH;
    dscoreV = dsapi.create_core(srcH, dstH, &dspara);
}
Descale::~Descale() noexcept
{
    dsapi.free_core(dscoreH);
    dsapi.free_core(dscoreV);
}
void Descale::process(const ac::core::Image& src, ac::core::Image& dst) noexcept
{
    ac::core::Image temp{ dst.width(), src.height(), 1, ac::core::Image::Float32 };

    dsapi.process_vectors(dscoreH, DESCALE_DIR_HORIZONTAL, src.height(), src.stride() / sizeof(float), temp.stride() / sizeof(float), static_cast<float*>(src.ptr()), static_cast<float*>(temp.ptr()));
    dsapi.process_vectors(dscoreV, DESCALE_DIR_VERTICAL, dst.width(), temp.stride() / sizeof(float), dst.stride() / sizeof(float), static_cast<float*>(temp.ptr()), static_cast<float*>(dst.ptr()));
}
