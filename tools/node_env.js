// Emscripten's libc ENV does not automatically inherit Node's process.env.
// Transfer the harness options before any C boot code reads getenv().
if (typeof process === 'object' && process.env) {
  Module.preRun = Module.preRun || [];
  Module.preRun.push(function () {
    for (const name of Object.keys(process.env)) {
      if (name.startsWith('DD2_')) ENV[name] = process.env[name];
    }
  });
}
