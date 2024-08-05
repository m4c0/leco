var leco_exports;

(function() {
  var leco_buffer;
  var leco_table;
  function leco_dump(fn, iovs, iovs_len, nwritten) {
    const view = new DataView(leco_buffer);
    const decoder = new TextDecoder()
    var written = 0;
    var text = ''

    for (var i = 0; i < iovs_len; i++) {
      const ptr = iovs + i * 8;
      const buf = view.getUint32(ptr, true);
      const buf_len = view.getUint32(ptr + 4, true);
      text += decoder.decode(new Uint8Array(leco_buffer, buf, buf_len));
      written += buf_len;
    }

    view.setUint32(nwritten, written, true);
    fn(text);
    return 0;
  }

  const leco = {
    request_animation_frame : (fn) => window.requestAnimationFrame(leco_table.get(fn)),
    set_timeout : (fn, timeout) => setTimeout(leco_table.get(fn), timeout),
  };
  const wasi_snapshot_preview1 = new Proxy({
    clock_time_get : (id, precision, out) => {
      if (id != 0) console.log("Unsupported clock type", id);
      var arr = new BigUint64Array(leco_buffer, out, 3);
      arr[0] = BigInt(Date.now() * 1000000);
    }, 
    fd_close : (fd) => 0,
    fd_fdstat_get : (fd, stat) => {
      if (fd != 1 && fd != 2) return 24;

      const view = new DataView(leco_buffer);
      view.setUint8(stat, 2);
      view.setUint32(stat + 2, 1, true);
      view.setBigUint64(stat + 8, 64n, true);
      view.setBigUint64(stat + 16, 64n, true);
      return 0;
    },
    fd_write : (fd, iovs, iovs_len, nwritten) => {
      switch (fd) {
        case 1: return leco_dump(console.log, iovs, iovs_len, nwritten);
        case 2: return leco_dump(console.error, iovs, iovs_len, nwritten);
        default: return 24;
      }
    },
    proc_exit : () => { throw "Application exit abruptly" },
  }, {
    get(obj, prop) {
      return prop in obj ? obj[prop] : (... args) => {
        console.log(prop, ... args);
        throw prop + " is not defined";
      };
    },
  });
  const imp = { leco, wasi_snapshot_preview1 };
  const loc = location.pathname.replace(/^.*[/]/, "").replace(/[.].*$/, "");
  fetch(loc + ".wasm")
    .then(response => response.arrayBuffer())
    .then(bytes => WebAssembly.instantiate(bytes, imp))
    .then(obj => {
      leco_exports = obj.instance.exports;
      leco_table = obj.instance.exports.__indirect_function_table;
      leco_buffer = obj.instance.exports.memory.buffer;
      obj.instance.exports._initialize();
    });
})();
