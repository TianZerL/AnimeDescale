# AnimeDescale
A simple tool to guess and recover native resolution of Anime.


# Build
Build tools:
- CMake
- A C++20 compatible compiler

Dependency:
- Anime4KCPP
- CLI11
- hashpp

If you have internet access, CMake will automatically download and configure required dependencies.


# Usage
You should only input **1080P** images.

```shell
descale -i input1.png input2.png -o output_dir
```


# Descale

Descale implementation comes from [Irrational-Encoding-Wizardry](https://github.com/Irrational-Encoding-Wizardry/descale) with some modifications.
