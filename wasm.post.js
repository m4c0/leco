(function() {
  const loc = location.pathname.replace(/^.*[/]/, "").replace(/[.].*$/, "");
  fetch(loc + ".wasm")
    .then(response => response.arrayBuffer())
    .then(bytes => WebAssembly.instantiate(bytes, leco_imports))
    .then(obj => {
      leco_exports = obj.instance.exports;
      obj.instance.exports._initialize();
    });
})();
