# Running the renderer server
Why is this in the FFL-Testing repo under another branch? This server is built off of the FFL-Testing sample by Abood (or in other words, hackily patched from it).
1. Follow the instructions included below, which are also in master, in order
    * I don't think the server portion will work at all on a Wii U, so don't try.
    * Get to the point where it shows you spinny Miis.
2. Run either the Python or the Go server.
    * Python
        - Needs Flask
        - Run `server-impl/ffl-testing-web-server-old.py`
        - (Note that the Python server is limited in features and performance)
    * Go
        - Inside `server-impl` just `go run .` or run the Go file
3. Make sure FFL-Testing is running and listening, and try a request like this to the server: http://localhost:5000/miis/image.png?data=005057676b565c6278819697bbc3cecad3e6edf301080a122e303a381c235f4a52595c4e51494f585c5f667d848b96&width=512
4. If that works, great! Keep these in mind:
    - If you are hosting this, **always use the SERVER_ONLY=1 environment variable when running.**
        * Hides the window, doesn't swap buffers, and pauses when idle.
    - If you are running this on a VPS, build with **OSMesa support** so that you don't have to run an X11 server.
        * Build and also apply optimizations: ``CXXFLAGS="-O3 -march=native" DEFS="-DRIO_USE_OSMESA" make``
        * Note that OSMesa does **NOT support hardware acceleration**, and should only be used if you don't have a GPU! If you do, you'll need to constantly run Xvfb or something.


**Please be aware of the limitations to this in production - probably won't scale well.**
    * TBD: Improved accuracy (proper body rendering), server rewrite, multi-threading, all-in-one renderer and HTTP server in the same binary.


Note: For the frontend on mii-unsecure.ariankordi.net, I am using [nwf-mii-cemu-toy, ffl-renderer-proto-integrate branch](https://github.com/ariankordi/nwf-mii-cemu-toy/tree/ffl-renderer-proto-integrate) as the HTTP server for now, where I've more or less copied the Go server. It supports load balancing to multiple instances of this server.

# FFL-Testing
Sample using [the FFL decomp](https://github.com/aboood40091/ffl) and [RIO framework](https://github.com/aboood40091/rio), all originally written by [AboodXD](https://github.com/aboood40091), to render Miis on PC and Wii U via WUT.

https://github.com/user-attachments/assets/cdc358e4-0b1f-4b93-b7fd-f61fa4d215e1

## Running
1. Clone the repo, _recursively_.
    ```
    git clone --recursive https://github.com/ariankordi/FFL-Testing
    ```
2. Install requirements.

    This project needs GLFW3, zlib, and OpenGL dev libraries. The commands below will install them.

    * Ubuntu/Debian: `sudo apt install libglfw3-dev zlib1g-dev libgl1-mesa-dev`
    * Fedora/RHEL: `sudo dnf install glfw-devel zlib-devel mesa-libGL-devel`
    * Arch/Manjaro: `sudo pacman -S glew glfw zlib` (⚠️ UNTESTED)
    * MSYS2 MINGW64 (Windows): `pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-zlib`
        - Make sure to also install the basics: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-pkg-config`
        - **Use `mingw32-make` instead of `make`**
        - As of 2024-06-02, this doesn't copy DLLs from MinGW64's environment to yours so you can launch the program in the command line, you can't click on the exe to run it.
        - _Also note that it should build with MSVC, but I don't have a cxxproj for you to use / this should be ported to CMake at some point..._

    If you are building with WUT, you need to install `ppc-zlib`.
3. Build using the makefile.

    If you don't already have them, you will also need: git, g++, make, pkg-config.
    ```
    make -j4
    ```
    * To build with WUT, use: `make wut -j4`.

    * This also supports OpenGL ES 3.0, for mobile devices or using ANGLE. To build targeting that, define this as an environment variable: ``DEFS="-DRIO_GLES"``
    * To build for Emscripten, use ``make -f Makefile.emscripten -j4``
      - Make sure FFLResHigh.dat is in the current working directory. After this builds, open the html file.
3. Obtain the resource file, FFLResHigh.dat.
    * You can get it from many sources:
        - It can be extracted from a Wii U:
            - `sys/title/0005001b/10056000/content/FFLResHigh.dat`
        - You can extract it from a Miitomo install (_it's not in the APK/IPA_), or download it from archive.org:
            * https://web.archive.org/web/20180502054513/http://download-cdn.miitomo.com/native/20180125111639/android/v2/asset_model_character_mii_AFLResHigh_2_3_dat.zip
            * Extract the above and rename AFLResHigh_2_3.dat to FFLResHigh.dat.
        - (As well as AFLResHigh_2_3.dat, AFLResHigh.dat will work too)
    * Place that file in the root of this repo.
        - This file contains models and textures needed to render Miis and this program will not work without it.
4. Run `ffl_testing_2_debug64`, and pray that it works.
    * If it crashes with a segfault or shows you a blank screen, make sure you have `FFLResHigh.dat` and it can open it.

### Showing your own Miis
As of 2024-06-14, I added a change that would let you change which Miis this program renders, by reading files in the `place_ffsd_files_here` folder.

As the name implies, you should be able to take any Mii in a 96 byte FFSD/FFLStoreData file, and place it in that folder.

* Only the length of the file is checked, not the extension or name. It needs to be 96 bytes.

* Then, it will just read each file sequentially, looping through all of them.
* You can also just put one file there and that one Mii will spin continuously.

#### If you don't have any FFSD files...
* You can search for any *.ffsd or *.cfsd (CFLStoreData = FFLStoreData) file in this repo:
    - https://github.com/HEYimHeroic/MiiDataFiles

* Grab the Mii from your Nintendo Network ID using the mii-unsecure.ariankordi.net API, if it is still up by the time you are reading this.
    - `curl --verbose --header "Accept: application/octet-stream" https://mii-unsecure.ariankordi.net/mii_data/JasmineChlora --output JasmineChlora.ffsd`
        * Replace `JasmineChlora` with your own NNID, of course.
* I believe this script decrypts Mii QR Code data directly to FFSD/CFSD format: https://gist.github.com/jaames/96ce8daa11b61b758b6b0227b55f9f78
    - I use it with ZBar like so: `zbarimg --quiet --raw --oneshot -Sbinary image.jpg | python3 mii-qr.py /dev/stdin ./mii.ffsd`
        * Replace: `image.jpg`, `mii.ffsd`
    - I also made a scanner in JS, though it gives you Base64 that you have to decode: https://jsfiddle.net/arian_/h31tdg7r/1/

### Compiling GLFW3
<details>
<summary>
GLFW3 is needed to run any RIO application on PC, which this is.

If you can't install it (not on Linux or MSYS2), or need the latest version, let's build it.

**(You probably don't need to follow these)**
</summary>

### GLFW3:
* `git clone https://github.com/glfw/glfw && cd glfw`
* `cmake -S . -B build`
	- If you are cross compiling, append: `-D CMAKE_TOOLCHAIN_FILE=CMake/x86_64-w64-mingw32.cmake -D CMAKE_INSTALL_PREFIX=/usr/local/x86_64-w64-mingw32/`
* `cmake --build build -j8`
* (sudo) `cmake --install build`
### Now it should be available to pkg-config
Try: `pkg-config --libs zlib glfw3`

(Unless it complains about needing `glu`)
#### If you are still reading
NOTE from 2024-06-02: To cross compile this from Linux to Windows, I used the following command:
`
TOOLCHAIN_PREFIX=x86_64-w64-mingw32- make LDFLAGS="-L/dev/shm/glfw/build/src/ -lz -L/dev/shm/glew/lib/ -lglew32 -lglfw3 -lopengl32 -lgdi32 -lws2_32
`

Where I have glew and glfw built at /dev/shm.

While pkg-config worked, letting me need only the TOOLCHAIN_PREFIX set, for whatever reason it wasn't building and threw lots of linking errors saying it couldn't link tons of symbols from glew32 even though it literally finds it and opens the library, so... IDK.
</details>
