https://github.com/user-attachments/assets/6b408c3b-9b55-430b-aa07-9ec7ea397ddf \
###**About**\
It take any media as input and convert to ascii art. Works any picture, animation or videos. It has mutliple-threading which helps speedup processing.\

###**How to use**\
Please download the [Release](releases/release-0.1v) both media_to_asciiart and one .ttf.\
Then give executable:\
`chmod +x media_to_asciiart`\
Then finally you can run:\
`./media_to_asciiart -input yourmediahere.mp4 --style-path yourttfpathhere.ttf`\
Output will in finish directory. Enjoy.\

###**Explain the Options**\
- `-input` Input any media. Default: NULL\
- `--quality <number>` Higher number means more details but will increase resolution, be careful with it. Default: 240\
- `--mode <option>` Pick ascii type. Four options: simple, detailed, reverse_simple and reverse_detailed. Default: detailed\
- `--fps <number>` Set fps of output. Default: 24\
- `--type <file type>` Pick different output file type than input if you wish otherwise pick same type from input. Available options: gif, mp4 and webm. Default: NULL\
- `--style-path <path>` Select .ttf for ascii type. Requirement to add. Default: NULL\
