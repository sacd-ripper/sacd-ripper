Enhanced client sacd_extract for Windows(64bit) & Linux(64bit) & macOS by euflo

Version: 0.3.9.3 pre-release

This app is fully compatible with SACDExtractGui.

***************************************************************************************
Here are options and how to use sacd_extract with command prompt:
***************************************************************************************

Usage: sacd_extract [options] -i[=FILE]
Options:
  -2, --2ch-tracks                : Export two channel tracks (default)
  -m, --mch-tracks                : Export multi-channel tracks
  -e, --output-dsdiff-em          : output as Philips DSDIFF (Edit Master) file
  -p, --output-dsdiff             : output as Philips DSDIFF file
  -s, --output-dsf                : output as Sony DSF file
  -z, --dsf-nopad                 : Do not zero pad DSF (cannot be used with -t)
  -t, --select-track              : only output selected track(s) (ex. -t 1,5,13)
  -k, --concatenate               : concatenate consecutive selected track(s) (ex. -k -t 2,3,4)
  -I, --output-iso                : output as RAW ISO
  -w, --concurrent                : Concurrent ISO+DSF/DSDIFF processing mode
  -c, --convert-dst               : convert DST to DSD
  -C, --export-cue                : Export a CUE Sheet
  -o, --output-dir[=DIR]          : Output directory for ISO or DSDIFF Edit Master
  -y, --output-dir-conc[=DIR]     : Output directory for DSF or DSDIFF 
  -P, --print                     : display disc and track information
  -A, --artist                    : artist name is added in folder name. Default is disabled
  -a, --performer                 : performer name is added in track filename. Default is disabled
  -b, --pauses                    : all pauses will be included. Default is disabled
  -v, --version                   : Display version

  -i, --input[=FILE]              : set source and determine if "iso" image, 
                                    device or server (ex. -i 192.168.1.10:2002)

Help options:
  -?, --help                      : Show this help message
  --usage                         : Display brief usage message

  

Besides all the options found in modern sacd_extract, this version has new options which can be set at command prompt:

-A, --artist 		: artist name is added in folder name. Default is disabled;
-a, --performer 	: performer name is added in track filename. Default is disabled;
-b, --pauses		: all pauses will be included. Default is disabled. 
			  If pauses are disabled, all the audioframes that has the timecode outside the interval
		 	  [tracklist_time_start_track, tracklist_time_start_track+track_duration] will be discarded;
-k, --concatenate 	: concatenate consecutive selected tracks (ex. sacd_extract -k -t 2,3,4 ...-i 'iso file') 


******************************************************************************************
Configuration file 'sacd_extract.cfg'
******************************************************************************************

All above 4 options (artist, performer, pauses, concatenate) can be set in a configuration file 'sacd_extract.cfg' and will override settings from command prompt. 
Cofiguration file must be put in working directory (which can be shown by -v option or TEST button in SACDExtractGUI).
The presences of configuration file 'sacd_extract.cfg' is optional. 

The text lines can be:

artist=1	: artist name will be added at folder name. If =0 then artist name will not be added in folder name; 
performer=1	: performer name will be added in track filename. If =0 then performer name will not be included; 
pauses=1	: all pauses will be included. If =0 then pauses will not be included;
concatenate=1	: concatenation mode is activated. If =0 then concatenation mode is inactive.
 

Additional, there are 3 more options which can be set in 'sacd_extract.cfg' file:

nopad=1 	: padding-less option will be activated. If =0 then will be inactive. 		
		Can be used even when concatenation mode is activated and tracks selection is made;
		This option will override the padding-less option in SACDExtractGui. 
 
id3tag=0 (or =1, =2, =3, =4, =5)	at DSF conversion the ID3 tags will be created:
	-(id3tag=1) in ID3V2.3 version with full metadata. String encodings is UTF-16 with BOM (UCS-2);
	-(id3tag=2) in ID3V2.3 version with minimal medatadata*. String encodings is UTF-16 with BOM (UCS-2);
	-(id3tag=3) in ID3V2.3 version with full metadata. String encodings is ISO-8859-1 (ASCII);
	-(id3tag=4) in ID3V2.4 version with full metadata. String encodings is UTF-8;
	-(id3tag=5) in ID3V2.4 version with minimal metadata*. String encodings is UTF-8;
	-(id3tag=0) ID3tag will not be created (for testing and experimental purpose).
	Now the default format for ID3 tags will be ID3v2.4 format (with UTF-8 encodings). 
	
	*Minimal metadata contains only 4 elements: Track Title, Album Title, Artist Name and Track Number.


logging=1	:logging will be activated. All messages during execution of sacd_extract will be saved in 'logfile-sacd_extract.txt'.

 
For example a configuration file can contains text lines like this:
artist=0
performer=0
pauses=0
nopad=0
concatenate=0
id3tag=2
logging=1

Which means: no artist and no performer will be added in folder/filename, no pauses are included, padding-less option will be inactive, no concatenation mode, ID3tag will be created in ID3V2.3 version with minimal medatadata and logging will be active. 


**********************************************
Lists of improvements:
*********************************************

a) The app didn't hang with infinite loop 99% when iso files are corrupted;
b) added Performer frame id3 to dsf files;
c) multi-byte chars in filenames is supported for Windows when input files are iso (using wmain);
d) better management in creating folders/files/tracks names (including albums with multiple discs);
	folders are created like this: Album name/(Disc no)/(Stereo or 5ch or 6ch). 
	If configuration file sacd_extract.cfg exist in working folder of app (which can be shown with -v option) 
	and has text lines: 'artist=1' (and/or 'performer=1') then it adds artist at the name of folder and/or adds performer at the filename of tracks;
e) changed priority in title/artist extracting metadata. 
	Now disc_title/disc_artist came first instead of album_title/album_artist (useful in case of multiple discs of an album);
f) added -w (--concurrent) for compatibility with SACDExtractGUI;
g) added -o (--output-dir[=DIR] ) and -y (--output-dir-conc[=DIR]) options for setting explicit output folder (for compatibility with SACDExtractGUI);
h) eliminates crashes when disk for output is full;
i) corrected behavior in case of discs with only one multi-channel area (with no stereo area);
j) added total play time for each area when printing disc info;
k) now it strips out ending slash or backslash, if exists, at --output-dir or --output-dir-conc to be compatible with BatchEncoder;
l) added xml metadata file export option (is activated when Cue sheet is checked). Library libxml2 is used;
m) added more integrity checks of the audio frames;
n) added -z (--dsf-nopad) option, (now is fully compatible with SACDExtractGUI);
o) added 'pauses=0' option in sacd_extract.cfg. The pops/crackles noise is reduced by trimming frames at track boundaries;
	audio frames are trimmed out (based on timecode) at boundaries to reflect the correct playing time of tracks specified in Tracklist. 
	This is done for DSF/DFF files (no for DFF-EM);.
p) for dsf/dff option, audioframes counter is added for each tracks and compares with total duration to see if it misses one.
q) added concatenation option -k (--concatenate). Used in conjunction with track selection to concatenate selected tracks. Designed only for DSF format. 
	Also, concatenate option can be set in 'sacd_extract.cfg' file as 'concatenate=1'. 
	This allow to work ok with SACDExtractGui; 
	Padding-less option for concatenation can be set also in 'sacd_extract.cfg' (to override the same option in SACDExtractGui which is disabled when tracks 		selection is enabled). Example 'nopad=1';
r) ID3 tags are using now UTF-8 encoding;
s) ID3 tags can be created in ID3V2.3 version ('id3tag=1' or 2) or in ID3V2.4 version ('id3tag=4' or 5). 
	These tags can contain minimal metadata ('id3tag=2' or 5) or full metadata ('id3tag=1' or 4). 
	ID3tags can be eliminated using id3tag=0.
t) now 'artist' -A (--artist), 'performer' -a (--performer) and 'pauses' -b (--pauses) options can be declared in command line.



*********************************
Notes on DSF pops/crackles:
*********************************
Some players are unhappy when playing dsf files and generates louder pops/crackles at transitions between tracks.
This is specially very annoying when tracks has no pauses in between and the level of audio signal is high.

Causes are multiple:
- dsf files has ID3tags located at the end of file and players spents more time seeking and reading at the end of file then came back at the beginning of file to start playing;
- huge ID3tags with lots of information to process at playing time;
- the last audio blocks (with fixed size of 4096 bytes) are filled with padding zero bytes after audio data ends;
- dsf files have incorrect data in dsd header or audio streams;
- dsd audio data stream is improper processed by players; 

To reduce pops/crackles you can try made DSF files:
-  with minimal ID3V2.3 tags (id3tag=2 option), the benefit is smaller ID3 data to process; 
-  with minimal ID3V2.4 tags (id3tag=5 option), here the benefit is using syncsafe integers (of ID3 frame size) which will not interfere with audio data stream;
-  with no ID3 tags (only to experiment) (id3tag=0 option);
-  with padding-less option activated (nopad=1); 
	The app translates the remains of audio data in the last incompletely audio block to the beginning of the next track; 
	In concatenation mode the last incomplete audio bock is discarded (so max 13 milsec of audio data is lost);
-  with concatenation mode activated (concatenate=1); The app joins selected tracks. The metadata from the first track in list is used.


*****************************************
Working in conjunction with BatchEncoder

*****************************************
 
If you are interested in converting very rapid a lots of iso's in dsf format, you can use:

BatchEncoder

https://wieslawsoltes.github.io/BatchEncoder/



Settings for BatchEncoder:

a) In Preset, at Default in command line options:

-2 -s  (for Stereo and DSF format output).

It can be addded more presets like those:

-m -s (for Multichannel and DSF format output);

-2 -e -C -c -i (stereo, dff edit master, cue sheet, dst decompress)....

b) In Format, in command line template:

$EXE $OPTIONS -i $INFILE -o $OUTPATH

 
c) In progress function lua :

function GetProgress(s)
  if string.match(s, 'We are done exporting') ~= nil then return "100";
  else return string.match(s, 'Total: (%d+)%%'); end;
end

 

Note: The working folder of BatchEncoder and the apps  is C:\Users\username\AppData\Roaming\BatchEncoder\
In this folder can be put 'sacd_extract.cfg' file.





 








 
 
