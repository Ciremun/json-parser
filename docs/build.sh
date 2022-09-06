clang -DWASM -DNDEBUG -nostdlib --target=wasm32 -flto -Oz -Wl,--lto-O3 -Wl,--no-entry -Wl,--allow-undefined -Wl,--import-memory pg.c -o main.wasm
cat main.wasm | base64 | sed -e "$$ ! {/./s/$$/ \\\\/}" > blob_b64
[[ -f subst ]] || cc subst.c -o subst
./subst template.js -s -f BLOB blob_b64 -o pg.js
rm main.wasm blob_b64
