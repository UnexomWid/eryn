var shiyou = require('@exom-dev/jshiyou');
var eryn = require("../index.js");
var path = require("path");
var fs = require("fs");

// Used when generating the tests.
const KEEP_OUTPUT_FILES = false;
// Empty callback.
const NOP = () => { };

eryn.setOptions({
    debugDumpOSH: true,
    workingDirectory: path.join(__dirname, 'input')
});

function oshTestFactory(name) {
    return () => {
        eryn.compile(`${name}.eryn`);
        
        let result = fs.readFileSync(path.join(__dirname, `input/${name}.eryn.osh`));
        let expected = fs.readFileSync(path.join(__dirname, `expected/${name}.eryn.osh`));
        
        if(!KEEP_OUTPUT_FILES)
            fs.unlink(path.join(__dirname, `input/${name}.eryn.osh`), NOP);
        
        return result.equals(expected);
    }
}

function renderTestFactory(name) {
    return () => {
        let result = eryn.render(`${name}.eryn`, {
            conditional_one: 1,
            loop_numbers: [0, 1, 2, 3, 4]
        });

        if(KEEP_OUTPUT_FILES) {
            fs.writeFile(path.join(__dirname, `input/${name}.eryn.rendered`), result, NOP);
        } else {
            fs.unlink(path.join(__dirname, `input/${name}.eryn.rendered`), NOP);
            fs.unlink(path.join(__dirname, `input/${name}.eryn.osh`), NOP);
        }

        let expected = fs.readFileSync(path.join(__dirname, `expected/${name}.eryn.rendered`));
        
        return result.equals(expected);
    }
}

shiyou.test('OSH', 'Empty', oshTestFactory('empty'));
shiyou.test('OSH', 'Plain text', oshTestFactory('plain_text'));
shiyou.test('OSH', 'Conditional', oshTestFactory('conditional'));
shiyou.test('OSH', 'Conditional + plaintext', oshTestFactory('conditional_plaintext'));
shiyou.test('OSH', 'Conditional + else', oshTestFactory('conditional_else'));
shiyou.test('OSH', 'Conditional + else + else conditional', oshTestFactory('conditional_else_conditional'));
shiyou.test('OSH', 'Conditional + else + else conditional (multiple)', oshTestFactory('conditional_else_conditional_multiple'));
shiyou.test('OSH', 'Conditional + else + else conditional (multiple) + plaintext', oshTestFactory('conditional_else_conditional_multiple_plaintext'));
shiyou.test('OSH', 'Loop', oshTestFactory('loop'));
shiyou.test('OSH', 'Loop + plaintext', oshTestFactory('loop_plaintext'));

shiyou.test('Render', 'Empty', renderTestFactory('empty'));
shiyou.test('Render', 'Plain text', renderTestFactory('plain_text'));
shiyou.test('Render', 'Conditional', renderTestFactory('conditional'));
shiyou.test('Render', 'Conditional + plaintext', renderTestFactory('conditional_plaintext'));
shiyou.test('Render', 'Conditional + else', renderTestFactory('conditional_else'));
shiyou.test('Render', 'Conditional + else + else conditional', renderTestFactory('conditional_else_conditional'));
shiyou.test('Render', 'Conditional + else + else conditional (multiple)', renderTestFactory('conditional_else_conditional_multiple'));
shiyou.test('Render', 'Conditional + else + else conditional (multiple) + plaintext', renderTestFactory('conditional_else_conditional_multiple_plaintext'));
shiyou.test('Render', 'Loop', renderTestFactory('loop'));
shiyou.test('Render', 'Loop + plaintext', renderTestFactory('loop_plaintext'));
shiyou.test('Render', 'Component', renderTestFactory('component/component'));
shiyou.test('Render', 'Component + content', renderTestFactory('component_content/component_content'));
shiyou.test('Render', 'Component + content + plaintext', renderTestFactory('component_content_plaintext/component_content_plaintext'));
shiyou.test('Render', 'Component (nested)', renderTestFactory('component_nested/component_nested'));
shiyou.test('Render', 'Component + content (nested)', renderTestFactory('component_content_nested/component_content_nested'));
shiyou.test('Render', 'Component + content + plaintext (nested)', renderTestFactory('component_content_plaintext_nested/component_content_plaintext_nested'));
shiyou.test('Render', 'Mixed', renderTestFactory('mixed/mixed'));

shiyou.run();