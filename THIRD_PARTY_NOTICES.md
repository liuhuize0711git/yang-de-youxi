# Third-party notices

Lumenfall's original source, procedural artwork and synthesized sound are licensed under the MIT license in `LICENSE`. The third-party sources retain their own copyrights and licenses.

| Component | Pinned version | License / notice |
|---|---|---|
| SFML | 3.0.2 | zlib/png-style license; `licenses/SFML.txt` and `third_party/SFML/license.md` |
| FreeType | 2.13.2 | FreeType License; `licenses/FreeType.txt` and `third_party/freetype/LICENSE.TXT` |
| libogg | 1.3.5 | BSD-style license; `licenses/Ogg.txt` |
| libvorbis | 1.3.7 | BSD-style license; `licenses/Vorbis.txt` |
| libFLAC | 1.4.3 | Xiph BSD-style license for the library; `licenses/FLAC.txt` |
| MinGW-w64 runtime | Windows binary toolchain | See `licenses/MinGW-runtime.txt` |
| GCC / libstdc++ runtime | Windows binary toolchain | See `licenses/GCC-runtime.txt`; GCC Runtime Library Exception applies to the relevant runtime portions |

This software is based in part on the work of the FreeType Team.

SFML bundles additional third-party headers, including miniaudio, stb_image / stb_image_write, glad, dr_mp3, and Vulkan declarations. Their notices are retained in `third_party/SFML/extlibs` and in the individual source headers. SFML dependency CMake patches have already been applied to the bundled FreeType, Ogg, Vorbis and FLAC sources, as they are in an ordinary SFML source build. Apart from these upstream SFML build patches, these dependency trees are supplied as third-party sources.

Some dependency source trees include separately licensed tools, documentation and tests; their own license files apply to those files. The game uses the respective libraries.

The prebuilt Windows executable imports only standard Windows system DLLs. No assets from Sheepy: A Short Adventure are distributed with this project.
