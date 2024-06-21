package("supercell_compression")
    set_homepage("https://github.com/Chi-EEE/SupercellCompression")
    set_description("C++ library for compressing and decompressing Supercell assets")
    set_license("MIT")

    add_urls("https://github.com/Chi-EEE/SupercellCompression.git")
    add_versions("2024.06.21", "b358bc2f3ab595a3b0dcf14675b423b3fe15cb42")

    add_deps("lzham_codec", "lzma", "zstd", "astc-encoder")
    add_deps("supercell_core")

    on_install(function (package)
        local configs = {}
        import("package.tools.xmake").install(package, configs)
    end)
