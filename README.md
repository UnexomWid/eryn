<p align="center">
  <img src="public/logo.png" alt="eryn">
</p>

# About <a href="https://cmake.org/cmake/help/v3.0/release/3.0.0.html"><img align="right" src="https://img.shields.io/badge/CMake-3.0-BA1F28?logo=CMake" alt="CMake 3.0" /></a><a href="https://en.wikipedia.org/wiki/C%2B%2B17"><img align="right" src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=C%2B%2B" alt="C++ 17" /></a><a href="https://nodejs.org/api/n-api.html"><img align="right" src="https://img.shields.io/badge/N--API-3-339933?logo=Node.js&logoColor=FFFFFF" alt="N-API 3" /></a>

**eryn** is a native template engine for [NodeJS](https://nodejs.org).

It was built with high performance in mind, such that it can be used for fast server-side rendering.

**This is a pre-release and, as such, is not considered stable or ready for production.**

# Getting started

You can install eryn just like any other npm package.

```shell
npm i eryn --save
```

> Note: this packages includes a declaration file for TypeScript.

If a prebuild is already available for your platform, you can jump straight to [quick examples](#Quick-examples). If not, see below [compiling the package](#Compiling-the-Package).

The list of prebuilds can be found [here](https://github.com/UnexomWid/eryn/wiki/Getting-Started#prebuilds).

## Compiling the Package

You'll need to install a C/C++ compiler, as well as [CMake](https://cmake.org).

The package will be compiled when you install it through npm, if a prebuild is not available. Please note that the `devDependencies` listed in [package.json](https://github.com/UnexomWid/eryn/blob/master/package.json) are required for building the package.

If you're missing either a compiler or [CMake](https://cmake.org), an error will be shown. Make sure you have both, and try again.

For more details, see [the wiki](https://github.com/UnexomWid/eryn/wiki/Getting-started#compiling-the-package).

To manually compile the package, run:

```shell
npm run rebuild
```

...or directly run [cmake-js](https://github.com/cmake-js/cmake-js):

```shell
npx cmake-js compile
```

...or globally install [cmake-js](https://github.com/cmake-js/cmake-js) to be able to run it anywhere:

```shell
npm i -g cmake-js

cmake-js compile
```

## Scripts

- **install** - installs the package and compiles it if no (pre)build is available
- **rebuild** - compiles the package
- **prebuild** - generates prebuilds the package (you need to provide your own generator)
- **check** - checks if a build or prebuild is available (if not, exits with code `1`)

## Quick examples

Here's a basic example.

> Note: if you don't like the syntax, see below [Changing the syntax](#Changing-the-syntax).

**test.js**

```js
var path = require("path");
var eryn = require("eryn");

// Pass the absolute path to the file. Relative paths might not be safe (see the wiki).
var data = eryn.render(path.join(__dirname, "test.eryn"), {
    firstName: "Tyler",
    lastName: "Bounty",
    greeting: "Hey there",
    numbers: [10, 20, 30, 40]
});
```

**test.eryn**

```js
Welcome, [|context.firstName|] [|context.lastName|]!

[|? context.greeting.length > 5 |]
The greeting has more than 5 characters!
[|end?|]

This is a basic loop:
[|@ num : context.numbers|]
Current number: [|num|]
[|end@|]

There is also support for components!
[|% comp.eryn : {message: "Hello"} |]
This is some content for the component!
It can use the component context: [|context.message|]
[|/|]

And self-closing components too!
[|% comp2.eryn : {test: "world"} /|]
```

**comp.eryn**

```js
This is a component!

It has context which is automatically stringified: [|context|]
...and works as usual: [|context.message|]

And also some content:
[|content|]
```

**comp2.eryn**

```js
Hello, [|context.test|]!
This is a self closing component with no content!
```

The **render** function will return a `buffer`, containing:

```
Welcome, Tyler Bounty!


The greeting has more than 5 characters!


This is a basic loop:

Current number: 10

Current number: 20

Current number: 30

Current number: 40


There's also support for components!
This is a component!

It has context which is automatically stringified: {"message":"Hello"}
...and works as usual: Hello

And also some content:

This is some content for the component!
It can use the component context: Hello


And self-closing components too!
Hello, world!
This is a self closing component with no content!
```

You can use this buffer however you want (e.g. write it to a file, use it as-is, etc).

## Changing the syntax

If you don't like the default syntax, you can change it by calling the `setOptions` function before rendering the file. Here's an example:

```js
eryn.setOptions({
    templateStart: "{{",
    templateEnd: "}}",

    conditionalStart: "if",
    conditionalEnd: "endif",

    loopStart: "for",
    loopEnd: "endfor",
    loopSeparator: "of",
    
    componentStart: "component",
    componentEnd: "endcomp",
    componentSeparator: "with",
    componentSelf: "self"
});

eryn.render(...);
```

> Note: you can call the `setOptions` function as many times as you want.
> Changes will take effect immediately.

The files can now be written using this syntax. Here's how the first file would look:

**test.eryn**

```js
Welcome, {{context.firstName}} {{context.lastName}}!

{{if context.greeting.length > 5 }}
The greeting has more than 5 characters!
{{endif}}

This is a basic loop:
{{for num of context.numbers}}
Current number: {{num}}
{{endfor}}

There is also support for components!
{{component comp.eryn with {message: "Hello"} }}
This is some content for the component!
It can use the component context: {{context.message}}
{{endcomp}}

And self-closing components too!
{{component comp2.eryn with {test: "world"} self}}
```

This will give the exact same result.

> Note: you have to change the syntax in all files.
>
> Also: change the syntax wisely. Otherwise, you might run into some problems (see [here](https://github.com/UnexomWid/eryn/wiki/Options#remarks)).

# Documentation

For the complete documentation, and some more examples, check [the wiki](https://github.com/UnexomWid/eryn/wiki).

# Releases

[0.1.0](https://github.com/UnexomWid/eryn/releases/tag/0.1.0) - May 2nd, 2020

# Contributing

You can contribute by reporting bugs and requesting features on the [issue page](https://github.com/UnexomWid/eryn/issues).

However, please see [known issues](https://github.com/UnexomWid/eryn/wiki/Known-Issues) before reporting bugs.

#### Pull Requests

I don't think I'll accept any of them yet, as the engine is still in its early stages and the code might change in the future.

However, when I release a stable version, I'll also release documentation for the code and I might start reviewing PRs.

Until then, feel free to use the [issue page](https://github.com/UnexomWid/eryn/issues).

# TODO

- Replace the global static variables with environment-dependent data

- Add support for reversed loop templates ([#1](https://github.com/UnexomWid/eryn/issues/1))

- ~~Add support for inverted conditional templates ([#2](https://github.com/UnexomWid/eryn/issues/2))~~ [[771c5d5](https://github.com/UnexomWid/eryn/commit/771c5d5b3363949df7afd827a68d0fba6995f80e)]

- Add an option to throw an exception if compileDir encounters an error ([#5](https://github.com/UnexomWid/eryn/issues/5))

- Support for `Buffer` in templates

- ~~Store the context locally~~ [[51a9428](https://github.com/UnexomWid/eryn/commit/51a942819305688bde41063b2291c1332336d669)]

- ~~Store the loop iterators in a local map~~ [[0cd30a](https://github.com/UnexomWid/eryn/commit/0cd30a060524d05f9e46e1e655b3b2632f2bd494)]

# License <a href="https://github.com/UnexomWid/eryn/blob/master/LICENSE"><img align="right" src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License: MIT" /></a>

**eryn** was created by UnexomWid. It is licensed under the [MIT license](https://github.com/UnexomWid/eryn/blob/master/LICENSE).

This project uses first-party and third-party dependencies. They are listed below, along with their licenses.

# Dependencies

## NPM Packages (dev)

- [nodejs/**node-addon-api**](https://github.com/nodejs/node-addon-api) - wrapper classes for N-API ([MIT License](https://github.com/nodejs/node-addon-api/blob/master/LICENSE.md))
- [cmake-js/**cmake-js**](https://github.com/cmake-js/cmake-js) - native addon build tool based on CMake ([MIT License](https://github.com/cmake-js/cmake-js/blob/master/LICENSE))
- [moxystudio/**cross-spawn**](https://github.com/moxystudio/node-cross-spawn) - cross-platform alternative for spawn ([MIT License](https://github.com/moxystudio/node-cross-spawn/blob/master/LICENSE))
- [substack/**minimist**](https://github.com/substack/minimist) - argument parser ([MIT License](https://github.com/substack/minimist/blob/master/LICENSE))
- [sindresorhus/**npm-run-path**](https://github.com/sindresorhus/npm-run-path) - PATH for locally installed binaries ([MIT License](https://github.com/sindresorhus/npm-run-path/blob/master/license))

## First-Party (C/C++)

- [UnexomWid/**BDP**](https://github.com/UnexomWid/bdp) - 64 bit packaging format for binary data ([MIT License](https://github.com/UnexomWid/BDP/blob/master/LICENSE))
- [UndexomWid/**timerh**](https://github.com/UnexomWid/timerh) - library for measuring execution time ([MIT License](https://github.com/UnexomWid/timerh/blob/master/LICENSE))

## Third-Party (C/C++)

- [tronkko/**dirent**](https://github.com/tronkko/dirent) - dirent port for Windows ([MIT License](https://github.com/UnexomWid/eryn/blob/master/include/dirent.LICENSE))