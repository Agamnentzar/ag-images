{
    "targets" : [
        {
            "target_name" : "zlib",
            "type" : "static_library",
            "cflags" : [],
            "include_dirs": [
                "config/<(OS)/",
                "zlib"
            ],
            "defines": [
                "HAVE_STDARG_H"
            ],
            "conditions": [
                ['OS=="win"', {
                    'defines': [
                        'ZLIB_WINAPI'
                    ]
                }, {
                    'defines': [
                        'HAVE_UNISTD_H'
                    ]
                }]
            ],
            "sources" : [
                "zlib/adler32.c",
                "zlib/compress.c",
                "zlib/crc32.c",
                "zlib/deflate.c",
                "zlib/gzclose.c",
                "zlib/gzlib.c",
                "zlib/gzread.c",
                "zlib/gzwrite.c",
                "zlib/infback.c",
                "zlib/inffast.c",
                "zlib/inflate.c",
                "zlib/inftrees.c",
                "zlib/trees.c",
                "zlib/uncompr.c",
                "zlib/zutil.c"
            ]
        }
    ]
}
