var leco_exports;
var leco_imports = {};

(function() {
  function ifn(fn) { return leco_exports.__indirect_function_table.get(fn); }
  function arr(ptr, size) { return new Uint8Array(leco_exports.memory.buffer, ptr, size); }

  leco_imports.leco = {
    console_error : (ptr, size) => console.error(new TextDecoder().decode(arr(ptr, size))),
    console_log : (ptr, size) => console.log(new TextDecoder().decode(arr(ptr, size))),
    request_animation_frame : (fn) => window.requestAnimationFrame(ifn(fn)),
    set_timeout : (fn, timeout) => setTimeout(ifn(fn), timeout),
  };
})();

(function() {
  document.addEventListener("keydown", (e) => leco_exports.casein_key(1, e.keyCode));
  document.addEventListener("keyup", (e) => leco_exports.casein_key(0, e.keyCode));
})();

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
