[![Language](https://img.shields.io/badge/language-C-blue)](#)
[![Platform](https://img.shields.io/badge/platform-Linux-green)](#)

https://github.com/user-attachments/assets/93596272-45ef-435a-baa0-45eb01baf34f

^ click fullscreen and right click to enable loop for better view. (warning may hurt your eyes)
### **About**
It take any media as input and convert to ascii art. Works any picture, animation or videos. It has mutliple-threading which helps speedup processing.

### **How to use**
Please download the [Release](https://github.com/Keeze392/media_to_ascii_art/releases/tag/release-0.1v) both media_to_asciiart and one .ttf. \
Then give executable: \
`chmod +x media_to_asciiart` \
Then finally you can run: \
`./media_to_asciiart -input yourmediahere.mp4 --style-path yourttfpathhere.ttf` \
Output will in finish directory. Enjoy.

Please remember if your file exist in finish directory, it will overwrite.

### **Explain the Options**
- `-input` Input any media. Default: NULL
- `--quality <number>` Higher number means more details but will increase resolution, be careful with it. Default: 240
- `--mode <option>` Pick ascii type. Four options: simple, detailed, reverse_simple and reverse_detailed. Default: detailed
- `--fps <number>` Set fps of output. Default: 24
- `--type <file type>` Pick different output file type than input if you wish otherwise pick same type from input. Available options: gif, mp4 and webm. Default: NULL
- `--style-path <path>` Select .ttf for ascii type. Requirement to add. Default: NULL

### **Used library credits**
- [freetype](https://github.com/freetype/freetype/tree/738905b34bd1f5a8ff51bd2bc8e38a2d8be9bfd6)
- [DejaVu fonts](https://github.com/dejavu-fonts/dejavu-fonts)
- [ffmpeg](https://github.com/FFmpeg/FFmpeg)
