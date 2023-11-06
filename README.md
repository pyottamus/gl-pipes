# gl-pipes

# Dependencies

- [GLM](https://github.com/g-truc/glm)
- [GLFW3](https://github.com/glfw/glfw.git)
- [GLEW](https://github.com/nigels-com/glew)

# Warning

The next branch is unstable. It may get rebased and squashed.

# Build Instructions

All dependencies are Included as submodules. This angers Bill, our savior, as he gave unto us vcpkg. I apoligize for my transgressions and beg for your forgiveness, oh great creator(of DOS and NT, totally original products he came coded by himself).

## Non-Windows x86_64 Platforms

**NOTE** : This project has not been tested or built on anything but windows, the greatest of all operating systems.

As we all know, windows is not only the best operating system, but also the only one approved by the great and powerfull Bill. However, if you are some kind of uncivalized masachist, the following is a theorized process for building this project on some sort of *hypothetical* penguin-based toy-story-themed operating system.

No comments are added, as the process is so arcane and obstruse that any attemp to explain it would surely drive the reader insane.

```
sudo apt-get update
sudo apt-get install build-essentials git cmake
git clone https://github.com/pyottamus/gl-pipes.git
cd gl-pipes
mkdir build
cd build
cmake ../
cmake --build . --target gl_pipes
```

As you can see, building on platforms other than the great and powerfull Bill's is truely a nighmare.

## Windows x86_64

Windows, being the only operating system to be blessed by the creator of all that is good and not jank, Bill Gates, is far easier to build software on. By his emense loving-kindness, Bill has made it so that, with only a few dozen gigabytes of tooling, software can be easily compiled and run on his perfect platform.

### Required Tools

To build anything more complex than a hello world on Windows, you must first aquire the folowing:

- [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/). This contains the .lib files required to link against the DLLs already present on your system. It would be **_truely_** uncivalized to be able to link against DLLs without extra files that contain the symbol names already present in the DLLs now, wouldn't it.
- [MSBuild Toolset](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022). This comes preinstalled with visual studio. It is nessasary, even if you intend on using a far inferior compiler, such as clang, for it contains the ancient, all powerfull, batch file `vcvars64.bat`, without wich any attempt to build software will be fruitless.
- [CMake](https://cmake.org/). This software is needed to generate the nessasary build files. As I am not fluent in the language of the Gods, I cannot write a build file in the **_TRUE_** language of Windows development, Ancient Batch.
- [Git](https://git-scm.com/). This software is not _strictly_ nessasary(maybe?), as you can download from Git-Hub as a zip. **However**, as we all know, Git was won by Bill Gates in an chair highjump competition against the evil one(Linus Torvalds), and has now been bleessed by our savior, who has added many important fetures, such as AI that dosn't, has never, and will never, steal anyones IP.

### Building

After having installed all nessasary toolings, you must next donwload and build the project. On Windows, this is an extreemly simple and not at all overly complicated process.

- Create a folder somewhere and open a console in it.

- IF you happen to be using powershell, the modern console language for windows that has replaced cmd.exe, that is bad. **_NEVER_** use powershell. It is evil, and goes against the true intentions of our savior, Bill. In that heathen terminal, type cmd so that you may experience the original vision of our savior, Bill.

- Find the path to `vcvars64.bat`. If you installed the MSBuild tools as part of Visual Studio, and you're using Visual Studio 2022 Comunity, it will be in the folder `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build`. Otherwise, consult a local priest(MSDN).

- Run `vcvars64.bat` so you have the nessasary environment to build software. As we know, standard environment variables such as `INCLUDE`, `LIB`, and `CXX` are evil, and so arn't used on windows. In fact, `INCLUDE` is actively ignored by the blessed compiler, MSVC. **DO NOT** run `vcvars64.bat` more than once. This makes Bill, the all powerfull angry, and his wrath shall
  
  ```
  The input line is too long.
  The syntax of the command is incorrect.
  ```

- Use git as folows:
  
  ```
  git clone https://github.com/pyottamus/gl-pipes.git
  cd gl_pipes
  git checkout next
  ```
  
    This pleases Bill

- Next, perform the folowing
  
  ```
  mkdir build
  cd build
  cmake ../
  ```

- Now, you may open the visual studio files, right click on gl_pipes, and click "Set as Startup Project"