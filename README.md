# compactxf
compact with excluded files

Windows 10 has a nice tool called "compact.exe" which can compress files with advanced compression algorithms, and not just the standard old NTFS compression. This means, you can compress various games or folders with compressable files (i.e. text) transparently, i.e. you won't actually know it's compressed.

The problem is you can only give it wildcards, i.e. `*.*`, or specific files. Some files are already compressed in a way that shouldn't even be tried to be compressed, like videos, music, pictures.

So this is a little wrapper, which uses an exclude list next to the exe, in a textfile named "compactxf.txt" which includes a list of extensions that should be ignored, one extension per line.

For example:

```
png
mkv
mp4
jpg
mp3
m4a
```

For compressing a folder, use the following syntax:

`compactxf c folder`

It will compress all files in that folder recursively, excluding files in the ignore list.

You can optionally choose the compression level, which is documented here: https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/compact

The options compactxf knows are `4/8/16/lzx`, which should self explanatory.

The default is `XPRESS16K`, which compresses Killing Floor 2 from ~80Gb to ~40GB. That's almost 50% space saved, at almost no runtime cost. `LZX` is slightly better, but more CPU intensive, and I've noticed some texture load problems when using this.

For uncompressing a folder, use the following syntax:

`compactxf u folder`

This uncompressed all files in that folder recursively.

If you noticed some files that shouldn't be compressed while the program is running, you can add more to your list, and then run

`compactxf i folder`

which will uncompress only files on the ignore list in a specific folder, recursively.
