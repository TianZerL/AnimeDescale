#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

#include <hashpp.h>

#include <AC/Core/Image.hpp>
#include <AC/Util/Parallel.hpp>

#include "Options.hpp"
#include "Descale.hpp"

std::unordered_map<int, const char*> ModeNameMap{
    {ac::core::IMRESIZE_POINT, "POINT"},
    {ac::core::IMRESIZE_CATMULL_ROM, "CATMULL_ROM"},               // b = 0, c = 0.5 or a = -0.5
    {ac::core::IMRESIZE_MITCHELL_NETRAVALI, "MITCHELL_NETRAVALI"}, // b = 1/3, c = 1/3
    {ac::core::IMRESIZE_BICUBIC_0_60, "BICUBIC_0_60"},             // b = 0, c = 0.6 or a = -0.6
    {ac::core::IMRESIZE_BICUBIC_0_75, "BICUBIC_0_75"},             // b = 0, c = 0.75 or a = -0.75
    {ac::core::IMRESIZE_BICUBIC_0_100, "BICUBIC_0_100"},           // b = 0, c = 1 or a = -1
    {ac::core::IMRESIZE_BICUBIC_20_50, "BICUBIC_20_50"},           // b = 0.2, c = 0.5
    {ac::core::IMRESIZE_SOFTCUBIC50, "SOFTCUBIC50"},               // b = 0.5, c = 0.5
    {ac::core::IMRESIZE_SOFTCUBIC75, "SOFTCUBIC75"},               // b = 0.75, c = 0.25
    {ac::core::IMRESIZE_SOFTCUBIC100, "SOFTCUBIC100"},             // b = 1, c = 0
    {ac::core::IMRESIZE_LANCZOS2, "LANCZOS2"},
    {ac::core::IMRESIZE_LANCZOS3, "LANCZOS3"},
    {ac::core::IMRESIZE_LANCZOS4, "LANCZOS4"},
    {ac::core::IMRESIZE_SPLINE16, "SPLINE16"},
    {ac::core::IMRESIZE_SPLINE36, "SPLINE36"},
    {ac::core::IMRESIZE_SPLINE64, "SPLINE64"},
    {ac::core::IMRESIZE_BILINEAR, "BILINEAR"},
};

static double mae(const ac::core::Image& a, const ac::core::Image& b)
{
    constexpr int crop = 5;
    constexpr double threshold = 0.015;
    double diff = 0.0;
    for (int i = crop; i < a.height() - crop; i++)
    {
        for (int j = crop; j < a.width() - crop; j++)
        {
            double temp = std::abs(*static_cast<const float*>(a.ptr(j, i)) - *static_cast<const float*>(b.ptr(j, i)));
            diff += temp > threshold ? temp : 0.0;
        }
    }
    return diff / ((a.height() - 2 * crop) * (a.width() - 2 * crop));
}

static std::pair<int /* height - lo */, double /* ratio */> findBestGuess(const std::vector<double>& diff)
{
    std::vector<double> ratios{0.0f};

    int idx = 0;
    double maxRatio = 0.0;
    for (std::size_t i = 1; i < diff.size(); i++)
    {
        auto last = diff[i - 1];
        auto curr = diff[i];
        if (last < std::numeric_limits<double>::epsilon()) last += std::numeric_limits<double>::epsilon();
        auto ratio = last / curr;
        ratios.emplace_back(ratio);
        if (ratio > maxRatio)
        {
            maxRatio = ratio;
            idx = i;
        }
    }

    return { idx, maxRatio };
}

static std::pair<int /* height */, int /* frequency */> findMostFrequent(const std::vector<int>& vec) {
    std::unordered_map<int, int> frequency{};

    for (int num : vec) frequency[num]++;

    auto maxElement = std::max_element(
        frequency.begin(),
        frequency.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

    return { maxElement->first, maxElement->second };
}

static void analyze(const ac::core::Image& src, int lo, int hi, const std::vector<int>& modeList, int& bestW, int& bestH, int& bestMode)
{
    std::vector<int> heightList{};
    std::unordered_map<int, std::vector<double>> diffMap{};

    for (auto mode : modeList) diffMap.emplace(mode, hi - lo);

    ac::util::parallelFor(lo, hi, [&](int i) {
        int dstW = static_cast<int>(i * (static_cast<double>(src.width()) / static_cast<double>(src.height())) + 0.5);
        int dstH = i;
        ac::core::Image dst{ dstW, dstH, 1, ac::core::Image::Float32 };
        ac::core::Image res{ src.width(), src.height(), 1, ac::core::Image::Float32 };
        for (auto mode : modeList)
        {
            Descale descale{ src.width(), src.height(), dstW, dstH, mode };
            descale.process(src, dst);
            ac::core::resize(dst, res, 0.0, 0.0, mode);

            int idx = i - lo;
            diffMap[mode][idx] = mae(src, res);
        }
        });

    for (auto mode : modeList)
    {
        auto bestGuess = findBestGuess(diffMap[mode]);
        auto height = bestGuess.first + lo;
        heightList.emplace_back(height);

        printf("mode: %-20s, height: %d, diff: %lf, relative_diff: %lf\n", ModeNameMap[mode], height, diffMap[mode][bestGuess.first], bestGuess.second);
    }

    auto mostFrequent = findMostFrequent(heightList);
    if (mostFrequent.second > 1) bestH = mostFrequent.first;
    else
    {
        std::sort(heightList.begin(), heightList.end());
        bestH = heightList[heightList.size() / 2];
    }
    bestW = static_cast<int>(bestH * (static_cast<double>(src.width()) / static_cast<double>(src.height())) + 0.5);
    // Select the algorithm with the smallest difference as the final guess.
    double minDiff = 255.0;
    for (auto mode : modeList)
    {
        auto diff = diffMap[mode][bestH - lo];
        if (diff < minDiff)
        {
            minDiff = diff;
            bestMode = mode;
        }
    }
}

int main(int argc, char* argv[])
{
    auto options = parse(argc, argv);

    constexpr int chunkSize = 8;

    std::size_t totalImages = options.inputs.size();
    std::size_t testImages = totalImages > chunkSize ? std::lround(totalImages / static_cast<double>(chunkSize)) : 1;

    int lo = 700;
    int hi = 1000;

    const std::vector<int> resizeModeList{
        ac::core::IMRESIZE_CATMULL_ROM,
        ac::core::IMRESIZE_MITCHELL_NETRAVALI,
        ac::core::IMRESIZE_BICUBIC_0_75,
        ac::core::IMRESIZE_BICUBIC_0_100,
        ac::core::IMRESIZE_BICUBIC_20_50,
        ac::core::IMRESIZE_SOFTCUBIC50,
        ac::core::IMRESIZE_LANCZOS2,
        ac::core::IMRESIZE_LANCZOS3,
        ac::core::IMRESIZE_SPLINE16,
        ac::core::IMRESIZE_SPLINE36,
        ac::core::IMRESIZE_BILINEAR,
    };

    std::vector<int> heightList{};

    std::vector<int> bestWList(testImages);
    std::vector<int> bestHList(testImages);
    std::vector<int> bestModeList(testImages);

    int bestW = 0, bestH = 0, bestMode = ac::core::IMRESIZE_BILINEAR;
    int srcW = 0, srcH = 0;

    std::printf("total images: %zu\n", totalImages);

    std::random_device rd{};
    std::mt19937 gen{ rd() };
    std::uniform_int_distribution<int> dist{ 0, chunkSize - 1 };

    for (int i = 0; i < testImages; i++)
    {
        int idx = dist(gen) + chunkSize * i;
        auto testImagePath = options.inputs[idx < totalImages ? idx : totalImages - 1].c_str();
        std::printf("image for testing: %s\n", testImagePath);

        auto img = ac::core::imread(testImagePath, ac::core::IMREAD_GRAYSCALE);
        if (img.empty())
        {
            std::printf("failed to load image for testing.\n");
            return 0;
        }

        img = ac::core::astype(img, ac::core::Image::Float32);
        if (srcW <= 0 || srcH <= 0)
        {
            srcW = img.width();
            srcH = img.height();
        }
        analyze(img, lo, hi, resizeModeList, bestWList[i], bestHList[i], bestModeList[i]);

        std::printf("best guess for this one:\n  resolution: %d x %d, mode: %s\n", bestWList[i], bestHList[i], ModeNameMap[bestModeList[i]]);
    }

    int bestSizeIdx = 0;
    bestH = bestHList[bestSizeIdx];
    for(int i = 0; i < bestHList.size(); i++)
        if (bestHList[i] < bestH)
        {
            bestH = bestHList[i];
            bestSizeIdx = i;
        }
    bestW = bestWList[bestSizeIdx];
    bestMode = findMostFrequent(bestModeList).first;

    std::printf("best guess for all:\n  resolution: %d x %d, mode: %s\n", bestW, bestH, ModeNameMap[bestMode]);

    if (!options.detectMode)
    {
        auto outputDir = options.outputDir.empty() ? std::filesystem::current_path() : std::filesystem::path{options.outputDir};
        std::filesystem::create_directories(outputDir);

        std::string id =  hashpp::get::getHash(hashpp::ALGORITHMS::MD5, std::filesystem::path{ options.inputs.front() }.parent_path().filename().string());
        id.resize(8);

        ac::core::Image yout{}, uout{}, vout{}, aout{};
        Descale descale{ srcW, srcH, bestW, bestH, bestMode };

        std::printf("output to: %s\n", outputDir.generic_string().c_str());

        for (int i = 0; i < totalImages; i++)
        {
            auto img = ac::core::imread(options.inputs[i].c_str(), options.gray ? ac::core::IMREAD_GRAYSCALE : ac::core::IMREAD_UNCHANGED);
            if (!img.empty())
            {
                ac::core::Image y{}, u{}, v{}, a{}, out{};
                img = ac::core::astype(img, ac::core::Image::Float32);

                if (img.channels() > 1)
                {
                    if (img.channels() == 4) ac::core::rgba2yuva(img, y, u, v, a);
                    else  ac::core::rgb2yuv(img, y, u, v);
                }
                else y = img;

                if (yout.empty())
                    yout.create(bestW, bestH, 1, ac::core::Image::Float32);
                descale.process(y, yout);

                if (!u.empty())
                {
                    if (uout.empty())
                        uout.create(bestW, bestH, 1, ac::core::Image::Float32);
                    descale.process(u, uout);
                }

                if (!v.empty())
                {
                    if (vout.empty())
                        vout.create(bestW, bestH, 1, ac::core::Image::Float32);
                    descale.process(v, vout);
                }

                if (!a.empty())
                {
                    if (aout.empty())
                        aout.create(bestW, bestH, 1, ac::core::Image::Float32);
                    descale.process(a, aout);
                }

                if (img.channels() > 1)
                {
                    if (img.channels() == 4) ac::core::yuva2rgba(yout, uout, vout, aout, out);
                    else  ac::core::yuv2rgb(yout, uout, vout, out);
                }
                else out = yout;

                out = ac::core::astype(out, ac::core::Image::UInt8);
                if (options.crop && (out.height() > 720)) // crop to 720p
                    out = ac::core::crop(out, (out.width() - 1280) / 2, (out.height() - 720) / 2, 1280, 720);
                ac::core::imwrite((outputDir / std::format("frame_{}p_descaled_{}_{:04d}.png", bestH, id, i + 1)).string().c_str(), out);
            }
        }
    }
    return 0;
}
