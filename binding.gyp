{
    "targets": [
        {
            "target_name": "ag_images",
            "dependencies": [
                "./deps/libpng.gyp:libpng"
            ],
            "cflags": [
                "-Wall",
                "-Wno-unused-parameter",
                "-Wno-missing-field-initializers",
                "-Wextra",
                "-O3",
                "-fno-strict-aliasing",
                "-fvisibility=hidden",
                "-DFPNG_NO_STDIO",
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "./deps/libpng",
                "./deps/zlib"
            ],
            "sources": [
                "./src/main.cpp"
            ],
            'conditions': [
                ['OS=="linux" and target_arch=="x64"', {
                    'cflags': [
                        '-msse4.1',
                        '-mpclmul'
                    ]
                }],
                ['OS!="linux" or target_arch!="x64"', {
                    'cflags': [
                        '-DFPNG_NO_SSE'
                    ]
                }]
            ]
        }
    ]
}
