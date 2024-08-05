(function() {
  const wasi_snapshot_preview1 = {};
  const imp = { wasi_snapshot_preview1, ...leco_imports };
  const loc = location.pathname.replace(/^.*[/]/, "").replace(/[.].*$/, "");
  fetch(loc + ".wasm")
    .then(response => response.arrayBuffer())
    .then(bytes => WebAssembly.instantiate(bytes, imp))
    .then(obj => {
      leco_exports = obj.instance.exports;
      obj.instance.exports._initialize();
    });
})();
