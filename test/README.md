# Tests

This is a script that runs various tests for eryn. It uses [jshiyou](https://github.com/exom-dev/jshiyou).

To run the tests, simply install [jshiyou](https://github.com/exom-dev/jshiyou) and run the script.

```shell
npm i @exom-dev/jshiyou
node test.js
```

## Generating tests

If you want to generate a test:

- create your test file inside the `input` dir
- in `test.js`, set `KEEP_OUTPUT_FILES` to true
- add a jshiyou test for both OSH and Render (copy paste, replace  with your file name)
- run the test script
- go inside the `input` dir
- copy the generated `.eryn.osh` and `.eryn.rendered` files inside `expected` (keep the directory structure).
Before doing this, however, manually check the generated files and ensure that they are 100% correct
- set `KEEP_OUTPUT_FILES` back to false
- run the tests one more time (all output files will be deleted)
- if everything passed, you're good to go. If a future commit breaks something, the tests
will help narrow down the problem

## Important

- OSH tests that include components will not work properly. This is because OSH translates relative
paths into absolute paths for performance reasons. This means that the tests WILL fail on other machines.

- pay attention to EOL characters. Currently, all tests use LF. However, if you use CRLF for the input files, the expected
test files MUST also use CRLF. Otherwise, the test will not pass (because the files are not exactly the same). Why not
consider LF and CRLF the same? Because **eryn** can also render binary files.