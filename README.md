# <img src="resources/AppIcon.png" valign="middle" alt="Icon" width="50" height="50"> Automator #

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/automator/blob/master/README.md)

Introduction
------------

<img src="resources/Automator.png" style="padding-bottom: 20px;" />

Automator is a user-friendly Mac application designed for batch processing files according to predefined job descriptions. It enables the creation of versatile tool chains to facilitate drag-and-drop processing, streamlining workflows for efficiency and ease of use. The application boasts a straightforward drag-and-drop interface for file submission, complemented by a dedicated log window. This window provides real-time monitoring of processing progress, ensuring users can track each step of their workflow with clarity and precision.

|  Download        | Description |
| ----------------| ----------- |
|  [<img src="resources/Download.png" valign="middle" alt="Icon" width="16" height="16"> Automator v1.0.03](https://github.com/mikaelsundell/automator/releases/download/release-v1.0.0/Automator_macOS12_arm64_release.dmg) | [Apple Silicon macOS12+](https://github.com/mikaelsundell/automator/releases/download/release-v1.0.0/Automator_macOS12_arm64_release.dmg)
|  [<img src="resources/Download.png" valign="middle" alt="Icon" width="16" height="16"> Automator v1.0.0](https://github.com/mikaelsundell/automator/releases/download/release-v1.0.0/Automator_macOS12_x86_64_release.dmg) | [Intel x86_64 macOS12+](https://github.com/mikaelsundell/automator/releases/download/release-v1.0.0/Automator_macOS12_x86_64_release.dmg)


Documentation
-------------

**Getting Started**

Begin by selecting a preset, then drag and drop your files onto the designated file drop area within Automator. The application will automatically commence processing your files in accordance with the chosen preset's specifications and associated tasks.

**Understanding preset file format**

A preset file articulates a sequence of tasks to be executed during file processing. It allows for the specification of commands, file extensions, arguments, and initial directories. Additionally, it supports the definition of dependencies among tasks using the dependson attribute, enabling complex processing chains.

Here's an example of a preset file format, tailored for converting camera RAW files:


```shell
{
  "name": "Convert Camera RAW",
  "tasks": [
    {
      "id": "@1",
      "name": "Convert Camera RAW",
      "command": "/Volumes/Build/pipeline/bin/dcraw_emu",
      "extension": "%inputext%.tiff",
      "arguments": "-T -4 -h -W -o 7 -q 3 -H 0 -b 1.0 -6 -v %inputfile%",
      "startin": "/Volumes/Build/pipeline/bin",
      "documentation": [
        "-T, write TIFF instead of PPM",
        "-4, Linear 16-bit",
        "-h, Half-size color image",
        "-w, Don't automatically brighten the image",
        "-o, Output colorspace is DCI-P3",
        "-q, AHD interpolation preserving the homogeneity of the color regions",
        "-H, Clip highlights",
        "-b, Adjust brightness standard 1.0",
        "-6, Write 16-bit output"
      ]
    },
    {
      "name": "Change the name to 16-bit tiff only",
      "extension": "tiff",
      "command": "mv",
      "arguments": "%inputfile%.tiff %outputdir%/%outputbase%.%outputext%",
      "dependson": "@1"
    }
    
  ]
}

```


**Supported Variables**

Preset files support various variables that can be used to customize arguments during processing. These variables are dynamically replaced based on the context of the input and output files.

**Input Variables:**

```shell
%inputdir%         Replaces the variable with the directory path of the input file.
%inputfile%        Replaces the variable with the full path of the input file.
%inputext%         Replaces the variable with the file extension of the input file.
%inputbase%        Replaces the variable with the base name (filename without extension) of the input file.
````

**Output Variables:**

```shell
%outputdir%        Replaces the variable with the directory path of the output file.
%outputfile%       Replaces the variable with the full path of the output file.
%outputext%        Replaces the variable with the file extension of the output file.
%outputbase%       Replaces the variable with the base name of the output file.
````

Each variable is designed to simplify the scripting and automation within preset configurations, ensuring that file paths and details are handled efficiently without manual specification in every command.


Automator Advanced
--------

## Build configuration ##

To initiate the build process, use the following command in your terminal, specifying either debug or release mode according to your requirements:


```shell
./build.sh debug|release
```

For deployment into a Disk Image (DMG), include the --deploy flag along with your selected build mode. This step packages the application into a DMG file, suitable for distribution or installation on macOS systems:

```shell
./build.sh debug|release --deploy
```

Web Resources
-------------

* GitHub page:        https://github.com/mikaelsundell/automator
* Issues              https://github.com/mikaelsundell/automator/issues

Copyright
---------

**3rdparty libraries acknowledgment and copyright notice**

This product includes software developed by third parties. The copyrights and terms of use of these third-party libraries are fully acknowledged and respected. Below is a list of said libraries and their respective copyright notices:

App icon: Copyright flaticon.com

The Qt Company Ltd.: Copyright (C) 2016 by The Qt Company Ltd. All rights reserved.

The use of the above-mentioned software within this product is in compliance with the respective licenses and/or terms and conditions of the copyright holders. The inclusion of these third-party libraries does not imply any endorsement by the copyright holders of the products developed using their software.

