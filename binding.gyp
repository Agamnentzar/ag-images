{
    "targets" : [
        {
            "target_name" : "ag_images",
            "dependencies" : [
                "./deps/libpng.gyp:libpng"
            ],
            "cflags" : [
                "-Wall",
                "-Wno-unused-parameter",
                "-Wno-missing-field-initializers",
                "-Wextra",
                "-O3"
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "./deps/libpng",
                "./deps/zlib"
            ],
            "sources" : [
                "./src/main.cpp"
            ]
        }
    ]
}
