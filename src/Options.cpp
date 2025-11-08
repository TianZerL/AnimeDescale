#include <cstdlib>

#include <CLI/CLI.hpp>

#include "Options.hpp"

Options parse(const int argc, const char* const* argv) noexcept
{
    Options options{};
    CLI::App app{ "Anime4KCPP: A high performance anime upscaler." };

    app.add_option("-i,--input,input", options.inputs, "input files.")
        ->required()
        ->check(CLI::ExistingFile);
    app.add_option("-o,--output_dir", options.outputDir, "output dir.")
        ->check(CLI::ExistingDirectory);

    app.add_flag("-d,--detect_mode", options.detectMode, "detect without processing to save.");
    app.add_flag("-c,--crop", options.crop, "crop to 720P.");
    app.add_flag("-g,--gray", options.gray, "convert to gray.");

    try { app.parse(argc, argv); }
    catch (const CLI::ParseError& e) { std::exit(app.exit(e)); }

    return options;
}
