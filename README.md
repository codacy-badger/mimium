# mimium

a programming language as an infrastructure for sound and music

stable: [![build status(master)](https://github.com/mimium-org/mimium/workflows/build%20&%20test/badge.svg?branch=master)](https://github.com/mimium-org/mimium/actions) dev: [![build status(dev)](https://github.com/mimium-org/mimium/workflows/build%20&%20test/badge.svg?branch=dev)](https://github.com/mimium-org/mimium/actions) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/de5190beb61f4ea9a337becdb21f8328)](https://www.codacy.com/manual/tomoyanonymous/mimium?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=tomoyanonymous/mimium&amp;utm_campaign=Badge_Grade) 

[![website badge](https://img.shields.io/badge/mimium.org-Website-d6eff7)](https://mimium.org) [![Gitter](https://badges.gitter.im/mimium-dev/community.svg)](https://gitter.im/mimium-dev/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge) [![License Badge-MPL2.0](https://img.shields.io/badge/LICENSE-MPLv2.0-blue)](./LICENSE.md)

![mimium_logo_slanted](./mimium_logo_slant.svg)

mimium(*MInimal-Musical-medIUM*) is a domain specific programming language for describing/generating sound and music.

With this language, you can write a low-level audio processing with an easy expression and high-performance powered by LLVM.

```rust
fn lpf(input:float,fb:float){    
    return (1-fb)*input + fb*self
}
```

A special keyword `self` can be used in function, which is a last return value of the function.
This enables an easy and clean expression of feedback connection of signal chain, inspired by [Faust](https://faust.grame.fr).

you can also write a note-level processing by using a temporal recursion, inspired by [Extempore](https://extemporelang.github.io/).

```rust

fn noteloop()->void{
    freq =  (freq+1200)%4000
    noteloop()@(now + 48000)
}

```

Calling function with `@` specifies the time when the function will be executed.
An event scheduling for this mechanism is driven by a clock from an audio driver thus have a sample-accuracy.

<!-- More specific info about language is currently in [design](design/design-proposal.md) section. -->

# Installation

You can download a built binary from [release](https://github.com/mimium-org/mimium/releases) section.

mimium can run on macOS(x86), Linux(ALSA backend), Windows(WASAPI backend). WebAssemby backend will be supported for future.

On macOS and Linux, installation via [Homebrew](https://brew.sh/) is recommended.

Open your terminal application and type 

```sh
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)" 
```

to install homebrew itself. After installation, you can install with the commands below.

```sh
brew tap mimium-org/mimium
brew install mimium
```

# Build from Source

See also [GitHub Action Workflow](https://github.com/mimium-org/mimium/blob/dev/.github/workflows/build_and_test.yml).

## Dependencies

### build 

- cmake
- bison >= 3.3
- flex
- llvm 9 ~ 11

### build and runtime

- Libsndfile
- RtAudio(cmake will automatically download)

```sh
git clone https://github.com/mimium-org/mimium
cd mimium
mkdir build && cd build
# configure. if you want to install to specific directory, add -DCMAKE_INSTALL_PREFIX=/your/directory
cmake .. 
# build
cmake --build . --target default_build -j
#install
cmake --build . --target install
```

### notes for linux build

At Ubuntu 18.04(Bionic), bison from apt is version 3.0.4, which will not work. Please install latest version manually.

# Author

Tomoya Matsuura/松浦知也

<https://matsuuratomoya.com/en>

# [License](LICENSE.md)

The source code is lisenced under [Mozilla Puclic License 2.0](LICENSE.md).

The source code uses some third party libraries with BSD-like lincenses, see [COPYRIGHT](./COPYRIGHT).

# Acknowledgements

This project is supported by 2019 Exploratory IT Human Resources Project ([The MITOU Program](https://www.ipa.go.jp/jinzai/mitou/portal_index.html)) by IPA: INFORMATION-TECHNOLOGY PROMOTION AGENCY, Japan.