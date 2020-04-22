# About

**eryn** is a native template processor for NodeJS. It was built to be used for server-side rendering.

# Getting started

You can install eryn just like any other npm package.

```shell
npm i eryn
```

If a prebuild is already available for your platform, you can jump straight to Quick examples. If not, read below.

## Compiling

You'll need to install C/C++ compiler, as well as CMake.

The package will be compiled when you install it through npm. If you're missing either a compiler or CMake, an error will be shown. Make sure you have both, and try again.

To manually compile the package, run:

```shell
npx cmake-js compile
```

...or you can globally install cmake-js to be able to run it directly.

```shell
npm i -g cmake-js
cmake-js compile
```

## Quick examples

Here's a basic example.

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

There's also support for components!
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

**test.js**

```js
var path = require("path");
var eryn = require("eryn");

var data = eryn.render(path.join(__dirname, "test.eryn"), {
    firstName: "Tyler",
    lastName: "Bounty",
    greeting: "Hey there",
    numbers: [10, 20, 30, 40]
});
```

The **data** variable will be a byte buffer, containing:

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

If you don't like the default syntax, you can change it by calling the setOptions function before rendering the file. Here's an example:

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

The previous files can now be written like this:

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

There's also support for components!
{{component comp.eryn with {message: "Hello"} }}
This is some content for the component!
It can use the component context: {{context.message}}
{{endcomp}}

And self-closing components too!
{{component comp2.eryn with {test: "world"} self}}
```

**comp.eryn**

```js
This is a component!

It has context which is automatically stringified: {{context}}
...and works as usual: {{context.message}}

And also some content:
{{content}}
```

**comp2.eryn**

```js
Hello, {{context.test}}!
This is a self closing component with no content!
```

This will give the exact same output.

For the complete documentation and more examples, check the wiki.
