// Copyright (c) 2011-2012,2013,2016,2018,2019,2020 <>< Charles Lohr
// This file may be licensed under the MIT/x11 license, NewBSD or CC0 licenses
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of this file.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

let term = document.getElementById('term');
term.addEventListener('keydown', (e) => {
    if (e.keyCode === 8 && term.value === '> ')
        e.preventDefault();
});

let memory = new WebAssembly.Memory({ initial: 4096 });
let HEAP8 = new Int8Array(memory.buffer);
let HEAPU8 = new Uint8Array(memory.buffer);
let HEAP16 = new Int16Array(memory.buffer);
let HEAPU16 = new Uint16Array(memory.buffer);
let HEAP32 = new Uint32Array(memory.buffer);
let HEAPU32 = new Uint32Array(memory.buffer);
let HEAPF32 = new Float32Array(memory.buffer);
let HEAPF64 = new Float64Array(memory.buffer);

let toUtf8Decoder = new TextDecoder("utf-8");

function toUTF8(ptr) {
    let len = 0 | 0; ptr |= 0;
    for (let i = ptr; HEAPU8[i] != 0; i++) len++;
    return toUtf8Decoder.decode(HEAPU8.subarray(ptr, ptr + len));
}

let blob = atob(`${BLOB}`);
let array = new Uint8Array(new ArrayBuffer(blob.length));

for (let i = 0; i < blob.length; i++)
    array[i] = blob.charCodeAt(i);

let imports = {
    env: {
        memory: memory,
        print: console.log,
        output_result: (str) => {
            term.value += toUTF8(str);
        },
    }
}

WebAssembly.instantiate(array, imports).then(
    function (wa) {
        term.addEventListener('textInput', (e) => {
            if (e.data.slice(-1)[0] == '\n')
            {
                e.preventDefault();
                let lines = term.value.split('\n');
                let command = lines[lines.length - 1].split(' ').slice(1).join(' ');
                term.value += '\n> ';
                term.scrollTop = term.scrollHeight;
                if (!command)
                    return;
                let encoder = new TextEncoder();
                let bytes = encoder.encode(command);
                let ptr = wa.instance.exports.wasm_alloc(BigInt(bytes.byteLength));
                let buffer = new Uint8Array(memory.buffer, ptr, bytes.byteLength + 1);
                buffer.set(bytes);
                wa.instance.exports.parse_json(ptr);
                term.value += "\n> ";
                term.scrollTop = term.scrollHeight;
            }
            else if (term.value === '') term.value = '> ';
        });
    }
);
