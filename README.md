<h2 align="center"><b>HTTTRPG</b><br></h2>
<p align="center"><img src="https://pride-badges.pony.workers.dev/static/v1?label=trans%20rights&stripeWidth=6&stripeColors=5BCEFA,F5A9B8,FFFFFF,F5A9B8,5BCEFA">   <img alt="GitHub Repo stars" src="https://img.shields.io/github/stars/mildly-kelpish/Httrpg?style=flat">
<br><img src="https://raw.githubusercontent.com/mildly-kelpish/Httrpg/refs/heads/main/bitmap.png" height=250></p><br>

---
### usage!
> "truly it is a throw of the dice as to whether or not this works for you"<br>

**as for the server:**<br>
the server (portion written in python) is rather easy to set up
1. `git clone` this repo
2. modify the included config.toml to change what you need to change
3. in the cloned repo, `uv run uvicorn server:app` (that is, assuming you have uv)
OR <br>
1. grab the binary server from the release page
2. run it in a terminal

**as for the client:**<br>
either get a pre built binary from the releases(`HTttrpg-client` for linux `HTttrpg-client-cosmo` for windows and mac (compiled with cosmopolitan libc)) or build it yourself
1. if building yourself, `git clone` the repo and go into the client directory
2. get the following libraries and put them in a folder called `libraries/` in the client folder
   - [argy](https://github.com/mshenoda/argy)
   - [httplib](https://github.com/yhirose/cpp-httplib)
   - [json](https://github.com/nlohmann/json)
   - [toml](https://github.com/ToruNiina/toml11)
3. build the way you typically would any c++ project
   - personally, i used zig and the command was just `zig c++ main.cpp`
  
once you have the compiled client it has some configuration that must be done
* on linux, create a file named `config.toml` in the same folder as your client, there is an example in the client folder on this repo
* on windows (and presumably mac), you must use command line options, use --help for the options that must be used


