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

let blob = atob(`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
QQI2AgAgASAEIAV8NwMIDwsgAEEENgIIIABBBjYCAAtEAQJ/A0AgAlAEQEEADwsgAkJ/fCECIAEt
AAAhAyAALQAAIQQgAEEBaiEAIAFBAWohASADIARGDQALQX9BASAEIANJGwvwAQIEfwN+IAEpAwgh
ByACBEAgASAHQgF8Igc3AwgLIAdCAXwhCCABKAIEIgUgB6ciBmohBEIAIQcDQCAELQAAIgNBDU1B
AEEBIAN0QYHMAHEbIANBIEYgA0EsRnJyIANB3QBGIANB/QBGcnJFBEAgASAHIAh8NwMIIARBAWoh
BCAHQgF8IQcMAQsLIAUgBmohAUIAIQgDQCAHUEUEQCABMAAAQlB8IglCCloEQCAAQQQ2AgggAEEG
NgIADwUgB0J/fCEHIAFBAWohASAJIAhCCn58IQgMAgsACwsgAEEENgIAIABCACAIfSAIIAIbNwMI
CwuSAgIAQYAIC4ECSlNPTl9FUlJPUjogAAAAADQEAABABAAATAQAAFYEAABgBAAAbAQAAHcEAABV
TktOT1dOAEpTT05fT0JKRUNUAEpTT05fU1RSSU5HAEpTT05fQk9PTABKU09OX05VTEwASlNPTl9O
VU1CRVIASlNPTl9BUlJBWQBKU09OX0VSUk9SAAAAmAQAAKsEAAC/BAAA0AQAAOAEAABKU09OX0tF
WV9OT1RfRk9VTkQASlNPTl9VTkVYUEVDVEVEX0VPRgBKU09OX1BBUlNFX0VSUk9SAEpTT05fVFlQ
RV9FUlJPUgBKU09OX01FTU9SWV9FUlJPUgB0cnVlAGZhbHNlAG51bGwAQYQKCwMQBQEALwlwcm9k
dWNlcnMBDHByb2Nlc3NlZC1ieQEFY2xhbmcPMTAuMC4wLTR1YnVudHUx`);
let array = new Uint8Array(new ArrayBuffer(blob.length));

for (let i = 0; i < blob.length; i++)
    array[i] = blob.charCodeAt(i);

let imports = {
    env: {
        memory: memory,
        print: console.log,
        output_result: (str) => {
            term.value += toUTF8(str);
            term.value += "\n> ";
            term.scrollTop = term.scrollHeight;
        },
    }
}

WebAssembly.instantiate(array, imports).then(
    function (wa) {
        term.addEventListener('textInput', (e) => {
            if (e.data.slice(-1)[0] == '\n')
            {
                e.preventDefault();
                console.log("should run the function!");
                let lines = term.value.split('\n');
                console.log(lines);
                let command = lines[lines.length - 1].split(' ').slice(1).join(' ');
                console.log(command);
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
            }
            else if (term.value === '') term.value = '> ';
        });
    }
);