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