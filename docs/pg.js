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
    if (e.keyCode === 13)
        e.preventDefault();
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
ACACQRBqIANBKGopAwA3AwAgAkEIaiADQSBqKQMANwMAIAEQAwRAIAEgASkDCEIBfDcDCCAHQn98
IQcgAkEYaiECDAIFIABBAzYCCCAAQQY2AgAMDQsACwsgACAFNgIIIABBBTYCACAAQRBqIAk3AwAM
CgsgACABQQFB8ghCBBAHDAkLIAAgAUEAQfcIQgUQBwwICyAFQf0IQgQQCEUEQCAAQQA2AgggAEED
NgIAIAEgCEIEfDcDCAwICyAAQQQ2AgggAEEGNgIADAcLIARBAWsOFwICAgICAgICAgIEAgIDAwMD
AwMDAwMDBQsgAEEANgIIIABBBTYCACAAQRBqQgA3AwAgASAIQgF8NwMIDAULIABBBjYCCCAAQQY2
AgAMBAsgAEEENgIIIABBBjYCAAwDCyAAIAFBABAJDAILIAAgAUEBEAkMAQsgACABEAYLIANBMGok
AAuKAQIEfwJ+IAApAwgiB0IBfCEGIAAoAgQgB6dqQX9qIQIDQCACQQFqIgQtAABFBEBBAw8LIAAg
BjcDCEEBIQMgAkEBai0AACICQXdqIgVBF01BAEEBIAV0QYOAgARxG0UEQCACQQ1GIQMLIAZCAXwh
BiAEIQIgAw0AC0EBQQQgAi0AACABQf8BcUYbC58CAgR/A34gASkDCCIGQgF8IQggASgCBCAGp2oh
AkIAIQYDQAJAIAEgBiAIfCIHNwMIIAIgBGoiA0EBai0AACIFQSJGDQAgAy0AAEHcAEYNACAFBEAg
BEEBaiEEIAZCAXwhBgwCBSAAQQM2AgggAEEGNgIADwsACwsCQCAGUARAQQAhAgwBCyAGQgF8IAEo
AgAoAgQRAAAiAkUEQCAAQQY2AgggAEEGNgIADwtCACAGfSEHIAEoAgQgCKdqIQVBACEDA0AgB1BF
BEAgAiADaiADIAVqLQAAOgAAIANBAWohAyAHQgF8IQcMAQsLIAIgBGpBADoAACABKQMIIQcLIAAg
AjYCCCAAQQE2AgAgAEEQaiAGNwMAIAEgB0IBfDcDCAtDAQF+IAEoAgQgASkDCCIFp2ogAyAEEAhF
BEAgACACNgIIIABBAjYCACABIAQgBXw3AwgPCyAAQQQ2AgggAEEGNgIAC0QBAn8DQCACUARAQQAP
CyACQn98IQIgAS0AACEDIAAtAAAhBCAAQQFqIQAgAUEBaiEBIAMgBEYNAAtBf0EBIAQgA0kbC/AB
AgR/A34gASkDCCEHIAIEQCABIAdCAXwiBzcDCAsgB0IBfCEIIAEoAgQiBSAHpyIGaiEEQgAhBwNA
IAQtAAAiA0ENTUEAQQEgA3RBgcwAcRsgA0EgRiADQSxGcnIgA0HdAEYgA0H9AEZyckUEQCABIAcg
CHw3AwggBEEBaiEEIAdCAXwhBwwBCwsgBSAGaiEBQgAhCANAIAdQRQRAIAEwAABCUHwiCUIKWgRA
IABBBDYCCCAAQQY2AgAPBSAHQn98IQcgAUEBaiEBIAkgCEIKfnwhCAwCCwALCyAAQQQ2AgAgAEIA
IAh9IAggAhs3AwgLC5IBAgBBgAgLgQEkBAAAMAQAADwEAABGBAAAUAQAAFwEAABnBAAAVU5LTk9X
TgBKU09OX09CSkVDVABKU09OX1NUUklORwBKU09OX0JPT0wASlNPTl9OVUxMAEpTT05fTlVNQkVS
AEpTT05fQVJSQVkASlNPTl9FUlJPUgB0cnVlAGZhbHNlAG51bGwAQYQJCwOQBAEALwlwcm9kdWNl
cnMBDHByb2Nlc3NlZC1ieQEFY2xhbmcPMTAuMC4wLTR1YnVudHUx`);
let array = new Uint8Array(new ArrayBuffer(blob.length));

for (let i = 0; i < blob.length; i++)
    array[i] = blob.charCodeAt(i);

let imports = {
    env: {
        memory: memory,
        print: console.log,
        output_result: (str) => {
            term.value += toUTF8(str);
            term.value += '\n> ';
            term.scrollTop = term.scrollHeight;
        },
    }
}

WebAssembly.instantiate(array, imports).then(
    function (wa) {
        term.addEventListener('keyup', (e) => {
            if (e.keyCode === 13)
            {
                let lines = term.value.split('\n');
                let command = lines[lines.length - 1].split(' ').slice(1).join(' ');
                term.value += '\n> ';
                if (!command)
                    return;
                let encoder = new TextEncoder();
                let bytes = encoder.encode(command);
                let ptr = wa.instance.exports.wasm_alloc(BigInt(bytes.byteLength));
                let buffer = new Uint8Array(memory.buffer, ptr, bytes.byteLength + 1);
                buffer.set(bytes);
                wa.instance.exports.parse_json(ptr);
                return;
            }
            if (term.value === '') term.value = '> ';
        });
    }
);